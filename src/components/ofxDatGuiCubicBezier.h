/*
    Cubic Bezier editor component for ofxDatGui
    - P0 = (0,0), P3 = (1,1) fixed (CSS-like)
    - Draggable P1(x1,y1) and P2(x2,y2) inside [0..1]x[0..1]
    - Emits onCubicBezierEvent with x1,y1,x2,y2 when changed
*/

#pragma once
#include "ofxDatGuiComponent.h"
#include <functional>
#include <iomanip>
#include <sstream>

class ofxDatGuiCubicBezier;

// ---------- Component ----------
class ofxDatGuiCubicBezier : public ofxDatGuiComponent {
public:
	ofxDatGuiCubicBezier(string label, float _x1 = 0.25f, float _y1 = 0.1f, float _x2 = 0.25f, float _y2 = 1.0f)
		: ofxDatGuiComponent(label) {
		mType = ofxDatGuiType::CUBIC_BEZIER; // If you add a new enum value, set it to that instead.
		setPoints(_x1, _y1, _x2, _y2, /*dispatch*/ false);
		setTheme(ofxDatGuiComponent::getTheme());
	}

	void setTheme(const ofxDatGuiTheme * theme) override {
		setComponentStyle(theme);
		// Height similar to 2D pad for a nice editor canvas
		mStyle.height = theme->layout.pad2d.height;
		// Reuse pad2d/slider colors for consistency
		mStyle.stripe.color = theme->stripe.pad2d;
		mColors.fill = theme->color.inputAreaBackground;
		mColors.grid = ofColor(255, 30);
		mColors.axis = ofColor(255, 60);
		mColors.curve = theme->color.slider.fill;
		mColors.handle = theme->color.pad2d.ball;
		mColors.handleHL = ofColor::white;
		mHandleRadius = 6;
		mCurveThickness = 3;
		setWidth(theme->layout.width, theme->layout.labelWidth);
	}

	void setWidth(int width, float labelWidth = 1) override {
		ofxDatGuiComponent::setWidth(width, labelWidth);
		ofxDatGuiComponent::positionLabel();
	}

	// --- API ---
	void setPoints(float _x1, float _y1, float _x2, float _y2, bool dispatch = true) {
		x1 = clamp01(_x1);
		y1 = clamp01(_y1);
		x2 = clamp01(_x2);
		y2 = clamp01(_y2);
		if (dispatch) dispatchEvent();
	}

	void getPoints(float & _x1, float & _y1, float & _x2, float & _y2) const {
		_x1 = x1;
		_y1 = y1;
		_x2 = x2;
		_y2 = y2;
	}

	std::string getCssString(int precision = 3) const {
		std::ostringstream ss;
		ss << "cubic-bezier(" << f(x1, precision) << ", " << f(y1, precision)
		   << ", " << f(x2, precision) << ", " << f(y2, precision) << ")";
		return ss.str();
	}

	// Register listener (same pattern as other components)
	template <class Listener>
	void onCubicBezierEvent(Listener * owner, void (Listener::*listenerMethod)(ofxDatGuiCubicBezierEvent)) {
		cubicBezierEventCallback = std::bind(listenerMethod, owner, std::placeholders::_1);
	}

	// --- Core ---
	void update(bool acceptEvents = true) override {
		ofxDatGuiComponent::update(acceptEvents);
	}

	void draw() override {
		if (!mVisible) return;

		ofPushStyle();
		ofxDatGuiComponent::draw();

		// Compute pad rect
		mPad.x = x + mLabel.width;
		mPad.y = y + mStyle.padding;
		mPad.width = mStyle.width - mStyle.padding - mLabel.width;
		mPad.height = mStyle.height - 2 * mStyle.padding;

		// Background
		ofSetColor(mColors.fill);
		ofDrawRectangle(mPad);

		// Grid (quarter lines)
		ofSetLineWidth(1);
		ofSetColor(mColors.grid);
		for (int i = 1; i <= 3; i++) {
			float gx = mPad.x + (mPad.width * i) / 4.f;
			float gy = mPad.y + (mPad.height * i) / 4.f;
			ofDrawLine(gx, mPad.y, gx, mPad.y + mPad.height);
			ofDrawLine(mPad.x, gy, mPad.x + mPad.width, gy);
		}

		// Axes (x & y)
		ofSetColor(mColors.axis);
		ofDrawLine(mPad.x, mPad.y + mPad.height, mPad.x + mPad.width, mPad.y + mPad.height); // x-axis (bottom)
		ofDrawLine(mPad.x, mPad.y, mPad.x, mPad.y + mPad.height); // y-axis (left)

		// Control points (to screen)
		auto P0 = normToScreen({ 0, 1 }); // remember GUI Y grows down; (0,0) is bottom-left in logical space
		auto P3 = normToScreen({ 1, 0 });
		auto P1 = normToScreen({ x1, 1.f - y1 });
		auto P2 = normToScreen({ x2, 1.f - y2 });

		// Curve
		ofNoFill();
		ofSetLineWidth(mCurveThickness);
		ofSetColor(mColors.curve);
		drawCubicBezier(P0, P1, P2, P3);

		// Tangent lines
		ofSetLineWidth(1);
		ofSetColor(mColors.grid);
		ofDrawLine(P0, P1);
		ofDrawLine(P2, P3);

		// Handles
		drawHandle(P1, dragging == Dragging::P1);
		drawHandle(P2, dragging == Dragging::P2);

		//// Small label of current values in the input area (top-right of pad)
		//ofSetColor(255);
		//ofDrawBitmapStringHighlight(getCssString(), mPad.x + mPad.width - 215, mPad.y + 16);

		ofPopStyle();
	}

	// --- Interaction ---
	bool hitTest(ofPoint m) override {
		// Accept events anywhere on this component’s rect.
		return (m.x >= x && m.x <= x + mStyle.width && m.y >= y && m.y <= y + mStyle.height);
	}

protected:
	void onMousePress(ofPoint m) override {
		ofxDatGuiComponent::onMousePress(m);
		auto P1s = normToScreen({ x1, 1.f - y1 });
		auto P2s = normToScreen({ x2, 1.f - y2 });

		if (dist2(m, P1s) <= mHandleRadius * mHandleRadius * 4) {
			dragging = Dragging::P1;
		} else if (dist2(m, P2s) <= mHandleRadius * mHandleRadius * 4) {
			dragging = Dragging::P2;
		} else if (mPad.inside(m)) {
			// choose closest
			dragging = ((m - P1s).lengthSquared() < (m - P2s).lengthSquared() ? Dragging::P1 : Dragging::P2);
		} else {
			dragging = Dragging::None;
		}
	}

	void onMouseDrag(ofPoint m) override {
		if (dragging == Dragging::None) return;

		// clamp to pad and convert back to normalized
		float nx = ofClamp((m.x - mPad.x) / mPad.width, 0.f, 1.f);
		float ny = ofClamp((m.y - mPad.y) / mPad.height, 0.f, 1.f);

		// invert y for logical space
		float ly = 1.f - ny;

		if (dragging == Dragging::P1) {
			x1 = nx;
			y1 = ly;
		} else if (dragging == Dragging::P2) {
			x2 = nx;
			y2 = ly;
		}
		dispatchEvent();
	}

	void onMouseRelease(ofPoint m) override {
		ofxDatGuiComponent::onFocusLost(); // drop focus so siblings can receive clicks
		ofxDatGuiComponent::onMouseRelease(m);
		dragging = Dragging::None;
	}

	void onFocusLost() override {
		ofxDatGuiComponent::onFocusLost();
		dragging = Dragging::None;
	}

	void dispatchEvent() {
		if (cubicBezierEventCallback != nullptr) {
			ofxDatGuiCubicBezierEvent e(this, x1, y1, x2, y2);
			cubicBezierEventCallback(e);
		} else {
			ofxDatGuiLog::write(ofxDatGuiMsg::EVENT_HANDLER_NULL);
		}
	}

public:
	static ofxDatGuiCubicBezier * getInstance() { return new ofxDatGuiCubicBezier("Cubic Bezier"); }

private:
	enum class Dragging { None,
		P1,
		P2 };
	Dragging dragging = Dragging::None;

	// logical values (CSS-like space)
	float x1 = 0.25f, y1 = 0.1f, x2 = 0.25f, y2 = 1.f;

	// drawing
	ofRectangle mPad;
	int mHandleRadius = 6;
	int mCurveThickness = 3;
	struct {
		ofColor fill, grid, axis, curve, handle, handleHL;
	} mColors;

	// callback
	std::function<void(ofxDatGuiCubicBezierEvent)> cubicBezierEventCallback = nullptr;

	// helpers
	static float clamp01(float v) { return v < 0 ? 0 : (v > 1 ? 1 : v); }
	static float f(float v, int p) {
		std::ostringstream ss;
		ss << std::fixed << std::setprecision(p) << v;
		return ofToFloat(ss.str());
	}
	static float dist2(const ofPoint & a, const ofPoint & b) {
		float dx = a.x - b.x, dy = a.y - b.y;
		return dx * dx + dy * dy;
	}
	ofPoint normToScreen(ofPoint n) const {
		// n in [0..1]^2 (with n.y already flipped if desired by caller)
		return {
			mPad.x + n.x * mPad.width,
			mPad.y + n.y * mPad.height
		};
	}
	void drawHandle(const ofPoint & p, bool hl) {
		ofPushStyle();
		ofFill();
		ofSetColor(hl ? mColors.handleHL : mColors.handle);
		ofDrawCircle(p, mHandleRadius);
		ofNoFill();
		ofSetColor(0, 50);
		ofDrawCircle(p, mHandleRadius + 2);
		ofPopStyle();
	}
	static ofPoint cubic(const ofPoint & P0, const ofPoint & P1, const ofPoint & P2, const ofPoint & P3, float t) {
		float u = 1.f - t;
		float uu = u * u;
		float uuu = uu * u;
		float tt = t * t;
		float ttt = tt * t;
		ofPoint p = P0 * uuu; // (1-t)^3 P0
		p += P1 * (3 * uu * t); // 3(1-t)^2 t P1
		p += P2 * (3 * u * tt); // 3(1-t) t^2 P2
		p += P3 * ttt; // t^3 P3
		return p;
	}
	void drawCubicBezier(const ofPoint & P0, const ofPoint & P1, const ofPoint & P2, const ofPoint & P3) {
		const int samples = 64;
		ofPolyline pl;
		pl.addVertex(P0);
		for (int i = 1; i <= samples; i++) {
			float t = (float)i / (float)samples;
			pl.addVertex(cubic(P0, P1, P2, P3, t));
		}
		pl.draw();
	}
};
