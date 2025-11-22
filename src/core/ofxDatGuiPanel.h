#pragma once

#include "ofxDatGuiContainer.h"
#include "ofxDatGuiEvents.h"
#include <algorithm>

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
		, mSpacing(0) {
		mType = ofxDatGuiType::PANEL;

		// Initialize spacing & base style from the global default theme
		// so layoutChildren() has sane values even before setTheme() is called.
		const ofxDatGuiTheme * t = ofxDatGuiComponent::getTheme();
		if (t != nullptr) {
			setComponentStyle(t);
			mSpacing = t->layout.vMargin;
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
		// Let base set our own width & internal style, but we do NOT want
		// the default behaviour of forcing all children to this width,
		// because we are going to lay them out ourselves.
		//
		// So: temporarily clear our children list, call base setWidth,
		// then restore children and relayout.
		std::vector<ComponentPtr> savedChildren;
		savedChildren.swap(children);

		ofxDatGuiComponent::setWidth(width, labelWidth);

		children.swap(savedChildren);

		layoutChildren();
	}

	int getHeight() override {
		return mHeight;
	}

	bool getIsExpanded() override {
		// Panels don't implement collapse/expand; treat as always expanded.
		return true;
	}

	void update(bool acceptEvents = true) override {
		// Panels are layout-only containers; we don't want them to
		// steal mouse presses from their children. The base
		// ofxDatGuiComponent::update() uses mStyle.height as a
		// "header" height for containers (anything below that is
		// treated as child region and should belong to children).
		//
		// Our panel places children starting at y (no header),
		// so we temporarily collapse the header height to zero
		// while running the base event logic.

		float oldHeight = mStyle.height;
		mStyle.height = 0;

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

protected:
	// ---------------------------------------------------------------------
	// Layout
	// ---------------------------------------------------------------------

	void layoutChildren() override {
		mHeight = 0;

		if (children.empty()) {
			return;
		}

		if (mOrientation == Orientation::VERTICAL) {
			int cursorY = y;
			const int spacing = mSpacing;

			for (auto & c : children) {
				if (!c->getVisible()) continue;
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
				mHeight = 0;
				return;
			}

			int availableWidth = mStyle.width;
			const int spacing = mSpacing;

			if (availableWidth <= 0) {
				// Fallback: at least make them butt up against each other
				// using their own widths.
				int cursorX = x;
				int rowHeight = 0;

				for (auto * c : visible) {
					c->setPosition(cursorX, y);
					cursorX += c->getWidth() + spacing;
					rowHeight = std::max(rowHeight, c->getHeight());
				}

				mHeight = rowHeight;
				return;
			}

			const int count = static_cast<int>(visible.size());
			const int totalSpacing = spacing * std::max(0, count - 1);
			int childWidth = (availableWidth - totalSpacing) / std::max(1, count);
			if (childWidth < 1) childWidth = 1;

			int cursorX = x;
			int rowHeight = 0;

			for (int i = 0; i < count; ++i) {
				auto * c = visible[i];
				// Give each child the computed width.
				c->setWidth(childWidth, 1.f); // labelWidth not important here
				c->setPosition(cursorX, y);

				cursorX += childWidth;
				if (i + 1 < count) {
					cursorX += spacing;
				}

				rowHeight = std::max(rowHeight, c->getHeight());
			}

			mHeight = rowHeight;
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


	Orientation mOrientation;
	int mHeight;
	int mSpacing;
};
