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

#include "ofxDatGui.h"
#include <unordered_map>
#include <algorithm>

namespace {
// Compute which edge is closest for sliding off-screen.
enum class SlideDir { LEFT, RIGHT, TOP, BOTTOM };

bool edgeAllowed(ofxDatGui::SlideMask mask, SlideDir dir) {
    using SE = ofxDatGui::SlideEdge;
    switch (dir) {
        case SlideDir::LEFT:   return (mask & static_cast<uint8_t>(SE::LEFT)) != 0;
        case SlideDir::RIGHT:  return (mask & static_cast<uint8_t>(SE::RIGHT)) != 0;
        case SlideDir::TOP:    return (mask & static_cast<uint8_t>(SE::TOP)) != 0;
        case SlideDir::BOTTOM: return (mask & static_cast<uint8_t>(SE::BOTTOM)) != 0;
    }
    return true;
}

SlideDir closestEdge(ofxDatGuiComponent* c, int winW, int winH, ofxDatGui::SlideMask allowed) {
    int x = c->getX();
    int y = c->getY();
    int w = c->getWidth();
    int h = c->getHeight();
    if (w <= 0 || h <= 0 || winW <= 0 || winH <= 0) return SlideDir::LEFT;

    int distLeft = x;
    int distRight = std::max(0, winW - (x + w));
    int distTop = y;
    int distBottom = std::max(0, winH - (y + h));

    int best = std::numeric_limits<int>::max();
    SlideDir dir = SlideDir::LEFT;

    auto consider = [&](SlideDir d, int dist) {
        if (!edgeAllowed(allowed, d)) return;
        if (dist < best) {
            best = dist;
            dir = d;
        }
    };

    consider(SlideDir::LEFT, distLeft);
    consider(SlideDir::RIGHT, distRight);
    consider(SlideDir::TOP, distTop);
    consider(SlideDir::BOTTOM, distBottom);

    return dir;
}

ofxDatGui::SlideMask maskFromEdges(std::initializer_list<ofxDatGui::SlideEdge> edges) {
    ofxDatGui::SlideMask mask = 0;
    for (auto e : edges) mask |= static_cast<ofxDatGui::SlideMask>(e);
    return mask;
}
}

// Local helper: true only on the frame the mouse transitions Up -> Down.
namespace {
    static bool mousePressedThisFrameGui() {
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
}

ofxDatGui* ofxDatGui::mActiveGui;
vector<ofxDatGui*> ofxDatGui::mGuis;

ofxDatGui::ofxDatGui()
{
    mPosition.x = 0;
    mPosition.y = 0;
}

ofxDatGui::ofxDatGui(int x, int y)
{
    setup(x, y);
}

ofxDatGui::ofxDatGui(ofxDatGuiAnchor anchor)
{
    setup(anchor);
}

ofxDatGui::~ofxDatGui()
{
    if (!mIsSetup) return;
    mGuis.erase(std::remove(mGuis.begin(), mGuis.end(), this), mGuis.end());
    if (mActiveGui == this) mActiveGui = mGuis.size() > 0 ? mGuis[0] : nullptr;
    ofRemoveListener(ofEvents().draw, this, &ofxDatGui::onDraw, OF_EVENT_ORDER_AFTER_APP + mIndex);
    ofRemoveListener(ofEvents().update, this, &ofxDatGui::onUpdate, OF_EVENT_ORDER_BEFORE_APP - mIndex);
    ofRemoveListener(ofEvents().windowResized, this, &ofxDatGui::onWindowResized, OF_EVENT_ORDER_BEFORE_APP);
}

void ofxDatGui::init()
{
    if (mIsSetup) return;
    mMoving = false;
    mVisible = true;
    mEnabled = true;
    mExpanded = true;
    mAlphaChanged = false;
    mWidthChanged = false;
    mThemeChanged = false;
    mAlignmentChanged = false;
    mAlignment = ofxDatGuiAlignment::LEFT;
    mAlpha = 1.0f;
    mClampPanelsToWindow = false;
    mClampPanelsMinVisibleWidth = 0;
    mClampPanelsMinVisibleHeight = 0;
    const ofxDatGuiTheme* defaultTheme = ofxDatGuiComponent::getTheme();
    mWidth = defaultTheme->layout.width;
    mLabelWidth = defaultTheme->layout.labelWidth;
    mRowSpacing = defaultTheme->layout.vMargin;
    mGuiBackground = defaultTheme->color.guiBackground;

	mOrientation = Orientation::VERTICAL;

// autodraw disabled by default; caller must invoke update()/draw()
    setAutoDraw(false, mGuis.size());
    
// assign focus to this newly created gui //
    mActiveGui = this;
    mGuis.push_back(this);
    ofAddListener(ofEvents().windowResized, this, &ofxDatGui::onWindowResized, OF_EVENT_ORDER_BEFORE_APP);
    mIsSetup = true;
}

void ofxDatGui::setup()
{
    setup(ofxDatGuiAnchor::NO_ANCHOR);
}

void ofxDatGui::setup(int x, int y)
{
    if (mIsSetup) return;
    mPosition.x = x;
    mPosition.y = y;
    mManualLayout = true;
    init();
}

void ofxDatGui::setup(ofxDatGuiAnchor anchor)
{
    if (mIsSetup) return;
    mManualLayout = true;
    mPosition.x = 0;
    mPosition.y = 0;
    init();
}

void ofxDatGui::ensureSetup()
{
    if (!mIsSetup) {
        setup(ofxDatGuiAnchor::NO_ANCHOR);
    }
}
/* 
    public api
*/

void ofxDatGui::focus()
{
    ensureSetup();
    if (mActiveGui!= this){
    // enable and make visible if hidden //
        mVisible = true;
        mEnabled = true;
        mActiveGui = this;
    // update the draw order //
        for (int i=0; i<mGuis.size(); i++) {
            if (mGuis[i] == mActiveGui) {
                std::swap(mGuis[i], mGuis[mGuis.size()-1]);
                break;
            }
        }
        for (int i=0; i<mGuis.size(); i++) {
            if (mGuis[i]->getAutoDraw()) mGuis[i]->setAutoDraw(true, i);
        }
    }
}

void ofxDatGui::expand()
{
    ensureSetup();
    mExpanded = true;
}

void ofxDatGui::collapse()
{
    ensureSetup();
    mExpanded = false;
}

void ofxDatGui::toggle()
{
    ensureSetup();
    mExpanded ? collapse() : expand();
}

bool ofxDatGui::getVisible()
{
    ensureSetup();
    return mVisible;
}

bool ofxDatGui::getFocused()
{
    ensureSetup();
    return mActiveGui == this;
}

void ofxDatGui::setWidthInternal(int width, float labelWidth, bool markUser)
{
    mWidth = width;
    mLabelWidth = labelWidth;
    mWidthChanged = true;
    if (markUser) {
        mUserWidthSet = true;
    }
}

void ofxDatGui::applyThemeWidth(int width, float labelWidth)
{
    (void)width;
    // Keep theme from overriding caller-defined widths; only refresh stored label width.
    if (mUserWidthSet) return;
    mLabelWidth = labelWidth;
}

void ofxDatGui::setOrientation(Orientation orientation) {
	ensureSetup();
	if (mOrientation == orientation) return;
	mOrientation = orientation;

	// stripes autoflip:
	// for (auto* item : items) {
	//     if (!item) continue;
	//     item->setStripePosition(
	//         mOrientation == Orientation::HORIZONTAL ?
	//             ofxDatGuiComponent::StripePosition::BOTTOM :
	//             ofxDatGuiComponent::StripePosition::LEFT
	//     );
	// }

	layoutGui();
}


void ofxDatGui::setTheme(ofxDatGuiTheme* t, bool applyImmediately)
{
    ensureSetup();
    if (applyImmediately){
        if (t) {
            mRowSpacing = t->layout.vMargin;
            mGuiBackground = t->color.guiBackground;
            applyThemeWidth(t->layout.width, t->layout.labelWidth);
            ofxDatGuiComponent::ThemeWidthScope scope;
            for (auto & item : items) applyThemeRecursive(item.get(), t);
            layoutGui();
        }
        // Borrowed theme, do not take ownership.
        mPendingBorrowedTheme = nullptr;
        mPendingOwnedTheme.reset();
        mBorrowedTheme = mOwnedTheme ? nullptr : t;
        mThemeChanged = false;
    }   else{
        // apply on next update call //
        mPendingBorrowedTheme = t;
        mPendingOwnedTheme.reset();
        mThemeChanged = true;
    }
}

void ofxDatGui::setTheme(std::unique_ptr<ofxDatGuiTheme> t, bool applyImmediately)
{
    ensureSetup();
    if (!t) return;
    if (applyImmediately){
        mOwnedTheme = std::move(t);
        setTheme(mOwnedTheme.get(), true);
    } else {
        mPendingOwnedTheme = std::move(t);
        mPendingBorrowedTheme = nullptr;
        mThemeChanged = true;
    }
}

void ofxDatGui::setOpacity(float opacity)
{
    ensureSetup();
    mAlpha = opacity;
    mAlphaChanged = true;
}

void ofxDatGui::setVisible(bool visible)
{
    ensureSetup();
    mVisible = visible;
}

void ofxDatGui::setEnabled(bool enabled)
{
    ensureSetup();
    mEnabled = enabled;
}

void ofxDatGui::setAutoDraw(bool autodraw, int priority)
{
    mAutoDraw = autodraw;
    ofRemoveListener(ofEvents().draw, this, &ofxDatGui::onDraw, OF_EVENT_ORDER_AFTER_APP + mIndex);
    ofRemoveListener(ofEvents().update, this, &ofxDatGui::onUpdate, OF_EVENT_ORDER_BEFORE_APP - mIndex);
    if (mAutoDraw){
        mIndex = priority;
        ofAddListener(ofEvents().draw, this, &ofxDatGui::onDraw, OF_EVENT_ORDER_AFTER_APP + mIndex);
        ofAddListener(ofEvents().update, this, &ofxDatGui::onUpdate, OF_EVENT_ORDER_BEFORE_APP - mIndex);
    }
}

bool ofxDatGui::getAutoDraw()
{
    ensureSetup();
    return mAutoDraw;
}

bool ofxDatGui::getMouseDown()
{
    ensureSetup();
    return mMouseDown;
}

void ofxDatGui::setMouseCapture(ofxDatGuiComponent* c)
{
	ensureSetup();
	mMouseCaptureOwner = c;
    if (mBringToFrontOnInteract && c != nullptr) {
        bringItemToFront(c);
    }
    // Track last focused top-level for muting/unmuting.
    if (c != nullptr) {
        ofxDatGuiComponent* top = c;
        while (top->getParent() != nullptr) top = top->getParent();
        if (top->getRoot() == this) mLastFocusedPanel = top;
    }
}

ofxDatGuiComponent* ofxDatGui::getMouseCapture() const
{
	return mMouseCaptureOwner;
}

void ofxDatGui::setLabelAlignment(ofxDatGuiAlignment align)
{
    ensureSetup();
    mAlignment = align;
    mAlignmentChanged = true;
}

void ofxDatGui::slidePanelsOffscreen(bool respectClamp, bool animate, SlideMask allowedEdges)
{
    ensureSetup();
    if (mPanelsSlidOut || mSlideAnimating) return;
    if (allowedEdges == 0) return;

    const int winW = ofGetWidth();
    const int winH = ofGetHeight();
    const int minVisW = (respectClamp && mClampPanelsToWindow) ? mClampPanelsMinVisibleWidth : 0;
    const int minVisH = (respectClamp && mClampPanelsToWindow) ? mClampPanelsMinVisibleHeight : 0;
    mSlideRespectClamp = respectClamp;
    mSlideAnimTargets.clear();

    for (auto & item : items) {
        auto* c = item.get();
        if (!c || !c->getVisible()) continue;
        if (!c->getParticipatesInRootSlide()) continue;
        mSavedPanelPositions[c] = ofPoint(c->getX(), c->getY());
        mSavedPanelOpacities[c] = c->getOpacity();

        const int w = c->getWidth();
        const int h = c->getHeight();
        SlideDir dir = closestEdge(c, winW, winH, allowedEdges);

        int targetX = c->getX();
        int targetY = c->getY();
        switch (dir) {
            case SlideDir::LEFT:
                targetX = respectClamp ? -(w - std::min(w, minVisW)) : -w;
                break;
            case SlideDir::RIGHT:
                targetX = respectClamp ? winW - std::min(w, minVisW) : winW;
                break;
            case SlideDir::TOP:
                targetY = respectClamp ? -(h - std::min(h, minVisH)) : -h;
                break;
            case SlideDir::BOTTOM:
                targetY = respectClamp ? winH - std::min(h, minVisH) : winH;
                break;
        }
        if (respectClamp) {
            // Respect top clamp so header stays reachable.
            if (targetY < 0) targetY = 0;
        }

        const float startOpacity = c->getOpacity();
        const float targetOpacity = mSlideHiddenOpacity;
        if (animate) {
            mSlideAnimTargets[c] = {ofPoint(c->getX(), c->getY()), ofPoint(targetX, targetY), startOpacity, targetOpacity};
        } else {
            c->setPosition(targetX, targetY);
            c->setOpacity(targetOpacity);
        }
    }

    if (animate && !mSlideAnimTargets.empty()) {
        mSlideToOff = true;
        mSlideAnimating = true;
        mSlideProgress = 0.f;
    } else {
        mPanelsSlidOut = true;
    }
}

void ofxDatGui::slidePanelsOffscreen(bool respectClamp, bool animate, std::initializer_list<SlideEdge> allowedEdges)
{
    slidePanelsOffscreen(respectClamp, animate, maskFromEdges(allowedEdges));
}

void ofxDatGui::slidePanelsBack(bool animate)
{
    if (mSlideAnimating) return;
    if (!mPanelsSlidOut && mSavedPanelPositions.empty()) return;

    mSlideAnimTargets.clear();

    for (auto & kv : mSavedPanelPositions) {
        if (!kv.first) continue;
        if (!kv.first->getParticipatesInRootSlide()) continue;
        const ofPoint target(kv.second.x, kv.second.y);
        auto itOp = mSavedPanelOpacities.find(kv.first);
        const float targetOpacity = (itOp != mSavedPanelOpacities.end()) ? itOp->second : kv.first->getOpacity();
        if (animate) {
            mSlideAnimTargets[kv.first] = {ofPoint(kv.first->getX(), kv.first->getY()), target, kv.first->getOpacity(), targetOpacity};
        } else {
            kv.first->setPosition(static_cast<int>(target.x), static_cast<int>(target.y));
            kv.first->setOpacity(targetOpacity);
        }
    }

    if (animate && !mSlideAnimTargets.empty()) {
        mSlideToOff = false;
        mSlideAnimating = true;
        mSlideProgress = 0.f;
    } else {
        mPanelsSlidOut = false;
        mSavedPanelPositions.clear();
        mSavedPanelOpacities.clear();
    }
}

int ofxDatGui::getWidth()
{
    ensureSetup();
    return mWidth;
}

int ofxDatGui::getHeight()
{
    ensureSetup();
    return mHeight;
}

ofPoint ofxDatGui::getPosition()
{
    ensureSetup();
    return ofPoint(mPosition.x, mPosition.y);
}

bool ofxDatGui::isInTextInputFocusBranch(const ofxDatGuiComponent* c) const
{
    if (c == nullptr || mFocusedTextInput == nullptr) return false;
    for (auto* it = mFocusedTextInput; it != nullptr; it = it->getParent()) {
        if (it == c) return true;
    }
    for (auto* it = c; it != nullptr; it = it->getParent()) {
        if (it == mFocusedTextInput) return true;
    }
    return false;
}

void ofxDatGui::setAssetPath(string path)
{
    ofxDatGuiTheme::AssetPath = path;
}

string ofxDatGui::getAssetPath()
{
    return ofxDatGuiTheme::AssetPath;
}

const ofxDatGuiTheme* ofxDatGui::getActiveTheme() const
{
    if (mOwnedTheme) return mOwnedTheme.get();
    if (mBorrowedTheme) return mBorrowedTheme;
    return ofxDatGuiComponent::getTheme();
}

void ofxDatGui::relayout() {
	layoutGui();
}

void ofxDatGui::applyThemeRecursive(ofxDatGuiComponent* node, const ofxDatGuiTheme* t) {
	if (!node || !t) return;
    ofxDatGuiComponent::ThemeWidthScope scope;
	node->setTheme(t);
	node->forEachChild([&](ofxDatGuiComponent* c) {
		applyThemeRecursive(c, t);
	});
}

/* 
    add component methods
*/


// ofxDatGui.cpp

ofxDatGuiPanel * ofxDatGui::addPanel(ofxDatGuiPanel::Orientation orientation) {
	ensureSetup();
	return &createPanel("", orientation);
}

ofxDatGuiPanel& ofxDatGui::createPanel(const std::string& label, ofxDatGuiPanel::Orientation orientation) {
    ensureSetup();
    auto panel = makeOwned<ofxDatGuiPanel>(orientation);
    auto * raw = static_cast<ofxDatGuiPanel*>(panel.get());
    if (!label.empty()) raw->setLabel(label);

    const ofxDatGuiTheme* themeToUse = getActiveTheme();
    {
        ofxDatGuiComponent::ThemeWidthScope scope;
        raw->setTheme(themeToUse);
    }
    raw->setWidth(mWidth, mLabelWidth);
    attachItem(std::move(panel));
    return *raw;
}


void ofxDatGui::attachItem(ComponentPtr item, bool applyTheme)
{
    if (!item) return;
    ensureSetup();

    auto * raw = item.get();
    if (applyTheme) {
        // Apply current theme to the new item (owned, pending-owned, or default).
        const ofxDatGuiTheme* themeToUse = mPendingOwnedTheme ? mPendingOwnedTheme.get()
            : (mPendingBorrowedTheme ? mPendingBorrowedTheme : getActiveTheme());
        if (themeToUse) {
            ofxDatGuiComponent::ThemeWidthScope scope;
            raw->setTheme(themeToUse);
        }
    }
    items.push_back(std::move(item));
    raw->setRoot(this);
    raw->setParent(nullptr);
    raw->onInternalEvent(this, &ofxDatGui::onInternalEventCallback);
    layoutGui();
}

void ofxDatGui::bringItemToFront(ofxDatGuiComponent* component) {
    if (!component) return;
    // Climb to the top-most component owned by this gui.
    ofxDatGuiComponent* top = component;
    while (top->getParent() != nullptr) {
        top = top->getParent();
    }
    if (top->getRoot() != this) return;

    auto it = std::find_if(items.begin(), items.end(), [&](const ComponentPtr& ptr) {
        return ptr.get() == top;
    });
    if (it == items.end()) return;
    // Already front-most
    if (std::next(it) == items.end()) return;

    // Preserve manual layout positions so reordering only affects z-order.
    std::unordered_map<ofxDatGuiComponent*, ofPoint> positions;
    if (mManualLayout) {
        for (auto & item : items) {
            positions[item.get()] = ofPoint(item->getX(), item->getY());
        }
    }

    ComponentPtr moved = std::move(*it);
    items.erase(it);
    items.push_back(std::move(moved));

    // Refresh indices for consistent callbacks.
    for (int i = 0; i < items.size(); ++i) {
        items[i]->setIndex(i);
    }

    if (mManualLayout) {
        for (auto & item : items) {
            auto posIt = positions.find(item.get());
            if (posIt != positions.end()) {
                item->setPosition(posIt->second.x, posIt->second.y);
            }
        }
        // Update bounds only.
        layoutGui();
    } else {
        layoutGui();
    }
}

/*
    event callbacks
*/

void ofxDatGui::onButtonEventCallback(ofxDatGuiButtonEvent e)
{
    if (buttonEventCallback != nullptr) {
        buttonEventCallback(e);
    }   else{
        ofxDatGuiLog::write(ofxDatGuiMsg::EVENT_HANDLER_NULL);
    }
}

void ofxDatGui::onToggleEventCallback(ofxDatGuiToggleEvent e)
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

void ofxDatGui::onSliderEventCallback(ofxDatGuiSliderEvent e)
{
    if (sliderEventCallback != nullptr) {
        sliderEventCallback(e);
    }   else{
        ofxDatGuiLog::write(ofxDatGuiMsg::EVENT_HANDLER_NULL);
    }
}

void ofxDatGui::onTextInputEventCallback(ofxDatGuiTextInputEvent e)
{
    if (textInputEventCallback != nullptr) {
        textInputEventCallback(e);
    }   else{
        ofxDatGuiLog::write(ofxDatGuiMsg::EVENT_HANDLER_NULL);
    }
}

void ofxDatGui::onDropdownEventCallback(ofxDatGuiDropdownEvent e)
{
    if (dropdownEventCallback != nullptr) {
        dropdownEventCallback(e);
    }   else{
        ofxDatGuiLog::write(ofxDatGuiMsg::EVENT_HANDLER_NULL);
    }
// adjust the gui after a dropdown is closed //
    layoutGui();
}
// LoopyDev: Radio Groups
void ofxDatGui::onRadioGroupEventCallback(ofxDatGuiRadioGroupEvent e) {
	if (radioGroupEventCallback != nullptr) {
		radioGroupEventCallback(e);
	} else {
		ofxDatGuiLog::write(ofxDatGuiMsg::EVENT_HANDLER_NULL);
	}
}

void ofxDatGui::on2dPadEventCallback(ofxDatGui2dPadEvent e)
{
    if (pad2dEventCallback != nullptr) {
        pad2dEventCallback(e);
    }   else{
        ofxDatGuiLog::write(ofxDatGuiMsg::EVENT_HANDLER_NULL);
    }
}

void ofxDatGui::onColorPickerEventCallback(ofxDatGuiColorPickerEvent e)
{
    if (colorPickerEventCallback != nullptr) {
        colorPickerEventCallback(e);
    }   else{
        ofxDatGuiLog::write(ofxDatGuiMsg::EVENT_HANDLER_NULL);
    }
}

void ofxDatGui::onMatrixEventCallback(ofxDatGuiMatrixEvent e)
{
    if (matrixEventCallback != nullptr) {
        matrixEventCallback(e);
    }   else{
        ofxDatGuiLog::write(ofxDatGuiMsg::EVENT_HANDLER_NULL);
    }
}

void ofxDatGui::onInternalEventCallback(ofxDatGuiInternalEvent e)
{
// these events are not dispatched out to the main application //
    if (e.type == ofxDatGuiEventType::GROUP_TOGGLED){
        layoutGui();
    }   else if (e.type == ofxDatGuiEventType::GUI_TOGGLED){
        mExpanded ? collapse() : expand();
    }   else if (e.type == ofxDatGuiEventType::VISIBILITY_CHANGED){
        layoutGui();
    }
}

/*
    layout, position, anchor and check for focus
*/

bool ofxDatGui::hitTest(ofPoint pt)
{
    if (mMoving){
        return true;
    }   else{
        return mGuiBounds.inside(pt);
    }
}

void ofxDatGui::moveGui(ofPoint pt)
{
    mPosition.x = pt.x;
    mPosition.y = pt.y;
    positionGui();
}

void ofxDatGui::layoutGui() {
	if (mManualLayout) {
		// In manual layout mode, do not reposition items; just compute bounds.
		if (items.empty()) {
			mHeight = 0;
			mGuiBounds = ofRectangle(mPosition.x, mPosition.y, mWidth, 0);
			return;
		}
		int minX = std::numeric_limits<int>::max();
		int minY = std::numeric_limits<int>::max();
		int maxX = std::numeric_limits<int>::min();
		int maxY = std::numeric_limits<int>::min();
		for (auto & item : items) {
			if (!item) continue;
			minX = std::min(minX, item->getX());
			minY = std::min(minY, item->getY());
			maxX = std::max(maxX, item->getX() + item->getWidth());
			maxY = std::max(maxY, item->getY() + item->getHeight());
		}
		mWidth = maxX - minX;
		mHeight = maxY - minY;
		mGuiBounds = ofRectangle(minX, minY, mWidth, mHeight);
		return;
	}
	// Always keep indices up to date
	for (int i = 0; i < items.size(); i++) {
		items[i]->setIndex(i);
	}

	// Collect visible items
	std::vector<ofxDatGuiComponent *> visible;
	visible.reserve(items.size());
	for (auto & item : items) {
		if (item->getVisible()) {
			visible.push_back(item.get());
		}
	}

	if (visible.empty()) {
		mHeight = 0;
		positionGui();
		return;
	}

	if (mOrientation == Orientation::VERTICAL) {
		// Original behaviour
		mHeight = 0;
		for (auto * item : visible) {
			mHeight += item->getHeight() + mRowSpacing;
		}
	} else {
		// HORIZONTAL: single "row" of items, width split across them.
		int availableWidth = mWidth;
		const int spacing = mRowSpacing;
		const int count = static_cast<int>(visible.size());

		// Derive a safe label fraction from the stored label width so we don't
		// hand children a label that eats their entire width when the row is crowded.
		float labelFrac = mLabelWidth;
		if (labelFrac > 1.f && mWidth > 0) {
			labelFrac = mLabelWidth / static_cast<float>(mWidth);
		}
		if (labelFrac <= 0.f) labelFrac = 0.35f;
		if (labelFrac > 0.95f) labelFrac = 0.95f;

		const int totalSpacing = spacing * std::max(0, count - 1);
		int childWidth = (availableWidth - totalSpacing) / std::max(1, count);
		if (childWidth < 1) childWidth = 1;

		int rowHeight = 0;
		for (auto * c : visible) {
			// Give each child an equal share of the width.
			c->setWidth(childWidth, labelFrac);
			rowHeight = std::max(rowHeight, c->getHeight());
		}

		// Match the vertical semantics: one row = child height + bottom margin.
		mHeight = rowHeight + mRowSpacing;
	}

	positionGui();
}


void ofxDatGui::positionGui() {
	if (mManualLayout) {
		// Bounds based on existing item positions (layoutGui computed them)
		if (mGuiBounds.getWidth() == 0 && mGuiBounds.getHeight() == 0 && !items.empty()) {
			layoutGui();
		}
		return;
	}

	// Expanded: place visible items according to orientation.
	if (mOrientation == Orientation::VERTICAL) {
		int h = 0;
		for (int i = 0; i < items.size(); i++) {
			if (!items[i]->getVisible()) continue;
			items[i]->setPosition(mPosition.x, mPosition.y + h);
			h += items[i]->getHeight() + mRowSpacing;
		}
	} else {
		// HORIZONTAL
		std::vector<ofxDatGuiComponent *> visible;
		visible.reserve(items.size());
		for (auto & item : items) {
			if (item->getVisible()) visible.push_back(item.get());
		}

		int x = mPosition.x;
		const int spacing = mRowSpacing;

		for (int i = 0; i < visible.size(); ++i) {
			auto * c = visible[i];
			c->setPosition(x, mPosition.y);
			x += c->getWidth();
			if (i + 1 < visible.size()) {
				x += spacing;
			}
		}
	}

	// Bounds use mWidth x mHeight (as computed by layoutGui)
	mGuiBounds = ofRectangle(mPosition.x, mPosition.y, mWidth, mHeight);
}


/* 
    update & draw loop
*/

void ofxDatGui::update()
{
    ensureSetup();
    if (!mIsSetup) return;
    if (!mVisible) return;

    // check if we need to update components //
    for (int i=0; i<items.size(); i++) {
        if (mAlphaChanged) items[i]->setOpacity(mAlpha);
        if (mWidthChanged) items[i]->setWidth(mWidth, mLabelWidth);
        if (mAlignmentChanged) items[i]->setLabelAlignment(mAlignment);
    }

    if (mThemeChanged || mWidthChanged) layoutGui();

    mAlphaChanged = false;
    mWidthChanged = false;
    mAlignmentChanged = false;

    if (mSlideAnimating) {
        mSlideProgress += static_cast<float>(ofGetLastFrameTime());
        float t = std::clamp(mSlideProgress / mSlideDuration, 0.f, 1.f);
        for (auto & kv : mSlideAnimTargets) {
            auto* c = kv.first;
            if (!c) continue;
            const ofPoint start = kv.second.start;
            const ofPoint target = kv.second.target;
            ofPoint pos = start + (target - start) * t;
            c->setPosition(static_cast<int>(pos.x), static_cast<int>(pos.y));
            const float op = kv.second.startOpacity + (kv.second.targetOpacity - kv.second.startOpacity) * t;
            c->setOpacity(op);
        }
        if (t >= 1.f) {
            for (auto & kv : mSlideAnimTargets) {
                if (kv.first) {
                    kv.first->setPosition(static_cast<int>(kv.second.target.x), static_cast<int>(kv.second.target.y));
                    kv.first->setOpacity(kv.second.targetOpacity);
                }
            }
            mSlideAnimTargets.clear();
            mSlideAnimating = false;
            mPanelsSlidOut = mSlideToOff;
            if (!mPanelsSlidOut) {
                mSavedPanelPositions.clear();
                mSavedPanelOpacities.clear();
            }
        }
    }

    // Snapshot the currently focused text input (if any) before we potentially
    // blur it. We use this to lock interactions for the rest of this frame.
    mFocusedTextInput = nullptr;
    {
        std::function<ofxDatGuiComponent*(ofxDatGuiComponent*)> findFocused =
            [&](ofxDatGuiComponent* node) -> ofxDatGuiComponent* {
                if (node == nullptr || !node->getVisible()) return nullptr;
                if (node->hasFocusedTextInputField()) {
                    return node;
                }
                ofxDatGuiComponent* hit = nullptr;
                node->forEachChild([&](ofxDatGuiComponent* c) {
                    if (hit == nullptr) hit = findFocused(c);
                });
                return hit;
            };
        for (auto & item : items) {
            mFocusedTextInput = findFocused(item.get());
            if (mFocusedTextInput != nullptr) break;
        }
    }

    if (!mEnabled) {
        // disabled: no interaction
        for (int i = 0; i < items.size(); i++)
            items[i]->update(false);
    } else {
        const ofPoint mouse(ofGetMouseX(), ofGetMouseY());
        // On a new mouse press, blur any focused text inputs if the click
        // landed outside all text inputs; then continue normal interaction.
        if (mousePressedThisFrameGui()) {
            bool clickedInsideAnyTextInput = false;
            ofxDatGuiComponent* clickedTextInput = nullptr;

            std::function<void(ofxDatGuiComponent*)> scan =
                [&](ofxDatGuiComponent* node) {
                    if (!node || clickedInsideAnyTextInput) return;
                    if (!node->getVisible()) return;
                    if (node->hitTestTextInputField(mouse)) {
                        clickedInsideAnyTextInput = true;
                        clickedTextInput = node;
                        return;
                    }
                    node->forEachChild(scan);
                };
            for (auto & item : items) {
                scan(item.get());
                if (clickedInsideAnyTextInput) break;
            }

            if (!clickedInsideAnyTextInput) {
                std::function<void(ofxDatGuiComponent*)> blur =
                    [&](ofxDatGuiComponent* node) {
                        if (!node || !node->getVisible()) return;
                        if (node->hasFocusedTextInputField()) {
                            node->onFocusLost();
                        }
                        node->forEachChild(blur);
                    };
                for (auto & item : items) {
                    blur(item.get());
                }
            } else {
                // Switching between text inputs: blur any currently focused ones
                // that weren't clicked, then steer focus/locking toward the clicked input.
                std::function<void(ofxDatGuiComponent*)> blurOthers =
                    [&](ofxDatGuiComponent* node) {
                        if (!node || !node->getVisible()) return;
                        if (node != clickedTextInput && node->hasFocusedTextInputField()) {
                            node->onFocusLost();
                        }
                        node->forEachChild(blurOthers);
                    };
                for (auto & item : items) {
                    blurOthers(item.get());
                }
                if (clickedTextInput != nullptr) {
                    mFocusedTextInput = clickedTextInput;
                }
            }
        }

        // Determine which top-level item should receive interaction:
        // - If there's an active mouse capture, route to that item's top-level owner.
        // - Otherwise, only the topmost visible item under the mouse gets hover/press.
        auto toTopLevel = [&](ofxDatGuiComponent* c) -> ofxDatGuiComponent* {
            while (c && c->getParent() != nullptr) {
                c = c->getParent();
            }
            if (c && c->getRoot() != this) return nullptr;
            return c;
        };
        auto containsPoint = [&](auto&& self, ofxDatGuiComponent* node, const ofPoint& pt) -> bool {
            if (!node || !node->getVisible()) return false;
            if (node->hitTest(pt)) return true;
            bool hit = false;
            node->forEachChild([&](ofxDatGuiComponent* c) {
                if (!hit && self(self, c, pt)) hit = true;
            });
            return hit;
        };

        ofxDatGuiComponent* hoverTarget = nullptr;
        for (int i = static_cast<int>(items.size()) - 1; i >= 0; --i) {
            auto * it = items[i].get();
            if (!it->getVisible()) continue;
            if (containsPoint(containsPoint, it, mouse)) {
                hoverTarget = it;
                break;
            }
        }

        ofxDatGuiComponent* interactionTarget = nullptr;
        if (mFocusedTextInput != nullptr) {
            interactionTarget = toTopLevel(mFocusedTextInput);
        } else {
            interactionTarget = toTopLevel(mMouseCaptureOwner);
        }
        if (interactionTarget == nullptr && mActiveOnHover) {
            interactionTarget = hoverTarget;
        } else if (interactionTarget == nullptr && !mActiveOnHover) {
            // No capture and hover-to-activate disabled: only set on explicit press.
            if (ofGetMousePressed()) {
                interactionTarget = hoverTarget;
            }
        }
        if (interactionTarget != nullptr) {
            mLastFocusedPanel = interactionTarget;
        }
        // Dispatch target for this frame: prefer capture/focus, otherwise the topmost hovered item.
        ofxDatGuiComponent* dispatchTarget = interactionTarget != nullptr ? interactionTarget : hoverTarget;

        if (mThemeChanged) {
            const ofxDatGuiTheme* pending = mPendingOwnedTheme ? mPendingOwnedTheme.get() : mPendingBorrowedTheme;
            if (pending) {
                mRowSpacing = pending->layout.vMargin;
                mGuiBackground = pending->color.guiBackground;
                applyThemeWidth(pending->layout.width, pending->layout.labelWidth);
                ofxDatGuiComponent::ThemeWidthScope scope;
                for (auto & item : items) applyThemeRecursive(item.get(), pending);
                // Promote owned pending to active owned theme
                if (mPendingOwnedTheme) {
                    mOwnedTheme = std::move(mPendingOwnedTheme);
                    mBorrowedTheme = nullptr;
                } else {
                    mBorrowedTheme = mPendingBorrowedTheme ? mPendingBorrowedTheme : mBorrowedTheme;
                }
                layoutGui();
            }
            mPendingBorrowedTheme = nullptr;
            mThemeChanged = false;
        }

        mMoving = false;
        mMouseDown = false;
        auto clearHover = [&](auto&& self, ofxDatGuiComponent* node) -> void {
            if (!node || !node->getVisible()) return;
            node->onMouseLeave(mouse);
            node->forEachChild([&](ofxDatGuiComponent* c) {
                self(self, c);
            });
        };
        // this gui is enabled, always allow mouse/keyboard to reach children
        // 1) Update every item; component layer uses root-managed capture
        //    so only the owning component reacts. Limit to the topmost hovered item when stacked.
        for (int i = 0; i < items.size(); ++i) {
            bool allowEvents = true;
            if (dispatchTarget != nullptr && items[i].get() != dispatchTarget) {
                const bool underMouse = containsPoint(containsPoint, items[i].get(), mouse);
                if (underMouse) {
                    allowEvents = false;
                    clearHover(clearHover, items[i].get());
                }
            }
            items[i]->update(allowEvents);
        }

        // 2) Panel-level mMouseDown = any descendant is down
        // Helper: recursively check descendants for mouse down, against raw child pointers (legacy).
        std::function<bool(ofxDatGuiComponent *)> anyMouseDown = [&](ofxDatGuiComponent * node) -> bool {
            if (!node) return false;
            if (node->getMouseDown()) return true;
            bool hit = false;
            node->forEachChild([&](ofxDatGuiComponent* c) {
                if (!hit && anyMouseDown(c)) hit = true;
            });
            return hit;
        };
        mMouseDown = false;
        for (int i = 0; i < items.size() && !mMouseDown; ++i) {
            if (anyMouseDown(items[i].get())) mMouseDown = true;
        }
	}

  //  if (!getFocused() || !mEnabled){
  //  // update children but ignore mouse & keyboard events //
  //      for (int i=0; i<items.size(); i++) items[i]->update(false);
  //  }   else {
  //      mMoving = false;
  //      mMouseDown = false;
  //  // this gui has focus so let's see if any of its components were interacted with //
  //      if (mExpanded == false){
  //          mGuiFooter->update();
  //          mMouseDown = mGuiFooter->getMouseDown();
		//} else {
		//	// 1) Update every item; root-managed capture in the component layer guarantees only the owner reacts.
		//	for (int i = 0; i < items.size(); ++i) {
		//		items[i]->update(true);
		//	}

		//	// 2) Panel-level mMouseDown = any descendant is down
		//	auto anyMouseDown = [&](auto && self, ofxDatGuiComponent * node) -> bool {
		//		if (node->getMouseDown()) return true;
		//		for (auto * c : node->children)
		//			if (self(self, c)) return true;
		//		return false;
		//	};
		//	mMouseDown = false;
		//	for (int i = 0; i < items.size() && !mMouseDown; ++i) {
		//		if (anyMouseDown(anyMouseDown, items[i])) mMouseDown = true;
		//	}

		//	// 3) Only drag the panel when the header itself is pressed (so other presses don't move it)
		//	if (mGuiHeader != nullptr && mGuiHeader->getDraggable() && mGuiHeader->getMouseDown()) {
		//		mMoving = true;
		//		ofPoint mouse(ofGetMouseX(), ofGetMouseY());
		//		moveGui(mouse - mGuiHeader->getDragOffset());
		//	}
		//

		//}

  //  }
update_epilogue:
// empty the trash //
    for (int i=0; i<trash.size(); i++) delete trash[i];
    trash.clear();
}

void ofxDatGui::draw()
{
    ensureSetup();
    if (!mIsSetup) return;
    if (mVisible == false) return;

    // Determine focused item for muting: prefer last interacted panel if still visible, otherwise topmost visible.
    auto isOwned = [&](ofxDatGuiComponent* c) -> bool {
        if (!c) return false;
        auto it = std::find_if(items.begin(), items.end(), [&](const ComponentPtr& p){ return p.get() == c; });
        return it != items.end();
    };
    ofxDatGuiComponent* topVisible = nullptr;
    for (int i = static_cast<int>(items.size()) - 1; i >= 0; --i) {
        if (items[i]->getVisible()) {
            topVisible = items[i].get();
            break;
        }
    }
    ofxDatGuiComponent* focusForMute = nullptr;
    if (mLastFocusedPanel && mLastFocusedPanel->getVisible() && isOwned(mLastFocusedPanel)) {
        focusForMute = mLastFocusedPanel;
    } else {
        focusForMute = topVisible;
    }

    ofPushStyle();
        if (mExpanded) {
            // Apply muting recursively to everything except the topmost visible item.
            std::vector<std::pair<ofxDatGuiComponent*, float>> muted;
            std::function<void(ofxDatGuiComponent*)> muteTree;
            muteTree = [&](ofxDatGuiComponent* c) {
                if (!c || c == focusForMute) return;
                if (c->getPreventMuting()) return;
                muted.emplace_back(c, c->getOpacity());
                c->applyMutedPalette(getActiveTheme(), true);
                c->forEachChild(muteTree);
            };

            if (mMuteUnfocusedPanels && focusForMute != nullptr) {
                for (auto & item : items) {
                    muteTree(item.get());
                }
            }

            for (int i=0; i<items.size(); i++) {
                items[i]->draw();
                items[i]->drawColorPicker();
            }

            // Restore original opacities.
            for (auto & pair : muted) {
                pair.first->setOpacity(pair.second);
                pair.first->applyMutedPalette(getActiveTheme(), false);
            }
        }
    ofPopStyle();
}

void ofxDatGui::onDraw(ofEventArgs &e)
{
    draw();
}

void ofxDatGui::onUpdate(ofEventArgs &e)
{
    update();
}

void ofxDatGui::onWindowResized(ofResizeEventArgs &e)
{
    // Re-anchor panels to current viewport.
    for (auto & item : items) {
        if (item && item->getType() == ofxDatGuiType::PANEL) {
            auto* panel = static_cast<ofxDatGuiPanel*>(item.get());
            panel->applyAnchor(e.width, e.height);
        }
    }

}


// -----------------------------------------------------------------------------
// Out-of-line definitions from ofxDatGuiGroups.h
// -----------------------------------------------------------------------------

ofxDatGuiDropdown * ofxDatGuiFolder::addDropdown(std::string label,
	const std::vector<std::string> & options) {
	auto * dd = new ofxDatGuiDropdown(std::move(label), options);
	dd->setStripeColor(mStyle.stripe.color);
	dd->onDropdownEvent(this, &ofxDatGuiFolder::dispatchDropdownEvent);
	attachItem(dd);
	return dd;
}

void ofxDatGuiGroup::collapse() {
	releaseMouseCapture();
	auto blurInputs = [&](auto&& self, ofxDatGuiComponent* node) -> void {
		if (node == nullptr) return;
		if (node->hasFocusedTextInputField()) {
			node->onFocusLost();
		}
		node->forEachChild([&](ofxDatGuiComponent* c) {
			self(self, c);
		});
	};
	blurInputs(blurInputs, this);
	mIsExpanded = false;
	layout();
	onGroupToggled();
}

void ofxDatGuiFolder::collapse() {
	releaseMouseCapture();
	auto blurInputs = [&](auto&& self, ofxDatGuiComponent* node) -> void {
		if (node == nullptr) return;
		if (node->hasFocusedTextInputField()) {
			node->onFocusLost();
		}
		node->forEachChild([&](ofxDatGuiComponent* c) {
			self(self, c);
		});
	};
	blurInputs(blurInputs, this);
	mIsExpanded = false;
	layoutChildren();
	onFolderToggled();
}
