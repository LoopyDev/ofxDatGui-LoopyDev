#pragma once
#include "ofxDatGuiButton.h"
#include "ofxDatGuiComponent.h"
#include "ofxDatGuiEvents.h" // for ofxDatGuiButtonEvent

// Horizontal bar of buttons with optional bottom stripes + radio-style mode.
class ofxDatGuiButtonBar : public ofxDatGuiComponent {
public:
	ofxDatGuiButtonBar(const std::string & label,
		const std::vector<std::string> & buttons)
		: ofxDatGuiComponent(label)
		, mLabelVisible(true)
		, mLabelFrac(1.f)
		, mRadioMode(false)
		, mSelectedIndex(-1) {
		mType = ofxDatGuiType::BUTTON_BAR;

		for (auto & s : buttons) {
			addButton(s);
		}

		setTheme(ofxDatGuiComponent::getTheme());
	}

	static ofxDatGuiButtonBar * getInstance() {
		return new ofxDatGuiButtonBar("X", {});
	}

	// Add a new button to the bar
	ofxDatGuiButton * addButton(const std::string & label) {
		auto * btn = new ofxDatGuiButton(label);

		// Use current theme + kill their own vertical stripe
		btn->setTheme(ofxDatGuiComponent::getTheme());
		btn->setStripeVisible(false);

		// Hook into the button event so we can implement "radio" behaviour.
		btn->onButtonEvent(this, &ofxDatGuiButtonBar::onButtonPressed);

		children.push_back(btn);
		mButtons.push_back(btn);

		layoutChildren();
		return btn;
	}

	// ---------------- ofxDatGuiComponent overrides ----------------

	void setTheme(const ofxDatGuiTheme * t) override {
		// Apply base style (sets mStyle, mLabel, etc.)
		setComponentStyle(t);

		// Remember how much of the width the label is using so we can
		// restore it if the user hides/shows the label later.
		if (mStyle.width > 0) {
			mLabelFrac = mLabel.width / mStyle.width;
		}

		// Propagate to child buttons
		for (auto * b : mButtons) {
			b->setTheme(t);
			b->setStripeVisible(false); // we draw our own bottom stripe
		}

		layoutChildren();
	}

	void setWidth(int width, float labelWidth = 1.f) override {
		// Store the intended label fraction from the caller.
		mLabelFrac = labelWidth;

		// If the label is hidden, don't reserve any width for it.
		float effectiveLabelFrac = mLabelVisible ? labelWidth : 0.f;

		ofxDatGuiComponent::setWidth(width, effectiveLabelFrac);
		layoutChildren();
	}

	void setPosition(int x, int y) override {
		this->x = x;
		this->y = y;
		layoutChildren();
	}

	int getHeight() override {
		// Single row, same height as component style
		return static_cast<int>(mStyle.height);
	}

	bool getIsExpanded() override {
		return false;
	}

	void update(bool acceptEvents = true) override {
		// Just update the child buttons
		for (auto * btn : mButtons) {
			if (btn->getVisible()) {
				btn->update(acceptEvents);
			}
		}
	}

	void draw() override {
		if (!mVisible) return;

		// Background for the whole bar
		drawBackground();
		drawBorder();
		// Do NOT call drawStripe() here — the vertical stripe is disabled;
		// we handle bottom stripes per button.

		// Optional label area on the left
		if (mLabelVisible) {
			drawLabel();
		}

		// Draw buttons + bottom stripes
		for (std::size_t i = 0; i < mButtons.size(); ++i) {
			auto * btn = mButtons[i];
			btn->draw();
			drawButtonBottomStripe(i);
		}
	}

	// Give access to the buttons if you want to hook them directly
	const std::vector<ofxDatGuiButton *> & getButtons() const { return mButtons; }

	// ---------------- Label visibility (and reclaim space) -------------------

	void setLabelVisible(bool visible) {
		if (visible == mLabelVisible) return;

		mLabelVisible = visible;
		mLabel.visible = visible;

		// Re-run width logic so label space is actually removed/added.
		int w = (mStyle.width > 0) ? static_cast<int>(mStyle.width) : getWidth();
		setWidth(w, mLabelFrac);
	}

	bool isLabelVisible() const { return mLabelVisible; }

	// ---------------- Radio-style behaviour ---------------------------------

	// Enable/disable radio mode (mutually exclusive selection).
	void setRadioMode(bool enabled = true) {
		mRadioMode = enabled;
		// Optionally you could reset selection here if you want:
		// if (!mRadioMode) mSelectedIndex = -1;
	}

	bool isRadioMode() const { return mRadioMode; }

	// Select a button by index (shows stripe for this one, hides others).
	void setSelectedIndex(int index) {
		if (index < 0 || index >= static_cast<int>(mButtons.size())) {
			mSelectedIndex = -1;
		} else {
			mSelectedIndex = index;
		}
	}

	int getSelectedIndex() const { return mSelectedIndex; }

	std::string getSelectedLabel() const {
		if (mSelectedIndex >= 0 && mSelectedIndex < static_cast<int>(mButtons.size())) {
			return mButtons[mSelectedIndex]->getLabel();
		}
		return "";
	}

private:
	struct ButtonBounds {
		int x = 0;
		int y = 0;
		int w = 0;
		int h = 0;
	};

	std::vector<ofxDatGuiButton *> mButtons;
	std::vector<ButtonBounds> mButtonBounds;

	bool mLabelVisible;
	float mLabelFrac; // fraction of total width that the label uses when visible

	bool mRadioMode; // if true: one selected button at a time
	int mSelectedIndex; // -1 = none

	// Lay out the buttons in a single horizontal row.
	// Lay out the buttons in a single horizontal row with even gaps.
	void layoutChildren() {
		if (mButtons.empty()) return;

		// Label area (if visible)
		int labelW = mLabelVisible ? static_cast<int>(mLabel.width) : 0;
		int barX = x + labelW;
		int barW = static_cast<int>(mStyle.width) - labelW;

		if (barW <= 0) return;

		int count = static_cast<int>(mButtons.size());
		if (count <= 0) return;

		// Horizontal spacing between buttons (integerised)
		int spacing = static_cast<int>(std::round(mStyle.padding));
		if (spacing < 0) spacing = 0;

		int totalSpacing = spacing * (count - 1);
		int availForButtons = barW - totalSpacing;
		if (availForButtons <= 0) return;

		// Base width for each button, plus leftover pixels to distribute
		int baseButtonW = availForButtons / count;
		int remainder = availForButtons - baseButtonW * count; // 0..count-1

		int buttonH = static_cast<int>(mStyle.height);
		int buttonY = y;

		mButtonBounds.resize(count);

		int cursorX = barX;
		for (int i = 0; i < count; ++i) {
			// Spread the leftover pixels over the first 'remainder' buttons
			int w = baseButtonW + (i < remainder ? 1 : 0);

			auto * btn = mButtons[i];
			btn->setWidth(w, 1.f);
			btn->setPosition(cursorX, buttonY);

			mButtonBounds[i].x = cursorX;
			mButtonBounds[i].y = buttonY;
			mButtonBounds[i].w = w;
			mButtonBounds[i].h = buttonH;

			cursorX += w + spacing;
		}
	}


	// Draw a horizontal stripe at the bottom of the i-th button, using the
	// component's stripe style (mStyle.stripe).
	void drawButtonBottomStripe(std::size_t index) {
		if (!mStyle.stripe.visible) return;
		if (index >= mButtonBounds.size()) return;

		// In radio mode: only draw for the selected button
		if (mRadioMode && static_cast<int>(index) != mSelectedIndex) return;

		const auto & b = mButtonBounds[index];
		if (b.w <= 0 || b.h <= 0) return;

		float stripeH = static_cast<float>(mStyle.stripe.width);
		if (stripeH <= 0) return;

		float sx = static_cast<float>(b.x);
		float sy = static_cast<float>(b.y + b.h) - stripeH;

		ofPushStyle();
		ofSetColor(mStyle.stripe.color);
		ofDrawRectangle(sx, sy, static_cast<float>(b.w), stripeH);
		ofPopStyle();
	}

	// Internal handler: called when any child button is clicked.
	void onButtonPressed(ofxDatGuiButtonEvent e) {
		int idx = -1;
		for (int i = 0; i < static_cast<int>(mButtons.size()); ++i) {
			if (mButtons[i] == e.target) {
				idx = i;
				break;
			}
		}
		if (idx < 0) return;

		if (mRadioMode) {
			// Radio semantics: always keep one selected; clicking the
			// selected one again keeps it selected.
			if (mSelectedIndex != idx) {
				mSelectedIndex = idx;
			}
		}
		// If you want normal button behaviour too, you can still use
		// per-button callbacks by grabbing mButtons[i] via getButtons().
	}
};
