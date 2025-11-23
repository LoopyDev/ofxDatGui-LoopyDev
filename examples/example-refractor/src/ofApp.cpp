#include "ofApp.h"

void ofApp::setup()
{
    gui.setup(); // stack-based gui, manual layout default

    // Borrowed panels (stack-owned)
    positioningPanel.setOrientation(ofxDatGuiPanel::Orientation::HORIZONTAL);
    positioningPanel.setTheme(ofxDatGuiComponent::getTheme());
    //positioningPanel.setPosition(0, 0);
	positioningPanel.setWidth(ofGetWidth(), 0.35f); // set width after theme so it sticks

    auto * posBtn = positioningPanel.addButton("P1: Button 1");
	auto * posToggle = positioningPanel.addToggle("P1: Toggle", false);

    auto & shading = gui.attachPanel(shadingPanel, "Shading", ofxDatGuiPanel::Orientation::HORIZONTAL);
	shading.setHeaderEnabled(true);
    shading.setWidth(620, 0.35f);
    shading.setPosition(40, 140);

    auto* shadeBtn = shading.addButton("P2: Button 1");
    auto* shadeToggle = shading.addToggle("P2: Toggle", false);

    // Dynamic/owned panel
    auto& dynPanel = gui.createPanel("Advanced", ofxDatGuiPanel::Orientation::VERTICAL);
    dynPanel.setPosition(100,460);
	dynPanel.setHeaderEnabled(true);
    dynPanel.setWidth(600, 0.35f);
    //dynPanel.addSlider("Rocket Speed", 0, 10, 3.0f);
	auto * helloButton = dynPanel.addButton("Hello");
	auto * helloButton1 = dynPanel.addButton("Hello1");

    // Inline callbacks to avoid header pointers.
    posBtn->onButtonEvent([=](ofxDatGuiButtonEvent e){
        if (e.target != posBtn) return;
        numClicks++;
        posBtn->setLabel(numClicks == 1 ? "YOU CLICKED ME ONCE" : "YOU CLICKED ME "+ofToString(numClicks)+" TIMES");
    });

    posToggle->onToggleEvent([=](ofxDatGuiToggleEvent e){
        if (e.target != posToggle) return;
        isFullscreen = !isFullscreen;
        ofSetFullscreen(isFullscreen);
        if (!isFullscreen) {
            ofSetWindowShape(1280, 720);
        }
    });

    shadeBtn->onButtonEvent([=](ofxDatGuiButtonEvent e){
        if (e.target != shadeBtn) return;
        static int shadeClicks = 0;
        shadeClicks++;
        shadeBtn->setLabel(shadeClicks == 1 ? "YOU CLICKED ME ONCE" : "YOU CLICKED ME "+ofToString(shadeClicks)+" TIMES");
    });

    shadeToggle->onToggleEvent([=](ofxDatGuiToggleEvent e){
        if (e.target != shadeToggle) return;
        isFullscreen = !isFullscreen;
        ofSetFullscreen(isFullscreen);
        if (!isFullscreen) {
            ofSetWindowShape(1280, 720);
        }
    });
}

void ofApp::update()
{
    //gui.update();
	positioningPanel.update();
}

void ofApp::draw()
{
	ofSetBackgroundColor(ofColor::darkGrey);
    //gui.draw();
	positioningPanel.draw();
}

void ofApp::windowResized(int w, int h) {
	positioningPanel.setPosition(0, 0);
	positioningPanel.setWidth(w, 0.35f); // set width after theme so it sticks
}
