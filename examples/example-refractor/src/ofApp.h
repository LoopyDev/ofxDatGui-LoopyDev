#pragma once

#include "ofMain.h"
#include "ofxDatGui.h"

class ofApp : public ofBaseApp{

	public:
		void setup();
        void update();
        void draw();
        void windowResized(int w, int h);
		void hello();
		void onTextInput(ofxDatGuiTextInputEvent e);
        ofxDatGui gui; // defaults to NO_ANCHOR manual layout

        ofxDatGuiPanel* positioningPanel = nullptr;
        ofxDatGuiPanel* shadingPanel = nullptr;

		int numClicks = 0;
        bool isFullscreen = false;

        ofParameter<float> helloParam{"Hello", 0.f, -1.f, 1.f};
		ofParameter<string> testString { "Hello" };
    
};
