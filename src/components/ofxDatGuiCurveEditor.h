//------------------------------------------------------------------------------
// ofxDatGuiCurveEditor.h
//
// A multi-point curve editor component for ofxDatGui.Renders a polyline through normalized
// points, draggable handles, and per-point numeric inputs (x, y) with an
// "+ Add point" button.
//
// Coordinate system:
//   - Model points are normalized to [0..1] in both X and Y.
//   - The pad displays Y with screen-style origin at the top (i.e., drawn as 1 - y).
//
// Interaction:
//   - Drag handles within the pad to change point positions.
//   - Right-click (or hold Alt) on a handle to remove it (at least two points are kept).
//   - Type into the X/Y inputs to edit values; X re-sorts rows to remain ascending.
//   - Click the button to append a new point; it will be inserted sorted by X.
//
// Public API:
//   - setPoints(std::vector<ofPoint> [, bool dispatch = true])
//   - getPoints() -> std::vector<ofPoint>
//   - getPolylineNormalized([flipY=false]) -> ofPolyline
//   - getPolylineMapped(ofRectangle, [flipY=true]) -> ofPolyline
//   - getPathMapped(ofRectangle, [closeShape=false], [flipY=true]) -> ofPath
//   - onCurveEditorEvent(listener, method)  // callback receives { this, std::vector<ofPoint> }
//
// Event:
//   - Emitted on any change to the points (dragging, inputs, add/remove).
//
// Dependencies:
//   - openFrameworks (ofPoint, ofPolyline, ofPath, ofRectangle, ofColor, etc).
//   - ofxDatGui-LoopyDev core types (component, button, text inputs).
//
// Threading:
//   - UI component; call and access from the main (render) thread only.
//------------------------------------------------------------------------------

#pragma once

#include "ofPath.h" // export helpers (getPathMapped)
#include "ofxDatGuiButton.h"
#include "ofxDatGuiComponent.h"
#include "ofxDatGuiTextInputField.h"

struct ofxDatGuiCurveEditorEvent; // defined in ofxDatGuiEvents.h

class ofxDatGuiCurveEditor : public ofxDatGuiComponent {
public:
	/**
     * @brief Construct a curve editor.
     * @param label      Header label (drawn like RadioGroup).
     * @param padAspect  Pad height/width ratio (1.0 = square). Clamped to >= 0.05.
     */
	ofxDatGuiCurveEditor(string label, float padAspect = 1.0f)
		: ofxDatGuiComponent(label)
		, btnAdd("+ Add point") {

		// Borrow an existing type for theme/stripe color integration.
		mType = ofxDatGuiType::PAD2D;

		mPadAspect = std::max(0.05f, padAspect);

		// Reasonable default shape (descending line with a mid anchor).
		points = { { 0.f, 1.f }, { 0.5f, 0.5f }, { 1.f, 0.f } };

		rebuildRows();
		setTheme(ofxDatGuiComponent::getTheme());
	}

	/// Always "expanded" so the framework routes events consistently.
	bool getIsExpanded() override { return true; }

	// -------------------------------------------------------------------------
	// Theming / sizing
	// -------------------------------------------------------------------------

	/**
     * @brief Apply theme (colors, metrics) to this component and child inputs.
     */
	void setTheme(const ofxDatGuiTheme * theme) override {
		setComponentStyle(theme);

		// Header height matches other list-like components (e.g., RadioGroup).
		headerH = theme->layout.height;

		// Colors kept consistent with ofxDatGui visual language.
		colors.fill = theme->color.inputAreaBackground;
		colors.grid = ofColor(255, 30);
		colors.axis = ofColor(255, 60);
		colors.curve = theme->color.slider.fill;
		colors.handle = theme->color.pad2d.ball;
		colors.handleHL = ofColor::white;

		// Propagate theme to child widgets.
		for (auto & r : rows) {
			r.x.setTheme(theme);
			r.y.setTheme(theme);
		}
		btnAdd.setTheme(theme);
		btnAdd.setLabel("+ Add point");

		// Metrics tuned for visibility and overlap safety.
		handleRadius = 6;
		curveThickness = 3;
		innerPadV = std::max(handleRadius + 2, 6);
		innerPadH = std::max(handleRadius + 2, 6);
		inputsHeight = theme->layout.height;
		inputsGap = 8;
		inputsTopGapMin = theme->layout.vMargin + 6;
		inputsBottomGap = theme->layout.vMargin;

		// Use the standard label column in the header; pad is full-width below.
		setWidth(theme->layout.width, theme->layout.labelWidth);
	}

	/**
     * @brief Set component width and label column width.
     * @details Recomputes total height to keep background and stripe correct.
     */
	void setWidth(int w, float labelW = 1.f) override {
		ofxDatGuiComponent::setWidth(w, labelW);
		ofxDatGuiComponent::positionLabel();
		recomputeTotalHeight();
	}

	/**
     * @brief Place component at pixel position (top-left).
     * @details The header/label remains pinned to the top of this component.
     */
	void setPosition(int px, int py) override {
		ofxDatGuiComponent::setPosition(px, py);
		ofxDatGuiComponent::positionLabel();
		// Pad and inputs are recomputed in update().
	}

	// -------------------------------------------------------------------------
	// Public API
	// -------------------------------------------------------------------------

	/**
     * @brief Replace all points.
     * @param pts       New points (will be clamped to [0..1]).
     * @param dispatch  Emit change event after update (default: true).
     * @note Rows are rebuilt to match the new model; X-sorting is preserved
     *       as entered (use resortAndResync() if you require ascending X).
     */
	void setPoints(const std::vector<ofPoint> & pts, bool dispatch = true) {
		points.clear();
		points.reserve(pts.size());
		for (auto p : pts)
			points.push_back(clamp01(p));
		rebuildRows();
		recomputeTotalHeight();
		if (dispatch) dispatchEvent();
	}

	/// @return A copy of the normalized points (each in [0..1]).
	/// @param flipY If true, return points with Y flipped (screen-style): y = 1 - y.
	std::vector<ofPoint> getPoints(bool flipY = false) const {
		if (!flipY) return points; // fast path (no extra alloc)

		std::vector<ofPoint> out;
		out.reserve(points.size());
		for (const auto & p : points) {
			out.emplace_back(p.x, 1.f - p.y); // z defaults to 0
		}
		return out;
	}

	/**
     * @brief Get a polyline of normalized points.
     * @param flipY  If true, invert Y to match screen space (top-left origin).
     */
	ofPolyline getPolylineNormalized(bool flipY = false) const {
		ofPolyline pl;
		for (const auto & p : points) {
			float y = flipY ? 1.f - p.y : p.y;
			pl.addVertex(glm::vec3(p.x, y, 0.f));
		}
		return pl;
	}

	/**
     * @brief Get a polyline mapped into an arbitrary rectangle.
     * @param rect   Destination rectangle.
     * @param flipY  If true, invert Y so higher values draw higher on screen.
     */
	ofPolyline getPolylineMapped(const ofRectangle & rect, bool flipY = true) const {
		ofPolyline pl;
		for (const auto & p : points) {
			float sx = ofMap(p.x, 0.f, 1.f, rect.getLeft(), rect.getRight());
			float sy = ofMap(flipY ? 1.f - p.y : p.y, 0.f, 1.f, rect.getTop(), rect.getBottom());
			pl.addVertex(glm::vec3(sx, sy, 0.f));
		}
		return pl;
	}

	/**
     * @brief Get an ofPath mapped into an arbitrary rectangle.
     * @param rect        Destination rectangle.
     * @param closeShape  Close the path to form a polygon.
     * @param flipY       If true, invert Y so higher values draw higher on screen.
     */
	ofPath getPathMapped(const ofRectangle & rect, bool closeShape = false, bool flipY = true) const {
		ofPath path;
		path.setFilled(false);
		bool first = true;
		for (const auto & p : points) {
			float sx = ofMap(p.x, 0.f, 1.f, rect.getLeft(), rect.getRight());
			float sy = ofMap(flipY ? 1.f - p.y : p.y, 0.f, 1.f, rect.getTop(), rect.getBottom());
			if (first) {
				path.moveTo(sx, sy);
				first = false;
			} else {
				path.lineTo(sx, sy);
			}
		}
		if (closeShape) path.close();
		return path;
	}

	/**
     * @brief Register a change callback.
     * @tparam Listener      Class type of the listener.
     * @param owner          Pointer to listener instance.
     * @param fn             Member function taking (ofxDatGuiCurveEditorEvent).
     */
	template <class Listener>
	void onCurveEditorEvent(Listener * owner, void (Listener::*fn)(ofxDatGuiCurveEditorEvent)) {
		curveEventCallback = std::bind(fn, owner, std::placeholders::_1);
	}

	// -------------------------------------------------------------------------
	// Lifecycle
	// -------------------------------------------------------------------------

	/// @return Total height (header + content); used by layout managers.
	int getHeight() override { return mTotalHeight; }

	/**
     * @brief Update layout and propagate updates to child inputs.
     * @param acceptEvents Forward input events to children this frame.
     */
	void update(bool acceptEvents = true) override {
		ofxDatGuiComponent::update(acceptEvents);
		computePadRect();
		layoutInputs();
	}

	/**
     * @brief Draw the full component: background, stripe, header, pad, curve, handles, inputs, button.
     */
	void draw() override {
		if (!mVisible) return;

		// 1) Paint full component background (panel)
		const int savedH = mStyle.height;
		mStyle.height = mTotalHeight;
		drawBackground();

		// 2) Draw the colored stripe the full height of the component
		drawStripe();

		// Restore header height so the label sits in the header band
		mStyle.height = savedH;

		// 3) Header label (keep it like RadioGroup)
		drawLabel();

		// 4) Pad, grid, axes
		ofPushStyle();
		ofFill();
		ofSetColor(colors.fill);
		ofDrawRectangle(pad);

		ofSetColor(colors.grid);
		for (int i = 1; i < 4; ++i) {
			float t = i / 4.f;
			ofDrawLine(pad.x + t * pad.width, pad.y, pad.x + t * pad.width, pad.y + pad.height);
			ofDrawLine(pad.x, pad.y + t * pad.height, pad.x + pad.width, pad.y + t * pad.height);
		}
		ofNoFill();
		ofSetColor(colors.axis);
		ofDrawRectangle(pad);

		// Curve (polyline through normalized points; drawn with screen-style Y)
		ofPolyline pl;
		for (auto & p : points)
			pl.addVertex(normToScreen({ p.x, 1.f - p.y }));
		ofSetColor(colors.curve);
		ofSetLineWidth(curveThickness);
		pl.draw();

		// Handles
		for (size_t i = 0; i < points.size(); ++i) {
			auto s = normToScreen({ points[i].x, 1.f - points[i].y });
			drawHandle(s, draggingIdx == (int)i);
		}
		ofPopStyle();

		// 5) Inputs & add button
		for (auto & r : rows) {
			r.x.draw();
			r.y.draw();
		}
		btnAdd.draw();
	}

	/**
     * @brief Hit-test against full component bounds (header + content).
     */
	bool hitTest(ofPoint m) override {
		if (!mEnabled || !mVisible) return false;
		return (m.x >= x && m.x <= x + mStyle.width && m.y >= y && m.y <= y + mTotalHeight);
	}

	// -------------------------------------------------------------------------
	// Interaction
	// -------------------------------------------------------------------------

	/**
     * @brief Mouse press routing: inputs first, then button, then pad/handles.
     * @details Right-click (or Alt) on a handle removes it (min 2 points).
     */
	void onMousePress(ofPoint m) override {
		ofxDatGuiComponent::onMousePress(m);
		if (!mFocused) ofxDatGuiComponent::onFocus();

		computePadRect();
		layoutInputs();

		// Inputs take precedence.
		for (auto & r : rows) {
			if (r.x.hitTest(m)) {
				focusOnly(r.x);
				return;
			}
			if (r.y.hitTest(m)) {
				focusOnly(r.y);
				return;
			}
		}

		// Add point
		if (btnAdd.hitTest(m)) {
			addPoint({ 0.5f, 0.5f });
			return;
		}

		// Pad / handles
		if (pad.inside(m)) {
			const float r2 = float((handleRadius + 2) * (handleRadius + 2));
			int hit = -1;
			for (size_t i = 0; i < points.size(); ++i) {
				auto s = normToScreen({ points[i].x, 1.f - points[i].y });
				if (dist2(m, s) <= r2) {
					hit = (int)i;
					break;
				}
			}
			if (hit >= 0) {
				// Remove on secondary button or Alt
				if (ofGetMousePressed(2) || ofGetKeyPressed(OF_KEY_ALT)) {
					if (points.size() > 2) removePoint((size_t)hit);
					return;
				}
				draggingIdx = hit;
			} else {
				// Click empty pad selects nearest for drag
				draggingIdx = nearestPointIdx(m);
			}
		} else {
			draggingIdx = -1;
		}
	}

	/**
     * @brief Drag active handle within pad; updates model and inputs, emits event.
     */
	void onMouseDrag(ofPoint m) override {
		if (draggingIdx < 0) return;
		const float nx = ofClamp((m.x - pad.x) / pad.width, 0.f, 1.f);
		const float ny = ofClamp((m.y - pad.y) / pad.height, 0.f, 1.f);
		points[(size_t)draggingIdx].x = nx;
		points[(size_t)draggingIdx].y = 1.f - ny;
		syncRowFromPoint((size_t)draggingIdx);
		dispatchEvent();
	}

	/**
     * @brief Release drag; if no input has focus, yield focus to siblings.
     */
	void onMouseRelease(ofPoint m) override {
		ofxDatGuiComponent::onMouseRelease(m);
		draggingIdx = -1;

		bool any = false;
		for (auto & r : rows)
			any |= (r.x.hasFocus() || r.y.hasFocus());
		if (!any) {
			blurAll();
			ofxDatGuiComponent::onFocusLost();
		}
	}

	/// Clear drag state and blur inline fields when focus is lost.
	void onFocusLost() override {
		ofxDatGuiComponent::onFocusLost();
		draggingIdx = -1;
		blurAll();
	}

	/**
     * @brief Keyboard routing: send to focused input, otherwise default.
     */
	void onKeyPressed(int key) override {
		for (auto & r : rows) {
			if (r.x.hasFocus()) {
				r.x.onKeyPressed(key);
				return;
			}
			if (r.y.hasFocus()) {
				r.y.onKeyPressed(key);
				return;
			}
		}
		ofxDatGuiComponent::onKeyPressed(key);
	}

private:
	// -------------------------------------------------------------------------
	// Model (normalized 0..1)
	// -------------------------------------------------------------------------
	std::vector<ofPoint> points;

	// -------------------------------------------------------------------------
	// UI elements
	// -------------------------------------------------------------------------
	struct Row {
		ofxDatGuiTextInputField x;
		ofxDatGuiTextInputField y;
	};
	std::vector<Row> rows;
	ofxDatGuiButton btnAdd;

	// -------------------------------------------------------------------------
	// Drawing metrics
	// -------------------------------------------------------------------------
	ofRectangle pad;
	int handleRadius = 6;
	int curveThickness = 3;
	int innerPadV = 6;
	int innerPadH = 6;
	float mPadAspect = 1.0f;

	int inputsHeight = 28;
	int inputsTopGapMin = 12;
	int inputsGap = 8;
	int inputsBottomGap = 10;

	int headerH = 24; // set by theme; height of the header row (label band)
	int mTotalHeight = 0; // header + content

	int draggingIdx = -1;

	struct {
		ofColor fill, grid, axis, curve, handle, handleHL;
	} colors;

	std::function<void(ofxDatGuiCurveEditorEvent)> curveEventCallback = nullptr;

	// -------------------------------------------------------------------------
	// Helpers
	// -------------------------------------------------------------------------

	/// Clamp a point to [0..1] in both axes.
	static ofPoint clamp01(ofPoint p) {
		p.x = ofClamp(p.x, 0.f, 1.f);
		p.y = ofClamp(p.y, 0.f, 1.f);
		return p;
	}
	static float clamp01f(float v) { return v < 0 ? 0 : (v > 1 ? 1 : v); }

	/// Fixed-precision formatter (used by inputs).
	static std::string fmt(float v, int p = 3) {
		std::ostringstream ss;
		ss << std::fixed << std::setprecision(p) << v;
		return ss.str();
	}
	static float dist2(const ofPoint & a, const ofPoint & b) {
		float dx = a.x - b.x, dy = a.y - b.y;
		return dx * dx + dy * dy;
	}

	/// Convert normalized (x, y) to screen inside the pad (expects y already flipped if desired).
	ofPoint normToScreen(ofPoint n) const {
		return { pad.x + n.x * pad.width, pad.y + n.y * pad.height };
	}

	/// Index of the nearest point to a screen-space mouse position.
	int nearestPointIdx(ofPoint ms) const {
		int idx = -1;
		float best = 1e9f;
		for (size_t i = 0; i < points.size(); ++i) {
			auto s = normToScreen({ points[i].x, 1.f - points[i].y });
			float d = dist2(ms, s);
			if (d < best) {
				best = d;
				idx = (int)i;
			}
		}
		return idx;
	}

	/// Handle rendering with subtle halo when highlighted.
	void drawHandle(const ofPoint & p, bool highlight) {
		ofPushStyle();
		ofFill();
		ofSetColor(highlight ? ofColor::white : colors.handle);
		ofDrawCircle(p, handleRadius);
		ofNoFill();
		ofSetColor(0, 50);
		ofDrawCircle(p, handleRadius + 2);
		ofPopStyle();
	}

	/**
     * @brief Recompute the component's total height (header + content).
     * @details Keeps background and stripe rendering in sync with content height.
     */
	void recomputeTotalHeight() {
		const int innerW = std::max(1, (int)mStyle.width - 2 * (int)mStyle.padding - 2 * innerPadH);
		const int padH = std::max(1, (int)std::round(innerW * mPadAspect));
		const int rowsH = (int)rows.size() * inputsHeight + (rows.empty() ? 0 : ((int)rows.size() - 1) * inputsGap);
		const int btnH = inputsHeight;

		const int contentBelowHeader = (int)mStyle.padding
			+ innerPadV + padH + innerPadV
			+ std::max(inputsTopGapMin, inputsGap)
			+ rowsH + (rows.empty() ? 0 : inputsGap)
			+ btnH + inputsBottomGap
			+ (int)mStyle.padding;

		mTotalHeight = headerH + contentBelowHeader;
	}

	/// Compute the pad rectangle (full inner width below the header band).
	void computePadRect() {
		const int innerLeft = x + (int)mStyle.padding + innerPadH;
		const int innerRight = x + (int)mStyle.width - (int)mStyle.padding - innerPadH;
		const int innerW = std::max(1, innerRight - innerLeft);

		const int padW = innerW;
		const int padH = std::max(1, (int)std::round(padW * mPadAspect));
		const int topY = y + headerH + (int)mStyle.padding + innerPadV;

		pad.set(innerLeft, topY, padW, padH);
	}

	/// Layout two-column inputs (X, Y) and the add button beneath the pad.
	void layoutInputs() {
		const int innerLeft = x + (int)mStyle.padding + innerPadH;
		const int innerRight = x + (int)mStyle.width - (int)mStyle.padding - innerPadH;
		const int innerW = std::max(1, innerRight - innerLeft);

		const int twoFieldW = innerW;
		const int fieldW = (twoFieldW - inputsGap) / 2;

		int curY = (int)(pad.y + pad.height + std::max(inputsTopGapMin, inputsGap));

		for (auto & r : rows) {
			r.x.setWidth(fieldW);
			r.y.setWidth(fieldW);
			r.x.setPosition(innerLeft, curY);
			r.y.setPosition(innerLeft + fieldW + inputsGap, curY);
			curY += inputsHeight + inputsGap;
		}

		// Add point button spans both columns.
		btnAdd.setWidth(twoFieldW);
		btnAdd.setPosition(innerLeft, curY);
	}

	/// Remove focus from all inline fields (used on mouse release/focus loss).
	void blurAll() {
		for (auto & r : rows) {
			if (r.x.hasFocus()) r.x.onFocusLost();
			if (r.y.hasFocus()) r.y.onFocusLost();
		}
	}

	/// Give focus to a single input field, blurring any others.
	void focusOnly(ofxDatGuiTextInputField & f) {
		for (auto & r : rows) {
			if (&f != &r.x && r.x.hasFocus()) r.x.onFocusLost();
			if (&f != &r.y && r.y.hasFocus()) r.y.onFocusLost();
		}
		ofxDatGuiComponent::onFocus();
		f.onFocus();
	}

	// --- Rows & binding ------------------------------------------------------

	/// Rebuild input rows to mirror the current model (text reflects current values).
	void rebuildRows() {
		rows.clear();
		rows.reserve(points.size());
		for (size_t i = 0; i < points.size(); ++i) {
			Row r;
			r.x.setTextInputFieldType(ofxDatGuiInputType::NUMERIC);
			r.y.setTextInputFieldType(ofxDatGuiInputType::NUMERIC);

			r.x.onInternalEvent(this, &ofxDatGuiCurveEditor::onRowXChanged);
			r.y.onInternalEvent(this, &ofxDatGuiCurveEditor::onRowYChanged);

			r.x.setText(fmt(points[i].x));
			r.y.setText(fmt(points[i].y));
			rows.push_back(std::move(r));
		}
	}

	/// Insert a (clamped) point; rows/height updated; event dispatched.
	void addPoint(ofPoint p) {
		p = clamp01(p);
		auto it = std::lower_bound(points.begin(), points.end(), p.x,
			[](const ofPoint & a, float vx) { return a.x < vx; });
		points.insert(it, p);
		rebuildRows();
		recomputeTotalHeight();
		dispatchEvent();
	}

	/// Remove a point if >2 remain; rows/height updated; event dispatched.
	void removePoint(size_t idx) {
		if (points.size() <= 2 || idx >= points.size()) return;
		points.erase(points.begin() + (long)idx);
		rebuildRows();
		recomputeTotalHeight();
		dispatchEvent();
	}

	/// Update a row's text from the model (used during drag).
	void syncRowFromPoint(size_t idx) {
		if (idx >= rows.size()) return;
		rows[idx].x.setText(fmt(points[idx].x));
		rows[idx].y.setText(fmt(points[idx].y));
	}

	/// Parse a float in [0..1]; accepts comma as decimal separator.
	static float parse01(const std::string & s, float fallback) {
		std::string t = s;
		for (auto & c : t)
			if (c == ',') c = '.';
		try {
			return clamp01f(ofToFloat(t));
		} catch (...) {
			return clamp01f(fallback);
		}
	}

	/// Read both X/Y inputs for all rows into the model (clamped to [0..1]).
	void syncModelFromRows() {
		const size_t n = std::min(points.size(), rows.size());
		for (size_t i = 0; i < n; ++i) {
			float nx = parse01(rows[i].x.getText(), points[i].x);
			float ny = parse01(rows[i].y.getText(), points[i].y);
			points[i].x = nx;
			points[i].y = ny;
		}
	}

	/// X-field changed: read inputs, sort by X to keep the curve tidy, rebuild rows, dispatch.
	void onRowXChanged(ofxDatGuiInternalEvent) {
		syncModelFromRows();
		resortAndResync();
		recomputeTotalHeight();
		dispatchEvent();
	}

	/// Y-field changed: read inputs and dispatch (no resort needed).
	void onRowYChanged(ofxDatGuiInternalEvent) {
		syncModelFromRows();
		dispatchEvent();
	};

	/// Stable-sort by X then rebuild rows to reflect new order.
	void resortAndResync() {
		std::stable_sort(points.begin(), points.end(),
			[](const ofPoint & a, const ofPoint & b) { return a.x < b.x; });
		rebuildRows();
	}

	/// Emit the change event if a callback is registered.
	void dispatchEvent() {
		if (!curveEventCallback) return;
		curveEventCallback(ofxDatGuiCurveEditorEvent(this, points));
	}
};
