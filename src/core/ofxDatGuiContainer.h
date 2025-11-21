#pragma once

#include "ofxDatGuiComponent.h"
#include <memory>
#include <vector>

// Shared container base: owns children via unique_ptr and centralises
// update/draw traversal. Subclasses only implement layout hooks.
class ofxDatGuiContainer : public ofxDatGuiComponent {
public:
	using ComponentPtr = std::unique_ptr<ofxDatGuiComponent>;

	explicit ofxDatGuiContainer(const std::string & label = "")
		: ofxDatGuiComponent(label) {}

	~ofxDatGuiContainer() override = default;

	// Add a child of type T, forwarding constructor args.
	// Returns raw pointer for convenience (non-owning).
	template <typename T, typename... Args>
	T * addChild(Args&&... args) {
		auto child = std::make_unique<T>(std::forward<Args>(args)...);
		T * raw = child.get();
		emplaceChild(std::move(child));
		return raw;
	}

	// Directly take ownership of an already-created component.
	// Will layout after insertion.
	void emplaceChild(ComponentPtr child);

	// Child traversal
	void update(bool parentEnabled) override;
	void draw() override;

	// Subclasses must lay out children when these are invoked.
	virtual void layoutChildren() = 0;
	virtual void onChildrenChanged() { layoutChildren(); }

	// Expose read-only view for callers that need to inspect.
	const std::vector<ComponentPtr> & getChildren() const { return children; }

	// Propagate root pointer downwards when container's root changes.
	void setRoot(ofxDatGui* r) override;

protected:
	std::vector<ComponentPtr> children;
};
