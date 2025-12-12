#pragma once

#include "ofxDatGuiContainer.h"
#include "ofxDatGuiEvents.h"
#include "../components/ofxDatGuiButton.h" // contains ofxDatGuiToggle too
#include "../components/ofxDatGuiSlider.h"
#include "../components/ofxDatGuiLabel.h"
#include "../components/ofxDatGuiTextInput.h"
#include "../components/ofxDatGui2dPad.h"
#include "../components/ofxDatGuiTimeGraph.h"
#include <algorithm>
#include <initializer_list>

class ofxDatGui; // forward declaration for getRoot/bringToFront
class ofxDatGuiDropdown;
class ofxDatGuiFolder;

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

	// Simple edge anchoring for manual layout: pin to screen edges.
	enum class Anchor {
		NONE   = 0,
		TOP    = 1 << 0,
		BOTTOM = 1 << 1,
		LEFT   = 1 << 2,
		RIGHT  = 1 << 3
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
		if (mAnchorMask != 0) {
			cacheAnchorOffsets(ofGetWidth(), ofGetHeight());
		}
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

		if (mAnchorMask != 0) {
			cacheAnchorOffsets(ofGetWidth(), ofGetHeight());
		}

		layoutChildren();
		if (mAnchorMask != 0) {
			applyAnchor(ofGetWidth(), ofGetHeight());
		}
	}

	int getHeight() override {
		return mHeight;
	}

	bool getIsExpanded() override {
		// Panels don't implement collapse/expand; treat as always expanded.
		return true;
	}

	// ---------------------------------------------------------------------
	// Anchoring
	// ---------------------------------------------------------------------
	void setAnchor(Anchor anchor) {
		mAnchorMask = static_cast<int>(anchor);
		cacheAnchorOffsets(ofGetWidth(), ofGetHeight());
		applyAnchor(ofGetWidth(), ofGetHeight());
	}

	void setAnchor(std::initializer_list<Anchor> anchors) {
		mAnchorMask = 0;
		for (auto a : anchors) {
			mAnchorMask |= static_cast<int>(a);
		}
		cacheAnchorOffsets(ofGetWidth(), ofGetHeight());
		applyAnchor(ofGetWidth(), ofGetHeight());
	}

	Anchor getAnchor() const { return static_cast<Anchor>(mAnchorMask); }

	// Recompute anchored position for a given viewport size.
	void applyAnchor(int windowW, int windowH) {
		if (mAnchorMask == 0) return;
		if (mApplyingAnchor) return;

		mApplyingAnchor = true;
		int newX = x;
		int newY = y;

		if (hasAnchor(Anchor::LEFT)) {
			newX = mAnchorOffsetLeft;
		} else if (hasAnchor(Anchor::RIGHT)) {
			newX = windowW - getWidth() - mAnchorOffsetRight;
		}

		if (hasAnchor(Anchor::TOP)) {
			newY = mAnchorOffsetTop;
		} else if (hasAnchor(Anchor::BOTTOM)) {
			newY = windowH - getHeight() - mAnchorOffsetBottom;
		}

		x = newX;
		y = newY;
		layoutChildren();
		mApplyingAnchor = false;
	}

	// Enable a simple header bar that shows the panel label and allows dragging.
	void setHeaderEnabled(bool enable = true, int headerHeight = 24) {
		mHeaderEnabled = enable;
		mHeaderHeight = headerHeight;
		mStyle.height = mHeaderEnabled ? mHeaderHeight : 0;
		layoutChildren();
	}

	bool hasHeader() const { return mHeaderEnabled; }

	// Control whether the panel can be dragged by its header.
	void setDraggable(bool enable = true) {
		mDraggable = enable;
		if (!mDraggable) mDragging = false;
	}
	bool getDraggable() const { return mDraggable; }

	// Prevent dragging the panel outside the window bounds.
	// Calling this opts the panel out of inheriting the root setting.
	void setClampDragToWindow(bool enable = true) { mClampDragOverride = true; mClampDragToWindow = enable; }
	// Return to inheriting the root clamp setting.
	void setClampDragToWindowInherit() { mClampDragOverride = false; }
	bool getClampDragToWindow() const { return mClampDragOverride ? mClampDragToWindow : isRootClampingPanels(); }

	void update(bool acceptEvents = true) override {
		// Panels are layout-only containers; we don't want them to
		// steal mouse presses from their children. The base
		// ofxDatGuiComponent::update() uses mStyle.height as a
		// "header" height for containers (anything below that is
		// treated as child region and should belong to children).
		// If we have no header, collapse the header height to zero while updating.

		float oldHeight = mStyle.height;
		if (!mHeaderEnabled) mStyle.height = 0;

		// Delegate interaction/update to container traversal (handles text-input locks).
		ofxDatGuiContainer::update(acceptEvents);

		// Restore whatever height the theme/layout wants to use for drawing/layout.
		mStyle.height = oldHeight;
	}


	void draw() override {
		if (!mVisible) return;

		ofPushStyle();

		ofColor headerColor = mStyle.color.panelHeader;

		// Paint a single backdrop covering the whole panel so spacing gaps aren't transparent.
		int panelHeight = mHeight;
		if (panelHeight <= 0 && mHeaderEnabled) panelHeight = mHeaderHeight;
		if (panelHeight > 0) {
			ofSetColor(mStyle.color.panelBackground, mStyle.opacity);
			ofDrawRectangle(x, y, mStyle.width, panelHeight);
		}

		if (mHeaderEnabled) {
			ofSetColor(headerColor, mStyle.opacity);
			ofDrawRectangle(x, y, mStyle.width, mHeaderHeight);
			ofSetColor(mLabel.color);
			if (mFont) {
				mFont->draw(mLabel.rendered, x + mLabel.x, y + mHeaderHeight/2 + mLabel.rect.height/2);
			}
		}

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
	ofxDatGuiFolder* addFolder(const std::string & label, ofColor color = ofColor::white);

	ofxDatGuiLabel* addLabel(const std::string & label) {
		auto item = std::make_unique<ofxDatGuiLabel>(label);
		item->setStripeColor(mStyle.stripe.color);
		return attachOwned(std::move(item));
	}

	ofxDatGui2dPad* add2dPad(const std::string & label) {
		auto item = std::make_unique<ofxDatGui2dPad>(label);
		item->setStripeColor(mStyle.stripe.color);
		return attachOwned(std::move(item));
	}

	ofxDatGuiWaveMonitor* addWaveMonitor(const std::string & label, float frequency, float amplitude) {
		auto item = std::make_unique<ofxDatGuiWaveMonitor>(label, frequency, amplitude);
		item->setStripeColor(mStyle.stripe.color);
		return attachOwned(std::move(item));
	}

	ofxDatGuiValuePlotter* addValuePlotter(const std::string & label, float min, float max) {
		auto item = std::make_unique<ofxDatGuiValuePlotter>(label, min, max);
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
			if (frac > 0.6f) frac = 0.6f; // limit label dominance to keep sliders/inputs roomy
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

		// If anchored, reposition after any size change (guarded to avoid recursion).
		if (mAnchorMask != 0 && !mApplyingAnchor) {
			applyAnchor(ofGetWidth(), ofGetHeight());
		}
	}

	// Internal events coming from children (visibility changes, etc.).
	void onInternalChildEvent(ofxDatGuiInternalEvent e) {
		if (e.type == ofxDatGuiEventType::VISIBILITY_CHANGED || e.type == ofxDatGuiEventType::GROUP_TOGGLED) {
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
		if (mHeaderEnabled && mDraggable) {
			bool inHeader = m.x >= x && m.x <= x + mStyle.width && m.y >= y && m.y <= y + mHeaderHeight;
			if (inHeader) {
				mDragging = true;
				mDragOffset.set(m.x - x, m.y - y);
			}
		}
		ofxDatGuiComponent::onMousePress(m);
	}
	void onMouseDrag(ofVec3f m) override {
		if (mHeaderEnabled && mDraggable && mDragging) {
			int newX = static_cast<int>(m.x - mDragOffset.x);
			int newY = static_cast<int>(m.y - mDragOffset.y);

			bool clamp = mClampDragOverride ? mClampDragToWindow : isRootClampingPanels();

			if (clamp) {
				const int winW = ofGetWidth();
				const int winH = ofGetHeight();
				const int w = getWidth();
				int h = getHeight();
				if (h <= 0 && mHeaderEnabled) h = mHeaderHeight;

				const int minVisW = std::min(w, getRootClampMinVisibleWidth());
				const int minVisH = std::min(h, getRootClampMinVisibleHeight());

				if (winW > 0 && w > 0) {
					int maxX = winW - minVisW;
					int minX = -std::max(0, w - minVisW);
					newX = std::clamp(newX, minX, maxX);
				}
				if (winH > 0 && h > 0) {
					int maxY = winH - minVisH;
					// Never allow the panel to move above the top edge; keep header reachable.
					int minY = 0;
					newY = std::clamp(newY, minY, maxY);
				}
			}

			setPosition(newX, newY);
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

	bool hasAnchor(Anchor anchor) const {
		return (mAnchorMask & static_cast<int>(anchor)) != 0;
	}

	void cacheAnchorOffsets(int windowW, int windowH) {
		if (mAnchorMask == 0) return;

		if (hasAnchor(Anchor::LEFT)) {
			mAnchorOffsetLeft = x;
		}
		if (hasAnchor(Anchor::TOP)) {
			mAnchorOffsetTop = y;
		}
		if (hasAnchor(Anchor::RIGHT)) {
			mAnchorOffsetRight = std::max(0, windowW - (x + getWidth()));
		}
		if (hasAnchor(Anchor::BOTTOM)) {
			mAnchorOffsetBottom = std::max(0, windowH - (y + getHeight()));
		}
	}


	Orientation mOrientation;
	int mHeight;
	int mSpacing;
	bool mHeaderEnabled;
	int mHeaderHeight;
	bool mDragging = false;
	bool mDraggable = true;
	bool mClampDragOverride = false;
	bool mClampDragToWindow = false;
	ofPoint mDragOffset;

	// Anchoring state
	int mAnchorMask = 0;
	int mAnchorOffsetLeft = 0;
	int mAnchorOffsetRight = 0;
	int mAnchorOffsetTop = 0;
	int mAnchorOffsetBottom = 0;
	bool mApplyingAnchor = false;

	// Helper to take ownership and return raw for convenience.
	template<typename T>
	T* attachOwned(std::unique_ptr<T> item) {
		if (!item) return nullptr;
		auto* raw = item.release(); // attachItem will wrap in ComponentPtr
		attachItem(raw);
		return raw;
	}
};
