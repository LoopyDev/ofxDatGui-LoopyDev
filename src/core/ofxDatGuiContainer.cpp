#include "ofxDatGuiContainer.h"
#include <utility>

void ofxDatGuiContainer::emplaceChild(ComponentPtr child) {
	if (!child) return;

	child->setIndex(static_cast<int>(children.size()));
	child->setRoot(getRoot());
	children.emplace_back(std::move(child));
	onChildrenChanged();
}

void ofxDatGuiContainer::update(bool parentEnabled) {
	// Containers themselves don't intercept input beyond visibility/enabled.
	// Let base update handle our own hover/press, then update children.
	ofxDatGuiComponent::update(parentEnabled);

	const bool enabled = parentEnabled && getEnabled();
	for (auto & child : children) {
		child->update(enabled);
	}
}

void ofxDatGuiContainer::draw() {
	if (!mVisible) return;
	for (auto & child : children) {
		if (!child->getVisible()) continue;
		child->draw();
		child->drawColorPicker();
	}
}

void ofxDatGuiContainer::setRoot(ofxDatGui* r) {
	ofxDatGuiComponent::setRoot(r);
	for (auto & child : children) {
		child->setRoot(r);
	}
}
