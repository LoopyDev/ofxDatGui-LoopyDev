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
#include "ofxDatGuiIntObject.h"
#include <functional>

class ofxDatGui;

class ofxDatGuiComponent : public ofxDatGuiInteractiveObject
{
    public:

		// LoopyDev: Stripe config
		// Where the stripe is drawn around the component.
		enum class StripePosition {
			LEFT,
			RIGHT,
			TOP,
			BOTTOM
		};

        ofxDatGuiComponent(string label);
        virtual ~ofxDatGuiComponent();
    
        int     getX();
        int     getY();
        void    setIndex(int index);
        int     getIndex();
        void    setName(string name);
        string  getName();
        bool    is(string name);
    
        void    setLabel(string label);
        string  getLabel();
        void    setLabelColor(ofColor color);
        ofColor getLabelColor();
        void    setLabelUpperCase(bool toUpper);
        bool    getLabelUpperCase();
    
        void    setBackgroundColor(ofColor color);
        void    setBackgroundColorOnMouseOver(ofColor color);
        void    setBackgroundColorOnMouseDown(ofColor color);
        void    setBackgroundColors(ofColor bkgd, ofColor mOver, ofColor mDown);
    
        void    setStripe(ofColor color, int width);
        void    setStripeWidth(int width);
        void    setStripeColor(ofColor color);
        void    setStripeVisible(bool visible);
		// LoopyDev: Stripe config
		void setStripePosition(StripePosition position);
		StripePosition getStripePosition() const;
		ofColor getStripeColor() const { return mStyle.stripe.color; }
		int getStripeWidth() const { return mStyle.stripe.width; }
		bool getStripeVisible() const { return mStyle.stripe.visible; }

        void    setBorder(ofColor color, int width);
        void    setBorderVisible(bool visible);
    
        void    setMask(const ofRectangle &mask);
        void    setAnchor(ofxDatGuiAnchor anchor);
        void    setEnabled(bool visible);
        bool    getEnabled();
        void    setVisible(bool visible);
        bool    getVisible();
        void    setFocused(bool focused);
        bool    getFocused();
        void    setOpacity(float opacity);
        bool    getMouseDown();
        ofxDatGuiType getType();
    
        vector<ofxDatGuiComponent*> children;
    
        virtual void draw();
        virtual void update(bool acceptEvents = true);
        virtual bool hitTest(ofPoint m);

        virtual void setPosition(int x, int y);
        virtual void setTheme(const ofxDatGuiTheme* theme) = 0;
        virtual void setWidth(int width, float labelWidth);
        virtual void setLabelAlignment(ofxDatGuiAlignment align);
    
        virtual int  getWidth();
        virtual int  getHeight();
        virtual bool getIsExpanded();
        virtual void drawColorPicker();
        // Iterate direct children; containers override to walk owned children.
        virtual void forEachChild(const std::function<void(ofxDatGuiComponent*)> & fn) const;
        void releaseMouseCapture();

        // Root association (non-owning).
        virtual void setRoot(ofxDatGui* r) { mRoot = r; }
        ofxDatGui* getRoot() const { return mRoot; }
        void setParent(ofxDatGuiComponent* p) { mParent = p; }
        ofxDatGuiComponent* getParent() const { return mParent; }

        virtual void onFocus();    
        virtual void onFocusLost();
        virtual void onWindowResized();
        virtual void onKeyPressed(int key);
        virtual void onMouseEnter(ofPoint m);
        virtual void onMousePress(ofPoint m);
        virtual void onMouseDrag(ofPoint m);
        virtual void onMouseLeave(ofPoint m);
        virtual void onMouseRelease(ofPoint m);
        void onKeyPressed(ofKeyEventArgs &e);
        void onWindowResized(ofResizeEventArgs &e);

        static const ofxDatGuiTheme* getTheme();
    
    protected:
    
        int x;
        int y;
        int mIndex;
        string mName;
        bool mFocused;
        bool mVisible;
        bool mEnabled;
        bool mMouseOver;
        bool mMouseDown;
        ofxDatGui* mRoot = nullptr;
        ofxDatGuiComponent* mParent = nullptr;

        ofRectangle mMask;
        ofxDatGuiType mType;
        ofxDatGuiAnchor mAnchor;
        shared_ptr<ofxSmartFont> mFont;
    
        struct{
            float width;
            float height;
            float padding;
            float vMargin;
            float opacity;
            struct{
                ofColor inputArea;
                ofColor background;
                ofColor onMouseOver;
                ofColor onMouseDown;
            } color;
            struct{
                int width;
                bool visible;
                ofColor color;
            } border;
			struct {
				int width;
				bool visible;
				ofColor color;
				StripePosition position;
			} stripe;
            ofColor guiBackground;
        } mStyle;
    
        struct{
            int x;
            string text;
            string rendered;
            bool visible;
            ofColor color;
            float width;
            int margin;
            int rightAlignedXpos;
            ofRectangle rect;
            bool forceUpperCase;
            ofxDatGuiAlignment alignment;
        } mLabel;
    
        struct {
            int x;
            int y;
            int size;
            ofColor color;
        } mIcon;
    
        void drawLabel();
        void drawBorder();
        void drawStripe();
        void drawBackground();
        void positionLabel();
        void setComponentStyle(const ofxDatGuiTheme* t);
    
    private:
    
        static std::unique_ptr<ofxDatGuiTheme> theme;
    
};

