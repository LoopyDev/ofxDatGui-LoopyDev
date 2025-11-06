#include "ofApp.h"
#include <algorithm> // std::max/min

// --------------------------------------------------------------
void ofApp::setup() {
	ofSetWindowTitle("example-BezierCurve (ofxDatGuiCubicBezier)");
	ofSetFrameRate(60);
	ofBackground(12);

	gui = std::make_unique<ofxDatGui>(ofxDatGuiAnchor::TOP_LEFT);
	gui->setWidth(420, 170);
	gui->setAssetPath(""); // avoid default font warnings if not using assets

	// Cubic-bezier editor
	bezier = gui->addCubicBezier("Cubic Bezier", 0.25f, 0.10f, 0.25f, 1.00f, 0.75f);
	bezier->onCubicBezierEvent(this, &ofApp::onBezierEvent);

	cssString = bezier->getCssString(3);

	// Demo area for the curve preview + animated dot
	demoRect.set(480, 120, 680, 120);

	animPlaying = true;
	animT = 0.f;
}

// --------------------------------------------------------------
void ofApp::update() {
	gui->update();

	if (animPlaying) {
		// ~4s loop over 0..1
		animT += 1.0f / 240.0f;
		if (animT > 1.f) animT -= 1.f;
	}
}

// Triangle ping-pong: 0..0.5 -> 0..1, 0.5..1 -> 1..0
static inline float pingpong(float t) {
	return (t < 0.5f) ? (t * 2.f) : (2.f - t * 2.f);
}

// --------------------------------------------------------------
void ofApp::draw() {
	gui->draw();

	ofSetColor(255);
	ofDrawBitmapStringHighlight("CSS:", 480, 80, ofColor(20, 20, 20, 225), ofColor(255, 255, 255, 220));
	ofDrawBitmapString(cssString, 520, 80);

	// Frame for the preview/demo
	ofPushStyle();
	ofNoFill();
	ofSetColor(180);
	ofDrawRectangle(demoRect);
	ofPopStyle();

	// Current bezier control points (normalized)
	float x1, y1, x2, y2;
	bezier->getPoints(x1, y1, x2, y2);

	// ====================== Shared timing (SYNC EVERYTHING) =====================
	// Use triangle (ping-pong) directly so BOTH examples truly go out-and-back.
	float tShared = pingpong(animT); // 0..1..0
	// ===========================================================================

	// ---------------------------- Path preview (one-way) ------------------------
	ofPushStyle();
	ofNoFill();
	ofSetColor(90, 180, 255);
	ofBeginShape();
	const int samples = 100;
	for (int i = 0; i <= samples; ++i) {
		float tt = i / float(samples);
		float ee = evalEase(x1, y1, x2, y2, tt);
		float xx = ofMap(tt, 0.f, 1.f, demoRect.getLeft() + 10, demoRect.getRight() - 10);
		float yy = ofMap(ee, 1.f, 0.f, demoRect.getTop() + 10, demoRect.getBottom() - 10);
		ofVertex(xx, yy);
	}
	ofEndShape(false);
	ofPopStyle();
	// ---------------------------------------------------------------------------

	// ---------------------------- Demo dot (synced) -----------------------------
	// Horizontal uses linear t; vertical uses eased(t). Both use the triangle tShared.
	float t = tShared;
	float e = evalEase(x1, y1, x2, y2, t);

	float x = ofMap(t, 0.f, 1.f, demoRect.getLeft() + 10, demoRect.getRight() - 10);
	float y = ofMap(e, 1.f, 0.f, demoRect.getTop() + 10, demoRect.getBottom() - 10);

	ofSetColor(255);
	ofDrawCircle(x, y, 7);
	// ---------------------------------------------------------------------------

	// -------------------------- Slider track (synced) ---------------------------
	const float trackLeft = 520.f;
	const float trackRight = 1120.f;
	const float trackY = demoRect.getBottom() + 60.f;
	const float knobRadius = 9.f;

	// Track line
	ofPushStyle();
	ofSetColor(70);
	ofSetLineWidth(4.f);
	ofDrawLine(trackLeft, trackY, trackRight, trackY);
	ofPopStyle();

	// Eased knob vs linear "ghost" knob — both use tShared (true ping-pong)
	float eased = evalEase(x1, y1, x2, y2, tShared);
	float linear = tShared;

	float knobX = ofLerp(trackLeft, trackRight, eased);
	float ghostX = ofLerp(trackLeft, trackRight, linear);

	ofSetColor(130); // linear ghost
	ofDrawCircle(ghostX, trackY, 5.f);

	ofSetColor(255); // eased knob
	ofDrawCircle(knobX, trackY, knobRadius);

	ofSetColor(200);
	ofDrawBitmapString("Eased (white) vs linear (grey) — true ping-pong motion", trackLeft, trackY + 24);
	// ---------------------------------------------------------------------------

	// Legend
	ofSetColor(220);
	ofDrawBitmapString("Space: play/pause  |  Left/Right: scrub  |  R: reset", 480, demoRect.getBottom() + 100);
}

// --------------------------------------------------------------
void ofApp::keyPressed(int key) {
	switch (key) {
	case ' ':
		animPlaying = !animPlaying;
		break;
	case OF_KEY_LEFT:
		animT = std::max(0.f, animT - 0.01f);
		break;
	case OF_KEY_RIGHT:
		animT = std::min(1.f, animT + 0.01f);
		break;
	case 'r':
	case 'R':
		animT = 0.f;
		break;
	default:
		break;
	}
}

// --------------------------------------------------------------
void ofApp::onBezierEvent(ofxDatGuiCubicBezierEvent e) {
	// If the event doesn't carry the points, query the widget:
	float x1, y1, x2, y2;
	bezier->getPoints(x1, y1, x2, y2);
	cssString = bezier->getCssString(3);
	(void)e;
}

// --------------------------------------------------------------
// Evaluate cubic-bezier easing (CSS style).
// Solve x(s)=t with Newton iterations, then return y(s).
float ofApp::evalEase(float x1, float y1, float x2, float y2, float t) const {
	auto xCubic = [&](float s) {
		float u = 1.f - s;
		return 3.f * u * u * s * x1 + 3.f * u * s * s * x2 + s * s * s; // P0.x=0, P3.x=1
	};
	auto yCubic = [&](float s) {
		float u = 1.f - s;
		return 3.f * u * u * s * y1 + 3.f * u * s * s * y2 + s * s * s; // P0.y=0, P3.y=1
	};
	auto dxds = [&](float s) {
		// derivative of xCubic(s)
		float u = 1.f - s;
		return 3.f * (u * u * x1 + 2.f * u * s * (x2 - x1) + s * s * (1.f - x2));
	};

	float s = ofClamp(t, 0.f, 1.f); // initial guess = t
	for (int i = 0; i < 8; ++i) {
		float f = xCubic(s) - t;
		float df = dxds(s);
		if (fabsf(df) < 1e-6f) break;
		s -= f / df;
		s = ofClamp(s, 0.f, 1.f);
	}

	return ofClamp(yCubic(s), 0.f, 1.f);
}

// --------------------------------------------------------------
// Kept for reference; not used by evalEase
float ofApp::cubic(float a, float b, float c, float d, float t) {
	float u = 1.f - t;
	return a * u * u * u + 3.f * b * u * u * t + 3.f * c * u * t * t + d * t * t * t;
}
