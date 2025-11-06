#pragma once
#include "ofMain.h"
#include "ofxDatGui.h"
#include "ofxDatGuiCubicBezier.h" // your header

// If your project defines a custom event struct, make sure the signature matches:
// struct ofxDatGuiCubicBezierEvent { ofxDatGuiCubicBezier* target; float x1,y1,x2,y2; ... };

class ofApp : public ofBaseApp {
public:
	void setup() override;
	void update() override;
	void draw() override;

	void keyPressed(int key) override;

	// Listener for the bezier component
	void onBezierEvent(ofxDatGuiCubicBezierEvent e);

private:
	// UI
	std::unique_ptr<ofxDatGui> gui;
	ofxDatGuiCubicBezier * bezier = nullptr;

	// Readout + demo
	std::string cssString;
	ofRectangle demoRect;

	// Animation state
	float animT = 0.f; // 0..1 looping
	bool animPlaying = true;

	// Helpers
	float evalEase(float x1, float y1, float x2, float y2, float t) const; // y at given linear t
	static float cubic(float a, float b, float c, float d, float t); // scalar cubic
};
