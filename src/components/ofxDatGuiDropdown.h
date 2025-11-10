#pragma once
/*
    ofxDatGuiDropdown.h
    Split out from ofxDatGuiGroups.h

    Depends on:
      - ofxDatGuiGroups.h  (for ofxDatGuiGroup base)
      - ofxDatGuiButton.h  (for Button/Toggle)
*/

#include "ofxDatGuiButton.h"
#include "ofxDatGuiGroups.h"

// Behaviour matches your existing enum that lived in Groups.h
enum class ofxDatGuiDropdownBehavior {
	SelectCloses,
	RadioStaysOpen
};

// -----------------------------------------------------------------------------
// Dropdown Option (single row inside a dropdown)
// -----------------------------------------------------------------------------
class ofxDatGuiDropdownOption : public ofxDatGuiButton {
public:
	explicit ofxDatGuiDropdownOption(std::string label)
		: ofxDatGuiButton(std::move(label))
		, mIsRadio(false)
		, mChecked(false) {
		mType = ofxDatGuiType::DROPDOWN_OPTION;
		setTheme(ofxDatGuiComponent::getTheme());
	}

	void setTheme(const ofxDatGuiTheme * theme) override {
		ofxDatGuiButton::setTheme(theme);
		mStyle.stripe.color = theme->stripe.dropdown;
	}

	void setWidth(int width, float labelWidth = 1) override {
		ofxDatGuiComponent::setWidth(width, labelWidth);
		mLabel.width = mStyle.width;
		mLabel.rightAlignedXpos = mIcon.x - mLabel.margin;
		ofxDatGuiComponent::positionLabel();
	}

	// Radio helpers
	void setRadio(bool b) { mIsRadio = b; }
	bool isRadio() const { return mIsRadio; }

	void setChecked(bool b) { mChecked = b; }
	bool getChecked() const { return mChecked; }

	void draw() override {
		if (!mVisible) return;

		ofPushStyle();
		ofxDatGuiButton::draw();

		// Optional radio indicator
		if (mIsRadio) {
			float cx = x + 12.f;
			float cy = y + mStyle.height * 0.5f;
			float r = 6.f;

			ofSetColor(mEnabled ? ofColor::white : ofColor(180));
			ofNoFill();
			ofDrawCircle(cx, cy, r);

			if (mChecked) {
				ofFill();
				ofDrawCircle(cx, cy, r * 0.55f);
			}
		}
		ofPopStyle();
	}

private:
	bool mIsRadio;
	bool mChecked;
};

// -----------------------------------------------------------------------------
// Dropdown (header + list of options)
// -----------------------------------------------------------------------------
class ofxDatGuiDropdown : public ofxDatGuiGroup {
public:
	ofxDatGuiDropdown(std::string label,
		const std::vector<std::string> & options = {},
		ofxDatGuiDropdownBehavior behavior = ofxDatGuiDropdownBehavior::SelectCloses)
		: ofxDatGuiGroup(std::move(label))
		, mOption(0)
		, mBehavior(behavior)
		, mUseToggleChildren(false) {
		mType = ofxDatGuiType::DROPDOWN;

		// Build children
		for (int i = 0; i < static_cast<int>(options.size()); ++i) {
			auto * opt = new ofxDatGuiDropdownOption(options[i]);
			opt->setIndex(static_cast<int>(children.size()));
			opt->onButtonEvent(this, &ofxDatGuiDropdown::onOptionSelected);
			if (mBehavior == ofxDatGuiDropdownBehavior::RadioStaysOpen) opt->setRadio(true);
			children.push_back(opt);
		}

		setTheme(ofxDatGuiComponent::getTheme());

		// Default radio check if applicable
		if (mBehavior == ofxDatGuiDropdownBehavior::RadioStaysOpen && size() > 0) {
			getChildAt(mOption)->setChecked(true);
		}
	}

	void setTheme(const ofxDatGuiTheme * theme) override {
		setComponentStyle(theme);
		mIconOpen = theme->icon.groupOpen;
		mIconClosed = theme->icon.groupClosed;
		mStyle.stripe.color = theme->stripe.dropdown;
		setWidth(theme->layout.width, theme->layout.labelWidth);
	}

	void setWidth(int width, float labelWidth = 1) override {
		ofxDatGuiComponent::setWidth(width, labelWidth);
		mLabel.width = mStyle.width;
		mLabel.rightAlignedXpos = mIcon.x - mLabel.margin;
		ofxDatGuiComponent::positionLabel();
	}

	// Select a child by index
	void select(int cIndex) {
		if (cIndex < 0 || cIndex >= static_cast<int>(children.size())) {
			ofLogError() << "ofxDatGuiDropdown->select(" << cIndex << ") is out of range";
			return;
		}

		mOption = cIndex;

		if (mBehavior == ofxDatGuiDropdownBehavior::SelectCloses) {
			// Mirror your current behaviour: set header label & collapse.
			setLabel(children[cIndex]->getLabel());
			collapse();
		} else {
			// Radio mode stays open; update checks
			for (int i = 0; i < static_cast<int>(children.size()); ++i) {
				if (auto * t = dynamic_cast<ofxDatGuiToggle *>(children[i])) {
					t->setChecked(i == cIndex);
				} else if (auto * opt = dynamic_cast<ofxDatGuiDropdownOption *>(children[i])) {
					if (opt->isRadio()) opt->setChecked(i == cIndex);
				}
			}
		}

		dispatchEvent();
	}

	int size() const { return static_cast<int>(children.size()); }

	ofxDatGuiDropdownOption * getChildAt(int index) {
		return static_cast<ofxDatGuiDropdownOption *>(children[index]);
	}
	ofxDatGuiDropdownOption * getSelected() {
		return static_cast<ofxDatGuiDropdownOption *>(children[mOption]);
	}

	// Event wiring
	void dispatchEvent() {
		if (dropdownEventCallback) {
			ofxDatGuiDropdownEvent e(this, mIndex, mOption);
			dropdownEventCallback(e);
		} else {
			ofxDatGuiLog::write(ofxDatGuiMsg::EVENT_HANDLER_NULL);
		}
	}

	static ofxDatGuiDropdown * getInstance() { return new ofxDatGuiDropdown("X"); }

	// Toggle "radio" behaviour at runtime (keeps API you already use)
	void setRadioMode(bool enabled) {
		mBehavior = enabled ? ofxDatGuiDropdownBehavior::RadioStaysOpen
							: ofxDatGuiDropdownBehavior::SelectCloses;

		if (mUseToggleChildren && enabled) {
			rebuildAs(true);
		} else if (mUseToggleChildren && !enabled) {
			rebuildAs(false);
		} else {
			for (int i = 0; i < static_cast<int>(children.size()); ++i) {
				if (auto * opt = dynamic_cast<ofxDatGuiDropdownOption *>(children[i])) {
					opt->setRadio(enabled);
					opt->setChecked(enabled && (i == mOption));
				}
			}
		}
	}

	bool isRadioMode() const { return mBehavior == ofxDatGuiDropdownBehavior::RadioStaysOpen; }

	// Optional: switch internal children to Toggles (you already had this hook)
	void useToggleChildren(bool b) { rebuildAs(b); }

private:
	// Button -> index mapper, then select
	void onOptionSelected(ofxDatGuiButtonEvent e) {
		int clicked = mOption;
		for (int i = 0; i < static_cast<int>(children.size()); ++i) {
			if (e.target == children[i]) {
				clicked = i;
				break;
			}
		}
		select(clicked);
	}

	// Toggle path for "useToggleChildren(true)"
	void onToggleSelected(ofxDatGuiToggleEvent e) {
		int clicked = mOption;
		for (int i = 0; i < static_cast<int>(children.size()); ++i) {
			if (e.target == children[i]) {
				clicked = i;
				break;
			}
		}
		select(clicked);
	}

	// Rebuild children as Toggle rows (or back to Option rows)
	void rebuildAs(bool useToggles) {
		std::vector<std::string> labels;
		labels.reserve(children.size());
		for (auto * c : children)
			labels.push_back(c->getLabel());

		for (auto * c : children) {
			// (Colour pickers are sometimes nested under folders. Preserve them if present.)
			if (c->getType() != ofxDatGuiType::COLOR_PICKER) delete c;
		}
		children.clear();

		for (int i = 0; i < static_cast<int>(labels.size()); ++i) {
			if (useToggles) {
				auto * t = new ofxDatGuiToggle(labels[i], i == mOption);
				t->setIndex(static_cast<int>(children.size()));
				t->onToggleEvent(this, &ofxDatGuiDropdown::onToggleSelected);
				children.push_back(t);
			} else {
				auto * opt = new ofxDatGuiDropdownOption(labels[i]);
				opt->setIndex(static_cast<int>(children.size()));
				opt->onButtonEvent(this, &ofxDatGuiDropdown::onOptionSelected);
				if (mBehavior == ofxDatGuiDropdownBehavior::RadioStaysOpen) {
					opt->setRadio(true);
					opt->setChecked(i == mOption);
				}
				children.push_back(opt);
			}
		}

		mUseToggleChildren = useToggles;
		setTheme(getTheme());
	}

private:
	int mOption;
	ofxDatGuiDropdownBehavior mBehavior;
	bool mUseToggleChildren;
};

// --- Folder helper to add dropdowns (definition) ---
inline ofxDatGuiDropdown * ofxDatGuiFolder::addDropdown(std::string label,
	const std::vector<std::string> & options) {
	auto * dd = new ofxDatGuiDropdown(std::move(label), options);
	dd->setStripeColor(mStyle.stripe.color);
	dd->onDropdownEvent(this, &ofxDatGuiFolder::dispatchDropdownEvent);
	attachItem(dd);
	return dd;
}
