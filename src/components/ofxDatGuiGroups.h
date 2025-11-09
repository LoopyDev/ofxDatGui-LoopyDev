/*
    Copyright (C) 2015 Stephen Braitsch [http://braitsch.io]

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.
*/

#pragma once
#include "ofxDatGuiLabel.h"
#include "ofxDatGuiButton.h"
#include "ofxDatGuiSlider.h"
#include "ofxDatGuiTextInput.h"
#include "ofxDatGuiFRM.h"
#include "ofxDatGui2dPad.h"
#include "ofxDatGuiColorPicker.h"
#include "ofxDatGuiMatrix.h"
#include "ofxDatGuiTimeGraph.h"
#include "ofxDatGuiScrollView.h"
// LoopyDev's additions
#include "ofxDatGuiCubicBezier.h"
#include "ofxDatGuiRadioGroup.h"

enum class ofxDatGuiDropdownBehavior {
    SelectCloses,   // default: existing behavior (select & collapse)
    RadioStaysOpen  // new: radio-like, mutually-exclusive, keep open
};

class ofxDatGuiGroup : public ofxDatGuiButton {

    public:
        ofxDatGuiGroup(string label) : ofxDatGuiButton(label), mHeight(0)
        {
            mIsExpanded = false;
            layout();
        }
    
        ~ofxDatGuiGroup()
        {
        // color pickers are deleted automatically when the group is destroyed //
            for (auto i:children) if (i->getType() != ofxDatGuiType::COLOR_PICKER) delete i;
        }
    
        void setPosition(int x, int y)
        {
            ofxDatGuiComponent::setPosition(x, y);
            layout();
        }
    
        void expand()
        {
            mIsExpanded = true;
            layout();
            onGroupToggled();
        }
    
        void toggle()
        {
            mIsExpanded = !mIsExpanded;
            layout();
            onGroupToggled();
        }
    
        void collapse()
        {
            mIsExpanded = false;
            layout();
            onGroupToggled();
        }
    
        int getHeight()
        {
            return mHeight;
        }
    
        bool getIsExpanded()
        {
            return mIsExpanded;
        }
    
        void draw()
        {
            if (mVisible){
                ofPushStyle();
                ofxDatGuiButton::draw();
                if (mIsExpanded) {
                    int mHeight = mStyle.height;
                    ofSetColor(mStyle.guiBackground, mStyle.opacity);
                    ofDrawRectangle(x, y+mHeight, mStyle.width, mStyle.vMargin);
                    for(int i=0; i<children.size(); i++) {
                        mHeight += mStyle.vMargin;
                        children[i]->draw();
                        mHeight += children[i]->getHeight();
                        if (i == children.size()-1) break;
                        ofSetColor(mStyle.guiBackground, mStyle.opacity);
                        ofDrawRectangle(x, y+mHeight, mStyle.width, mStyle.vMargin);
                    }
                    ofSetColor(mIcon.color);
                    mIconOpen->draw(x+mIcon.x, y+mIcon.y, mIcon.size, mIcon.size);
                    for(int i=0; i<children.size(); i++) children[i]->drawColorPicker();
                }   else{
                    ofSetColor(mIcon.color);
                    mIconClosed->draw(x+mIcon.x, y+mIcon.y, mIcon.size, mIcon.size);
                }
                ofPopStyle();
            }
        }
    
    protected:
    
        void layout()
        {
            mHeight = mStyle.height + mStyle.vMargin;
            for (int i=0; i<children.size(); i++) {
                if (children[i]->getVisible() == false) continue;
                if (mIsExpanded == false){
                    children[i]->setPosition(x, y + mHeight);
                }   else{
                    children[i]->setPosition(x, y + mHeight);
                    mHeight += children[i]->getHeight() + mStyle.vMargin;
                }
                if (i == children.size()-1) mHeight -= mStyle.vMargin;
            }
        }
    
        void onMouseRelease(ofPoint m)
        {
            if (mFocused){
            // open & close the group when its header is clicked //
                ofxDatGuiComponent::onFocusLost();
                ofxDatGuiComponent::onMouseRelease(m);
                mIsExpanded ? collapse() : expand();
            }
        }
    
    	void onGroupToggled()
	   	{
        // dispatch an event out to the gui panel to adjust its children //
            if (internalEventCallback != nullptr){
                ofxDatGuiInternalEvent e(ofxDatGuiEventType::GROUP_TOGGLED, mIndex);
                internalEventCallback(e);
            }
    	}
    
        void dispatchInternalEvent(ofxDatGuiInternalEvent e)
        {
            if (e.type == ofxDatGuiEventType::VISIBILITY_CHANGED) layout();
            internalEventCallback(e);
        }
    
        int mHeight;
        shared_ptr<ofImage> mIconOpen;
        shared_ptr<ofImage> mIconClosed;
        bool mIsExpanded;
    
};

class ofxDatGuiFolder : public ofxDatGuiGroup {

    public:
    
        ofxDatGuiFolder(string label, ofColor color = ofColor::white) : ofxDatGuiGroup(label)
        {
        // all items within a folder share the same stripe color //
            mStyle.stripe.color = color;
            mType = ofxDatGuiType::FOLDER;
            setTheme(ofxDatGuiComponent::getTheme());
        }
    
        void setTheme(const ofxDatGuiTheme* theme)
        {
            setComponentStyle(theme);
            mIconOpen = theme->icon.groupOpen;
            mIconClosed = theme->icon.groupClosed;
            setWidth(theme->layout.width, theme->layout.labelWidth);
        // reassign folder color to all components //
            for(auto i:children) i->setStripeColor(mStyle.stripe.color);
        }
    
        void setWidth(int width, float labelWidth = 1)
        {
            ofxDatGuiComponent::setWidth(width, labelWidth);
            mLabel.width = mStyle.width;
            mLabel.rightAlignedXpos = mIcon.x - mLabel.margin;
            ofxDatGuiComponent::positionLabel();
        }
    
        void drawColorPicker()
        {
            for(int i=0; i<pickers.size(); i++) pickers[i]->drawColorPicker();
        }
    
        void dispatchButtonEvent(ofxDatGuiButtonEvent e)
        {
            if (buttonEventCallback != nullptr) {
                buttonEventCallback(e);
            }   else{
                ofxDatGuiLog::write(ofxDatGuiMsg::EVENT_HANDLER_NULL);
            }
        }
    
        void dispatchToggleEvent(ofxDatGuiToggleEvent e)
        {
            if (toggleEventCallback != nullptr) {
                toggleEventCallback(e);
        // allow toggle events to decay into button events //
            }   else if (buttonEventCallback != nullptr) {
                buttonEventCallback(ofxDatGuiButtonEvent(e.target));
            }   else{
                ofxDatGuiLog::write(ofxDatGuiMsg::EVENT_HANDLER_NULL);
            }
        }
    
        void dispatchSliderEvent(ofxDatGuiSliderEvent e)
        {
            if (sliderEventCallback != nullptr) {
                sliderEventCallback(e);
            }   else{
                ofxDatGuiLog::write(ofxDatGuiMsg::EVENT_HANDLER_NULL);
            }
        }
    
        void dispatchTextInputEvent(ofxDatGuiTextInputEvent e)
        {
            if (textInputEventCallback != nullptr) {
                textInputEventCallback(e);
            }   else{
                ofxDatGuiLog::write(ofxDatGuiMsg::EVENT_HANDLER_NULL);
            }
        }
    
        void dispatchColorPickerEvent(ofxDatGuiColorPickerEvent e)
        {
            if (colorPickerEventCallback != nullptr) {
                colorPickerEventCallback(e);
            }   else{
                ofxDatGuiLog::write(ofxDatGuiMsg::EVENT_HANDLER_NULL);
            }
        }
    
        void dispatch2dPadEvent(ofxDatGui2dPadEvent e)
        {
            if (pad2dEventCallback != nullptr) {
                pad2dEventCallback(e);
            }   else{
                ofxDatGuiLog::write(ofxDatGuiMsg::EVENT_HANDLER_NULL);
            }
        }
    
        void dispatchMatrixEvent(ofxDatGuiMatrixEvent e)
        {
            if (matrixEventCallback != nullptr) {
                matrixEventCallback(e);
            }   else{
                ofxDatGuiLog::write(ofxDatGuiMsg::EVENT_HANDLER_NULL);
            }
        }

		// Loopydev - ofxDatGuiCubicBezier.h
		void dispatchCubicBezierEvent(ofxDatGuiCubicBezierEvent e) {
			if (cubicBezierEventCallback != nullptr) {
				cubicBezierEventCallback(e);
			} else {
				ofxDatGuiLog::write(ofxDatGuiMsg::EVENT_HANDLER_NULL);
			}
		}
		// --- RadioGroup event bridge ---
		using RadioGroupCB = std::function<void(ofxDatGuiRadioGroupEvent)>;

		template <typename T>
		void onRadioGroupEvent(T * listener, void (T::*handler)(ofxDatGuiRadioGroupEvent)) {
			radioGroupEventCallback = std::bind(handler, listener, std::placeholders::_1);
		}
		void onRadioGroupEvent(RadioGroupCB cb) { radioGroupEventCallback = std::move(cb); }

		void dispatchRadioGroupEvent(ofxDatGuiRadioGroupEvent e) {
			if (radioGroupEventCallback) {
				radioGroupEventCallback(e);
			} else {
				ofxDatGuiLog::write(ofxDatGuiMsg::EVENT_HANDLER_NULL);
			}
		}


    /*
        component add methods
    */

        ofxDatGuiLabel* addLabel(string label)
        {
            ofxDatGuiLabel* lbl = new ofxDatGuiLabel(label);
            lbl->setStripeColor(mStyle.stripe.color);
            attachItem(lbl);
            return lbl;
        }

        ofxDatGuiButton* addButton(string label)
        {
            ofxDatGuiButton* button = new ofxDatGuiButton(label);
            button->setStripeColor(mStyle.stripe.color);
            button->onButtonEvent(this, &ofxDatGuiFolder::dispatchButtonEvent);
            attachItem(button);
            return button;
        }
    
        ofxDatGuiToggle* addToggle(string label, bool enabled = false)
        {
            ofxDatGuiToggle* toggle = new ofxDatGuiToggle(label, enabled);
            toggle->setStripeColor(mStyle.stripe.color);
            toggle->onToggleEvent(this, &ofxDatGuiFolder::dispatchToggleEvent);
            attachItem(toggle);
            return toggle;
        }
    
        ofxDatGuiSlider* addSlider(string label, float min, float max)
        {
            ofxDatGuiSlider* slider = addSlider(label, min, max, (max+min)/2);
            return slider;
        }

        ofxDatGuiSlider* addSlider(string label, float min, float max, double val)
        {
            ofxDatGuiSlider* slider = new ofxDatGuiSlider(label, min, max, val);
            slider->setStripeColor(mStyle.stripe.color);
            slider->onSliderEvent(this, &ofxDatGuiFolder::dispatchSliderEvent);
            attachItem(slider);
            return slider;
        }

        ofxDatGuiSlider* addSlider(ofParameter<int> & p){
            ofxDatGuiSlider* slider = new ofxDatGuiSlider(p);
            slider->setStripeColor(mStyle.stripe.color);
            slider->onSliderEvent(this, &ofxDatGuiFolder::dispatchSliderEvent);
            attachItem(slider);
            return slider;
        }

        ofxDatGuiSlider* addSlider(ofParameter<float> & p){
            ofxDatGuiSlider* slider = new ofxDatGuiSlider(p);
            slider->setStripeColor(mStyle.stripe.color);
            slider->onSliderEvent(this, &ofxDatGuiFolder::dispatchSliderEvent);
            attachItem(slider);
            return slider;
        }
    
        ofxDatGuiTextInput* addTextInput(string label, string value)
        {
            ofxDatGuiTextInput* input = new ofxDatGuiTextInput(label, value);
            input->setStripeColor(mStyle.stripe.color);
            input->onTextInputEvent(this, &ofxDatGuiFolder::dispatchTextInputEvent);
            attachItem(input);
            return input;
        }
    
        ofxDatGuiColorPicker* addColorPicker(string label, ofColor color = ofColor::black)
        {
            shared_ptr<ofxDatGuiColorPicker> picker(new ofxDatGuiColorPicker(label, color));
            picker->setStripeColor(mStyle.stripe.color);
            picker->onColorPickerEvent(this, &ofxDatGuiFolder::dispatchColorPickerEvent);
            attachItem(picker.get());
            pickers.push_back(picker);
            return picker.get();
        }
    
        ofxDatGuiFRM* addFRM(float refresh = 1.0f)
        {
            ofxDatGuiFRM* monitor = new ofxDatGuiFRM(refresh);
            monitor->setStripeColor(mStyle.stripe.color);
            attachItem(monitor);
            return monitor;
        }

        ofxDatGuiBreak* addBreak()
        {
            ofxDatGuiBreak* brk = new ofxDatGuiBreak();
            attachItem(brk);
            return brk;
        }
    
        ofxDatGui2dPad* add2dPad(string label)
        {
            ofxDatGui2dPad* pad = new ofxDatGui2dPad(label);
            pad->setStripeColor(mStyle.stripe.color);
            pad->on2dPadEvent(this, &ofxDatGuiFolder::dispatch2dPadEvent);
            attachItem(pad);
            return pad;
        }

        ofxDatGuiMatrix* addMatrix(string label, int numButtons, bool showLabels = false)
        {
            ofxDatGuiMatrix* matrix = new ofxDatGuiMatrix(label, numButtons, showLabels);
            matrix->setStripeColor(mStyle.stripe.color);
            matrix->onMatrixEvent(this, &ofxDatGuiFolder::dispatchMatrixEvent);
            attachItem(matrix);
            return matrix;
        }
    
        ofxDatGuiWaveMonitor* addWaveMonitor(string label, float frequency, float amplitude)
        {
            ofxDatGuiWaveMonitor* monitor = new ofxDatGuiWaveMonitor(label, frequency, amplitude);
            monitor->setStripeColor(mStyle.stripe.color);
            attachItem(monitor);
            return monitor;
        }
    
        ofxDatGuiValuePlotter* addValuePlotter(string label, float min, float max)
        {
            ofxDatGuiValuePlotter* plotter = new ofxDatGuiValuePlotter(label, min, max);
            plotter->setStripeColor(mStyle.stripe.color);
            attachItem(plotter);
            return plotter;
        }

		// Loopydev - ofxDatGuiCubicBezier.h
		ofxDatGuiCubicBezier * addCubicBezier(string label, float x1 = 0.25f, float y1 = 0.1f, float x2 = 0.25f, float y2 = 1.0f) {
			ofxDatGuiCubicBezier * bez = new ofxDatGuiCubicBezier(label, x1, y1, x2, y2);
			bez->setStripeColor(mStyle.stripe.color);
			bez->onCubicBezierEvent(this, &ofxDatGuiFolder::dispatchCubicBezierEvent);
			attachItem(bez);
			return bez;
		}
		// Loopydev - ofxDatGuiRadioGroup.h
		ofxDatGuiRadioGroup * addRadioGroup(const std::string & label, const std::vector<std::string> & options) {
			auto * rg = new ofxDatGuiRadioGroup(label, options);
			rg->setStripeColor(mStyle.stripe.color);
			// bubble events up to this folder first
			rg->onRadioGroupEvent(this, &ofxDatGuiFolder::dispatchRadioGroupEvent);
			attachItem(rg);
			return rg;
		}

    
        void attachItem(ofxDatGuiComponent* item)
        {
            item->setIndex(children.size());
            item->onInternalEvent(this, &ofxDatGuiFolder::dispatchInternalEvent);
            children.push_back(item);
        }
    
        ofxDatGuiComponent* getComponent(ofxDatGuiType type, string label)
        {
            for (int i=0; i<children.size(); i++) {
                if (children[i]->getType() == type){
                    if (children[i]->is(label)) return children[i];
                }
            }
            return NULL;
        }

        static ofxDatGuiFolder* getInstance() { return new ofxDatGuiFolder("X"); }

		private:
		RadioGroupCB radioGroupEventCallback;

		//bool mHeaderPressed = false;

		//inline bool pointInHeader(const ofPoint & m) const {
		//	// header is the label bar: full width x mStyle.height tall
		//	return (m.x > x && m.x < x + mStyle.width && m.y > y && m.y < y + mStyle.height);
		//}

    protected:
			void onMouseRelease(ofPoint m) override {
				if (getMouseDown() && mFocused) { // require ownership of the press
					ofxDatGuiComponent::onFocusLost();
					ofxDatGuiComponent::onMouseRelease(m);
					mIsExpanded ? collapse() : expand();
				} else {
					ofxDatGuiComponent::onMouseRelease(m);
				}
			}
		//void onMousePress(ofPoint m) override {
		//	// Arm only if the press STARTED on the header
		//	mHeaderPressed = pointInHeader(m);

		//	if (mHeaderPressed) {
		//		// Set pressed + focus so getMouseDown() reflects a real header press
		//		ofxDatGuiComponent::onMousePress(m);
		//		if (!mFocused) onFocus(); // from ofxDatGuiComponent
		//	} else if (mIsExpanded) {
		//		// Forward to children when open
		//		for (auto * c : children)
		//			c->onMousePress(m);
		//	}
		//	// Do NOT call ofxDatGuiGroup/ofxDatGuiButton press here
		//}

		//void onMouseRelease(ofPoint m) override {
		//	const bool releaseOnHeader = pointInHeader(m);

		//	if (mHeaderPressed && releaseOnHeader) {
		//		// Valid header click -> toggle
		//		ofxDatGuiComponent::onMouseRelease(m); // clear pressed
		//		mIsExpanded ? collapse() : expand();
		//		ofxDatGuiComponent::onFocusLost(); // drop header focus
		//	} else {
		//		// Not a header click: just cleanup + forward to children
		//		ofxDatGuiComponent::onMouseRelease(m);
		//		if (mIsExpanded) {
		//			for (auto * c : children)
		//				c->onMouseRelease(m);
		//		}
		//	}
		//	mHeaderPressed = false;
		//}

        vector<shared_ptr<ofxDatGuiColorPicker>> pickers;
    
};

// class ofxDatGuiDropdownOption : public ofxDatGuiButton {

//     public:
    
//         ofxDatGuiDropdownOption(string label) : ofxDatGuiButton(label)
//         {
//             mType = ofxDatGuiType::DROPDOWN_OPTION;
//             setTheme(ofxDatGuiComponent::getTheme());
//         }
    
//         void setTheme(const ofxDatGuiTheme* theme)
//         {
//             ofxDatGuiButton::setTheme(theme);
//             mStyle.stripe.color = theme->stripe.dropdown;
//         }
    
//         void setWidth(int width, float labelWidth = 1)
//         {
//             ofxDatGuiComponent::setWidth(width, labelWidth);
//             mLabel.width = mStyle.width;
//             mLabel.rightAlignedXpos = mIcon.x - mLabel.margin;
//             ofxDatGuiComponent::positionLabel();
//         }

// };
class ofxDatGuiDropdownOption : public ofxDatGuiButton {

public:
    ofxDatGuiDropdownOption(string label)
    : ofxDatGuiButton(label)
    , mIsRadio(false)
    , mChecked(false)
    {
        mType = ofxDatGuiType::DROPDOWN_OPTION;
        setTheme(ofxDatGuiComponent::getTheme());
    }

    void setTheme(const ofxDatGuiTheme* theme) override
    {
        ofxDatGuiButton::setTheme(theme);
        mStyle.stripe.color = theme->stripe.dropdown;
    }

    void setWidth(int width, float labelWidth = 1) override
    {
        ofxDatGuiComponent::setWidth(width, labelWidth);
        mLabel.width = mStyle.width;
        mLabel.rightAlignedXpos = mIcon.x - mLabel.margin;
        ofxDatGuiComponent::positionLabel();
    }

    // ---- Radio support ----
    void setRadio(bool b) { mIsRadio = b; }
    bool isRadio() const { return mIsRadio; }

    void setChecked(bool b) { mChecked = b; }
    bool getChecked() const { return mChecked; }

    void draw() override
    {
        if (!mVisible) return;

        ofPushStyle();
        // Draw standard button background/hover/label
        ofxDatGuiButton::draw();

        // Draw radio indicator if enabled
        if (mIsRadio) {
            // radio circle at left margin inside label row
            float cx = x + 12;                      // circle center x
            float cy = y + mStyle.height * 0.5f;    // center vertically
            float r  = 6.0f;

            ofSetColor(mEnabled ? ofColor::white : ofColor(180));
            ofNoFill();
            ofDrawCircle(cx, cy, r);

            if (mChecked) {
                ofFill();
                ofDrawCircle(cx, cy, r * 0.55f);
            }

            // Nudge label right so it doesn't overlap the icon
            // (cheap & cheerful; avoids touching theme label metrics)
            // You can refine by exposing padding in theme.
            ofPushMatrix();
            ofTranslate(18, 0); // shift label drawing visually
            ofPopMatrix();
        }
        ofPopStyle();
    }

private:
    bool mIsRadio;
    bool mChecked;
};










//class ofxDatGuiDropdown : public ofxDatGuiGroup {
//
//    public:
//
//        ofxDatGuiDropdown(string label, const vector<string>& options = vector<string>()) : ofxDatGuiGroup(label)
//        {
//            mOption = 0;
//            mType = ofxDatGuiType::DROPDOWN;
//            for(int i=0; i<options.size(); i++){
//                ofxDatGuiDropdownOption* opt = new ofxDatGuiDropdownOption(options[i]);
//                opt->setIndex(children.size());
//                opt->onButtonEvent(this, &ofxDatGuiDropdown::onOptionSelected);
//                children.push_back(opt);
//            }
//            setTheme(ofxDatGuiComponent::getTheme());
//        }
//    
//        void setTheme(const ofxDatGuiTheme* theme)
//        {
//            setComponentStyle(theme);
//            mIconOpen = theme->icon.groupOpen;
//            mIconClosed = theme->icon.groupClosed;
//            mStyle.stripe.color = theme->stripe.dropdown;
//            setWidth(theme->layout.width, theme->layout.labelWidth);
//        }
//    
//        void setWidth(int width, float labelWidth = 1)
//        {
//            ofxDatGuiComponent::setWidth(width, labelWidth);
//            mLabel.width = mStyle.width;
//            mLabel.rightAlignedXpos = mIcon.x - mLabel.margin;
//            ofxDatGuiComponent::positionLabel();
//        }
//    
//        void select(int cIndex)
//        {
//        // ensure value is in range //
//            if (cIndex < 0 || cIndex >= children.size()){
//                ofLogError() << "ofxDatGuiDropdown->select("<<cIndex<<") is out of range";
//            }   else{
//                setLabel(children[cIndex]->getLabel());
//            }
//        }
//
//        int size()
//        {
//            return children.size();
//        }
//    
//        ofxDatGuiDropdownOption* getChildAt(int index)
//        {
//            return static_cast<ofxDatGuiDropdownOption*>(children[index]);
//        }
//    
//        ofxDatGuiDropdownOption* getSelected()
//        {
//            return static_cast<ofxDatGuiDropdownOption*>(children[mOption]);
//        }
//    
//        void dispatchEvent()
//        {
//            if (dropdownEventCallback != nullptr) {
//                ofxDatGuiDropdownEvent e(this, mIndex, mOption);
//                dropdownEventCallback(e);
//            }   else{
//                ofxDatGuiLog::write(ofxDatGuiMsg::EVENT_HANDLER_NULL);
//            }
//        }
//    
//        static ofxDatGuiDropdown* getInstance() { return new ofxDatGuiDropdown("X"); }
//    
//    private:
//    
//        void onOptionSelected(ofxDatGuiButtonEvent e)
//        {
//            for(int i=0; i<children.size(); i++) if (e.target == children[i]) mOption = i;
//            setLabel(children[mOption]->getLabel());
//           	collapse();
//            dispatchEvent();
//        }
//    
//        int mOption;
//    
//};
//
//
class ofxDatGuiDropdown : public ofxDatGuiGroup {

public:
	// New: optional behavior arg (defaults to legacy behavior)
	ofxDatGuiDropdown(string label,
		const vector<string> & options = vector<string>(),
		ofxDatGuiDropdownBehavior behavior = ofxDatGuiDropdownBehavior::SelectCloses)
		: ofxDatGuiGroup(label)
		, mOption(0)
		, mBehavior(behavior) {
		mType = ofxDatGuiType::DROPDOWN;
		for (int i = 0; i < options.size(); i++) {
			ofxDatGuiDropdownOption * opt = new ofxDatGuiDropdownOption(options[i]);
			opt->setIndex(children.size());
			opt->onButtonEvent(this, &ofxDatGuiDropdown::onOptionSelected);
			// if we’re in radio mode, configure option as radio
			if (mBehavior == ofxDatGuiDropdownBehavior::RadioStaysOpen) {
				opt->setRadio(true);
			}
			children.push_back(opt);
		}
		setTheme(ofxDatGuiComponent::getTheme());

		// Ensure exactly one checked in radio mode
		if (mBehavior == ofxDatGuiDropdownBehavior::RadioStaysOpen && size() > 0) {
			getChildAt(mOption)->setChecked(true);
		}
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
		if (cIndex < 0 || cIndex >= children.size()) {
			ofLogError() << "ofxDatGuiDropdown->select(" << cIndex << ") is out of range";
			return;
		}

		// Track the selected index in both modes
		mOption = cIndex;

		if (mBehavior == ofxDatGuiDropdownBehavior::SelectCloses) {
			setLabel(children[cIndex]->getLabel());
			collapse();
		} else {
			// Radio mode: enforce exclusivity but DON'T collapse.
			// --- REPLACEMENT LOOP STARTS HERE ---
			for (int i = 0; i < children.size(); ++i) {
				if (auto * t = dynamic_cast<ofxDatGuiToggle *>(children[i])) {
					t->setChecked(i == cIndex);
				} else if (auto * opt = dynamic_cast<ofxDatGuiDropdownOption *>(children[i])) {
					if (opt->isRadio()) opt->setChecked(i == cIndex);
				}
			}
			// --- REPLACEMENT LOOP ENDS HERE ---

			// (Optional) keep header label static in radio mode
			// setLabel(children[cIndex]->getLabel());
		}

		dispatchEvent();
	}


	int size() { return children.size(); }

	ofxDatGuiDropdownOption * getChildAt(int index) {
		return static_cast<ofxDatGuiDropdownOption *>(children[index]);
	}

	ofxDatGuiDropdownOption * getSelected() {
		return static_cast<ofxDatGuiDropdownOption *>(children[mOption]);
	}

	void dispatchEvent() {
		if (dropdownEventCallback != nullptr) {
			ofxDatGuiDropdownEvent e(this, mIndex, mOption);
			dropdownEventCallback(e);
		} else {
			ofxDatGuiLog::write(ofxDatGuiMsg::EVENT_HANDLER_NULL);
		}
	}

	static ofxDatGuiDropdown * getInstance() { return new ofxDatGuiDropdown("X"); }

	// --- New convenience toggles ---
	//void setRadioMode(bool enabled) {
	//	mBehavior = enabled ? ofxDatGuiDropdownBehavior::RadioStaysOpen
	//						: ofxDatGuiDropdownBehavior::SelectCloses;
	//	// Update all children
	//	for (int i = 0; i < children.size(); ++i) {
	//		auto * opt = static_cast<ofxDatGuiDropdownOption *>(children[i]);
	//		opt->setRadio(enabled);
	//		opt->setChecked(enabled && (i == mOption));
	//	}
	//}
	void setRadioMode(bool enabled) {
		// switch behavior
		mBehavior = enabled ? ofxDatGuiDropdownBehavior::RadioStaysOpen
							: ofxDatGuiDropdownBehavior::SelectCloses;

		// If we want true ofxDatGuiToggle children in radio mode, rebuild children.
		if (mUseToggleChildren && enabled) {
			rebuildAs(/*useToggles=*/true);
		} else if (mUseToggleChildren && !enabled) {
			// go back to button-style options
			rebuildAs(/*useToggles=*/false);
		} else {
			// legacy path: keep DropdownOption items, just toggle their radio flags
			for (int i = 0; i < children.size(); ++i) {
				auto * opt = static_cast<ofxDatGuiDropdownOption *>(children[i]);
				opt->setRadio(enabled);
				opt->setChecked(enabled && (i == mOption));
			}
		}
	}

	bool isRadioMode() const { return mBehavior == ofxDatGuiDropdownBehavior::RadioStaysOpen; }

	// Handle clicks from real Toggle children (radio mode)
	void onToggleSelected(ofxDatGuiToggleEvent e) {
		int clicked = mOption;
		for (int i = 0; i < children.size(); ++i) {
			if (e.target == children[i]) {
				clicked = i;
				break;
			}
		}
		select(clicked); // enforce exclusivity + dispatch
		// (no collapse in radio mode)
	}

private:
	void onOptionSelected(ofxDatGuiButtonEvent e) {
		int clicked = mOption;
		for (int i = 0; i < children.size(); i++)
			if (e.target == children[i]) {
				clicked = i;
				break;
			}
		select(clicked);
		// NOTE:
		// - select() now handles collapse behavior based on mBehavior
		// - still dispatches event
	}

	 // Rebuild all children either as DropdownOption (buttons) or real Toggles.
	void rebuildAs(bool useToggles) {
		// keep labels and current selection
		std::vector<std::string> labels;
		labels.reserve(children.size());
		for (auto * c : children)
			labels.push_back(c->getLabel());

		// delete old (non-color-picker) children and clear
		for (auto * c : children) {
			if (c->getType() != ofxDatGuiType::COLOR_PICKER) delete c;
		}
		children.clear();

		// recreate
		for (int i = 0; i < (int)labels.size(); ++i) {
			if (useToggles) {
				auto * t = new ofxDatGuiToggle(labels[i], i == mOption);
				t->setIndex(children.size());
				// Use dropdown’s event path: map Toggle -> dropdown selection
				t->onToggleEvent(this, &ofxDatGuiDropdown::onToggleSelected);
				children.push_back(t);
			} else {
				auto * opt = new ofxDatGuiDropdownOption(labels[i]);
				opt->setIndex(children.size());
				opt->onButtonEvent(this, &ofxDatGuiDropdown::onOptionSelected);
				if (isRadioMode()) opt->setRadio(true);
				children.push_back(opt);
			}
		}

		// Reapply theme/layout to children and check the current choice
		setTheme(ofxDatGuiComponent::getTheme());
		if (isRadioMode()) {
			// ensure exactly one checked when in radio mode
			for (int i = 0; i < children.size(); ++i) {
				if (useToggles) {
					if (auto * t = dynamic_cast<ofxDatGuiToggle *>(children[i])) t->setChecked(i == mOption);
				} else {
					if (auto * o = dynamic_cast<ofxDatGuiDropdownOption *>(children[i])) o->setChecked(i == mOption);
				}
			}
		}
	}

	bool mUseToggleChildren = true; // << new: default to using real Toggles in radio mode



	int mOption;
	ofxDatGuiDropdownBehavior mBehavior;
};
