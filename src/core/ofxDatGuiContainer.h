#pragma once

#include "ofxDatGuiComponent.h"
#include <memory>
#include <vector>

/*
    ofxDatGuiContainer
    ------------------
    The shared base for anything that owns child components (e.g. Panel, Folder).
    - Owns children with std::unique_ptr for clear lifetime/RAII.
    - Centralises update()/draw() traversal so containers only worry about layout.
    - Exposes layoutChildren() as the one hook subclasses must implement.
    - Propagates the GUI root pointer to all descendants via setRoot().
    - Provides addChild<>() and emplaceChild(...) helpers to standardise insertion.
    Containers are not leaf widgets; they orchestrate spacing/positioning and
    delegate the actual rendering/input to the child components themselves.
*/
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
	void forEachChild(const std::function<void(ofxDatGuiComponent*)> & fn) const override;

protected:
	// Phase 1: Children owned via unique_ptr for RAII and clear ownership.
	std::vector<ComponentPtr> children;
};
