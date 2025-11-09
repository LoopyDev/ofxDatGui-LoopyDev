// ofxDatGuiCubicBezier.h
// Cubic-bezier editor with 4 inline ofxDatGuiTextInputField boxes.
// Renders a curve pad (P0=(0,1)?P3=(1,0)), two draggable control points,
// and four numeric inputs (x1, y1, x2, y2). Designed to embed in ofxDatGui.
//
// Public API:
//  - setPoints(x1,y1,x2,y2[,dispatch])
//  - getPoints(...)
//  - getCssString([precision])
//  - onCubicBezierEvent(listener, method)  // event: (x1,y1,x2,y2)

#pragma once
#include "ofxDatGuiComponent.h"
#include "ofxDatGuiTextInputField.h"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>

struct ofxDatGuiCubicBezierEvent; // declared in ofxDatGuiEvents.h

class ofxDatGuiCubicBezier : public ofxDatGuiComponent {
public:
	// padAspect = padHeight / padWidth (1.0 = square)
	ofxDatGuiCubicBezier(string label,
		float _x1 = 0.25f, float _y1 = 0.10f,
		float _x2 = 0.25f, float _y2 = 1.00f,
		float padAspect = 1.0f)
		: ofxDatGuiComponent(label) {
		// Use an existing type so theme/stripe styling applies.
		mType = ofxDatGuiType::PAD2D;

		mPadAspect = std::max(0.05f, padAspect);
		x1 = clamp01(_x1);
		y1 = clamp01(_y1);
		x2 = clamp01(_x2);
		y2 = clamp01(_y2);

		// Numeric inputs.
		inX1.setTextInputFieldType(ofxDatGuiInputType::NUMERIC);
		inY1.setTextInputFieldType(ofxDatGuiInputType::NUMERIC);
		inX2.setTextInputFieldType(ofxDatGuiInputType::NUMERIC);
		inY2.setTextInputFieldType(ofxDatGuiInputType::NUMERIC);

		// Field change hooks.
		inX1.onInternalEvent(this, &ofxDatGuiCubicBezier::onX1Changed);
		inY1.onInternalEvent(this, &ofxDatGuiCubicBezier::onY1Changed);
		inX2.onInternalEvent(this, &ofxDatGuiCubicBezier::onX2Changed);
		inY2.onInternalEvent(this, &ofxDatGuiCubicBezier::onY2Changed);

		syncFieldsFromModel();
		setTheme(ofxDatGuiComponent::getTheme());
	}

	// Always "expanded" so the framework routes events consistently.
	bool getIsExpanded() override { return true; }

	// --- Theming / sizing ---------------------------------------------------

	void setTheme(const ofxDatGuiTheme * theme) override {
		setComponentStyle(theme);
		mStyle.height = theme->layout.height;

		// Colors.
		mColors.fill = theme->color.inputAreaBackground;
		mColors.grid = ofColor(255, 30);
		mColors.axis = ofColor(255, 60);
		mColors.curve = theme->color.slider.fill;
		mColors.handle = theme->color.pad2d.ball;
		mColors.handleHL = ofColor::white;

		// Fields adopt theme.
		inX1.setTheme(theme);
		inY1.setTheme(theme);
		inX2.setTheme(theme);
		inY2.setTheme(theme);

		// Metrics.
		mHandleRadius = 6;
		mCurveThickness = 3;
		mInnerPadV = std::max(mHandleRadius + 2, 6);
		mInnerPadH = std::max(mHandleRadius + 2, 6);
		mInputsHeight = theme->layout.height;
		mInputsTopGap = theme->layout.vMargin + 8; // minimum gap above inputs
		mInputsBottomGap = theme->layout.vMargin; // minimum gap below inputs
		mInputsGap = 8;

		setWidth(theme->layout.width, theme->layout.labelWidth);
	}

	void setWidth(int w, float labelW) override {
		ofxDatGuiComponent::setWidth(w, labelW);
		ofxDatGuiComponent::positionLabel();
		recomputeHeightForAspect();
	}

	void setPosition(int px, int py) override {
		ofxDatGuiComponent::setPosition(px, py);
		// Layout is recalculated each frame.
	}

	// --- Public API ---------------------------------------------------------

	/// Set control points in [0..1]. Optionally dispatch change event.
	void setPoints(float _x1, float _y1, float _x2, float _y2, bool dispatch = true) {
		x1 = clamp01(_x1);
		y1 = clamp01(_y1);
		x2 = clamp01(_x2);
		y2 = clamp01(_y2);
		syncFieldsFromModel();
		if (dispatch) dispatchEvent();
	}

	/// Get current control points.
	void getPoints(float & _x1, float & _y1, float & _x2, float & _y2) const {
		_x1 = x1;
		_y1 = y1;
		_x2 = x2;
		_y2 = y2;
	}

	/// Return a CSS cubic-bezier() string.
	std::string getCssString(int precision = 3) const {
		std::ostringstream ss;
		ss << "cubic-bezier(" << fmt(x1, precision) << ", " << fmt(y1, precision)
		   << ", " << fmt(x2, precision) << ", " << fmt(y2, precision) << ")";
		return ss.str();
	}

	/// Register for value change events: ofxDatGuiCubicBezierEvent(this,x1,y1,x2,y2)
	template <class Listener>
	void onCubicBezierEvent(Listener * owner, void (Listener::*fn)(ofxDatGuiCubicBezierEvent)) {
		cubicBezierEventCallback = std::bind(fn, owner, std::placeholders::_1);
	}

	// --- Lifecycle ----------------------------------------------------------

	void update(bool acceptEvents = true) override {
		ofxDatGuiComponent::update(acceptEvents);
		// Layout before hit-tests to keep rectangles current.
		computePadRect();
		layoutInputs();
	}

	void draw() override {
		if (!mVisible) return;
		ofxDatGuiComponent::draw();

		// Pad.
		ofPushStyle();
		ofFill();
		ofSetColor(mColors.fill);
		ofDrawRectangle(mPad);

		// Grid & axes.
		ofSetColor(mColors.grid);
		for (int i = 1; i < 4; ++i) {
			float t = i / 4.f;
			ofDrawLine(mPad.x + t * mPad.width, mPad.y, mPad.x + t * mPad.width, mPad.y + mPad.height);
			ofDrawLine(mPad.x, mPad.y + t * mPad.height, mPad.x + mPad.width, mPad.y + t * mPad.height);
		}
		ofNoFill();
		ofSetColor(mColors.axis);
		ofDrawRectangle(mPad);

		// Control points (CSS y flipped in screen space).
		const ofPoint P0s = normToScreen({ 0, 1 });
		const ofPoint P1s = normToScreen({ x1, 1.f - y1 });
		const ofPoint P2s = normToScreen({ x2, 1.f - y2 });
		const ofPoint P3s = normToScreen({ 1, 0 });

		// Tangents & curve.
		ofSetColor(255, 70);
		ofDrawLine(P0s, P1s);
		ofDrawLine(P2s, P3s);
		ofSetColor(mColors.curve);
		ofSetLineWidth(mCurveThickness);
		ofPolyline pl;
		for (int i = 0; i <= 64; ++i)
			pl.addVertex(cubic(P0s, P1s, P2s, P3s, i / 64.f));
		pl.draw();

		// Handles.
		drawHandle(P1s, dragging == Dragging::P1);
		drawHandle(P2s, dragging == Dragging::P2);
		ofPopStyle();

		// Inputs.
		inX1.draw();
		inY1.draw();
		inX2.draw();
		inY2.draw();
	}

	bool hitTest(ofPoint m) override {
		if (!mEnabled || !mVisible) return false;
		return (m.x >= x && m.x <= x + mStyle.width && m.y >= y && m.y <= y + mStyle.height);
	}

	// --- Interaction --------------------------------------------------------

	void onMousePress(ofPoint m) override {
		// Ensure component receives key events.
		ofxDatGuiComponent::onMousePress(m);
		if (!mFocused) ofxDatGuiComponent::onFocus();

		// Up-to-date layout for hit-tests.
		computePadRect();
		layoutInputs();

		// Inputs take precedence; focus the clicked field.
		if (inX1.hitTest(m)) {
			focusOnly(inX1);
			return;
		}
		if (inY1.hitTest(m)) {
			focusOnly(inY1);
			return;
		}
		if (inX2.hitTest(m)) {
			focusOnly(inX2);
			return;
		}
		if (inY2.hitTest(m)) {
			focusOnly(inY2);
			return;
		}

		// Otherwise, begin pad interaction.
		const ofPoint P1s = normToScreen({ x1, 1.f - y1 });
		const ofPoint P2s = normToScreen({ x2, 1.f - y2 });
		const float r2 = float((mHandleRadius + 2) * (mHandleRadius + 2));

		if (dist2(m, P1s) <= r2)
			dragging = Dragging::P1;
		else if (dist2(m, P2s) <= r2)
			dragging = Dragging::P2;
		else if (mPad.inside(m))
			dragging = ((m - P1s).lengthSquared() < (m - P2s).lengthSquared() ? Dragging::P1 : Dragging::P2);
		else
			dragging = Dragging::None;
	}

	void onMouseDrag(ofPoint m) override {
		if (dragging == Dragging::None) return;

		const float nx = ofClamp((m.x - mPad.x) / mPad.width, 0.f, 1.f);
		const float ny = ofClamp((m.y - mPad.y) / mPad.height, 0.f, 1.f);
		const float ly = 1.f - ny;

		if (dragging == Dragging::P1) {
			x1 = nx;
			y1 = ly;
		} else {
			x2 = nx;
			y2 = ly;
		}

		syncFieldsFromModel();
		dispatchEvent();
	}

	// ofxDatGuiCubicBezier.h  (inside class)
	void onMouseRelease(ofPoint m) override {
		ofxDatGuiComponent::onMouseRelease(m);
		dragging = Dragging::None;

		// If no inline field is active, release focus so siblings can get events
		if (!(inX1.hasFocus() || inY1.hasFocus() || inX2.hasFocus() || inY2.hasFocus())) {
			blurAll(); // keep things tidy
			ofxDatGuiComponent::onFocusLost();
		}
	}


	void onFocusLost() override {
		ofxDatGuiComponent::onFocusLost();
		dragging = Dragging::None;
		blurAll();
	}

	void onKeyPressed(int key) override {
		// Forward to focused field; fall back to base handler.
		if (inX1.hasFocus())
			inX1.onKeyPressed(key);
		else if (inY1.hasFocus())
			inY1.onKeyPressed(key);
		else if (inX2.hasFocus())
			inX2.onKeyPressed(key);
		else if (inY2.hasFocus())
			inY2.onKeyPressed(key);
		else
			ofxDatGuiComponent::onKeyPressed(key);
	}

private:
	enum class Dragging { None,
		P1,
		P2 };
	Dragging dragging = Dragging::None;

	// Model (normalized 0..1).
	float x1 = 0.25f, y1 = 0.10f, x2 = 0.25f, y2 = 1.00f;

	// Drawing metrics.
	ofRectangle mPad;
	int mHandleRadius = 6;
	int mCurveThickness = 3;
	int mInnerPadV = 6;
	int mInnerPadH = 6;
	float mPadAspect = 1.0f;

	// Inputs row metrics.
	int mInputsHeight = 28;
	int mInputsTopGap = 10;
	int mInputsGap = 8;
	int mInputsBottomGap = 10;

	// Inline fields.
	ofxDatGuiTextInputField inX1;
	ofxDatGuiTextInputField inY1;
	ofxDatGuiTextInputField inX2;
	ofxDatGuiTextInputField inY2;

	struct {
		ofColor fill, grid, axis, curve, handle, handleHL;
	} mColors;

	std::function<void(ofxDatGuiCubicBezierEvent)> cubicBezierEventCallback = nullptr;

	// --- Helpers ------------------------------------------------------------

	static float clamp01(float v) { return v < 0 ? 0 : (v > 1 ? 1 : v); }
	static std::string fmt(float v, int p) {
		std::ostringstream ss;
		ss << std::fixed << std::setprecision(p) << v;
		return ss.str();
	}
	static float dist2(const ofPoint & a, const ofPoint & b) {
		float dx = a.x - b.x, dy = a.y - b.y;
		return dx * dx + dy * dy;
	}

	ofPoint normToScreen(ofPoint n) const {
		return { mPad.x + n.x * mPad.width, mPad.y + n.y * mPad.height };
	}

	static ofPoint cubic(const ofPoint & P0, const ofPoint & P1, const ofPoint & P2, const ofPoint & P3, float t) {
		const float u = 1.f - t, uu = u * u, uuu = uu * u;
		const float tt = t * t, ttt = tt * t;
		ofPoint p = P0 * uuu;
		p += P1 * (3 * uu * t);
		p += P2 * (3 * u * tt);
		p += P3 * ttt;
		return p;
	}

	void drawHandle(const ofPoint & p, bool highlight) {
		ofPushStyle();
		ofFill();
		ofSetColor(highlight ? mColors.handleHL : mColors.handle);
		ofDrawCircle(p, mHandleRadius);
		ofNoFill();
		ofSetColor(0, 50);
		ofDrawCircle(p, mHandleRadius + 2);
		ofPopStyle();
	}

	// Height = outer padding + inner padding + pad + gaps + inputs.
	void recomputeHeightForAspect() {
		const int padAvailW = std::max(0, (int)(mStyle.width - mStyle.padding - (int)mLabel.width));
		const int innerW = std::max(0, padAvailW - 2 * mInnerPadH);
		int desiredPadH = std::max(1, (int)std::round(innerW * mPadAspect));
		const int extra = mInputsTopGap + mInputsHeight + mInputsBottomGap;
		mStyle.height = 2 * mStyle.padding + 2 * mInnerPadV + desiredPadH + extra;
	}

	// Compute pad rectangle and clamp size to leave required space for inputs.
	void computePadRect() {
		const int padAvailW = std::max(0, (int)(mStyle.width - mStyle.padding - (int)mLabel.width));
		const int padAvailH = std::max(0, (int)mStyle.height - (int)(2 * mStyle.padding));
		int innerW = std::max(1, padAvailW - 2 * mInnerPadH);
		int innerH = std::max(1, padAvailH - 2 * mInnerPadV);

		int padW = innerW;
		int padH = std::max(1, (int)std::round(padW * mPadAspect));
		const int reservedBelow = mInputsTopGap + mInputsHeight + mInputsBottomGap;
		if (padH > innerH - reservedBelow) {
			padH = std::max(1, innerH - reservedBelow);
			padW = std::max(1, (int)std::round(padH / mPadAspect));
		}

		mPad.width = padW;
		mPad.height = padH;
		mPad.x = x + mLabel.width + mStyle.padding + (innerW - padW) / 2;
		mPad.y = y + mStyle.padding + mInnerPadV;
	}

	// Place inputs below the pad. Top gap is centered within the remaining space,
	// clamped by minimum top/bottom gaps.
	void layoutInputs() {
		const float totalW = mPad.width;
		const float boxW = (totalW - 3 * mInputsGap) / 4.f;
		float xLeft = mPad.x;

		const int padAvailH = std::max(0, (int)mStyle.height - (int)(2 * mStyle.padding));
		const int innerH = std::max(0, padAvailH - 2 * mInnerPadV);
		const int remaining = std::max(0, innerH - (int)mPad.height);

		int idealTopGap = (remaining - mInputsHeight) / 2;
		idealTopGap = std::max(idealTopGap, mInputsTopGap);
		idealTopGap = std::min(idealTopGap, remaining - mInputsHeight - mInputsBottomGap);

		const int topGap = std::max(0, idealTopGap);
		const float yTop = mPad.y + mPad.height + topGap;

		auto place = [&](ofxDatGuiTextInputField & f) {
			f.setWidth((int)boxW);
			f.setPosition((int)xLeft, (int)yTop);
			xLeft += boxW + mInputsGap;
		};
		place(inX1);
		place(inY1);
		place(inX2);
		place(inY2);
	}

	// Field <-> model sync.
	void syncFieldsFromModel() {
		inX1.setText(fmt(x1, 3));
		inY1.setText(fmt(y1, 3));
		inX2.setText(fmt(x2, 3));
		inY2.setText(fmt(y2, 3));
	}

	static float parseClamped01(const std::string & s, float fallback) {
		std::string t = s;
		for (auto & c : t)
			if (c == ',') c = '.';
		try {
			return clamp01(ofToFloat(t));
		} catch (...) {
			return clamp01(fallback);
		}
	}

	void applyAndDispatch() {
		syncFieldsFromModel();
		dispatchEvent();
	}

	// Focus utilities. Ensure the component is focused so it receives keys.
	void blurAll() {
		if (inX1.hasFocus()) inX1.onFocusLost();
		if (inY1.hasFocus()) inY1.onFocusLost();
		if (inX2.hasFocus()) inX2.onFocusLost();
		if (inY2.hasFocus()) inY2.onFocusLost();
	}
	void focusOnly(ofxDatGuiTextInputField & f) {
		if (&f != &inX1 && inX1.hasFocus()) inX1.onFocusLost();
		if (&f != &inY1 && inY1.hasFocus()) inY1.onFocusLost();
		if (&f != &inX2 && inX2.hasFocus()) inX2.onFocusLost();
		if (&f != &inY2 && inY2.hasFocus()) inY2.onFocusLost();
		ofxDatGuiComponent::onFocus();
		f.onFocus();
	}

	// Field -> model handlers.
	void onX1Changed(ofxDatGuiInternalEvent) {
		x1 = parseClamped01(inX1.getText(), x1);
		applyAndDispatch();
	}
	void onY1Changed(ofxDatGuiInternalEvent) {
		y1 = parseClamped01(inY1.getText(), y1);
		applyAndDispatch();
	}
	void onX2Changed(ofxDatGuiInternalEvent) {
		x2 = parseClamped01(inX2.getText(), x2);
		applyAndDispatch();
	}
	void onY2Changed(ofxDatGuiInternalEvent) {
		y2 = parseClamped01(inY2.getText(), y2);
		applyAndDispatch();
	}

	void dispatchEvent() {
		if (!cubicBezierEventCallback) return;
		cubicBezierEventCallback(ofxDatGuiCubicBezierEvent(this, x1, y1, x2, y2));
	}
};
