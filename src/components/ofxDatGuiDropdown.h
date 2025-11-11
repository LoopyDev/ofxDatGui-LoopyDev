#pragma once
/*
    ofxDatGuiDropdown.h — no radio mode
    Minimal, production-ready dropdown that always: click -> select -> collapse.
*/

#include "ofxDatGuiButton.h"
#include "ofxDatGuiGroups.h"

// -----------------------------------------------------------------------------
// Dropdown Option (single row inside a dropdown)
// -----------------------------------------------------------------------------
class ofxDatGuiDropdownOption : public ofxDatGuiButton {
public:
	explicit ofxDatGuiDropdownOption(std::string label)
		: ofxDatGuiButton(std::move(label)) {
		mType = ofxDatGuiType::DROPDOWN_OPTION;
		setTheme(ofxDatGuiComponent::getTheme());
	}

	void setTheme(const ofxDatGuiTheme * theme) override {
		ofxDatGuiButton::setTheme(theme);
		mStyle.stripe.color = theme->stripe.dropdown;
	}

	void setWidth(int width, float labelWidth = 1) override {
		ofxDatGuiComponent::setWidth(width, labelWidth);
		mLabel.width = mStyle.width;
		mLabel.rightAlignedXpos = mIcon.x - mLabel.margin;
		ofxDatGuiComponent::positionLabel();
	}

	void draw() override {
		if (!mVisible) return;
		ofxDatGuiButton::draw();
	}
};

// -----------------------------------------------------------------------------
// Dropdown (header + list of options)
// -----------------------------------------------------------------------------
class ofxDatGuiDropdown : public ofxDatGuiGroup {
public:
	ofxDatGuiDropdown(std::string label,
		const std::vector<std::string> & options = {})
		: ofxDatGuiGroup(std::move(label))
		, mOption(0) {
		mType = ofxDatGuiType::DROPDOWN;

		for (int i = 0; i < static_cast<int>(options.size()); ++i) {
			auto * opt = new ofxDatGuiDropdownOption(options[i]);
			opt->setIndex(static_cast<int>(children.size()));
			opt->onButtonEvent(this, &ofxDatGuiDropdown::onOptionSelected);
			children.push_back(opt);
		}

		setTheme(ofxDatGuiComponent::getTheme());
	}

	void setTheme(const ofxDatGuiTheme * theme) override {
		setComponentStyle(theme);
		mIconOpen = theme->icon.groupOpen;
		mIconClosed = theme->icon.groupClosed;
		mStyle.stripe.color = theme->stripe.dropdown;
		setWidth(theme->layout.width, theme->layout.labelWidth);
	}

	void setWidth(int width, float labelWidth = 1) override {
		ofxDatGuiComponent::setWidth(width, labelWidth);
		mLabel.width = mStyle.width;
		mLabel.rightAlignedXpos = mIcon.x - mLabel.margin;
		ofxDatGuiComponent::positionLabel();
	}

	void select(int cIndex) {
		if (cIndex < 0 || cIndex >= static_cast<int>(children.size())) {
			ofLogError() << "ofxDatGuiDropdown->select(" << cIndex << ") out of range";
			return;
		}
		mOption = cIndex;
		setLabel(children[cIndex]->getLabel());
		collapse();
		dispatchEvent();
	}

	int size() const { return static_cast<int>(children.size()); }

	ofxDatGuiDropdownOption * getChildAt(int index) {
		return static_cast<ofxDatGuiDropdownOption *>(children[index]);
	}

	ofxDatGuiDropdownOption * getSelected() {
		return static_cast<ofxDatGuiDropdownOption *>(children[mOption]);
	}

	void dispatchEvent() {
		if (dropdownEventCallback) {
			ofxDatGuiDropdownEvent e(this, mIndex, mOption);
			dropdownEventCallback(e);
		} else {
			ofxDatGuiLog::write(ofxDatGuiMsg::EVENT_HANDLER_NULL);
		}
	}

	static ofxDatGuiDropdown * getInstance() { return new ofxDatGuiDropdown("X"); }

private:
	void onOptionSelected(ofxDatGuiButtonEvent e) {
		int clicked = mOption;
		for (int i = 0; i < static_cast<int>(children.size()); ++i) {
			if (e.target == children[i]) {
				clicked = i;
				break;
			}
		}
		select(clicked);
	}

private:
	int mOption;
};

// --- Folder helper to add dropdowns (definition) ---
inline ofxDatGuiDropdown * ofxDatGuiFolder::addDropdown(std::string label,
	const std::vector<std::string> & options) {
	auto * dd = new ofxDatGuiDropdown(std::move(label), options);
	dd->setStripeColor(mStyle.stripe.color);
	dd->onDropdownEvent(this, &ofxDatGuiFolder::dispatchDropdownEvent);
	attachItem(dd);
	return dd;
}
