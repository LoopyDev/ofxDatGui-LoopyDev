#pragma once

#include "ofMain.h"
#include "ofxDatGui.h"

class ofApp : public ofBaseApp{

	public:
        void setup();
        void update();
        void draw();
        void windowResized(int w, int h);

        ofxDatGui gui; // defaults to NO_ANCHOR manual layout

        ofxDatGuiPanel positioningPanel;
        ofxDatGuiPanel shadingPanel;

		int numClicks = 0;
        bool isFullscreen = false;
    
};
