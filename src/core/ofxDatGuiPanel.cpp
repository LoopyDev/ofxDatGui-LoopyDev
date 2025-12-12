#include "ofxDatGuiPanel.h"
#include "../components/ofxDatGuiDropdown.h"
#include "../components/ofxDatGuiGroups.h"

ofxDatGuiDropdown* ofxDatGuiPanel::addDropdown(const std::string & label, const std::vector<std::string> & options) {
	auto item = std::make_unique<ofxDatGuiDropdown>(label, options);
	item->setStripeColor(mStyle.stripe.color);
	return attachOwned(std::move(item));
}

ofxDatGuiFolder* ofxDatGuiPanel::addFolder(const std::string & label, ofColor color) {
	auto folder = std::make_unique<ofxDatGuiFolder>(label, color);
	auto* raw = attachOwned(std::move(folder));
	if (raw != nullptr) {
		raw->setStripeColor(color);
	}
	return raw;
}

