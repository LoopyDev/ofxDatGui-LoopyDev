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
#include <unordered_map>
#include <cstdint>
#include <initializer_list>


class ofxDatGui : public ofxDatGuiInteractiveObject
{
    
    public:
        using ComponentPtr = std::unique_ptr<ofxDatGuiComponent>;
        enum class Orientation {
            VERTICAL,
            HORIZONTAL
        };
        enum class SlideEdge : uint8_t { LEFT=1, RIGHT=2, TOP=4, BOTTOM=8 };
        using SlideMask = uint8_t;
        static constexpr SlideMask SlideAll = static_cast<SlideMask>(SlideEdge::LEFT) |
                                              static_cast<SlideMask>(SlideEdge::RIGHT) |
                                              static_cast<SlideMask>(SlideEdge::TOP) |
                                              static_cast<SlideMask>(SlideEdge::BOTTOM);
    
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
    
        void setVisible(bool visible);
        void setEnabled(bool enabled);
        void setOpacity(float opacity);
        void setTheme(ofxDatGuiTheme* t, bool applyImmediately = false);
        void setTheme(std::unique_ptr<ofxDatGuiTheme> t, bool applyImmediately = true);
        void setAutoDraw(bool autodraw, int priority = 0);
        void setManualLayout(bool manual) { mManualLayout = manual; }
        void setBringToFrontOnInteract(bool enable) { ensureSetup(); mBringToFrontOnInteract = enable; }
        void setMuteUnfocusedPanels(bool enable) { ensureSetup(); mMuteUnfocusedPanels = enable; }
        void setActiveOnHover(bool enable) { ensureSetup(); mActiveOnHover = enable; }
        void setClampPanelsToWindow(bool enable) { ensureSetup(); mClampPanelsToWindow = enable; }
        bool getClampPanelsToWindow() const { return mClampPanelsToWindow; }
        void setClampPanelsMinVisible(int minWidth, int minHeight) { ensureSetup(); mClampPanelsMinVisibleWidth = std::max(0, minWidth); mClampPanelsMinVisibleHeight = std::max(0, minHeight); }
        int getClampPanelsMinVisibleWidth() const { return mClampPanelsMinVisibleWidth; }
        int getClampPanelsMinVisibleHeight() const { return mClampPanelsMinVisibleHeight; }
        // Slide panels off-screen toward their nearest edge; respectClamp decides whether to leave the min-visible area on-screen.
        void slidePanelsOffscreen(bool respectClamp = true, bool animate = false, SlideMask allowedEdges = SlideAll);
        // Convenience overload: pass allowed edges as a list of flags.
        void slidePanelsOffscreen(bool respectClamp, bool animate, std::initializer_list<SlideEdge> allowedEdges);
        // Restore panels to their original positions after a slide.
        void slidePanelsBack(bool animate = false);
        // Whether panels are currently sliding or held off-screen.
        bool isSlidingPanels() const { return mSlideAnimating || mPanelsSlidOut; }
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
        ofxDatGuiComponent* getTextInputFocus() const { return mFocusedTextInput; }
        bool isTextInputFocusActive() const { return mFocusedTextInput != nullptr; }
        bool isAnyTextInputActive() const { return mFocusedTextInput != nullptr; }
        bool isInTextInputFocusBranch(const ofxDatGuiComponent* c) const;
    
        // Panel-only API: root no longer hosts components directly.
		// LoopyDev: add panel
		ofxDatGuiPanel * addPanel(ofxDatGuiPanel::Orientation orientation = ofxDatGuiPanel::Orientation::VERTICAL);
        ofxDatGuiPanel& createPanel(const std::string& label = "", ofxDatGuiPanel::Orientation orientation = ofxDatGuiPanel::Orientation::VERTICAL);

        // Component getters removed in panel-only mode. Retrieve components via panel references.

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
        bool mClampPanelsToWindow = false;
        int mClampPanelsMinVisibleWidth = 0;
        int mClampPanelsMinVisibleHeight = 0;
        bool mPanelsSlidOut = false;
        bool mSlideRespectClamp = true;
        std::unordered_map<ofxDatGuiComponent*, ofPoint> mSavedPanelPositions;
        struct SlideAnimTarget {
            ofPoint start;
            ofPoint target;
            float startOpacity;
            float targetOpacity;
        };
        std::unordered_map<ofxDatGuiComponent*, SlideAnimTarget> mSlideAnimTargets;
        bool mSlideAnimating = false;
        bool mSlideToOff = false;
        float mSlideProgress = 0.f;
        float mSlideDuration = 0.25f; // seconds
        float mSlideHiddenOpacity = 0.5f;
        std::unordered_map<ofxDatGuiComponent*, float> mSavedPanelOpacities;
        bool mUserWidthSet = false;
        ofxDatGuiComponent* mLastFocusedPanel = nullptr;
        ofxDatGuiComponent* mFocusedTextInput = nullptr;
        ofxDatGuiComponent* mMouseCaptureOwner = nullptr;
        bool mAlphaChanged;
        bool mWidthChanged;
        bool mThemeChanged;
        bool mAlignmentChanged;
        ofColor mGuiBackground;

		Orientation mOrientation;

        ofPoint mPosition;
        ofRectangle mGuiBounds;
        // Root now only manages panels; no header/footer or anchor.
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
