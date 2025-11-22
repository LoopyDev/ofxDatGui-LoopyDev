#include "ofxDatGuiGroups.h"
#include "ofxDatGuiDropdown.h"

ofxDatGuiDropdown * ofxDatGuiFolder::addDropdown(std::string label,
	const std::vector<std::string> & options) {
	auto * dd = new ofxDatGuiDropdown(std::move(label), options);
	dd->setStripeColor(mStyle.stripe.color);
	dd->onDropdownEvent(this, &ofxDatGuiFolder::dispatchDropdownEvent);
	attachItem(dd);
	return dd;
}
