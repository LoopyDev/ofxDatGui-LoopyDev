#include "ofApp.h"

void ofApp::setup()
{
	gui.setTheme(new ofxDatGuiThemeRetroGreen());
    gui.setup(); // stack-based gui, manual layout default
    gui.setBringToFrontOnInteract(true);
    gui.setMuteUnfocusedPanels(true);
	//gui.setAutoDraw(true);
    // GUI-owned panels
    positioningPanel = &gui.createPanel("Positioning", ofxDatGuiPanel::Orientation::HORIZONTAL);
	positioningPanel->setWidth(ofGetWidth(), 0.35f); // set width after theme so it sticks
    positioningPanel->setPosition(0, 0);
	positioningPanel->setPreventMuting(true);

    auto * posBtn = positioningPanel->addButton("P1: Button 1");
	auto * posToggle = positioningPanel->addToggle("P1: Toggle", false);


    shadingPanel = &gui.createPanel("Shading", ofxDatGuiPanel::Orientation::HORIZONTAL);
	shadingPanel->setHeaderEnabled(true);
    shadingPanel->setWidth(420, 0.35f);
    shadingPanel->setPosition(40, 140);

    auto* shadeToggle = shadingPanel->addToggle("P2: Toggle", false);
    auto* shadeBtn = shadingPanel->addButton("P2: Button 1");

    // Dynamic/owned panel
    auto * dynPanel = &gui.createPanel("Advanced", ofxDatGuiPanel::Orientation::VERTICAL);

	auto * slider = dynPanel->addSlider(helloParam);
	dynPanel->setPosition(100,460);
	dynPanel->setHeaderEnabled(true);
	dynPanel->setWidth(300, 0.35f);
	slider->setPrecision(1);        // one decimal place
	slider->setSnapIncrement(0.1f); // snap bar to 0.1 steps
	auto * helloButton1 = dynPanel->addButton("Hello1");
	helloButton1->addButtonListener(this, &ofApp::hello);

	auto * textInput = dynPanel->addTextInput("P2: Text Input", testString.get());
	textInput->addTextInputListener(this, &ofApp::onTextInput);

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

void ofApp::hello() {
	cout << "hello" << endl;
}
void ofApp::onTextInput(ofxDatGuiTextInputEvent e) {
	testString = e.text;
	cout << "text input: " << testString.get() << endl;
}
void ofApp::update() {
    gui.update();
	//positioningPanel.update(); // when attached to gui, let gui manage update/draw
}

void ofApp::draw()
{
	ofSetBackgroundColor(ofColor::darkGrey);
    gui.draw();
	//positioningPanel.draw();
}

void ofApp::windowResized(int w, int h) {
	if (positioningPanel) {
		positioningPanel->setWidth(w, 0.35f); // set width after theme so it sticks
		positioningPanel->setPosition(0, 0);
	}
}
