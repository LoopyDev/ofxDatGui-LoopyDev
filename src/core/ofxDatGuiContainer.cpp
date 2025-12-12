#include "ofxDatGuiContainer.h"
#include "ofxDatGui.h"
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

	ofxDatGui* root = getRoot();
	ofxDatGuiComponent* focusedInput = root ? root->getTextInputFocus() : nullptr;
	
	// If parent blocked interaction or we're disabled, children should also be blocked.
	if (!parentEnabled || !getEnabled()) {
		return;
	}

	// Collapsed containers should still update their own header state, but skip children.
	if (!getIsExpanded()) {
		return;
	}

	if (focusedInput != nullptr) {
		for (auto & child : children) {
			if (!child || !child->getVisible()) continue;
			const bool allow = root && root->isInTextInputFocusBranch(child.get());
			child->update(parentEnabled && allow);
		}
		return;
	}

	// Only the topmost visible child under the mouse should receive interaction;
	// others still tick but ignore input. This prevents stacked siblings from both
	// reacting when overlapped.
	ofxDatGuiComponent* hotChild = nullptr;
	ofPoint mouse(ofGetMouseX(), ofGetMouseY());
	for (int i = static_cast<int>(children.size()) - 1; i >= 0; --i) {
		auto & c = children[i];
		if (!c || !c->getVisible()) continue;
		if (c->hitTest(mouse)) {
			hotChild = c.get();
			break;
		}
	}

	for (auto & child : children) {
		const bool allow = (hotChild == nullptr) ? true : (child.get() == hotChild);
		child->update(allow);
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
