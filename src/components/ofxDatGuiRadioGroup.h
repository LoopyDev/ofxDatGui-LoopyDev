#pragma once
#include "ofxDatGuiComponent.h"
#include "ofxDatGuiControls.h"
#include "ofxDatGuiEvents.h"

// Radio-style group: one selected option at a time.
// Header can be shown/hidden; options area is always expanded.
class ofxDatGuiRadioGroup : public ofxDatGuiComponent {
public:
	enum class Orientation {
		VERTICAL,
		HORIZONTAL
	};


	ofxDatGuiRadioGroup(const std::string & label, const std::vector<std::string> & options)
		: ofxDatGuiComponent(label)
		, mSelected(-1)
		, mHeaderVisible(true)
		, mOrientation(Orientation::VERTICAL) // <- NEW
	{
		mType = ofxDatGuiType::RADIO_GROUP;
		for (auto & s : options)
			addOption(s);
		setTheme(ofxDatGuiComponent::getTheme());
	}

	static ofxDatGuiRadioGroup * getInstance() { return new ofxDatGuiRadioGroup("X", {}); }

	    void setOrientation(Orientation orientation) {
		if (mOrientation == orientation) return;
		mOrientation = orientation;
		layoutChildren(); // recompute positions
	}

	Orientation getOrientation() const { return mOrientation; }


	// Header visibility (collapses/restore header hit area)
	void setHeaderVisible(bool visible) {
		if (mHeaderVisible == visible) return;
		mHeaderVisible = visible;

		if (!mHeaderVisible) {
			if (mHeaderHeightCache < 0) mHeaderHeightCache = mStyle.height;
			mStyle.height = 0; // hide header hit area
		} else {
			if (mStyle.height == 0) {
				int fallback = ofxDatGuiComponent::getTheme()->layout.height;
				mStyle.height = (mHeaderHeightCache > 0) ? mHeaderHeightCache : fallback;
			}
		}
		layoutChildren();
	}
	bool isHeaderVisible() const { return mHeaderVisible; }
	void hideHeader(bool hide = true) { setHeaderVisible(!hide); }

	ofxDatGuiToggle * addOption(const std::string & label) {
		auto * t = new ofxDatGuiToggle(label, false);
		t->setStripeVisible(false);
		t->onToggleEvent(this, &ofxDatGuiRadioGroup::onOptionToggled);
		children.push_back(t);
		mOptions.push_back(t);
		layoutChildren();
		return t;
	}

	void setSelectedIndex(int index) {
		if (index < 0 || index >= (int)mOptions.size()) return;
		if (mSelected == index) {
			mOptions[index]->setChecked(true);
			return;
		}
		mSelected = index;
		for (int i = 0; i < (int)mOptions.size(); ++i)
			mOptions[i]->setChecked(i == index);
		dispatch();
	}

	int getSelectedIndex() const { return mSelected; }
	std::string getSelectedLabel() const {
		return (mSelected >= 0 && mSelected < (int)mOptions.size()) ? mOptions[mSelected]->getLabel() : "";
	}

	// ---- ofxDatGuiComponent overrides ----
	void setTheme(const ofxDatGuiTheme * t) override {
		setComponentStyle(t);
		if (!mHeaderVisible) mStyle.height = 0;
		layoutChildren();
	}

	void setWidth(int width, float labelWidth = 1.f) override {
		ofxDatGuiComponent::setWidth(width, labelWidth);
		for (auto * opt : mOptions)
			opt->setWidth(mStyle.width, mLabel.width);
		layoutChildren();
	}

	void setPosition(int x, int y) override {
		ofxDatGuiComponent::setPosition(x, y);
		layoutChildren();
	}

	    int getHeight() override {
		int h = mHeaderVisible ? mStyle.height : 0;

		if (mOptions.empty()) {
			return h;
		}

		if (mOrientation == Orientation::VERTICAL) {
			for (auto * opt : mOptions)
				h += opt->getHeight();
		} else {
			// HORIZONTAL: one row, take max option height
			int rowHeight = 0;
			for (auto * opt : mOptions)
				rowHeight = std::max(rowHeight, opt->getHeight());
			h += rowHeight;
		}

		return h;
	}


	bool getIsExpanded() override { return true; }

	void update(bool acceptEvents = true) override {
		ofxDatGuiComponent::update(acceptEvents);
		for (auto * opt : mOptions)
			opt->update(acceptEvents);
	}

	void draw() override {
		if (!mVisible) return;
		drawBackground();
		if (mHeaderVisible) {
			drawLabel();
			drawStripe();
		}
		for (auto * opt : mOptions)
			opt->draw();
	}

	// Event hookup
	template <typename T>
	void onRadioGroupEvent(T * listener, void (T::*handler)(ofxDatGuiRadioGroupEvent)) {
		mEventCallback = std::bind(handler, listener, std::placeholders::_1);
	}

private:
	Orientation mOrientation;
	int mHeaderHeightCache = -1;
	std::vector<ofxDatGuiToggle *> mOptions;
	int mSelected;
	bool mHeaderVisible;
	std::function<void(ofxDatGuiRadioGroupEvent)> mEventCallback;

	    void layoutChildren() {
		if (mOptions.empty()) return;

		const int headerH = mHeaderVisible ? mStyle.height : 0;

		if (mOrientation == Orientation::VERTICAL) {
			// Original behaviour: stack options downwards
			int cursorY = this->y + headerH;
			for (auto * opt : mOptions) {
				opt->setLabelAlignment(mLabel.alignment);
				opt->setPosition(this->x, cursorY);
				opt->setWidth(mStyle.width, mLabel.width);
				cursorY += opt->getHeight();
			}
		} else {
			// HORIZONTAL: options laid out in a single row
			int count = static_cast<int>(mOptions.size());
			if (count <= 0) return;

			int availableWidth = mStyle.width;
			if (availableWidth <= 0) availableWidth = mStyle.width; // sanity

			int spacing = mStyle.vMargin; // reuse vMargin horizontally
			int totalSpacing = spacing * std::max(0, count - 1);

			int optWidth = (availableWidth - totalSpacing) / std::max(1, count);
			if (optWidth < 1) optWidth = 1;

			int cursorX = this->x;
			int baseY = this->y + headerH;

			for (int i = 0; i < count; ++i) {
				auto * opt = mOptions[i];
				opt->setLabelAlignment(mLabel.alignment);
				opt->setWidth(optWidth, mLabel.width);
				opt->setPosition(cursorX, baseY);

				cursorX += optWidth;
				if (i + 1 < count) cursorX += spacing;
			}
		}
	}


	void onOptionToggled(ofxDatGuiToggleEvent e) {
		int idx = -1;
		for (int i = 0; i < (int)mOptions.size(); ++i)
			if (mOptions[i] == e.target) {
				idx = i;
				break;
			}
		if (idx < 0) return;

		if (idx != mSelected) {
			mSelected = idx;
			for (int i = 0; i < (int)mOptions.size(); ++i)
				mOptions[i]->setChecked(i == idx);
			dispatch();
		} else {
			mOptions[idx]->setChecked(true); // keep radio semantics
		}
	}

	void dispatch() {
		if (!mEventCallback) return;
		ofxDatGuiRadioGroupEvent evt(this, mSelected, getSelectedLabel());
		mEventCallback(evt);
	}
};
