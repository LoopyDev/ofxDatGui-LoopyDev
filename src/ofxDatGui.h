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
#include "ofxDatGuiGroups.h"
#include "ofxDatGuiDropdown.h"
// LoopyDev's Additions
//#include "ofxDatGuiRadioGroup.h"
#include "ofxDatGuiButtonBar.h"
#include "ofxDatGuiPanel.h"
#include <memory>


class ofxDatGui : public ofxDatGuiInteractiveObject
{
    
    public:
        using ComponentPtr = std::unique_ptr<ofxDatGuiComponent>;
        enum class Orientation {
            VERTICAL,
            HORIZONTAL
        };
    
        ofxDatGui(); // stack-friendly default (no anchor)
        ofxDatGui(int x, int y);
        explicit ofxDatGui(ofxDatGuiAnchor anchor);
        ~ofxDatGui();

        // ofxGui-like setup helpers so callers can default-construct and configure later.
        void setup(); // defaults to NO_ANCHOR
        void setup(int x, int y); // manual position
        void setup(ofxDatGuiAnchor anchor);
    
        void draw();
        void update();
        void focus();
        void expand();
        void toggle();
        void collapse();
    
        void setWidth(int width, float labelWidth = 0.35f);
        void setVisible(bool visible);
        void setEnabled(bool enabled);
        void setOpacity(float opacity);
        void setPosition(int x, int y);
        void setPosition(ofxDatGuiAnchor anchor);
        void setTheme(ofxDatGuiTheme* t, bool applyImmediately = false);
        void setTheme(std::unique_ptr<ofxDatGuiTheme> t, bool applyImmediately = true);
        void setAutoDraw(bool autodraw, int priority = 0);
        void setManualLayout(bool manual) { mManualLayout = manual; }
        void setBringToFrontOnInteract(bool enable) { ensureSetup(); mBringToFrontOnInteract = enable; }
        void setMuteUnfocusedPanels(bool enable) { ensureSetup(); mMuteUnfocusedPanels = enable; }
        void setActiveOnHover(bool enable) { ensureSetup(); mActiveOnHover = enable; }
        void relayout(); // explicit layout recompute without repositioning children
        void setLabelAlignment(ofxDatGuiAlignment align);

		void setOrientation(Orientation orientation);
		Orientation getOrientation() const { return mOrientation; }

        static void setAssetPath(string path);
        static string getAssetPath();
    
        int getWidth();
        int getHeight();
        bool getFocused();
        bool getVisible();
        bool getAutoDraw();
        bool getMouseDown();
        void setMouseCapture(ofxDatGuiComponent* c);
        ofxDatGuiComponent* getMouseCapture() const;
        ofPoint getPosition();
    
        ofxDatGuiHeader* addHeader(string label = "", bool draggable = true);
        ofxDatGuiFooter* addFooter();
        ofxDatGuiLabel* addLabel(string label);
        ofxDatGuiButton* addButton(string label);
        ofxDatGuiToggle* addToggle(string label, bool state = false);
        ofxDatGuiSlider* addSlider(string label, float min, float max);
        ofxDatGuiSlider* addSlider(string label, float min, float max, float val);
        ofxDatGuiSlider* addSlider(ofParameter<int> & p);
        ofxDatGuiSlider* addSlider(ofParameter<float> & p);
        ofxDatGuiTextInput* addTextInput(string label, string value = "");
        ofxDatGuiDropdown* addDropdown(string label, vector<string> options);
        ofxDatGuiFRM* addFRM(float refresh = 1.0f);
        ofxDatGuiBreak* addBreak();
        ofxDatGui2dPad* add2dPad(string label);
        ofxDatGui2dPad* add2dPad(string label, ofRectangle bounds);
        ofxDatGuiWaveMonitor* addWaveMonitor(string label, float min, float max);
        ofxDatGuiValuePlotter* addValuePlotter(string label, float min, float max);
        ofxDatGuiColorPicker* addColorPicker(string label, ofColor color = ofColor::black);
        ofxDatGuiMatrix* addMatrix(string label, int numButtons, bool showLabels = false);
        ofxDatGuiFolder* addFolder(string label, ofColor color = ofColor::white);
        ofxDatGuiFolder* addFolder(ofxDatGuiFolder* folder);
        // LoopyDev: add cubic-bezier editor
        ofxDatGuiCubicBezier* addCubicBezier(string label,
            float x1 = 0.25f, float y1 = 0.10f,
            float x2 = 0.25f, float y2 = 1.00f,
            float padAspect = 1.0f)
        {
            auto bez = makeOwned<ofxDatGuiCubicBezier>(label, x1, y1, x2, y2, padAspect);
            auto * raw = static_cast<ofxDatGuiCubicBezier*>(bez.get());
            // If you later want GUI-level routing, you can add a callback like other components.
            attachItem(std::move(bez));
            return raw;
        }
        // LoopyDev: add Radio Groups
        ofxDatGuiRadioGroup * addRadioGroup(const std::string & label, const std::vector<std::string> & options);
        // LoopyDev: add Curve Editor
        ofxDatGuiCurveEditor * addCurveEditor(string label, float padAspect) {
            auto curve = makeOwned<ofxDatGuiCurveEditor>("Response Curve", 0.6f /*padAspect*/);
            auto * raw = static_cast<ofxDatGuiCurveEditor*>(curve.get());
            attachItem(std::move(curve));
            return raw;
        };
		// --- LoopyDev: add horizontal button bar ---
        ofxDatGuiButtonBar * addButtonBar(const std::string & label,
			const std::vector<std::string> & buttons);
		// LoopyDev: add panel
		ofxDatGuiPanel * addPanel(ofxDatGuiPanel::Orientation orientation = ofxDatGuiPanel::Orientation::VERTICAL);
        ofxDatGuiPanel& createPanel(const std::string& label = "", ofxDatGuiPanel::Orientation orientation = ofxDatGuiPanel::Orientation::VERTICAL);

        ofxDatGuiHeader* getHeader();
        ofxDatGuiFooter* getFooter();
        ofxDatGuiLabel* getLabel(string label, string folder = "");
        ofxDatGuiButton* getButton(string label, string folder = "");
        ofxDatGuiToggle* getToggle(string label, string folder = "");
        ofxDatGuiSlider* getSlider(string label, string folder = "");
        ofxDatGui2dPad* get2dPad(string label, string folder = "");
        ofxDatGuiTextInput* getTextInput(string label, string folder = "");
        ofxDatGuiColorPicker* getColorPicker(string label, string folder = "");
        ofxDatGuiMatrix* getMatrix(string label, string folder = "");
        ofxDatGuiWaveMonitor* getWaveMonitor(string label, string folder = "");
        ofxDatGuiValuePlotter* getValuePlotter(string label, string folder = "");
        ofxDatGuiFolder* getFolder(string label);
        ofxDatGuiDropdown* getDropdown(string label);
		// LoopyDev: get Radio Groups
		ofxDatGuiRadioGroup * getRadioGroup(string label);
		// LoopyDev: get Button Bars
		ofxDatGuiButtonBar * getButtonBar(string label);

    private:
    
        int mIndex;
        int mWidth;
        int mHeight;
        int mRowSpacing;
        float mAlpha;
        float mLabelWidth;
        bool mMoving;
        bool mIsSetup = false;
        bool mVisible;
        bool mEnabled;
        bool mExpanded;
        bool mAutoDraw;
        bool mMouseDown;
        bool mManualLayout = true;
        bool mBringToFrontOnInteract = false;
        bool mMuteUnfocusedPanels = false;
        bool mActiveOnHover = false;
        bool mUserWidthSet = false;
        ofxDatGuiComponent* mLastFocusedPanel = nullptr;
        ofxDatGuiComponent* mMouseCaptureOwner = nullptr;
        bool mAlphaChanged;
        bool mWidthChanged;
        bool mThemeChanged;
        bool mAlignmentChanged;
        ofColor mGuiBackground;

		Orientation mOrientation;

        ofPoint mPosition;
        ofRectangle mGuiBounds;
        ofxDatGuiAnchor mAnchor;
        ofxDatGuiHeader* mGuiHeader;
        ofxDatGuiFooter* mGuiFooter;
        std::unique_ptr<ofxDatGuiTheme> mOwnedTheme;
        std::unique_ptr<ofxDatGuiTheme> mPendingOwnedTheme;
        const ofxDatGuiTheme* mBorrowedTheme = nullptr;
        ofxDatGuiTheme* mPendingBorrowedTheme = nullptr;
        ofxDatGuiAlignment mAlignment;
        std::vector<ComponentPtr> items;
        vector<ofxDatGuiComponent*> trash;
        static ofxDatGui* mActiveGui;
        static vector<ofxDatGui*> mGuis;
        static std::unique_ptr<ofxDatGuiTheme> theme;
    
        void init();
        void ensureSetup();
        void layoutGui();
    	void positionGui();
        void moveGui(ofPoint pt);
        bool hitTest(ofPoint pt);
        void attachItem(ComponentPtr item, bool applyTheme = true);
        void bringItemToFront(ofxDatGuiComponent* component);
        const ofxDatGuiTheme* getActiveTheme() const;
        void setWidthInternal(int width, float labelWidth, bool markUser);
        void applyThemeWidth(int width, float labelWidth);
        template<typename T, typename... Args>
        std::unique_ptr<T> makeOwned(Args&&... args) {
            return std::make_unique<T>(std::forward<Args>(args)...);
        }
        void applyThemeRecursive(ofxDatGuiComponent* node, const ofxDatGuiTheme* t);
    
        void onDraw(ofEventArgs &e);
        void onUpdate(ofEventArgs &e);
        void onWindowResized(ofResizeEventArgs &e);
    
        ofxDatGuiComponent* getComponent(string key);
        ofxDatGuiComponent* getComponent(ofxDatGuiType type, string label);
        void onInternalEventCallback(ofxDatGuiInternalEvent e);
        void onButtonEventCallback(ofxDatGuiButtonEvent e);
        void onToggleEventCallback(ofxDatGuiToggleEvent e);
        void onSliderEventCallback(ofxDatGuiSliderEvent e);
        void onTextInputEventCallback(ofxDatGuiTextInputEvent e);
        void onDropdownEventCallback(ofxDatGuiDropdownEvent e);
        void on2dPadEventCallback(ofxDatGui2dPadEvent e);
        void onColorPickerEventCallback(ofxDatGuiColorPickerEvent e);
        void onMatrixEventCallback(ofxDatGuiMatrixEvent e);
		// LoopyDev: Radio Groups
		void onRadioGroupEventCallback(ofxDatGuiRadioGroupEvent e);
		std::function<void(ofxDatGuiRadioGroupEvent)> radioGroupEventCallback;
		template <typename T>
		void onRadioGroupEvent(T * listener, void (T::*handler)(ofxDatGuiRadioGroupEvent)) {
			radioGroupEventCallback = std::bind(handler, listener, std::placeholders::_1);
		}


};
