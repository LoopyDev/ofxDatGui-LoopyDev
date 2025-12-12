#pragma once

#include "ofxDatGuiComponent.h"
#include <vector>
#include <deque>
#include <cmath>

// Base graph for time-series style widgets (wave monitor, value plotter).
class ofxDatGuiTimeGraph : public ofxDatGuiComponent {

public:
    void setDrawMode(ofxDatGuiGraph gMode) {
        switch (gMode) {
            case ofxDatGuiGraph::LINES:   mDrawFunc = &ofxDatGuiTimeGraph::drawLines;   break;
            case ofxDatGuiGraph::FILLED:  mDrawFunc = &ofxDatGuiTimeGraph::drawFilled;  break;
            case ofxDatGuiGraph::POINTS:  mDrawFunc = &ofxDatGuiTimeGraph::drawPoints;  break;
            case ofxDatGuiGraph::OUTLINE: mDrawFunc = &ofxDatGuiTimeGraph::drawOutline; break;
        }
    }

    // Leaf widgets don't own children.
    void setTheme(const ofxDatGuiTheme* theme) override {
        setComponentStyle(theme);
        mStyle.height = theme->layout.graph.height;
        mStyle.stripe.color = theme->stripe.graph;
        mColor.lines = theme->color.graph.lines;
        mColor.fills = theme->color.graph.fills;
        mPointSize = theme->layout.graph.pointSize;
        mLineWeight = theme->layout.graph.lineWeight;
        setWidth(theme->layout.width, theme->layout.labelWidth);
    }

    void setWidth(int width, float labelWidth) override {
        ofxDatGuiComponent::setWidth(width, labelWidth);
        mPlotterRect.x = mLabel.width;
        mPlotterRect.y = mStyle.padding;
        mPlotterRect.width = mStyle.width - mStyle.padding - mLabel.width;
        mPlotterRect.height = mStyle.height - (mStyle.padding * 2);
    }

    void draw() override {
        if (!mVisible) return;
        ofPushStyle();
            ofxDatGuiComponent::draw();
            ofSetColor(mStyle.color.inputArea);
            ofDrawRectangle(x + mPlotterRect.x, y + mPlotterRect.y, mPlotterRect.width, mPlotterRect.height);
            glColor3ub(mColor.fills.r, mColor.fills.g, mColor.fills.b);
            if (mDrawFunc != nullptr) (this->*mDrawFunc)();
        ofPopStyle();
    }

protected:
    explicit ofxDatGuiTimeGraph(std::string label)
        : ofxDatGuiComponent(std::move(label)) {
        mDrawFunc = &ofxDatGuiTimeGraph::drawFilled;
        setTheme(ofxDatGuiComponent::getTheme());
    }

    void drawFilled() {
        const float px = this->x + mPlotterRect.x;
        const float py = this->y + mPlotterRect.y;
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glBegin(GL_TRIANGLE_STRIP);
        for (int i = 0; i < static_cast<int>(pts.size()); i++) {
            glVertex2f(px + pts[i].x, py + mPlotterRect.height);
            glVertex2f(px + pts[i].x, py + pts[i].y);
        }
        glEnd();
    }

    void drawOutline() {
        const float px = this->x + mPlotterRect.x;
        const float py = this->y + mPlotterRect.y;
        glLineWidth(mLineWeight);
        glBegin(GL_LINE_LOOP);
        glVertex2f(px + mPlotterRect.width, py + mPlotterRect.height);
        for (int i = 0; i < static_cast<int>(pts.size()); i++) glVertex2f(px + pts[i].x, py + pts[i].y);
        glVertex2f(px, py + mPlotterRect.height);
        glEnd();
    }

    void drawLines() {
        const float px = this->x + mPlotterRect.x;
        const float py = this->y + mPlotterRect.y;
        glLineWidth(mLineWeight);
        glBegin(GL_LINE_STRIP);
        for (int i = 0; i < static_cast<int>(pts.size()); i++) glVertex2f(px + pts[i].x, py + pts[i].y);
        glEnd();
    }

    void drawPoints() {
        const float px = this->x + mPlotterRect.x;
        const float py = this->y + mPlotterRect.y;
        glPointSize(static_cast<float>(mLineWeight));
        glLineWidth(mLineWeight);
        glBegin(GL_POINTS);
        for (int i = 0; i < static_cast<int>(pts.size()); i++) glVertex2f(px + pts[i].x, py + pts[i].y);
        glEnd();
    }

    int mPointSize = 0;
    int mLineWeight = 0;
    struct {
        ofColor lines;
        ofColor fills;
    } mColor;
    std::vector<ofVec2f> pts;
    ofRectangle mPlotterRect;
    void (ofxDatGuiTimeGraph::*mDrawFunc)() = nullptr;
};

class ofxDatGuiWaveMonitor : public ofxDatGuiTimeGraph {

public:
    ofxDatGuiWaveMonitor(std::string label, float frequency, float amplitude)
        : ofxDatGuiTimeGraph(std::move(label)) {
        mFrequencyLimit = 100;
        setAmplitude(amplitude);
        setFrequency(frequency);
        mType = ofxDatGuiType::WAVE_MONITOR;
        setTheme(ofxDatGuiComponent::getTheme());
    }

    static ofxDatGuiWaveMonitor* getInstance() { return new ofxDatGuiWaveMonitor("X", 0, 0); }

    void setAmplitude(float amp) {
        mAmplitude = ofClamp(amp, 0.f, static_cast<float>(MAX_AMPLITUDE));
        graph();
    }

    void setFrequency(float freq) {
        mFrequencyHz = ofClamp(freq, 0.f, mFrequencyLimit);
        graph();
    }

    void setFrequencyLimit(float limit) {
        mFrequencyLimit = limit;
        setFrequency(mFrequencyHz);
    }

    // External feed: value in [-1, 1] is mapped to +/- amplitude.
    void pushSample(float sample) {
        mHasSample = true;
        mLastSample = ofClamp(sample, -1.f, 1.f);
    }

    void setTheme(const ofxDatGuiTheme* tmplt) override {
        ofxDatGuiTimeGraph::setTheme(tmplt);
        resizeSamples();
        rebuildPtsFromSamples();
    }

    void setWidth(int width, float labelWidth) override {
        ofxDatGuiTimeGraph::setWidth(width, labelWidth);
        resizeSamples();
        rebuildPtsFromSamples();
    }

    void update(bool acceptEvents = true) override {
        if (!acceptEvents) return;
        if (mPlotterRect.width <= 0) return;
        if (!mHasSample) return;
        pushToBuffer(mLastSample);
    }

private:
    void graph() {
        resizeSamples(true);
        rebuildPtsFromSamples();
    }

    void pushToBuffer(float normalizedSample, bool fill = false) {
        if (mPlotterRect.width <= 0) return;
        normalizedSample = ofClamp(normalizedSample, -1.f, 1.f);

        if (fill || mSamples.empty()) {
            mSamples.assign(static_cast<size_t>(mPlotterRect.width), normalizedSample);
        } else {
            mSamples.push_front(normalizedSample);
            while (static_cast<int>(mSamples.size()) > mPlotterRect.width) {
                mSamples.pop_back();
            }
        }

        rebuildPtsFromSamples();
    }

    void resizeSamples(bool forceFill = false) {
        if (mPlotterRect.width <= 0) {
            mSamples.clear();
            pts.clear();
            return;
        }
        if (forceFill || mSamples.empty()) {
            mSamples.assign(static_cast<size_t>(mPlotterRect.width), mLastSample);
        } else if (static_cast<int>(mSamples.size()) > mPlotterRect.width) {
            mSamples.erase(mSamples.begin() + mPlotterRect.width, mSamples.end());
        } else if (static_cast<int>(mSamples.size()) < mPlotterRect.width) {
            mSamples.insert(mSamples.end(), mPlotterRect.width - static_cast<int>(mSamples.size()), mLastSample);
        }
    }

    void rebuildPtsFromSamples() {
        pts.clear();
        if (mPlotterRect.width <= 0) return;
        const float yAmp = (mPlotterRect.height / 2.f) * (mAmplitude / static_cast<float>(MAX_AMPLITUDE));
        const float base = mPlotterRect.height / 2.f;
        for (int i = 0; i < static_cast<int>(mSamples.size()); ++i) {
            pts.emplace_back(static_cast<float>(i), base + (mSamples[i] * yAmp));
        }
    }

    float mAmplitude = 0.f;
    float mFrequencyHz = 0.f;
    float mFrequencyLimit = 100.f;
    bool mHasSample = false;
    float mLastSample = 0.f;
    std::deque<float> mSamples;
    static const int MAX_AMPLITUDE = 1;
};

class ofxDatGuiValuePlotter : public ofxDatGuiTimeGraph {

public:
    ofxDatGuiValuePlotter(std::string label, float min, float max)
        : ofxDatGuiTimeGraph(std::move(label)) {
        mSpeed = 5.0f;
        setRange(min, max);
        mType = ofxDatGuiType::VALUE_PLOTTER;
    }

    static ofxDatGuiValuePlotter* getInstance() { return new ofxDatGuiValuePlotter("X", 0, 0); }

    void setRange(float min, float max) {
        mMin = min;
        mMax = max;
        setValue((max + min) / 2);
    }

    void setSpeed(float speed) {
        if (speed != mSpeed) {
            pts.clear();
            mSpeed = speed;
        }
    }

    void setValue(float value) {
        mVal = value;
        if (pts.size() >= mPlotterRect.width) {
            pts.pop_back();
        }
        float pct = ofClamp((mVal - mMin) / (mMax - mMin), 0.f, 1.f);
        float y = mPlotterRect.height - (pct * mPlotterRect.height);
        pts.insert(pts.begin(), ofVec2f(1, y));
        // shift x positions
        for (int i = 0; i < static_cast<int>(pts.size()); ++i) {
            pts[i].x = static_cast<float>(i);
        }
    }

    void setTheme(const ofxDatGuiTheme* tmplt) override {
        ofxDatGuiTimeGraph::setTheme(tmplt);
        pts.clear();
    }

    void setWidth(int width, float labelWidth) override {
        ofxDatGuiTimeGraph::setWidth(width, labelWidth);
        pts.clear();
    }

    void update(bool acceptEvents = true) override {
        if (!acceptEvents) return;
        // Decimate based on speed; reuse setValue to push current value.
        mAccumulator += mSpeed;
        while (mAccumulator >= 1.f) {
            setValue(mVal);
            mAccumulator -= 1.f;
        }
    }

private:
    float mMin = 0.f;
    float mMax = 1.f;
    float mVal = 0.f;
    float mSpeed = 5.0f;
    float mAccumulator = 0.f;
};
