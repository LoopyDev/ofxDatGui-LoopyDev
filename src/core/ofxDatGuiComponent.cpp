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

#include "ofxDatGuiComponent.h"
#include "ofxDatGui.h"

bool ofxDatGuiLog::mQuiet = false;
string ofxDatGuiTheme::AssetPath = "";
std::unique_ptr<ofxDatGuiTheme> ofxDatGuiComponent::theme;
namespace {
    int gThemeWidthDepth = 0;
}

bool ofxDatGuiComponent::isApplyingThemeWidth()
{
    return gThemeWidthDepth > 0;
}

ofxDatGuiComponent::ThemeWidthScope::ThemeWidthScope()
{
    ++gThemeWidthDepth;
}

ofxDatGuiComponent::ThemeWidthScope::~ThemeWidthScope()
{
    --gThemeWidthDepth;
}

ofxDatGuiComponent::ofxDatGuiComponent(string label)
{
    mName = label;
    mVisible = true;
    mEnabled = true;
    mFocused = false;
    mMouseOver = false;
    mMouseDown = false;
    mStyle.opacity = 255;
    this->x = 0; this->y = 0;
    mAnchor = ofxDatGuiAnchor::NO_ANCHOR;
    mLabel.text = label;
    mLabel.alignment = ofxDatGuiAlignment::LEFT;
}

ofxDatGuiComponent::~ofxDatGuiComponent()
{
//  cout << "ofxDatGuiComponent "<< mName << " destroyed" << endl;
    ofRemoveListener(ofEvents().keyPressed, this, &ofxDatGuiComponent::onKeyPressed);
    ofRemoveListener(ofEvents().windowResized, this, &ofxDatGuiComponent::onWindowResized);

	// Clear mouse press ownership
	if (auto* root = getRoot()) {
		if (root->getMouseCapture() == this) root->setMouseCapture(nullptr);
	}
}

/*
    instance getters & setters
*/


void ofxDatGuiComponent::setIndex(int index)
{
    mIndex = index;
}

int ofxDatGuiComponent::getIndex()
{
    return mIndex;
}

void ofxDatGuiComponent::setName(string name)
{
    mName = name;
}

string ofxDatGuiComponent::getName()
{
    return mName;
}

bool ofxDatGuiComponent::is(string name)
{
    return ofToLower(mName) == ofToLower(name);
}

ofxDatGuiType ofxDatGuiComponent::getType()
{
    return mType;
}

const ofxDatGuiTheme* ofxDatGuiComponent::getTheme()
{
    if (theme == nullptr) theme = std::make_unique<ofxDatGuiTheme>(true);
    return theme.get();
}

void ofxDatGuiComponent::setComponentStyle(const ofxDatGuiTheme* theme)
{
    mStyle.height = theme->layout.height;
    mStyle.padding = theme->layout.padding;
    mStyle.vMargin = theme->layout.vMargin;
    mStyle.color.background = theme->color.background;
    mStyle.color.inputArea = theme->color.inputAreaBackground;
    mStyle.color.onMouseOver = theme->color.backgroundOnMouseOver;
    mStyle.color.onMouseDown = theme->color.backgroundOnMouseDown;
    if (!mUserStripeOverride) {
        mStyle.stripe.width = theme->stripe.width;
        mStyle.stripe.visible = theme->stripe.visible;
        mStyle.stripe.color = theme->stripe.label;
	    mStyle.stripe.position = StripePosition::LEFT; // preserve original behaviour
    }
    mStyle.border.width = theme->border.width;
    mStyle.border.color = theme->border.color;
    mStyle.border.visible = theme->border.visible;
    mStyle.guiBackground = theme->color.guiBackground;
    mFont = theme->font.ptr;
    mIcon.y = mStyle.height * .33;
    mIcon.color = theme->color.icons;
    mIcon.size = theme->layout.iconSize;
    mLabel.color = theme->color.label;
    mLabel.margin = theme->layout.labelMargin;
    mLabel.forceUpperCase = theme->layout.upperCaseLabels;
    setLabel(mLabel.text);
    {
        ThemeWidthScope scope;
        setWidth(theme->layout.width, theme->layout.labelWidth);
    }
    forEachChild([&](ofxDatGuiComponent* c){ c->setTheme(theme); });
}

void ofxDatGuiComponent::setWidth(int width, float labelWidth)
{
    const bool themeWidth = isApplyingThemeWidth();
    if (themeWidth && mUserWidthSet) {
        return;
    }
    mStyle.width = width;
    if (labelWidth > 1){
// we received a pixel value //
        mLabel.width = labelWidth;
    }   else{
// we received a percentage //
        mLabel.width = mStyle.width * labelWidth;
    }
    mIcon.x = mStyle.width - (mStyle.width * .05) - mIcon.size;
    mLabel.rightAlignedXpos = mLabel.width - mLabel.margin;
    forEachChild([&](ofxDatGuiComponent* c){ c->setWidth(width, labelWidth); });
    positionLabel();
    if (!themeWidth && mHasAppliedWidth) {
        mUserWidthSet = true;
    }
    mHasAppliedWidth = true;
}

int ofxDatGuiComponent::getWidth()
{
    return mStyle.width;
}

int ofxDatGuiComponent::getHeight()
{
    return mStyle.height;
}

int ofxDatGuiComponent::getX()
{
    return this->x;
}

int ofxDatGuiComponent::getY()
{
    return this->y;
}

void ofxDatGuiComponent::setPosition(int x, int y)
{
    this->x = x;
    this->y = y;
    int idx = 0;
    forEachChild([&](ofxDatGuiComponent* c){
        c->setPosition(x, this->y + (mStyle.height+mStyle.vMargin)*(idx+1));
        ++idx;
    });
}

void ofxDatGuiComponent::setVisible(bool visible)
{
    mVisible = visible;

	if (!visible) {
		if (auto* root = getRoot()) {
			if (root->getMouseCapture() == this) root->setMouseCapture(nullptr);
		}
	}

    if (internalEventCallback != nullptr){
        ofxDatGuiInternalEvent e(ofxDatGuiEventType::VISIBILITY_CHANGED, mIndex);
        internalEventCallback(e);
    }
}

bool ofxDatGuiComponent::getVisible()
{
    return mVisible;
}

void ofxDatGuiComponent::setOpacity(float opacity)
{
    mStyle.opacity = opacity * 255;
    forEachChild([&](ofxDatGuiComponent* c){ c->setOpacity(opacity); });
}

void ofxDatGuiComponent::setEnabled(bool enabled)
{
    mEnabled = enabled;
}

bool ofxDatGuiComponent::getEnabled()
{
    return mEnabled;
}

void ofxDatGuiComponent::setFocused(bool focused)
{
    if (focused){
        onFocus();
    }   else{
        onFocusLost();
    }
}

bool ofxDatGuiComponent::getFocused()
{
    return mFocused;
}

bool ofxDatGuiComponent::getMouseDown()
{
    return mMouseDown;
}

void ofxDatGuiComponent::setMask(const ofRectangle &mask)
{
    mMask = mask;
}

void ofxDatGuiComponent::forEachChild(const std::function<void(ofxDatGuiComponent*)> & fn) const {
	for (auto * c : children) fn(c);
}

void ofxDatGuiComponent::releaseMouseCapture() {
	if (auto* root = getRoot()) root->setMouseCapture(nullptr);
}

void ofxDatGuiComponent::setAnchor(ofxDatGuiAnchor anchor)
{
    mAnchor = anchor;
    if (mAnchor != ofxDatGuiAnchor::NO_ANCHOR){
        ofAddListener(ofEvents().windowResized, this, &ofxDatGuiComponent::onWindowResized);
    }   else{
        ofRemoveListener(ofEvents().windowResized, this, &ofxDatGuiComponent::onWindowResized);
    }
    onWindowResized();
}

bool ofxDatGuiComponent::getIsExpanded()
{
	return false;
}

/*
    component label
*/

void ofxDatGuiComponent::setLabel(string label)
{
    mLabel.text = label;
    mLabel.rendered = mLabel.forceUpperCase ? ofToUpper(mLabel.text) : mLabel.rendered = mLabel.text;
    mLabel.rect = mFont->rect(mLabel.rendered);
    positionLabel();
}

string ofxDatGuiComponent::getLabel()
{
    return mLabel.text;
}

void ofxDatGuiComponent::setLabelColor(ofColor c)
{
    mLabel.color = c;
}

ofColor ofxDatGuiComponent::getLabelColor()
{
    return mLabel.color;
}

void ofxDatGuiComponent::setLabelUpperCase(bool toUpper)
{
    mLabel.forceUpperCase = toUpper;
    setLabel(mLabel.text);
}

bool ofxDatGuiComponent::getLabelUpperCase()
{
    return mLabel.forceUpperCase;
}

void ofxDatGuiComponent::setLabelAlignment(ofxDatGuiAlignment align)
{
    mLabel.alignment = align;
    forEachChild([&](ofxDatGuiComponent* c){ c->setLabelAlignment(align); });
    positionLabel();
}

void ofxDatGuiComponent::positionLabel()
{
    if (mLabel.alignment == ofxDatGuiAlignment::LEFT){
        mLabel.x = mLabel.margin;
    }   else if (mLabel.alignment == ofxDatGuiAlignment::CENTER){
        mLabel.x = (mLabel.width / 2) - (mLabel.rect.width / 2);
    }   else if (mLabel.alignment == ofxDatGuiAlignment::RIGHT){
        mLabel.x = mLabel.rightAlignedXpos - mLabel.rect.width;
    }
}

/*
    visual customization
*/

void ofxDatGuiComponent::setBackgroundColor(ofColor color)
{
    mStyle.color.background = color;
}

void ofxDatGuiComponent::setBackgroundColorOnMouseOver(ofColor color)
{
    mStyle.color.onMouseOver = color;
}

void ofxDatGuiComponent::setBackgroundColorOnMouseDown(ofColor color)
{
    mStyle.color.onMouseDown = color;
}

void ofxDatGuiComponent::setBackgroundColors(ofColor c1, ofColor c2, ofColor c3)
{
    mStyle.color.background = c1;
    mStyle.color.onMouseOver = c2;
    mStyle.color.onMouseDown = c3;
}

void ofxDatGuiComponent::setStripe(ofColor color, int width)
{
    mStyle.stripe.color = color;
    mStyle.stripe.width = width;
    mStyle.stripe.visible = true;
    mUserStripeOverride = true;
}

void ofxDatGuiComponent::setStripeColor(ofColor color)
{
    mStyle.stripe.color = color;
    mUserStripeOverride = true;
}

void ofxDatGuiComponent::setStripeWidth(int width)
{
    mStyle.stripe.width = width;
    mUserStripeOverride = true;
}

void ofxDatGuiComponent::setStripeVisible(bool visible)
{
    mStyle.stripe.visible = visible;
    mUserStripeOverride = true;
}

// LoopyDev: Stripe config
void ofxDatGuiComponent::setStripePosition(StripePosition position) {
	mStyle.stripe.position = position;
    mUserStripeOverride = true;
}

ofxDatGuiComponent::StripePosition ofxDatGuiComponent::getStripePosition() const {
	return mStyle.stripe.position;
}

void ofxDatGuiComponent::applyMutedPalette(const ofxDatGuiTheme* theme, bool muted) {
	if (theme == nullptr) theme = ofxDatGuiComponent::getTheme();
	if (theme == nullptr) return;

	// Pick muted or normal palettes field-by-field to avoid type mismatch.
	ofColor bg = muted ? theme->color.muted.background : theme->color.background;
	ofColor bgOver = muted ? theme->color.muted.backgroundOnMouseOver : theme->color.backgroundOnMouseOver;
	ofColor bgDown = muted ? theme->color.muted.backgroundOnMouseDown : theme->color.backgroundOnMouseDown;
	ofColor label = muted ? theme->color.muted.label : theme->color.label;
	ofColor icon = muted ? theme->color.muted.icons : theme->color.icons;

	setBackgroundColors(bg, bgOver, bgDown);
	setLabelColor(label);
	setIconColor(icon);

	// Stripe mapping by component type; respect user stripe overrides.
	if (!mUserStripeOverride) {
		switch (getType()) {
		case ofxDatGuiType::LABEL:      setStripeColor(muted ? theme->stripe.muted.label : theme->stripe.label); break;
		case ofxDatGuiType::BUTTON:     setStripeColor(muted ? theme->stripe.muted.button : theme->stripe.button); break;
		case ofxDatGuiType::TOGGLE:     setStripeColor(muted ? theme->stripe.muted.toggle : theme->stripe.toggle); break;
		case ofxDatGuiType::SLIDER:     setStripeColor(muted ? theme->stripe.muted.slider : theme->stripe.slider); break;
		case ofxDatGuiType::PAD2D:      setStripeColor(muted ? theme->stripe.muted.pad2d : theme->stripe.pad2d); break;
		case ofxDatGuiType::MATRIX:     setStripeColor(muted ? theme->stripe.muted.matrix : theme->stripe.matrix); break;
		case ofxDatGuiType::DROPDOWN:   setStripeColor(muted ? theme->stripe.muted.dropdown : theme->stripe.dropdown); break;
		case ofxDatGuiType::TEXT_INPUT: setStripeColor(muted ? theme->stripe.muted.textInput : theme->stripe.textInput); break;
		case ofxDatGuiType::COLOR_PICKER: setStripeColor(muted ? theme->stripe.muted.colorPicker : theme->stripe.colorPicker); break;
		default:                        setStripeColor(muted ? theme->stripe.muted.label : theme->stripe.label); break;
		}
	}
}

void ofxDatGuiComponent::setBorder(ofColor color, int width)
{
    mStyle.border.color = color;
    mStyle.border.width = width;
    mStyle.border.visible = true;
}

void ofxDatGuiComponent::setBorderVisible(bool visible)
{
    mStyle.border.visible = visible;
}

/*
    draw methods
*/

// Only true on the exact frame the mouse transitions Up > Down
static bool mousePressedThisFrame() {
	static bool prev = false;
	static bool thisFrame = false;
	static uint64_t seenFrame = std::numeric_limits<uint64_t>::max();

	uint64_t f = ofGetFrameNum();
	if (f != seenFrame) {
		bool mp = ofGetMousePressed();
		thisFrame = mp && !prev;
		prev = mp;
		seenFrame = f;
	}
	return thisFrame;
}

void ofxDatGuiComponent::update(bool acceptEvents) {
	if (!acceptEvents || !mEnabled || !mVisible) {
		// Skip interaction/hover when events are blocked or we're disabled/hidden.
		mMouseOver = false;
		// Do not force-release capture here; owner manages it.
		if (this->getIsExpanded()) {
			forEachChild([&](ofxDatGuiComponent* c){
				if (c->getVisible()) c->update(false);
			});
		}
		return;
	}

	const bool mp = ofGetMousePressed();
	const bool justPressed = mousePressedThisFrame(); // only true on the transition frame

	// Absolute mouse (same space as component x/y)
	const ofPoint mouseAbs(ofGetMouseX(), ofGetMouseY());

	// Local to mask (only for vertical clip test)
	const ofPoint mouseLocal(mouseAbs.x - mMask.x, mouseAbs.y - mMask.y);

	auto* root = getRoot();
	ofxDatGuiComponent* capture = root ? root->getMouseCapture() : nullptr;

	// Only allow hover/highlight when not pressed, or when THIS component owns the press.
	const bool hoverAllowed = !(mp && capture != nullptr && capture != this);

	const bool overGeom = hitTest(mouseAbs) && (mMask.height <= 0 || (mouseLocal.y >= 0 && mouseLocal.y <= mMask.height));
	// If this is an expanded container, don't steal presses that begin in the child area (below header).
	bool hasChild = false;
	forEachChild([&](ofxDatGuiComponent*){ hasChild = true; });
	const bool pressInChildRegion = getIsExpanded() && hasChild && (ofGetMouseY() >= y + mStyle.height);

	// Block highlighting on drag-in or while another widget owns the press:
	const bool over = hoverAllowed && overGeom;

	if (over && !mMouseOver) onMouseEnter(mouseAbs);
	if (!over && mMouseOver) onMouseLeave(mouseAbs);

	if (acceptEvents && mEnabled && mVisible) {
		if (mp) {
			if (capture == this) {
				// We already own this press > keep dragging
				onMouseDrag(mouseAbs);
			} else if (capture == nullptr && overGeom && justPressed && !pressInChildRegion) {
				// Start a brand new press only if it BEGAN here, this frame
				mMouseDown = true;
				if (root) root->setMouseCapture(this);
				onMousePress(mouseAbs);
				if (!mFocused) onFocus();
			}
			// else: press started elsewhere > ignore (no capture on drag-in)
	} else {
			if (capture == this) {
				// Mouse went up; we were the owner ? release, even if mMouseDown was toggled elsewhere
				onMouseRelease(mouseAbs);
				mMouseDown = false;
				if (root) root->setMouseCapture(nullptr);
			} else if (!root && mMouseDown) {
				// No root to manage capture: release locally when mouse goes up.
				onMouseRelease(mouseAbs);
				mMouseDown = false;
			} else if (mMouseDown) {
				// Lost capture elsewhere; still dispatch a release to avoid stuck presses / missed clicks.
				onMouseRelease(mouseAbs);
				mMouseDown = false;
			}
		}

	}

		if (this->getIsExpanded()) {
			forEachChild([&](ofxDatGuiComponent* c){
				if (c->getVisible()) c->update(acceptEvents);
			});
		}
}



void ofxDatGuiComponent::draw()
{

    ofPushStyle();
        if (mStyle.border.visible) drawBorder();
        drawBackground();
        drawLabel();
        if (mStyle.stripe.visible) drawStripe();
    ofPopStyle();
}

void ofxDatGuiComponent::drawBackground()
{
    ofFill();
    ofSetColor(mStyle.color.background, mStyle.opacity);
    ofDrawRectangle(x, y, mStyle.width, mStyle.height);
}

void ofxDatGuiComponent::drawLabel()
{
    ofSetColor(mLabel.color);
    if (mType != ofxDatGuiType::DROPDOWN_OPTION){
        mFont->draw(mLabel.rendered, x+mLabel.x, y+mStyle.height/2 + mLabel.rect.height/2);
    }   else{
        mFont->draw("* "+mLabel.rendered, x+mLabel.x, y+mStyle.height/2 + mLabel.rect.height/2);
    }
}

//void ofxDatGuiComponent::drawStripe()
//{
//    ofSetColor(mStyle.stripe.color);
//    ofDrawRectangle(x, y, mStyle.stripe.width, mStyle.height);
//}
void ofxDatGuiComponent::drawStripe() {
	if (!mStyle.stripe.visible || mStyle.stripe.width <= 0) return;

	ofSetColor(mStyle.stripe.color);
	int w = mStyle.stripe.width;

	switch (mStyle.stripe.position) {
	case StripePosition::LEFT:
		ofDrawRectangle(x, y, w, mStyle.height);
		break;

	case StripePosition::RIGHT:
		ofDrawRectangle(x + mStyle.width - w, y, w, mStyle.height);
		break;

	case StripePosition::TOP:
		ofDrawRectangle(x, y, mStyle.width, w);
		break;

	case StripePosition::BOTTOM:
		ofDrawRectangle(x, y + mStyle.height - w, mStyle.width, w);
		break;
	}
}


void ofxDatGuiComponent::drawBorder()
{
    ofFill();
    int w = mStyle.border.width;
    ofSetColor(mStyle.border.color, mStyle.opacity);
    ofDrawRectangle(x-w, y-w, mStyle.width+(w*2), mStyle.height+(w*2));
}

void ofxDatGuiComponent::drawColorPicker() { }

/*
    events
*/

bool ofxDatGuiComponent::hitTest(ofPoint m)
{
    if (mMask.height > 0 && (m.y < 0 || m.y > mMask.height)) return false;
    return (m.x>=x && m.x<= x+mStyle.width && m.y>=y && m.y<= y+mStyle.height);
}

void ofxDatGuiComponent::onMouseEnter(ofPoint m)
{
    mMouseOver = true;
}

void ofxDatGuiComponent::onMouseLeave(ofPoint m)
{
    mMouseOver = false;
}

void ofxDatGuiComponent::onMousePress(ofPoint m)
{
    mMouseDown = true;
}

void ofxDatGuiComponent::onMouseRelease(ofPoint m)
{
    mMouseDown = false;
}

void ofxDatGuiComponent::onFocus()
{
    mFocused = true;
    ofAddListener(ofEvents().keyPressed, this, &ofxDatGuiComponent::onKeyPressed);
}

void ofxDatGuiComponent::onFocusLost()
{
    mFocused = false;
    mMouseDown = false;
    ofRemoveListener(ofEvents().keyPressed, this, &ofxDatGuiComponent::onKeyPressed);
}

void ofxDatGuiComponent::onKeyPressed(int key) { }
void ofxDatGuiComponent::onMouseDrag(ofPoint m) { }

void ofxDatGuiComponent::onKeyPressed(ofKeyEventArgs &e)
{
    onKeyPressed(e.key);
    if ((e.key == OF_KEY_RETURN || e.key == OF_KEY_TAB)){
        onFocusLost();
        ofRemoveListener(ofEvents().keyPressed, this, &ofxDatGuiComponent::onKeyPressed);
    }
}

void ofxDatGuiComponent::onWindowResized()
{
    if (mAnchor == ofxDatGuiAnchor::TOP_LEFT){
        setPosition(0, 0);
    }   else if (mAnchor == ofxDatGuiAnchor::TOP_RIGHT){
        setPosition(ofGetWidth()-mStyle.width, 0);
    }
}

void ofxDatGuiComponent::onWindowResized(ofResizeEventArgs &e)
{
    onWindowResized();
}
