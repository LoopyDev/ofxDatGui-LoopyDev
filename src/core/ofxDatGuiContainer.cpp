#include "ofxDatGuiContainer.h"
#include <utility>

void ofxDatGuiContainer::emplaceChild(ComponentPtr child) {
	if (!child) return;

	// Phase 1: Set up child with proper index and root pointer.
	child->setIndex(static_cast<int>(children.size()));
	child->setParent(this);
	child->setRoot(getRoot());
	
	// Take ownership and trigger layout update.
	children.emplace_back(std::move(child));
	onChildrenChanged();
}

void ofxDatGuiContainer::update(bool parentEnabled) {
	// Phase 1: Containers update themselves first, then children.
	// This allows containers to handle their own input/state before
	// delegating to children.
	ofxDatGuiComponent::update(parentEnabled);

	// If parent blocked interaction or we're disabled, children should also be blocked.
	if (!parentEnabled || !getEnabled()) {
		return;
	}

	// Update children with combined enabled state.
	const bool enabled = true; // parentEnabled && getEnabled() is already true here
	for (auto & child : children) {
		child->update(enabled);
	}
}

void ofxDatGuiContainer::draw() {
	// Phase 1: Containers only draw their children.
	// Subclasses that need to draw themselves should override
	// and call this base implementation after drawing container visuals.
	if (!mVisible) return;
	
	for (auto & child : children) {
		if (!child->getVisible()) continue;
		child->draw();
		child->drawColorPicker();
	}
}

void ofxDatGuiContainer::setRoot(ofxDatGui* r) {
	// Phase 1: Propagate root pointer to all children recursively.
	ofxDatGuiComponent::setRoot(r);
	for (auto & child : children) {
		child->setRoot(r);
	}
}

void ofxDatGuiContainer::forEachChild(const std::function<void(ofxDatGuiComponent*)> & fn) const {
	for (auto & child : children) fn(child.get());
}
