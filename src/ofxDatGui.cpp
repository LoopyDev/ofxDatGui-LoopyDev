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

ofxDatGui* ofxDatGui::mActiveGui;
vector<ofxDatGui*> ofxDatGui::mGuis;

ofxDatGui::ofxDatGui()
{
    mPosition.x = 0;
    mPosition.y = 0;
    mAnchor = ofxDatGuiAnchor::NO_ANCHOR;
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
    mGuiHeader = nullptr;
    mGuiFooter = nullptr;
    mAlphaChanged = false;
    mWidthChanged = false;
    mThemeChanged = false;
    mAlignmentChanged = false;
    mAlignment = ofxDatGuiAlignment::LEFT;
    mAlpha = 1.0f;
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
    mAnchor = ofxDatGuiAnchor::NO_ANCHOR;
    mManualLayout = true;
    init();
}

void ofxDatGui::setup(ofxDatGuiAnchor anchor)
{
    if (mIsSetup) return;
    // Anchors deprecated: always manual layout, ignore anchor input.
    mAnchor = ofxDatGuiAnchor::NO_ANCHOR;
    mManualLayout = true;
    mPosition.x = 0;
    mPosition.y = 0;
    init();
}

void ofxDatGui::ensureSetup()
{
    if (!mIsSetup) {
        setup(mAnchor);
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
    if (mGuiFooter != nullptr){
        mExpanded = true;
        mGuiFooter->setExpanded(mExpanded);
        mGuiFooter->setPosition(mPosition.x, mPosition.y + mHeight - mGuiFooter->getHeight() - mRowSpacing);
    }
}

void ofxDatGui::collapse()
{
    ensureSetup();
    if (mGuiFooter != nullptr){
        mExpanded = false;
        mGuiFooter->setExpanded(mExpanded);
        mGuiFooter->setPosition(mPosition.x, mPosition.y);
    }
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

void ofxDatGui::setWidth(int width, float labelWidth)
{
    ensureSetup();
    setWidthInternal(width, labelWidth, true);
}

void ofxDatGui::setWidthInternal(int width, float labelWidth, bool markUser)
{
    mWidth = width;
    mLabelWidth = labelWidth;
    mWidthChanged = true;
    if (markUser) {
        mUserWidthSet = true;
    }
    if (mAnchor != ofxDatGuiAnchor::NO_ANCHOR) positionGui();
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

void ofxDatGui::setPosition(int x, int y)
{
    ensureSetup();
    moveGui(ofPoint(x, y));
}

void ofxDatGui::setPosition(ofxDatGuiAnchor anchor)
{
    ensureSetup();
    // Anchors removed: no-op beyond keeping manual layout.
    mAnchor = ofxDatGuiAnchor::NO_ANCHOR;
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

ofxDatGuiHeader* ofxDatGui::addHeader(string label, bool draggable)
{
    ensureSetup();
    if (mGuiHeader == nullptr){
        auto header = makeOwned<ofxDatGuiHeader>(label, draggable);
        mGuiHeader = header.get();
        if (items.size() == 0){
            items.push_back(std::move(header));
        }   else{
    // always ensure header is at the top of the panel //
            items.insert(items.begin(), std::move(header));
        }
        mGuiHeader->setRoot(this);
        mGuiHeader->onInternalEvent(this, &ofxDatGui::onInternalEventCallback);
        layoutGui();
	}
    return mGuiHeader;
}

ofxDatGuiFooter* ofxDatGui::addFooter()
{
    ensureSetup();
    if (mGuiFooter == nullptr){
        auto footer = makeOwned<ofxDatGuiFooter>();
        mGuiFooter = footer.get();
        items.push_back(std::move(footer));
        mGuiFooter->setRoot(this);
        mGuiFooter->onInternalEvent(this, &ofxDatGui::onInternalEventCallback);
        layoutGui();
	}
    return mGuiFooter;
}

ofxDatGuiLabel* ofxDatGui::addLabel(string label)
{
    auto item = makeOwned<ofxDatGuiLabel>(label);
    auto* raw = static_cast<ofxDatGuiLabel*>(item.get());
    attachItem(std::move(item));
    return raw;
}

ofxDatGuiButton* ofxDatGui::addButton(string label)
{
    auto item = makeOwned<ofxDatGuiButton>(label);
    auto* raw = static_cast<ofxDatGuiButton*>(item.get());
    raw->onButtonEvent(this, &ofxDatGui::onButtonEventCallback);
    attachItem(std::move(item));
    return raw;
}

ofxDatGuiToggle* ofxDatGui::addToggle(string label, bool enabled)
{
    auto item = makeOwned<ofxDatGuiToggle>(label, enabled);
    auto* raw = static_cast<ofxDatGuiToggle*>(item.get());
    raw->onToggleEvent(this, &ofxDatGui::onToggleEventCallback);
    attachItem(std::move(item));
    return raw;
}

ofxDatGuiSlider* ofxDatGui::addSlider(ofParameter<int>& p)
{
    auto item = makeOwned<ofxDatGuiSlider>(p);
    auto* raw = static_cast<ofxDatGuiSlider*>(item.get());
    raw->onSliderEvent(this, &ofxDatGui::onSliderEventCallback);
    attachItem(std::move(item));
    return raw;
}

ofxDatGuiSlider* ofxDatGui::addSlider(ofParameter<float>& p)
{
    auto item = makeOwned<ofxDatGuiSlider>(p);
    auto* raw = static_cast<ofxDatGuiSlider*>(item.get());
    raw->onSliderEvent(this, &ofxDatGui::onSliderEventCallback);
    attachItem(std::move(item));
    return raw;
}

ofxDatGuiSlider* ofxDatGui::addSlider(string label, float min, float max)
{
// default to halfway between min & max values //
    ofxDatGuiSlider* slider = addSlider(label, min, max, (max+min)/2);
    return slider;
}

ofxDatGuiSlider* ofxDatGui::addSlider(string label, float min, float max, float val)
{
    auto item = makeOwned<ofxDatGuiSlider>(label, min, max, val);
    auto* raw = static_cast<ofxDatGuiSlider*>(item.get());
    raw->onSliderEvent(this, &ofxDatGui::onSliderEventCallback);
    attachItem(std::move(item));
    return raw;
}

ofxDatGuiTextInput* ofxDatGui::addTextInput(string label, string value)
{
    auto item = makeOwned<ofxDatGuiTextInput>(label, value);
    auto* raw = static_cast<ofxDatGuiTextInput*>(item.get());
    raw->onTextInputEvent(this, &ofxDatGui::onTextInputEventCallback);
    attachItem(std::move(item));
    return raw;
}

ofxDatGuiColorPicker* ofxDatGui::addColorPicker(string label, ofColor color)
{
    auto item = makeOwned<ofxDatGuiColorPicker>(label, color);
    auto* raw = static_cast<ofxDatGuiColorPicker*>(item.get());
    raw->onColorPickerEvent(this, &ofxDatGui::onColorPickerEventCallback);
    attachItem(std::move(item));
    return raw;
}

ofxDatGuiWaveMonitor* ofxDatGui::addWaveMonitor(string label, float frequency, float amplitude)
{
    auto item = makeOwned<ofxDatGuiWaveMonitor>(label, frequency, amplitude);
    auto* raw = static_cast<ofxDatGuiWaveMonitor*>(item.get());
    attachItem(std::move(item));
    return raw;
}

ofxDatGuiValuePlotter* ofxDatGui::addValuePlotter(string label, float min, float max)
{
    auto item = makeOwned<ofxDatGuiValuePlotter>(label, min, max);
    auto* raw = static_cast<ofxDatGuiValuePlotter*>(item.get());
    attachItem(std::move(item));
    return raw;
}

ofxDatGuiDropdown* ofxDatGui::addDropdown(string label, vector<string> options)
{
    auto item = makeOwned<ofxDatGuiDropdown>(label, options);
    auto* raw = static_cast<ofxDatGuiDropdown*>(item.get());
    raw->onDropdownEvent(this, &ofxDatGui::onDropdownEventCallback);
    attachItem(std::move(item));
    return raw;
}
ofxDatGuiRadioGroup * ofxDatGui::addRadioGroup(const std::string & label, const std::vector<std::string> & options) {
	auto item = makeOwned<ofxDatGuiRadioGroup>(label, options);
	auto* raw = static_cast<ofxDatGuiRadioGroup*>(item.get());
	raw->onRadioGroupEvent(this, &ofxDatGui::onRadioGroupEventCallback);
	attachItem(std::move(item));
	return raw;
}

ofxDatGuiFRM* ofxDatGui::addFRM(float refresh)
{
    auto item = makeOwned<ofxDatGuiFRM>(refresh);
    auto* raw = static_cast<ofxDatGuiFRM*>(item.get());
    attachItem(std::move(item));
    return raw;
}

ofxDatGuiBreak* ofxDatGui::addBreak()
{
    auto item = makeOwned<ofxDatGuiBreak>();
    auto* raw = static_cast<ofxDatGuiBreak*>(item.get());
    attachItem(std::move(item));
    return raw;
}

ofxDatGui2dPad* ofxDatGui::add2dPad(string label)
{
    auto item = makeOwned<ofxDatGui2dPad>(label);
    auto* raw = static_cast<ofxDatGui2dPad*>(item.get());
    raw->on2dPadEvent(this, &ofxDatGui::on2dPadEventCallback);
    attachItem(std::move(item));
    return raw;
}

ofxDatGui2dPad* ofxDatGui::add2dPad(string label, ofRectangle bounds)
{
    auto item = makeOwned<ofxDatGui2dPad>(label, bounds);
    auto* raw = static_cast<ofxDatGui2dPad*>(item.get());
    raw->on2dPadEvent(this, &ofxDatGui::on2dPadEventCallback);
    attachItem(std::move(item));
    return raw;
}

ofxDatGuiMatrix* ofxDatGui::addMatrix(string label, int numButtons, bool showLabels)
{
    auto item = makeOwned<ofxDatGuiMatrix>(label, numButtons, showLabels);
    auto* raw = static_cast<ofxDatGuiMatrix*>(item.get());
    raw->onMatrixEvent(this, &ofxDatGui::onMatrixEventCallback);
    attachItem(std::move(item));
    return raw;
}

ofxDatGuiFolder* ofxDatGui::addFolder(string label, ofColor color)
{
    auto item = makeOwned<ofxDatGuiFolder>(label, color);
    auto* folder = static_cast<ofxDatGuiFolder*>(item.get());
    folder->onButtonEvent(this, &ofxDatGui::onButtonEventCallback);
    folder->onToggleEvent(this, &ofxDatGui::onToggleEventCallback);
    folder->onSliderEvent(this, &ofxDatGui::onSliderEventCallback);
    folder->on2dPadEvent(this, &ofxDatGui::on2dPadEventCallback);
    folder->onMatrixEvent(this, &ofxDatGui::onMatrixEventCallback);
    folder->onTextInputEvent(this, &ofxDatGui::onTextInputEventCallback);
    folder->onColorPickerEvent(this, &ofxDatGui::onColorPickerEventCallback);
    folder->onInternalEvent(this, &ofxDatGui::onInternalEventCallback);
	// LoopyDev
	folder->onRadioGroupEvent(this, &ofxDatGui::onRadioGroupEventCallback);
	folder->onDropdownEvent(this, &ofxDatGui::onDropdownEventCallback);

    attachItem(std::move(item));
    return folder;
}

ofxDatGuiFolder* ofxDatGui::addFolder(ofxDatGuiFolder* folder)
{
    if (!folder) return nullptr;
    ComponentPtr ptr(folder);
    auto* raw = folder;
    attachItem(std::move(ptr));
    return raw;
}

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
    if (mGuiFooter != nullptr){
        items.insert(items.end()-1, std::move(item));
    }   else {
        items.push_back(std::move(item));
    }
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
    component retrieval methods
*/

ofxDatGuiLabel* ofxDatGui::getLabel(string bl, string fl){
    ofxDatGuiLabel* o = nullptr;
    if (fl != ""){
        ofxDatGuiFolder* f = static_cast<ofxDatGuiFolder*>(getComponent(ofxDatGuiType::FOLDER, fl));
        if (f) o = static_cast<ofxDatGuiLabel*>(f->getComponent(ofxDatGuiType::LABEL, bl));
    } else {
        o = static_cast<ofxDatGuiLabel*>(getComponent(ofxDatGuiType::LABEL, bl));
    }
    if (o==nullptr){
        o = ofxDatGuiLabel::getInstance();
        ofxDatGuiLog::write(ofxDatGuiMsg::COMPONENT_NOT_FOUND, fl!="" ? fl+"-"+bl : bl);
        trash.push_back(o);
    }
    return o;
}

ofxDatGuiButton* ofxDatGui::getButton(string bl, string fl)
{
    ofxDatGuiButton* o = nullptr;
    if (fl != ""){
        ofxDatGuiFolder* f = static_cast<ofxDatGuiFolder*>(getComponent(ofxDatGuiType::FOLDER, fl));
        if (f) o = static_cast<ofxDatGuiButton*>(f->getComponent(ofxDatGuiType::BUTTON, bl));
    }   else{
        o = static_cast<ofxDatGuiButton*>(getComponent(ofxDatGuiType::BUTTON, bl));
    }
    if (o==nullptr){
        o = ofxDatGuiButton::getInstance();
        ofxDatGuiLog::write(ofxDatGuiMsg::COMPONENT_NOT_FOUND, fl!="" ? fl+"-"+bl : bl);
        trash.push_back(o);
    }
    return o;
}

ofxDatGuiToggle* ofxDatGui::getToggle(string bl, string fl)
{
    ofxDatGuiToggle* o = nullptr;
    if (fl != ""){
        ofxDatGuiFolder* f = static_cast<ofxDatGuiFolder*>(getComponent(ofxDatGuiType::FOLDER, fl));
        if (f) o = static_cast<ofxDatGuiToggle*>(f->getComponent(ofxDatGuiType::TOGGLE, bl));
    }   else{
        o = static_cast<ofxDatGuiToggle*>(getComponent(ofxDatGuiType::TOGGLE, bl));
    }
    if (o==nullptr){
        o = ofxDatGuiToggle::getInstance();
        ofxDatGuiLog::write(ofxDatGuiMsg::COMPONENT_NOT_FOUND, fl!="" ? fl+"-"+bl : bl);
        trash.push_back(o);
    }
    return o;
}

ofxDatGuiSlider* ofxDatGui::getSlider(string sl, string fl)
{
    ofxDatGuiSlider* o = nullptr;
    if (fl != ""){
        ofxDatGuiFolder* f = static_cast<ofxDatGuiFolder*>(getComponent(ofxDatGuiType::FOLDER, fl));
        if (f) o = static_cast<ofxDatGuiSlider*>(f->getComponent(ofxDatGuiType::SLIDER, sl));
    }   else{
        o = static_cast<ofxDatGuiSlider*>(getComponent(ofxDatGuiType::SLIDER, sl));
    }
    if (o==nullptr){
        o = ofxDatGuiSlider::getInstance();
        ofxDatGuiLog::write(ofxDatGuiMsg::COMPONENT_NOT_FOUND, fl!="" ? fl+"-"+sl : sl);
        trash.push_back(o);
    }
    return o;
}

ofxDatGuiTextInput* ofxDatGui::getTextInput(string tl, string fl)
{
    ofxDatGuiTextInput* o = nullptr;
    if (fl != ""){
        ofxDatGuiFolder* f = static_cast<ofxDatGuiFolder*>(getComponent(ofxDatGuiType::FOLDER, fl));
        if (f) o = static_cast<ofxDatGuiTextInput*>(f->getComponent(ofxDatGuiType::TEXT_INPUT, tl));
    }   else{
        o = static_cast<ofxDatGuiTextInput*>(getComponent(ofxDatGuiType::TEXT_INPUT, tl));
    }
    if (o==nullptr){
        o = ofxDatGuiTextInput::getInstance();
        ofxDatGuiLog::write(ofxDatGuiMsg::COMPONENT_NOT_FOUND, fl!="" ? fl+"-"+tl : tl);
        trash.push_back(o);
    }
    return o;
}

ofxDatGui2dPad* ofxDatGui::get2dPad(string pl, string fl)
{
    ofxDatGui2dPad* o = nullptr;
    if (fl != ""){
        ofxDatGuiFolder* f = static_cast<ofxDatGuiFolder*>(getComponent(ofxDatGuiType::FOLDER, fl));
        if (f) o = static_cast<ofxDatGui2dPad*>(f->getComponent(ofxDatGuiType::PAD2D, pl));
    }   else{
        o = static_cast<ofxDatGui2dPad*>(getComponent(ofxDatGuiType::PAD2D, pl));
    }
    if (o==nullptr){
        o = ofxDatGui2dPad::getInstance();
        ofxDatGuiLog::write(ofxDatGuiMsg::COMPONENT_NOT_FOUND, fl!="" ? fl+"-"+pl : pl);
        trash.push_back(o);
    }
    return o;
}

ofxDatGuiColorPicker* ofxDatGui::getColorPicker(string cl, string fl)
{
    ofxDatGuiColorPicker* o = nullptr;
    if (fl != ""){
        ofxDatGuiFolder* f = static_cast<ofxDatGuiFolder*>(getComponent(ofxDatGuiType::FOLDER, fl));
        if (f) o = static_cast<ofxDatGuiColorPicker*>(f->getComponent(ofxDatGuiType::COLOR_PICKER, cl));
    }   else{
        o = static_cast<ofxDatGuiColorPicker*>(getComponent(ofxDatGuiType::COLOR_PICKER, cl));
    }
    if (o==nullptr){
        o = ofxDatGuiColorPicker::getInstance();
        ofxDatGuiLog::write(ofxDatGuiMsg::COMPONENT_NOT_FOUND, fl!="" ? fl+"-"+cl : cl);
        trash.push_back(o);
    }
    return o;
}

ofxDatGuiWaveMonitor* ofxDatGui::getWaveMonitor(string cl, string fl)
{
    ofxDatGuiWaveMonitor* o = nullptr;
    if (fl != ""){
        ofxDatGuiFolder* f = static_cast<ofxDatGuiFolder*>(getComponent(ofxDatGuiType::FOLDER, fl));
        if (f) o = static_cast<ofxDatGuiWaveMonitor*>(f->getComponent(ofxDatGuiType::WAVE_MONITOR, cl));
    }   else{
        o = static_cast<ofxDatGuiWaveMonitor*>(getComponent(ofxDatGuiType::WAVE_MONITOR, cl));
    }
    if (o==nullptr){
        o = ofxDatGuiWaveMonitor::getInstance();
        ofxDatGuiLog::write(ofxDatGuiMsg::COMPONENT_NOT_FOUND, fl!="" ? fl+"-"+cl : cl);
        trash.push_back(o);
    }
    return o;
}

ofxDatGuiValuePlotter* ofxDatGui::getValuePlotter(string cl, string fl)
{
    ofxDatGuiValuePlotter* o = nullptr;
    if (fl != ""){
        ofxDatGuiFolder* f = static_cast<ofxDatGuiFolder*>(getComponent(ofxDatGuiType::FOLDER, fl));
        if (f) o = static_cast<ofxDatGuiValuePlotter*>(f->getComponent(ofxDatGuiType::VALUE_PLOTTER, cl));
    }   else{
        o = static_cast<ofxDatGuiValuePlotter*>(getComponent(ofxDatGuiType::VALUE_PLOTTER, cl));
    }
    if (o==nullptr){
        o = ofxDatGuiValuePlotter::getInstance();
        ofxDatGuiLog::write(ofxDatGuiMsg::COMPONENT_NOT_FOUND, fl!="" ? fl+"-"+cl : cl);
        trash.push_back(o);
    }
    return o;
}

ofxDatGuiMatrix* ofxDatGui::getMatrix(string ml, string fl)
{
    ofxDatGuiMatrix* o = nullptr;
    if (fl != ""){
        ofxDatGuiFolder* f = static_cast<ofxDatGuiFolder*>(getComponent(ofxDatGuiType::FOLDER, fl));
        if (f) o = static_cast<ofxDatGuiMatrix*>(f->getComponent(ofxDatGuiType::MATRIX, ml));
    }   else{
        o = static_cast<ofxDatGuiMatrix*>(getComponent(ofxDatGuiType::MATRIX, ml));
    }
    if (o==nullptr){
        o = ofxDatGuiMatrix::getInstance();
        ofxDatGuiLog::write(ofxDatGuiMsg::COMPONENT_NOT_FOUND, fl!="" ? fl+"-"+ml : ml);
        trash.push_back(o);
    }
    return o;
}

ofxDatGuiDropdown* ofxDatGui::getDropdown(string dl)
{
    ofxDatGuiDropdown* o = static_cast<ofxDatGuiDropdown*>(getComponent(ofxDatGuiType::DROPDOWN, dl));
    if (o==NULL){
        o = ofxDatGuiDropdown::getInstance();
        ofxDatGuiLog::write(ofxDatGuiMsg::COMPONENT_NOT_FOUND, dl);
        trash.push_back(o);
    }
    return o;
}
// LoopyDev: Radio Groups
ofxDatGuiRadioGroup * ofxDatGui::getRadioGroup(string rl) {
	auto * o = static_cast<ofxDatGuiRadioGroup *>(getComponent(ofxDatGuiType::RADIO_GROUP, rl));
	if (o == nullptr) {
		o = ofxDatGuiRadioGroup::getInstance();
		ofxDatGuiLog::write(ofxDatGuiMsg::COMPONENT_NOT_FOUND, rl);
		trash.push_back(o);
	}
	return o;
}

ofxDatGuiButtonBar * ofxDatGui::getButtonBar(string bl) {
	auto * o = static_cast<ofxDatGuiButtonBar *>(getComponent(ofxDatGuiType::BUTTON_BAR, bl));
	if (o == nullptr) {
		o = ofxDatGuiButtonBar::getInstance();
		ofxDatGuiLog::write(ofxDatGuiMsg::COMPONENT_NOT_FOUND, bl);
		trash.push_back(o);
	}
	return o;
}


ofxDatGuiFolder* ofxDatGui::getFolder(string fl)
{
    ofxDatGuiFolder* o = static_cast<ofxDatGuiFolder*>(getComponent(ofxDatGuiType::FOLDER, fl));
    if (o==NULL){
        o = ofxDatGuiFolder::getInstance();
        ofxDatGuiLog::write(ofxDatGuiMsg::COMPONENT_NOT_FOUND, fl);
        trash.push_back(o);
    }
    return o;
}

ofxDatGuiHeader* ofxDatGui::getHeader()
{
    ofxDatGuiHeader* o;
    if (mGuiHeader != nullptr){
        o = mGuiHeader;
    }   else{
        o = new ofxDatGuiHeader("X");
        ofxDatGuiLog::write(ofxDatGuiMsg::COMPONENT_NOT_FOUND, "HEADER");
        trash.push_back(o);
    }
    return o;
}

ofxDatGuiFooter* ofxDatGui::getFooter()
{
    ofxDatGuiFooter* o;
    if (mGuiFooter != nullptr){
        o = mGuiFooter;
    }   else{
        o = new ofxDatGuiFooter();
        ofxDatGuiLog::write(ofxDatGuiMsg::COMPONENT_NOT_FOUND, "FOOTER");
        trash.push_back(o);
    }
    return o;
}

ofxDatGuiComponent* ofxDatGui::getComponent(ofxDatGuiType type, string label)
{
	std::function<ofxDatGuiComponent*(ofxDatGuiComponent*)> findMatch =
		[&](ofxDatGuiComponent* node) -> ofxDatGuiComponent* {
			if (!node) return nullptr;
			if (node->getType() == type && node->is(label)) return node;
			ofxDatGuiComponent* found = nullptr;
			node->forEachChild([&](ofxDatGuiComponent* c) {
				if (!found) found = findMatch(c);
			});
			return found;
		};

	for (auto & item : items) {
		if (auto* hit = findMatch(item.get())) return hit;
	}
	return nullptr;
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
    mAnchor = ofxDatGuiAnchor::NO_ANCHOR;
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
	if (!mExpanded && mGuiFooter != nullptr) {
		// Collapsed: footer only, same as before.
		mGuiFooter->setPosition(mPosition.x, mPosition.y);
		mGuiBounds = ofRectangle(mPosition.x, mPosition.y, mWidth, mGuiFooter->getHeight());
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
    
    if (!mEnabled) {
        // disabled: no interaction
        for (int i = 0; i < items.size(); i++)
            items[i]->update(false);
    } else {
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

        ofxDatGuiComponent* interactionTarget = toTopLevel(mMouseCaptureOwner);
        if (interactionTarget == nullptr && mActiveOnHover) {
            ofPoint mouse(ofGetMouseX(), ofGetMouseY());
            for (int i = static_cast<int>(items.size()) - 1; i >= 0; --i) {
                auto * it = items[i].get();
                if (!it->getVisible()) continue;
                if (containsPoint(containsPoint, it, mouse)) {
                    interactionTarget = it;
                    break;
                }
            }
        } else if (interactionTarget == nullptr && !mActiveOnHover) {
            // No capture and hover-to-activate disabled: only set on explicit press.
            if (ofGetMousePressed()) {
                ofPoint mouse(ofGetMouseX(), ofGetMouseY());
                for (int i = static_cast<int>(items.size()) - 1; i >= 0; --i) {
                    auto * it = items[i].get();
                    if (!it->getVisible()) continue;
                    if (containsPoint(containsPoint, it, mouse)) {
                        interactionTarget = it;
                        break;
                    }
                }
            }
        }
        if (interactionTarget != nullptr) {
            mLastFocusedPanel = interactionTarget;
        }

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
        // this gui is enabled, always allow mouse/keyboard to reach children
        if (mExpanded == false) {
            mGuiFooter->update();
			mMouseDown = mGuiFooter->getMouseDown();
		} else {
			// 1) Update every item; component layer uses root-managed capture
			//    so only the owning component reacts. Limit to the topmost hovered item when stacked.
            for (int i = 0; i < items.size(); ++i) {
                const bool allowEvents = (interactionTarget == nullptr) ? true : (items[i].get() == interactionTarget);
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

			// 3) Only drag the panel when the header itself is pressed
			if (mGuiHeader != nullptr && mGuiHeader->getDraggable() && mGuiHeader->getMouseDown()) {
				mMoving = true;
				ofPoint mouse(ofGetMouseX(), ofGetMouseY());
				moveGui(mouse - mGuiHeader->getDragOffset());
			}
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
        if (mExpanded == false && mGuiFooter != nullptr){
            mGuiFooter->draw();
        }   else{
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
    if (mAnchor != ofxDatGuiAnchor::NO_ANCHOR) positionGui();
}


ofxDatGuiButtonBar * ofxDatGui::addButtonBar(const std::string & label,
	const std::vector<std::string> & buttons) {
	auto bar = makeOwned<ofxDatGuiButtonBar>(label, buttons);
	auto * raw = static_cast<ofxDatGuiButtonBar*>(bar.get());

	// Wire each inner button into the gui's normal button callback,
	// so they behave like regular top-level buttons.
	for (auto * child : raw->children) {
		if (child->getType() == ofxDatGuiType::BUTTON) {
			auto * btn = static_cast<ofxDatGuiButton *>(child);
			btn->onButtonEvent(this, &ofxDatGui::onButtonEventCallback);
		}
	}

	attachItem(std::move(bar));
	return raw;
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
	mIsExpanded = false;
	layout();
	onGroupToggled();
}

void ofxDatGuiFolder::collapse() {
	releaseMouseCapture();
	mIsExpanded = false;
	layoutChildren();
	onFolderToggled();
}

