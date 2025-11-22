#pragma once

#include "ofxDatGuiComponent.h"
#include <memory>
#include <vector>

// Shared container base: owns children via unique_ptr and centralises
// update/draw traversal. Subclasses only implement layout hooks.
//
// Phase 1: Common container base for all components that own children.
// This unifies child management across Panel, Folder, and future containers.
class ofxDatGuiContainer : public ofxDatGuiComponent {
public:
	using ComponentPtr = std::unique_ptr<ofxDatGuiComponent>;

	explicit ofxDatGuiContainer(const std::string & label = "")
		: ofxDatGuiComponent(label) {}

	~ofxDatGuiContainer() override = default;

	// Add a child of type T, forwarding constructor args.
	// Returns raw pointer for convenience (non-owning).
	// Phase 1: Template method for type-safe child creation.
	template <typename T, typename... Args>
	T * addChild(Args&&... args) {
		auto child = std::make_unique<T>(std::forward<Args>(args)...);
		T * raw = child.get();
		emplaceChild(std::move(child));
		return raw;
	}

	// Directly take ownership of an already-created component.
	// Will layout after insertion via onChildrenChanged().
	void emplaceChild(ComponentPtr child);

	// Child traversal - Phase 1: Centralized update/draw logic.
	void update(bool parentEnabled) override;
	void draw() override;

	// Subclasses must implement layout logic.
	// Phase 1: Pure virtual - each container type has its own layout strategy.
	virtual void layoutChildren() = 0;
	
	// Called after children are added/removed. Default implementation
	// triggers layout. Subclasses can override for additional logic.
	virtual void onChildrenChanged() { layoutChildren(); }

	// Expose read-only view for callers that need to inspect children.
	const std::vector<ComponentPtr> & getChildren() const { return children; }

	// Propagate root pointer downwards when container's root changes.
	// Phase 1: Ensures all children know their root GUI.
	void setRoot(ofxDatGui* r) override;

protected:
	// Phase 1: Children owned via unique_ptr for RAII and clear ownership.
	std::vector<ComponentPtr> children;
};
