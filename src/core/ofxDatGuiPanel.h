#pragma once

#include "ofxDatGuiComponent.h"
#include "ofxDatGuiEvents.h"
#include <algorithm>

// Simple container panel that lays out its children either
// vertically (one per row) or horizontally (in a single row).
// Meant to be embedded inside folders/groups or used on its own.
class ofxDatGuiPanel : public ofxDatGuiComponent {
public:
	// Layout direction for children.
	enum class Orientation {
		VERTICAL,
		HORIZONTAL
	};

	explicit ofxDatGuiPanel(Orientation orientation = Orientation::VERTICAL)
		: ofxDatGuiComponent("")
		, mOrientation(orientation)
		, mHeight(0)
		, mSpacing(0) {
		mType = ofxDatGuiType::PANEL;

		// Initialize spacing & base style from the global default theme
		// so layout() has sane values even before setTheme() is called.
		const ofxDatGuiTheme * t = ofxDatGuiComponent::getTheme();
		if (t != nullptr) {
			setComponentStyle(t);
			mSpacing = t->layout.vMargin;
		}
	}

	virtual ~ofxDatGuiPanel() {
		// Mirror ofxDatGuiGroup / Folder semantics:
		// delete non-color-picker children (color pickers are usually
		// managed by shared_ptr in folders).
		for (auto * c : children) {
			if (c->getType() != ofxDatGuiType::COLOR_PICKER) {
				delete c;
			}
		}
		children.clear();
	}

	// ---------------------------------------------------------------------
	// Orientation
	// ---------------------------------------------------------------------

	void setOrientation(Orientation orientation) {
		if (mOrientation == orientation) return;
		mOrientation = orientation;
		layout();
	}

	Orientation getOrientation() const { return mOrientation; }

	// ---------------------------------------------------------------------
	// Core virtuals
	// ---------------------------------------------------------------------

	void setTheme(const ofxDatGuiTheme * theme) override {
		if (theme == nullptr) {
			theme = ofxDatGuiComponent::getTheme();
		}

		// Use theme only to pick up spacing & font etc.
		setComponentStyle(theme);
		mSpacing = theme->layout.vMargin;

		// Propagate theme to children
		for (auto * c : children) {
			c->setTheme(theme);
		}

		layout();
	}

	void setPosition(int px, int py) override {
		// IMPORTANT: do NOT call base setPosition(), because that will
		// auto-stack children vertically. We fully control child layout.
		x = px;
		y = py;
		layout();
	}

	void setWidth(int width, float labelWidth = 1.f) override {
		// Let base set our own width & internal style, but we do NOT want
		// the default behaviour of forcing all children to this width,
		// because we are going to lay them out ourselves.
		//
		// So: temporarily clear our children list, call base setWidth,
		// then restore children and relayout.
		auto savedChildren = children;
		children.clear();

		ofxDatGuiComponent::setWidth(width, labelWidth);

		children = std::move(savedChildren);

		layout();
	}

	int getHeight() override {
		return mHeight;
	}

	bool getIsExpanded() override {
		// Panels don’t implement collapse/expand; treat as always expanded.
		return true;
	}

	void update(bool acceptEvents = true) override {
		// Let the base class handle focus, mouse, keyboard, etc.
		ofxDatGuiComponent::update(acceptEvents);
		// Base update already walks our children via getIsExpanded()==true,
		// so we don't need to manually loop children here.
	}

	void draw() override {
		if (!mVisible) return;

		// By default, panel is just a transparent container.
		// If you want a framed look, you could call drawBackground()/drawBorder() here.
		for (auto * c : children) {
			if (c->getVisible()) {
				c->draw();
			}
		}
	}

	// ---------------------------------------------------------------------
	// Child management
	// ---------------------------------------------------------------------

	// Attach an existing component as a child of this panel.
	// The panel takes ownership and deletes it in the destructor
	// (except for color pickers, matching folder/group semantics).
	void attachItem(ofxDatGuiComponent * item) {
		if (!item) return;

		item->setIndex(static_cast<int>(children.size()));
		// Route child internal events via this panel first.
		item->onInternalEvent(this, &ofxDatGuiPanel::onInternalChildEvent);
		children.push_back(item);
		layout();
	}

	// Read-only access to children if you ever need to poke from outside.
	const std::vector<ofxDatGuiComponent *> & getChildren() const {
		return children;
	}

protected:
	// ---------------------------------------------------------------------
	// Layout
	// ---------------------------------------------------------------------

	void layout() {
		mHeight = 0;

		if (children.empty()) {
			return;
		}

		if (mOrientation == Orientation::VERTICAL) {
			int cursorY = y;
			const int spacing = mSpacing;

			for (auto * c : children) {
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
			for (auto * c : children) {
				if (c->getVisible()) visible.push_back(c);
			}

			if (visible.empty()) {
				mHeight = 0;
				return;
			}

			int availableWidth = mStyle.width;
			if (availableWidth <= 0) {
				// Fallback: at least make them butt up against each other
				// using their own widths.
				int cursorX = x;
				int rowHeight = 0;
				const int spacing = mSpacing;

				for (auto * c : visible) {
					c->setPosition(cursorX, y);
					cursorX += c->getWidth() + spacing;
					rowHeight = std::max(rowHeight, c->getHeight());
				}

				mHeight = rowHeight;
				return;
			}

			const int spacing = mSpacing;
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
			layout();
		}

		// Bubble the event up if someone is listening to this panel.
		if (internalEventCallback != nullptr) {
			internalEventCallback(e);
		}
	}

	Orientation mOrientation;
	int mHeight;
	int mSpacing;
};
