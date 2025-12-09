#pragma once

#include "ofxDatGuiContainer.h"
#include "ofxDatGuiEvents.h"
#include "../components/ofxDatGuiButton.h" // contains ofxDatGuiToggle too
#include "../components/ofxDatGuiSlider.h"
#include "../components/ofxDatGuiLabel.h"
#include "../components/ofxDatGuiTextInput.h"
#include <algorithm>

class ofxDatGui; // forward declaration for getRoot/bringToFront
class ofxDatGuiDropdown;

// Simple container panel that lays out its children either
// vertically (one per row) or horizontally (in a single row).
// Meant to be embedded inside folders/groups or used on its own.
class ofxDatGuiPanel : public ofxDatGuiContainer {
public:
	// Layout direction for children.
	enum class Orientation {
		VERTICAL,
		HORIZONTAL
	};

	explicit ofxDatGuiPanel(Orientation orientation = Orientation::VERTICAL)
		: ofxDatGuiContainer("")
		, mOrientation(orientation)
		, mHeight(0)
		, mSpacing(0)
		, mHeaderEnabled(false)
		, mHeaderHeight(24)
		, mDragging(false) {
		mType = ofxDatGuiType::PANEL;
		// Panels allow muting by default; caller can opt out via setPreventMuting(true).
		mPreventMuting = false;

		// Initialize spacing & base style from the global default theme
		// so layoutChildren() has sane values even before setTheme() is called.
		const ofxDatGuiTheme * t = ofxDatGuiComponent::getTheme();
		if (t != nullptr) {
			setComponentStyle(t);
			mSpacing = t->layout.vMargin;
			mStyle.height = mHeaderHeight;
		}
	}

	~ofxDatGuiPanel() override = default;

	// ---------------------------------------------------------------------
	// Orientation
	// ---------------------------------------------------------------------

	void setOrientation(Orientation orientation) {
		if (mOrientation == orientation) return;
		mOrientation = orientation;

		// When horizontal, stripes along the bottom of children.
		// When vertical, stripes along the left (default).
		for (auto & c : children) {
			if (!c) continue;
			if (mOrientation == Orientation::HORIZONTAL) {
				c->setStripePosition(ofxDatGuiComponent::StripePosition::BOTTOM);
			} else {
				c->setStripePosition(ofxDatGuiComponent::StripePosition::LEFT);
			}
		}

		layoutChildren();
	}


	Orientation getOrientation() const { return mOrientation; }

	// ---------------------------------------------------------------------
	// Core virtuals
	// ---------------------------------------------------------------------

	void setTheme(const ofxDatGuiTheme * theme) override {
		if (theme == nullptr) {
			theme = ofxDatGuiComponent::getTheme();
		}

		// Use theme to pick up spacing & style.
		setComponentStyle(theme);
		mSpacing = theme->layout.vMargin;
		mStyle.height = mHeaderEnabled ? mHeaderHeight : 0;

		// Propagate theme to children
		for (auto & c : children) {
			c->setTheme(theme);
		}

		layoutChildren();
	}

	void setPosition(int px, int py) override {
		// IMPORTANT: do NOT call base setPosition(), because that will
		// auto-stack children vertically. We fully control child layout.
		x = px;
		y = py;
		layoutChildren();
	}

	void setWidth(int width, float labelWidth = 1.f) override {
		// Respect user-defined widths: if a theme is being applied and the user
		// already set a width, don't override it. Otherwise, update our own width
		// and relayout children without forcing their widths directly.
		const bool themeWidth = ofxDatGuiComponent::isApplyingThemeWidth();
		if (themeWidth && mUserWidthSet) return;

		mStyle.width = width;
		if (labelWidth > 1) {
			mLabel.width = labelWidth;
		} else {
			mLabel.width = mStyle.width * labelWidth;
		}
		mIcon.x = mStyle.width - (mStyle.width * .05) - mIcon.size;
		mLabel.rightAlignedXpos = mLabel.width - mLabel.margin;
		positionLabel();

		mHasAppliedWidth = true;
		if (!themeWidth) {
			mUserWidthSet = true;
		}

		layoutChildren();
	}

	int getHeight() override {
		return mHeight;
	}

	bool getIsExpanded() override {
		// Panels don't implement collapse/expand; treat as always expanded.
		return true;
	}

	// Enable a simple header bar that shows the panel label and allows dragging.
	void setHeaderEnabled(bool enable = true, int headerHeight = 24) {
		mHeaderEnabled = enable;
		mHeaderHeight = headerHeight;
		mStyle.height = mHeaderEnabled ? mHeaderHeight : 0;
		layoutChildren();
	}

	bool hasHeader() const { return mHeaderEnabled; }

	void update(bool acceptEvents = true) override {
		// Panels are layout-only containers; we don't want them to
		// steal mouse presses from their children. The base
		// ofxDatGuiComponent::update() uses mStyle.height as a
		// "header" height for containers (anything below that is
		// treated as child region and should belong to children).
		// If we have no header, collapse the header height to zero while updating.

		float oldHeight = mStyle.height;
		if (!mHeaderEnabled) mStyle.height = 0;

		ofxDatGuiComponent::update(acceptEvents);

		// Restore whatever height the theme/layout wants to use
		// for drawing / layout purposes.
		mStyle.height = oldHeight;

		// Now update children with same enabled flag.
		const bool enabled = acceptEvents && getEnabled();
		for (auto & c : children) {
			c->update(enabled);
		}
	}


	void draw() override {
		if (!mVisible) return;

		ofPushStyle();

		ofColor headerColor = mStyle.color.background;
		if (const ofxDatGuiTheme* t = ofxDatGuiComponent::getTheme()) {
			headerColor = t->color.panelHeader;
		}

		if (mHeaderEnabled) {
			ofSetColor(headerColor, mStyle.opacity * 255);
			ofDrawRectangle(x, y, mStyle.width, mHeaderHeight);
			ofSetColor(mLabel.color);
			if (mFont) {
				mFont->draw(mLabel.rendered, x + mLabel.x, y + mHeaderHeight/2 + mLabel.rect.height/2);
			}
		}

		// Fill the spacing gaps with the same color as the header so they aren't transparent.
		if (mSpacing > 0) {
			ofSetColor(headerColor, mStyle.opacity * 255);
			if (mOrientation == Orientation::VERTICAL) {
				ofxDatGuiComponent* previous = nullptr;
				for (auto& c : children) {
					if (!c->getVisible()) continue;
					if (previous != nullptr) {
						int gapY = previous->getY() + previous->getHeight();
						int gapH = c->getY() - gapY;
						if (gapH > 0) {
							ofDrawRectangle(x, gapY, mStyle.width, gapH);
						}
					}
					previous = c.get();
				}
			} else {
				int rowY = y + (mHeaderEnabled ? mHeaderHeight : 0);
				int rowHeight = std::max(0, mHeight - (mHeaderEnabled ? mHeaderHeight : 0));
				if (rowHeight == 0) {
					for (auto& c : children) {
						if (!c->getVisible()) continue;
						rowHeight = std::max(rowHeight, c->getHeight());
					}
				}
				if (rowHeight > 0) {
					ofxDatGuiComponent* previous = nullptr;
					for (auto& c : children) {
						if (!c->getVisible()) continue;
						if (previous != nullptr) {
							int gapX = previous->getX() + previous->getWidth();
							int gapW = c->getX() - gapX;
							if (gapW > 0) {
								ofDrawRectangle(gapX, rowY, gapW, rowHeight);
							}
						}
						previous = c.get();
					}
				}
			}
		}
		// Panel itself stays visually transparent by default.
		// If you ever want a framed block, uncomment:
		// drawBackground();
		// drawBorder();

		for (auto & c : children) {
			if (!c->getVisible()) continue;

			// Let the child draw itself first
			c->draw();

			// In horizontal mode, draw a bottom stripe for each child,
			// using THIS PANEL's stripe style (just like ButtonBar).
			if (mOrientation == Orientation::HORIZONTAL) {
				drawChildBottomStripe(c.get());
			}
		}

		ofPopStyle();
	}

	// ---------------------------------------------------------------------
	// Child management
	// ---------------------------------------------------------------------

	// Attach an existing component as a child of this panel.
	// Takes ownership via unique_ptr but returns raw pointer for convenience.
	ofxDatGuiComponent * attachItem(ofxDatGuiComponent * item) {
		if (!item) return nullptr;

		item->setIndex(static_cast<int>(children.size()));
		// Store root in local variable to avoid incomplete type issues
		ofxDatGui* root = getRoot();
		item->setRoot(root);
		item->setStripeColor(mStyle.stripe.color);

		// Make new kids follow the panel's current stripe orientation
		if (mOrientation == Orientation::HORIZONTAL) {
			item->setStripePosition(ofxDatGuiComponent::StripePosition::BOTTOM);
		} else {
			item->setStripePosition(ofxDatGuiComponent::StripePosition::LEFT);
		}

		item->onInternalEvent(this, &ofxDatGuiPanel::onInternalChildEvent);
		emplaceChild(ComponentPtr(item));
		return item;
	}


	// Read-only access to children if you ever need to poke from outside.
	const std::vector<ComponentPtr> & getChildren() const {
		return children;
	}

	// Convenience adders to avoid manual new/attach in user code.
	ofxDatGuiButton* addButton(const std::string & label) {
		auto item = std::make_unique<ofxDatGuiButton>(label);
		item->setStripeColor(mStyle.stripe.color);
		return attachOwned(std::move(item));
	}

	ofxDatGuiToggle* addToggle(const std::string & label, bool state = false) {
		auto item = std::make_unique<ofxDatGuiToggle>(label, state);
		item->setStripeColor(mStyle.stripe.color);
		return attachOwned(std::move(item));
	}

	ofxDatGuiSlider* addSlider(const std::string & label, float min, float max, float value) {
		auto item = std::make_unique<ofxDatGuiSlider>(label, min, max, value);
		item->setStripeColor(mStyle.stripe.color);
		return attachOwned(std::move(item));
	}

	// Parameter-bound sliders for two-way syncing with ofParameter.
	ofxDatGuiSlider* addSlider(ofParameter<int> & p) {
		auto item = std::make_unique<ofxDatGuiSlider>(p);
		item->setStripeColor(mStyle.stripe.color);
		return attachOwned(std::move(item));
	}

	ofxDatGuiSlider* addSlider(ofParameter<float> & p) {
		auto item = std::make_unique<ofxDatGuiSlider>(p);
		item->setStripeColor(mStyle.stripe.color);
		return attachOwned(std::move(item));
	}

	ofxDatGuiTextInput* addTextInput(const std::string & label, const std::string & value = "") {
		auto item = std::make_unique<ofxDatGuiTextInput>(label, value);
		item->setStripeColor(mStyle.stripe.color);
		return attachOwned(std::move(item));
	}

	ofxDatGuiDropdown* addDropdown(const std::string & label, const std::vector<std::string> & options);

	ofxDatGuiLabel* addLabel(const std::string & label) {
		auto item = std::make_unique<ofxDatGuiLabel>(label);
		item->setStripeColor(mStyle.stripe.color);
		return attachOwned(std::move(item));
	}

protected:
	// ---------------------------------------------------------------------
	// Layout
	// ---------------------------------------------------------------------

	void layoutChildren() override {
		mHeight = 0;

		if (children.empty()) {
			mHeight = mHeaderEnabled ? mHeaderHeight : 0;
			return;
		}

		int cursorY = y + (mHeaderEnabled ? mHeaderHeight : 0);

		// Preserve each child's existing label/share ratio so components like sliders
		// don't end up with the label consuming the full row width.
		auto labelFrac = [](ofxDatGuiComponent* c) {
			// mLabel.width is stored in pixels; convert back to a fraction of the current width.
			const float w = static_cast<float>(c->getWidth());
			if (w <= 0.f) return 0.35f;
			float frac = c->getLabelWidth() / w;
			if (frac <= 0.f) frac = 0.35f;
			if (frac > 0.95f) frac = 0.95f; // always leave some room for the control itself
			return frac;
		};

		if (mOrientation == Orientation::VERTICAL) {
			const int spacing = mSpacing;

			for (auto & c : children) {
				if (!c->getVisible()) continue;
				// Force children to match panel width in vertical mode, but keep their label ratio.
				c->setWidth(mStyle.width, labelFrac(c.get()));
				c->setPosition(x, cursorY);
				cursorY += c->getHeight() + spacing;
			}

			// Remove the trailing spacing if we placed at least one child.
			if (cursorY > y) {
				cursorY -= spacing;
			}

			mHeight = cursorY - y;
		} else {
			// Horizontal row: try to keep everything inside our width.
			std::vector<ofxDatGuiComponent *> visible;
			visible.reserve(children.size());
			for (auto & c : children) {
				if (c->getVisible()) visible.push_back(c.get());
			}

			if (visible.empty()) {
				mHeight = mHeaderEnabled ? mHeaderHeight : 0;
				return;
			}

			int availableWidth = mStyle.width;
			const int spacing = mSpacing;

			if (availableWidth <= 0) {
				// Fallback: at least make them butt up against each other
				// using their own widths.
				int cursorX = x;
				int rowHeight = mHeaderEnabled ? mHeaderHeight : 0;

				for (auto * c : visible) {
					c->setPosition(cursorX, y + (mHeaderEnabled ? mHeaderHeight : 0));
					cursorX += c->getWidth() + spacing;
					rowHeight = std::max(rowHeight, c->getHeight());
				}

				mHeight = rowHeight;
				if (mHeaderEnabled) mHeight = rowHeight + mHeaderHeight;
				return;
			}

			const int count = static_cast<int>(visible.size());
			const int totalSpacing = spacing * std::max(0, count - 1);
			int childWidth = (availableWidth - totalSpacing) / std::max(1, count);
			if (childWidth < 1) childWidth = 1;
			int leftover = (availableWidth - totalSpacing) - childWidth * count;

			int cursorX = x;
			int rowHeight = mHeaderEnabled ? mHeaderHeight : 0;

			for (int i = 0; i < count; ++i) {
				auto * c = visible[i];
				int thisWidth = childWidth + (leftover > 0 ? 1 : 0);
				if (leftover > 0) --leftover;
				// Give each child the computed width.
				c->setWidth(thisWidth, labelFrac(c)); // keep existing label ratio when resizing
				c->setPosition(cursorX, y + (mHeaderEnabled ? mHeaderHeight : 0));

				cursorX += thisWidth;
				if (i + 1 < count) {
					cursorX += spacing;
				}

				rowHeight = std::max(rowHeight, c->getHeight());
			}

			mHeight = rowHeight;
			if (mHeaderEnabled) mHeight = rowHeight + mHeaderHeight;
		}
	}

	// Internal events coming from children (visibility changes, etc.).
	void onInternalChildEvent(ofxDatGuiInternalEvent e) {
		if (e.type == ofxDatGuiEventType::VISIBILITY_CHANGED) {
			layoutChildren();
		}

		// Bubble the event up if someone is listening to this panel.
		if (internalEventCallback != nullptr) {
			internalEventCallback(e);
		}
	}

	// Draw a horizontal stripe at the bottom edge of the given child,
	// using this panel's stripe style, similar to ofxDatGuiButtonBar.
	void drawChildBottomStripe(ofxDatGuiComponent * child) {
		if (!child) return;
		if (!child->getStripeVisible()) return;

		float stripeH = static_cast<float>(child->getStripeWidth());
		if (stripeH <= 0.f) return;

		float sx = static_cast<float>(child->getX());
		float sw = static_cast<float>(child->getWidth());
		float sy = static_cast<float>(child->getY() + child->getHeight()) - stripeH;

		if (sw <= 0.f) return;

		ofSetColor(child->getStripeColor()); // <-- use child color
		ofDrawRectangle(sx, sy, sw, stripeH);
	}

	// Basic dragging when header is enabled.
	void onMousePress(ofVec3f m) override {
		if (mHeaderEnabled) {
			bool inHeader = m.x >= x && m.x <= x + mStyle.width && m.y >= y && m.y <= y + mHeaderHeight;
			if (inHeader) {
				mDragging = true;
				mDragOffset.set(m.x - x, m.y - y);
			}
		}
		ofxDatGuiComponent::onMousePress(m);
	}
	void onMouseDrag(ofVec3f m) override {
		if (mHeaderEnabled && mDragging) {
			setPosition(static_cast<int>(m.x - mDragOffset.x), static_cast<int>(m.y - mDragOffset.y));
		}
		ofxDatGuiComponent::onMouseDrag(m);
	}
	void onMouseRelease(ofVec3f m) override {
		mDragging = false;
		ofxDatGuiComponent::onMouseRelease(m);
	}

	bool hitTest(ofPoint m) override {
		// Use full panel bounds (header + children), not just header height.
		int h = mHeight;
		// If layout hasn't run yet, fall back to header height.
		if (h <= 0) {
			h = mHeaderEnabled ? mHeaderHeight : 0;
		}
		return (m.x >= x && m.x <= x + mStyle.width && m.y >= y && m.y <= y + h);
	}


	Orientation mOrientation;
	int mHeight;
	int mSpacing;
	bool mHeaderEnabled;
	int mHeaderHeight;
	bool mDragging = false;
	ofPoint mDragOffset;

	// Helper to take ownership and return raw for convenience.
	template<typename T>
	T* attachOwned(std::unique_ptr<T> item) {
		if (!item) return nullptr;
		auto* raw = item.release(); // attachItem will wrap in ComponentPtr
		attachItem(raw);
		return raw;
	}
};
