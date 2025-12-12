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

#pragma once
#include "ofxDatGuiIntObject.h"
#include <algorithm>
#include <vector>

class ofxDatGuiTextInputField : public ofxDatGuiInteractiveObject{

    public:
    
        ofxDatGuiTextInputField()
        {
            mFocused = false;
            mTextChanged = false;
            mHighlightText = false;
            mMaxCharacters = 99;
            mType = ofxDatGuiInputType::ALPHA_NUMERIC;
            mCursorIndex = 0;
            setTheme(ofxDatGuiComponent::getTheme());
        }
    
        void setWidth(int w)
        {
            mInputRect.width = w;
        }
    
        void setPosition(int x, int y)
        {
            mInputRect.x = x;
            mInputRect.y = y;
        }
    
        void setTheme(const ofxDatGuiTheme* theme)
        {
            mFont = theme->font.ptr;
            mInputRect.height = theme->layout.height - (theme->layout.padding * 2);
            color.active.background = theme->color.textInput.backgroundOnActive;
            color.inactive.background = theme->color.inputAreaBackground;
            color.active.text = theme->color.label;
            color.inactive.text = theme->color.textInput.text;
            color.highlight = theme->color.textInput.highlight;
            mUpperCaseText = theme->layout.textInput.forceUpperCase;
            mHighlightPadding = theme->layout.textInput.highlightPadding;
            mSpaceWidth = mFont->charAdvance(' ');
            if (mSpaceWidth <= 0.f) {
                // if the font does not report an advance for space, fall back to a measured gap
                const float measured = mFont->rect("i i").width - mFont->rect("ii").width;
                mSpaceWidth = measured > 0.f ? measured : mFont->rect("1").width;
            }
            setText(mText);
        }

        float glyphWidth(int index) const
        {
            if (index < 0 || index >= static_cast<int>(mRendered.size())) return 0.f;
            const auto c = static_cast<uint32_t>(static_cast<unsigned char>(mRendered[index]));
            if (c == ' ') return mSpaceWidth;
            float w = mFont->charAdvance(c);
            return w > 0.f ? w : mFont->rect(mRendered.substr(index, 1)).width;
        }

        float hashWidth() const
        {
            float w = mFont->charAdvance('#');
            return w > 0.f ? w : mFont->rect("#").width;
        }

        float cursorWidthAt(int index) const
        {
            int clamped = std::max(0, std::min(index, static_cast<int>(mRendered.size())));
            float w = 0.f;
            for (int i = 0; i < clamped; ++i) w += glyphWidth(i);
            if (mType == ofxDatGuiInputType::COLORPICKER) w += hashWidth();
            return w;
        }

        void draw()
        {
            const int padding = static_cast<int>(mHighlightPadding);
            const float hashWidthValue = (mType == ofxDatGuiInputType::COLORPICKER) ? hashWidth() : 0.f;
            const float avail = std::max(0.f, mInputRect.width - 2 * padding - hashWidthValue);

            const int n = static_cast<int>(mRendered.size());
            const int cursorIndex = std::min<int>(mCursorIndex, n);

            // Precompute prefix widths: width from 0 to i (exclusive)
            std::vector<float> prefix(n + 1, 0.f);
            for (int i = 0; i < n; ++i) {
                prefix[i + 1] = prefix[i] + glyphWidth(i);
            }
            auto widthRange = [&](int i, int j) -> float {
                return prefix[j] - prefix[i]; // [i, j)
            };

            // 1) Move start leftwards as far as possible while the cursor still fits.
            int startIndex = cursorIndex;
            for (int s = cursorIndex; s >= 0; --s) {
                float w = widthRange(s, cursorIndex);
                if (w <= avail) {
                    startIndex = s;
                } else {
                    break;
                }
            }

            // 2) From that start, extend end as far right as possible.
            int endIndex = cursorIndex;
            for (int e = cursorIndex; e <= n; ++e) {
                float w = widthRange(startIndex, e);
                if (w <= avail) {
                    endIndex = e;
                } else {
                    break;
                }
            }

            // Fallback: if nothing fits (very narrow field), show at least the glyph under/after the cursor.
            if (endIndex == startIndex && cursorIndex < n) {
                startIndex = cursorIndex;
                endIndex = cursorIndex + 1;
            }

            // Build visible substring
            std::string display;
            if (startIndex < endIndex) {
                display = mRendered.substr(startIndex, endIndex - startIndex);
            }

            float tx = mInputRect.x + padding;

            // Use a stable height to avoid vertical jitter when the substring changes
            float fixedH = mFont->getLineHeight();
            if (fixedH <= 0.f) fixedH = mFont->rect("Hg").height;
            float ty = mInputRect.y + mInputRect.height / 2 + fixedH / 2;

            ofPushStyle();
                // background
                if (mFocused && mType != ofxDatGuiInputType::COLORPICKER){
                    ofSetColor(color.active.background);
                } else {
                    ofSetColor(color.inactive.background);
                }
                ofDrawRectangle(mInputRect);

                // highlight rectangle for selected range (if any)
                if (mHighlightText && mSelectionStart != mSelectionEnd) {
                    // Clamp selection to the visible window.
                    const int selStartVisible = std::max(startIndex, static_cast<int>(mSelectionStart));
                    const int selEndVisible   = std::min(endIndex,   static_cast<int>(mSelectionEnd));
                    if (selStartVisible < selEndVisible) {
                        float selX0 = widthRange(startIndex, selStartVisible);
                        float selX1 = widthRange(startIndex, selEndVisible);
                        ofRectangle hRect;
                        const float dispH = fixedH;
                        hRect.x = tx + hashWidthValue + selX0 - mHighlightPadding;
                        hRect.width = (selX1 - selX0) + (mHighlightPadding * 2);
                        hRect.y = ty - mHighlightPadding - dispH;
                        hRect.height = dispH + (mHighlightPadding * 2);
                        ofSetColor(color.highlight);
                        ofDrawRectangle(hRect);
                    }
                }

                // text
                ofColor tColor = mHighlightText ? color.active.text : color.inactive.text;
                ofSetColor(tColor);
                std::string toDraw;
                if (mType == ofxDatGuiInputType::COLORPICKER) toDraw.push_back('#');
                toDraw.append(display);
                mFont->draw(toDraw, tx, ty);

                // cursor
                if (mFocused) {
                    // cursor width from full string start, then subtract start offset
                    float fullToCursor = widthRange(0, cursorIndex);
                    float offsetToStart = widthRange(0, startIndex);
                    float cursorX = std::max(0.f, fullToCursor - offsetToStart);
                    if (cursorX > avail) cursorX = avail; // clamp within visible window
                    cursorX += hashWidthValue;
                    ofDrawLine(ofPoint(tx + cursorX, mInputRect.getTop()),
                               ofPoint(tx + cursorX, mInputRect.getBottom()));
                }
            ofPopStyle();
        }


    
        int getWidth()
        {
            return mInputRect.width;
        }
    
        int getHeight()
        {
            return mInputRect.height;
        }
    
        bool hasFocus()
        {
            return mFocused;
        }
    
        bool hitTest(ofPoint m)
        {
            return (m.x>=mInputRect.x && m.x<=mInputRect.x+mInputRect.width && m.y>=mInputRect.y && m.y<=mInputRect.y+mInputRect.height);
        }
    
        void setText(string text)
        {
            unsigned int maxChars = maxCharactersForType();
            if (text.size() > maxChars) {
                text = text.substr(0, maxChars);
            }
            mText = text;
            mTextChanged = true;
            mRendered = mUpperCaseText ? ofToUpper(mText) : mText;
            mTextRect = mFont->rect(mType == ofxDatGuiInputType::COLORPICKER ? "#" + mRendered : mRendered);
        }
    
        string getText()
        {
            return mText;
        }
    
        void setTextActiveColor(ofColor c)
        {
            color.active.text = c;
        }
    
        void setTextInactiveColor(ofColor c)
        {
            color.inactive.text = c;
        }
    
        void setTextUpperCase(bool toUpper)
        {
            mUpperCaseText = toUpper;
            setText(mText);
        }
    
        bool getTextUpperCase()
        {
            return mUpperCaseText;
        }
    
        void setTextInputFieldType(ofxDatGuiInputType type)
        {
            mType = type;
        }
    
        void setBackgroundColor(ofColor c)
        {
            color.inactive.background = c;
        }
    
        void setMaxNumOfCharacters(unsigned int max)
        {
            mMaxCharacters = max;
        }
    
        void onFocus()
        {
            mFocused = true;
            mTextChanged = false;
            setCursorIndex(mText.size());
            setSelection(0, static_cast<unsigned int>(mText.size()));
        }
    
        void onFocusLost()
        {
            mFocused = false;
            mHighlightText = false;
            if (mTextChanged){
                mTextChanged = false;
                ofxDatGuiInternalEvent e(ofxDatGuiEventType::INPUT_CHANGED, 0);
                internalEventCallback(e);
            }
        }

        void onMousePress(ofPoint m)
        {
            if (!hitTest(m)) return;

            uint64_t now = ofGetElapsedTimeMillis();
            bool isDoubleClick = (now - mLastClickTime) < 250;
            mLastClickTime = now;

            if (isDoubleClick) {
                // Select all text on double-click.
                setCursorIndex(static_cast<int>(mText.size()));
                setSelection(0, static_cast<unsigned int>(mText.size()));
                return;
            }

            // Single click: move cursor to clicked position and start a drag selection.
            unsigned int index = indexAtPosition(m.x);
            setCursorIndex(static_cast<int>(index));
            mSelectionAnchor = mCursorIndex;
            setSelection(mCursorIndex, mCursorIndex);
            mDragging = true;
        }

        void onMouseDrag(ofPoint m)
        {
            if (!mDragging) return;
            unsigned int index = indexAtPosition(m.x);
            setCursorIndex(static_cast<int>(index));
            setSelection(mSelectionAnchor, mCursorIndex);
        }

        void onMouseRelease(ofPoint /*m*/)
        {
            mDragging = false;
        }
    
        void onKeyPressed(int key)
        {
            if (!keyIsValid(key)) return;

            const unsigned int maxChars = maxCharactersForType();
            bool shiftDown = ofGetKeyPressed(OF_KEY_SHIFT);

            if (mHighlightText) {
                // if key is printable or delete
                if ((key >= 32 && key <= 255) || key == OF_KEY_BACKSPACE || key == OF_KEY_DEL) {
                    // Delete current selection before handling the key.
                    eraseSelection();
                }
            }
            if (key == OF_KEY_BACKSPACE){
            // delete character at cursor position //
                if (mHighlightText) {
                    eraseSelection();
                } else if (mCursorIndex > 0) {
                    setText(mText.substr(0, mCursorIndex - 1) + mText.substr(mCursorIndex));
                    setCursorIndex(mCursorIndex - 1);
                }
            } else if (key == OF_KEY_LEFT) {
                if (shiftDown) {
                    if (!mHighlightText) {
                        mSelectionAnchor = mCursorIndex;
                    }
                    setCursorIndex(std::max(static_cast<int>(mCursorIndex) - 1, 0));
                    setSelection(mSelectionAnchor, mCursorIndex);
                } else {
                    setCursorIndex(std::max(static_cast<int>(mCursorIndex) - 1, 0));
                    clearSelection();
                }
            } else if (key == OF_KEY_RIGHT) {
                if (shiftDown) {
                    if (!mHighlightText) {
                        mSelectionAnchor = mCursorIndex;
                    }
                    setCursorIndex(std::min( mCursorIndex + 1, static_cast<unsigned int>(mText.size())));
                    setSelection(mSelectionAnchor, mCursorIndex);
                } else {
                    setCursorIndex(std::min( mCursorIndex + 1, static_cast<unsigned int>(mText.size())));
                    clearSelection();
                }
            } else {
            // insert character at cursor position //
                if (!mHighlightText && mText.size() >= maxChars) {
                    mHighlightText = false;
                    return;
                }
                if (mHighlightText) {
                    eraseSelection();
                }
                setText(mText.substr(0, mCursorIndex) + static_cast<char>(key) + mText.substr(mCursorIndex));
                setCursorIndex(std::min(mCursorIndex + 1, static_cast<unsigned int>(mText.size())));
            }
            if (!shiftDown) {
                mHighlightText = false;
                clearSelection();
            }
        }
    
        void setCursorIndex(int index)
        {
            const int clamped = std::max(0, std::min(index, static_cast<int>(mRendered.size())));
            mCursorX = cursorWidthAt(clamped);
            mCursorIndex = static_cast<unsigned int>(clamped);
        }
    
    protected:
    
        bool keyIsValid(int key)
        {
            if (key == OF_KEY_BACKSPACE || key == OF_KEY_LEFT || key == OF_KEY_RIGHT){
                return true;
            }

            const unsigned int maxChars = maxCharactersForType();

            if (mType == ofxDatGuiInputType::COLORPICKER){
                // limit string length to hex characters //
                if (!mHighlightText && mText.size() >= maxChars){
                    return false;
                }
            // allow numbers 0-9 //
                if (key>=48 && key<=57){
                    return true;
            // allow letters a-f & A-F //
                }   else if ((key>=97 && key<=102) || (key>=65 && key<=70)){
                    return true;
                }   else{
            // an invalid key was entered //
                    return false;
                }
            }   else if (mType == ofxDatGuiInputType::NUMERIC){
                if (!mHighlightText && mText.size() >= maxChars){
                    return false;
                }
            // allow dash (-) or dot (.) //
                if (key==45 || key==46){
                    return true;
            // allow numbers 0-9 //
                }   else if (key>=48 && key<=57){
                    return true;
                }   else{
            // an invalid key was entered //
                    return false;
                }
            }   else if (mType == ofxDatGuiInputType::ALPHA_NUMERIC){
            // limit range to printable characters http://www.ascii-code.com //
                if (!mHighlightText && mText.size() >= maxChars){
                    return false;
                }
                if (key >= 32 && key <= 255) {
                    return true;
                }   else {
            // an invalid key was entered //
                    return false;
                }
            }   else{
            // invalid textfield type //
                return false;
            }
        }
    
    private:
    
        string mText;
        string mRendered;
        bool mFocused;
        bool mTextChanged;
        bool mHighlightText;
        bool mUpperCaseText;
        float mCursorX;
        ofRectangle mTextRect;
        ofRectangle mInputRect;
        unsigned int mCursorIndex;
        unsigned int mSelectionStart = 0;
        unsigned int mSelectionEnd = 0;
        unsigned int mSelectionAnchor = 0;
        float mSpaceWidth = 0.f;
        bool mDragging = false;
        uint64_t mLastClickTime = 0;
        unsigned int mMaxCharacters;
        unsigned int mHighlightPadding;
        struct{
            struct {
                ofColor text;
                ofColor background;
            } active;
            struct {
                ofColor text;
                ofColor background;
            } inactive;
            ofColor cursor;
            ofColor highlight;
        } color;
        ofxDatGuiInputType mType;
        shared_ptr<ofxSmartFont> mFont;

        unsigned int maxCharactersForType() const
        {
            if (mType == ofxDatGuiInputType::COLORPICKER){
                return std::min<unsigned int>(mMaxCharacters, 6);
            }
            return mMaxCharacters;
        }

        void clearSelection()
        {
            mSelectionStart = mSelectionEnd = mSelectionAnchor = mCursorIndex;
            mHighlightText = false;
        }

        void setSelection(unsigned int start, unsigned int end)
        {
            start = std::min(start, static_cast<unsigned int>(mRendered.size()));
            end = std::min(end, static_cast<unsigned int>(mRendered.size()));
            if (start == end) {
                clearSelection();
                return;
            }
            mSelectionStart = std::min(start, end);
            mSelectionEnd = std::max(start, end);
            mHighlightText = true;
        }

        void eraseSelection()
        {
            if (!mHighlightText || mSelectionStart == mSelectionEnd) return;
            const unsigned int start = mSelectionStart;
            const unsigned int end = mSelectionEnd;
            string before = mText.substr(0, start);
            string after = mText.substr(end);
            setText(before + after);
            setCursorIndex(static_cast<int>(start));
            clearSelection();
        }

        // Map an x-position to a character index in the full rendered string
        unsigned int indexAtPosition(float x) const
        {
            const int padding = static_cast<int>(mHighlightPadding);
            const float hashWidthValue = (mType == ofxDatGuiInputType::COLORPICKER) ? hashWidth() : 0.f;
            const float avail = std::max(0.f, mInputRect.width - 2 * padding - hashWidthValue);

            const int n = static_cast<int>(mRendered.size());
            const int cursorIndex = std::min<int>(mCursorIndex, n);

            std::vector<float> prefix(n + 1, 0.f);
            for (int i = 0; i < n; ++i) {
                prefix[i + 1] = prefix[i] + glyphWidth(i);
            }
            auto widthRange = [&](int i, int j) -> float {
                return prefix[j] - prefix[i];
            };

            int startIndex = cursorIndex;
            for (int s = cursorIndex; s >= 0; --s) {
                float w = widthRange(s, cursorIndex);
                if (w <= avail) {
                    startIndex = s;
                } else {
                    break;
                }
            }

            int endIndex = cursorIndex;
            for (int e = cursorIndex; e <= n; ++e) {
                float w = widthRange(startIndex, e);
                if (w <= avail) {
                    endIndex = e;
                } else {
                    break;
                }
            }

            if (endIndex == startIndex && cursorIndex < n) {
                startIndex = cursorIndex;
                endIndex = cursorIndex + 1;
            }

            float localX = x - (mInputRect.x + padding + hashWidthValue);
            if (localX <= 0.f) return static_cast<unsigned int>(startIndex);

            float acc = 0.f;
            for (int i = startIndex; i < endIndex; ++i) {
                float cw = glyphWidth(i);
                if (localX < acc + cw * 0.5f) {
                    return static_cast<unsigned int>(i);
                }
                acc += cw;
            }
            return static_cast<unsigned int>(endIndex);
        }

};
