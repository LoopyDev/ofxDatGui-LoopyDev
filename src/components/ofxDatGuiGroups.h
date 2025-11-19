/*
    ofxDatGuiGroups.h
*/

#pragma once

#include "ofxDatGui2dPad.h"
#include "ofxDatGuiButton.h"
#include "ofxDatGuiColorPicker.h"
#include "ofxDatGuiFRM.h"
#include "ofxDatGuiLabel.h"
#include "ofxDatGuiMatrix.h"
#include "ofxDatGuiScrollView.h"
#include "ofxDatGuiSlider.h"
#include "ofxDatGuiTextInput.h"
#include "ofxDatGuiTimeGraph.h"

// LoopyDev's Edits
//#include "ofxDatGuiDropdown.h"

// LoopyDev's Additions
#include "ofxDatGuiCubicBezier.h"
#include "ofxDatGuiRadioGroup.h"
#include "ofxDatGuiCurveEditor.h"
#include "ofxDatGuiPanel.h"

//enum class ofxDatGuiDropdownBehavior {
//	SelectCloses, // legacy behavior: select and collapse
//	RadioStaysOpen // radio-like: mutually exclusive, keep open
//};

// Forward Declarations
class ofxDatGuiDropdown;


// -----------------------------------------------------------------------------
// Group
// -----------------------------------------------------------------------------
class ofxDatGuiGroup : public ofxDatGuiButton {

public:
	ofxDatGuiGroup(string label)
		: ofxDatGuiButton(label)
		, mHeight(0)
		, mIsExpanded(false)
		, mHeaderPressed(false)
		, mToggledThisPress(false) {
		layout();
	}

	~ofxDatGuiGroup() {
		// delete non-picker children (pickers are managed by shared_ptrs in folders)
		for (auto i : children)
			if (i->getType() != ofxDatGuiType::COLOR_PICKER) delete i;
	}

	void setPosition(int x, int y) {
		ofxDatGuiComponent::setPosition(x, y);
		layout();
	}

	void expand() {
		mIsExpanded = true;
		layout();
		onGroupToggled();
	}
	void collapse() {
		ofxDatGuiComponent::clearGlobalPressOwner();
		mIsExpanded = false;
		layout();
		onGroupToggled();
	}
	void toggle() {
		mIsExpanded = !mIsExpanded;
		layout();
		onGroupToggled();
	}

	int getHeight() { return mHeight; }
	bool getIsExpanded() { return mIsExpanded; }

	void draw() {
		if (!mVisible) return;

		ofPushStyle();
		ofxDatGuiButton::draw();

		if (mIsExpanded) {
			int mh = mStyle.height;
			ofSetColor(mStyle.guiBackground, mStyle.opacity);
			ofDrawRectangle(x, y + mh, mStyle.width, mStyle.vMargin);

			for (int i = 0; i < (int)children.size(); i++) {
				mh += mStyle.vMargin;
				children[i]->draw();
				mh += children[i]->getHeight();
				if (i == (int)children.size() - 1) break;
				ofSetColor(mStyle.guiBackground, mStyle.opacity);
				ofDrawRectangle(x, y + mh, mStyle.width, mStyle.vMargin);
			}

			ofSetColor(mIcon.color);
			mIconOpen->draw(x + mIcon.x, y + mIcon.y, mIcon.size, mIcon.size);

			for (int i = 0; i < (int)children.size(); i++)
				children[i]->drawColorPicker();
		} else {
			ofSetColor(mIcon.color);
			mIconClosed->draw(x + mIcon.x, y + mIcon.y, mIcon.size, mIcon.size);
		}
		ofPopStyle();
	}

protected:
	void layout() {
		mHeight = mStyle.height + mStyle.vMargin;
		for (int i = 0; i < (int)children.size(); i++) {
			if (!children[i]->getVisible()) continue;
			children[i]->setPosition(x, y + mHeight);
			if (mIsExpanded) mHeight += children[i]->getHeight() + mStyle.vMargin;
			if (i == (int)children.size() - 1) mHeight -= mStyle.vMargin;
		}
	}

	inline bool pointInHeader(const ofPoint & m) const {
		return (m.x > x && m.x < x + mStyle.width && m.y > y && m.y < y + mStyle.height);
	}

	// Header press latch: arm only when press begins on header.
	void onMousePress(ofPoint m) override {
		mHeaderPressed = pointInHeader(m);
		mToggledThisPress = false;
		ofxDatGuiButton::onMousePress(m);
		if (mHeaderPressed && !mFocused) onFocus();
	}

	void onMouseDrag(ofPoint m) override {
		ofxDatGuiButton::onMouseDrag(m);
		// If you prefer to cancel when leaving header during drag, uncomment:
		// if (mHeaderPressed && !pointInHeader(m)) mHeaderPressed = false;
	}

	// Toggle once per press, only when released on header.
	void onMouseRelease(ofPoint m) override {
		const bool releaseOnHeader = pointInHeader(m);
		if (mHeaderPressed && releaseOnHeader && !mToggledThisPress) {
			mToggledThisPress = true;
			ofxDatGuiComponent::onMouseRelease(m);
			mIsExpanded ? collapse() : expand();
		} else {
			ofxDatGuiComponent::onMouseRelease(m);
		}
		ofxDatGuiComponent::onFocusLost();
		mHeaderPressed = false;
	}

	void onGroupToggled() {
		if (internalEventCallback != nullptr) {
			ofxDatGuiInternalEvent e(ofxDatGuiEventType::GROUP_TOGGLED, mIndex);
			internalEventCallback(e);
		}
	}

	void dispatchInternalEvent(ofxDatGuiInternalEvent e) {
		if (e.type == ofxDatGuiEventType::VISIBILITY_CHANGED) layout();
		internalEventCallback(e);
	}

	int mHeight = 0;
	shared_ptr<ofImage> mIconOpen;
	shared_ptr<ofImage> mIconClosed;

	bool mIsExpanded = false;
	bool mHeaderPressed = false;
	bool mToggledThisPress = false;
};

// -----------------------------------------------------------------------------
// Folder
// -----------------------------------------------------------------------------
class ofxDatGuiFolder : public ofxDatGuiGroup {

public:
	ofxDatGuiFolder(string label, ofColor color = ofColor::white)
		: ofxDatGuiGroup(label)
		, fHeaderPressed(false)
		, fToggledThisPress(false) {
		mStyle.stripe.color = color;
		mType = ofxDatGuiType::FOLDER;
		setTheme(ofxDatGuiComponent::getTheme());
	}

	void setTheme(const ofxDatGuiTheme * theme) override {
		setComponentStyle(theme);
		mIconOpen = theme->icon.groupOpen;
		mIconClosed = theme->icon.groupClosed;
		setWidth(theme->layout.width, theme->layout.labelWidth);
		for (auto i : children)
			i->setStripeColor(mStyle.stripe.color);
	}

	void setWidth(int width, float labelWidth = 1) override {
		ofxDatGuiComponent::setWidth(width, labelWidth);
		mLabel.width = mStyle.width;
		mLabel.rightAlignedXpos = mIcon.x - mLabel.margin;
		ofxDatGuiComponent::positionLabel();
	}

	void drawColorPicker() override {
		for (int i = 0; i < (int)pickers.size(); i++)
			pickers[i]->drawColorPicker();
	}

	// Event dispatchers
	void dispatchButtonEvent(ofxDatGuiButtonEvent e) {
		if (buttonEventCallback)
			buttonEventCallback(e);
		else
			ofxDatGuiLog::write(ofxDatGuiMsg::EVENT_HANDLER_NULL);
	}
	void dispatchToggleEvent(ofxDatGuiToggleEvent e) {
		if (toggleEventCallback)
			toggleEventCallback(e);
		else if (buttonEventCallback)
			buttonEventCallback(ofxDatGuiButtonEvent(e.target));
		else
			ofxDatGuiLog::write(ofxDatGuiMsg::EVENT_HANDLER_NULL);
	}
	void dispatchSliderEvent(ofxDatGuiSliderEvent e) {
		if (sliderEventCallback)
			sliderEventCallback(e);
		else
			ofxDatGuiLog::write(ofxDatGuiMsg::EVENT_HANDLER_NULL);
	}
	void dispatchTextInputEvent(ofxDatGuiTextInputEvent e) {
		if (textInputEventCallback)
			textInputEventCallback(e);
		else
			ofxDatGuiLog::write(ofxDatGuiMsg::EVENT_HANDLER_NULL);
	}
	void dispatchColorPickerEvent(ofxDatGuiColorPickerEvent e) {
		if (colorPickerEventCallback)
			colorPickerEventCallback(e);
		else
			ofxDatGuiLog::write(ofxDatGuiMsg::EVENT_HANDLER_NULL);
	}
	void dispatch2dPadEvent(ofxDatGui2dPadEvent e) {
		if (pad2dEventCallback)
			pad2dEventCallback(e);
		else
			ofxDatGuiLog::write(ofxDatGuiMsg::EVENT_HANDLER_NULL);
	}
	void dispatchMatrixEvent(ofxDatGuiMatrixEvent e) {
		if (matrixEventCallback)
			matrixEventCallback(e);
		else
			ofxDatGuiLog::write(ofxDatGuiMsg::EVENT_HANDLER_NULL);
	}
	void dispatchCubicBezierEvent(ofxDatGuiCubicBezierEvent e) {
		if (cubicBezierEventCallback)
			cubicBezierEventCallback(e);
		else
			ofxDatGuiLog::write(ofxDatGuiMsg::EVENT_HANDLER_NULL);
	}
	void dispatchCurveEditorEvent(ofxDatGuiCurveEditorEvent e) {
		if (curveEditorEventCallback)
			curveEditorEventCallback(e);
		else
			ofxDatGuiLog::write(ofxDatGuiMsg::EVENT_HANDLER_NULL);
	}

	// RadioGroup event bridge
	using RadioGroupCB = std::function<void(ofxDatGuiRadioGroupEvent)>;

	template <typename T>
	void onRadioGroupEvent(T * listener, void (T::*handler)(ofxDatGuiRadioGroupEvent)) {
		radioGroupEventCallback = std::bind(handler, listener, std::placeholders::_1);
	}
	void onRadioGroupEvent(RadioGroupCB cb) { radioGroupEventCallback = std::move(cb); }

	void dispatchRadioGroupEvent(ofxDatGuiRadioGroupEvent e) {
		if (radioGroupEventCallback)
			radioGroupEventCallback(e);
		else
			ofxDatGuiLog::write(ofxDatGuiMsg::EVENT_HANDLER_NULL);
	}

	// Dropdown event bridge
	using ofxDatGuiInteractiveObject::onDropdownEvent;


	using DropdownCB = std::function<void(ofxDatGuiDropdownEvent)>;
	template <typename T>
	void onDropdownEvent(T * listener, void (T::*handler)(ofxDatGuiDropdownEvent)) {
		dropdownEventCallback = std::bind(handler, listener, std::placeholders::_1);
	}
	void onDropdownEvent(DropdownCB cb) { dropdownEventCallback = std::move(cb); }
	void dispatchDropdownEvent(ofxDatGuiDropdownEvent e) {
		if (dropdownEventCallback)
			dropdownEventCallback(e);
		else
			ofxDatGuiLog::write(ofxDatGuiMsg::EVENT_HANDLER_NULL);
	}

	// Adders
	ofxDatGuiFolder * addFolder(string label, ofColor color = ofColor::white) {
		auto * sub = new ofxDatGuiFolder(label, color);
		sub->setStripeColor(mStyle.stripe.color);
		sub->onButtonEvent(this, &ofxDatGuiFolder::dispatchButtonEvent);
		sub->onToggleEvent(this, &ofxDatGuiFolder::dispatchToggleEvent);
		sub->onSliderEvent(this, &ofxDatGuiFolder::dispatchSliderEvent);
		sub->onTextInputEvent(this, &ofxDatGuiFolder::dispatchTextInputEvent);
		sub->onColorPickerEvent(this, &ofxDatGuiFolder::dispatchColorPickerEvent);
		sub->on2dPadEvent(this, &ofxDatGuiFolder::dispatch2dPadEvent);
		sub->onMatrixEvent(this, &ofxDatGuiFolder::dispatchMatrixEvent);
		sub->onInternalEvent(this, &ofxDatGuiFolder::dispatchInternalEvent);
		sub->onRadioGroupEvent(this, &ofxDatGuiFolder::dispatchRadioGroupEvent);
		sub->onCubicBezierEvent(this, &ofxDatGuiFolder::dispatchCubicBezierEvent);
		sub->onCurveEditorEvent(this, &ofxDatGuiFolder::dispatchCurveEditorEvent);

		attachItem(sub);
		return sub;
	}

	ofxDatGuiLabel * addLabel(string label) {
		auto * lbl = new ofxDatGuiLabel(label);
		lbl->setStripeColor(mStyle.stripe.color);
		attachItem(lbl);
		return lbl;
	}

	ofxDatGuiButton * addButton(string label) {
		auto * button = new ofxDatGuiButton(label);
		button->setStripeColor(mStyle.stripe.color);
		button->onButtonEvent(this, &ofxDatGuiFolder::dispatchButtonEvent);
		attachItem(button);
		return button;
	}

	ofxDatGuiToggle * addToggle(string label, bool enabled = false) {
		auto * toggle = new ofxDatGuiToggle(label, enabled);
		toggle->setStripeColor(mStyle.stripe.color);
		toggle->onToggleEvent(this, &ofxDatGuiFolder::dispatchToggleEvent);
		attachItem(toggle);
		return toggle;
	}

	ofxDatGuiSlider * addSlider(string label, float min, float max) {
		return addSlider(label, min, max, (max + min) / 2);
	}
	ofxDatGuiSlider * addSlider(string label, float min, float max, double val) {
		auto * slider = new ofxDatGuiSlider(label, min, max, val);
		slider->setStripeColor(mStyle.stripe.color);
		slider->onSliderEvent(this, &ofxDatGuiFolder::dispatchSliderEvent);
		attachItem(slider);
		return slider;
	}
	ofxDatGuiSlider * addSlider(ofParameter<int> & p) {
		auto * slider = new ofxDatGuiSlider(p);
		slider->setStripeColor(mStyle.stripe.color);
		slider->onSliderEvent(this, &ofxDatGuiFolder::dispatchSliderEvent);
		attachItem(slider);
		return slider;
	}
	ofxDatGuiSlider * addSlider(ofParameter<float> & p) {
		auto * slider = new ofxDatGuiSlider(p);
		slider->setStripeColor(mStyle.stripe.color);
		slider->onSliderEvent(this, &ofxDatGuiFolder::dispatchSliderEvent);
		attachItem(slider);
		return slider;
	}

	ofxDatGuiTextInput * addTextInput(string label, string value) {
		auto * input = new ofxDatGuiTextInput(label, value);
		input->setStripeColor(mStyle.stripe.color);
		input->onTextInputEvent(this, &ofxDatGuiFolder::dispatchTextInputEvent);
		attachItem(input);
		return input;
	}

	ofxDatGuiColorPicker * addColorPicker(string label, ofColor color = ofColor::black) {
		std::shared_ptr<ofxDatGuiColorPicker> picker(new ofxDatGuiColorPicker(label, color));
		picker->setStripeColor(mStyle.stripe.color);
		picker->onColorPickerEvent(this, &ofxDatGuiFolder::dispatchColorPickerEvent);
		attachItem(picker.get());
		pickers.push_back(picker);
		return picker.get();
	}

	ofxDatGuiFRM * addFRM(float refresh = 1.0f) {
		auto * monitor = new ofxDatGuiFRM(refresh);
		monitor->setStripeColor(mStyle.stripe.color);
		attachItem(monitor);
		return monitor;
	}

	ofxDatGuiBreak * addBreak() {
		auto * brk = new ofxDatGuiBreak();
		attachItem(brk);
		return brk;
	}

	ofxDatGui2dPad * add2dPad(string label) {
		auto * pad = new ofxDatGui2dPad(label);
		pad->setStripeColor(mStyle.stripe.color);
		pad->on2dPadEvent(this, &ofxDatGuiFolder::dispatch2dPadEvent);
		attachItem(pad);
		return pad;
	}

	ofxDatGuiMatrix * addMatrix(string label, int numButtons, bool showLabels = false) {
		auto * matrix = new ofxDatGuiMatrix(label, numButtons, showLabels);
		matrix->setStripeColor(mStyle.stripe.color);
		matrix->onMatrixEvent(this, &ofxDatGuiFolder::dispatchMatrixEvent);
		attachItem(matrix);
		return matrix;
	}

	ofxDatGuiWaveMonitor * addWaveMonitor(string label, float frequency, float amplitude) {
		auto * monitor = new ofxDatGuiWaveMonitor(label, frequency, amplitude);
		monitor->setStripeColor(mStyle.stripe.color);
		attachItem(monitor);
		return monitor;
	}

	ofxDatGuiValuePlotter * addValuePlotter(string label, float min, float max) {
		auto * plotter = new ofxDatGuiValuePlotter(label, min, max);
		plotter->setStripeColor(mStyle.stripe.color);
		attachItem(plotter);
		return plotter;
	}

	ofxDatGuiCubicBezier * addCubicBezier(string label,
		float x1 = 0.25f, float y1 = 0.1f,
		float x2 = 0.25f, float y2 = 1.0f) {
		auto * bez = new ofxDatGuiCubicBezier(label, x1, y1, x2, y2);
		bez->setStripeColor(mStyle.stripe.color);
		bez->onCubicBezierEvent(this, &ofxDatGuiFolder::dispatchCubicBezierEvent);
		attachItem(bez);
		return bez;
	}

	    ofxDatGuiCurveEditor * addCurveEditor(string label, float padAspect = 1.0f) {
		auto * ce = new ofxDatGuiCurveEditor(label, padAspect);
		ce->setStripeColor(mStyle.stripe.color);
		ce->onCurveEditorEvent(this, &ofxDatGuiFolder::dispatchCurveEditorEvent);
		attachItem(ce);
		return ce;
	}

	// Convenience overload to set initial points
	ofxDatGuiCurveEditor * addCurveEditor(string label,
		const std::vector<ofPoint> & points,
		float padAspect = 1.0f) {
		auto * ce = addCurveEditor(label, padAspect);
		ce->setPoints(points);
		return ce;
	}


	ofxDatGuiRadioGroup * addRadioGroup(const std::string & label, const std::vector<std::string> & options) {
		auto * rg = new ofxDatGuiRadioGroup(label, options);
		rg->setStripeColor(mStyle.stripe.color);
		rg->onRadioGroupEvent(this, &ofxDatGuiFolder::dispatchRadioGroupEvent);
		attachItem(rg);
		return rg;
	}

	ofxDatGuiDropdown * addDropdown(std::string label, const std::vector<std::string> & options);

	ofxDatGuiPanel * addPanel(ofxDatGuiPanel::Orientation orientation = ofxDatGuiPanel::Orientation::VERTICAL) {
		auto * p = new ofxDatGuiPanel(orientation);
		p->setStripeColor(mStyle.stripe.color);
		attachItem(p);
		return p;
	}


	void attachItem(ofxDatGuiComponent * item) {
		item->setIndex((int)children.size());
		item->onInternalEvent(this, &ofxDatGuiFolder::dispatchInternalEvent);
		children.push_back(item);
	}

	ofxDatGuiComponent * getComponent(ofxDatGuiType type, string label) {
		for (int i = 0; i < (int)children.size(); i++) {
			if (children[i]->getType() == type && children[i]->is(label)) return children[i];
		}
		return NULL;
	}

	static ofxDatGuiFolder * getInstance() { return new ofxDatGuiFolder("X"); }

private:
	DropdownCB dropdownEventCallback;

	RadioGroupCB radioGroupEventCallback;
	std::vector<std::shared_ptr<ofxDatGuiColorPicker>> pickers;

	bool fHeaderPressed = false;
	bool fToggledThisPress = false;

	inline bool pointInHeader(const ofPoint & m) const {
		return (m.x > x && m.x < x + mStyle.width && m.y > y && m.y < y + mStyle.height);
	}

protected:
	void onMousePress(ofPoint m) override {
		fHeaderPressed = pointInHeader(m);
		fToggledThisPress = false;
		ofxDatGuiComponent::onMousePress(m);
		if (fHeaderPressed && !mFocused) onFocus();
	}

	void onMouseDrag(ofPoint m) override {
		ofxDatGuiComponent::onMouseDrag(m);
		// To cancel toggle if leaving header while pressed, uncomment:
		// if (fHeaderPressed && !pointInHeader(m)) fHeaderPressed = false;
	}

	void onMouseRelease(ofPoint m) override {
		const bool releaseOnHeader = pointInHeader(m);
		if (fHeaderPressed && releaseOnHeader && !fToggledThisPress) {
			fToggledThisPress = true;
			ofxDatGuiComponent::onMouseRelease(m);
			mIsExpanded ? collapse() : expand();
		} else {
			ofxDatGuiComponent::onMouseRelease(m);
		}
		ofxDatGuiComponent::onFocusLost();
		fHeaderPressed = false;
	}
};
