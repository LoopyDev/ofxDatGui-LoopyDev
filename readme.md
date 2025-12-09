# ofxDatGui (LoopyDev fork)

**ofxDatGui** is a simple, fully customizable, high-resolution GUI for [openFrameworks](https://openframeworks.cc/), inspired by [dat.gui](https://github.com/dataarts/dat.gui).
> This repository is a maintained fork by **LoopyDev**.  
> Upstream: https://github.com/braitsch/ofxDatGui

![ofxDatGui](https://braitsch.github.io/ofxDatGui/img/ofxdatgui_.png "ofxDatGui")  
*(Image credit: upstream project)*

---

## What’s different in this fork
- Working in OF 0.12.1
- Mouse capture - prevents control when dragging clicked mouse over GUI
- Folder nesting
- Anchors deprecated; manual layout is now the default. Position panels explicitly.
### New components!
- Horizontal button bar/radio group (it's a bit messy right now but it works, expect changes)
- Cubic Bezier
- Radio Group
- Curve Editor

## Roadmap (LoopyDev)
- Positioning helpers for panels (reintroduce anchor-like conveniences per panel, optional)
- Continued cleanup of manual layout defaults and API polish
- More examples showcasing stack-owned panels + dynamic panels
- Bring-to-front z-ordering for GUI-owned panels (drag headers to raise)
- Panel drag bounds (prevent panels from being dragged outside the window)

![ofxDatGui-LoopyDev](https://loopydev.co.uk/img/software/ofxDatGui-LoopyDev/gallery/1.webp "ofxDatGui-LoopyDev")  

Temp:
Restructuring horizontally stacked gui components. To add a horizontally stacked GUI panel:

	gui = new ofxDatGui(); // anchors removed; manual layout by default
	gui->setWidth(ofGetWidth());
	auto * row = gui->addPanel(ofxDatGuiPanel::Orientation::HORIZONTAL);
	row->attachItem(new ofxDatGuiButton("Play"));
	row->attachItem(new ofxDatGuiButton("Pause"));
	row->attachItem(new ofxDatGuiButton("Stop"));
	row->attachItem(new ofxDatGuiButton("Okeyyeh"));

---

## Quick setup & panel patterns

Anchors are deprecated; everything is manual layout. `ofxDatGui` defaults to `setAutoDraw(true)` so you usually don't need to call `draw()`/`update()` yourself unless you disable autodraw.

```cpp
// ofApp.h
ofxDatGui gui;                // gui-owned components live inside this
ofxDatGuiPanel stackPanel;    // stack-owned panel you keep alive yourself
```

```cpp
// ofApp.cpp
void ofApp::setup() {
    gui.setup();                     // manual layout, auto draw/update enabled
    gui.setPosition(40, 40);
    gui.setWidth(320);

    auto& panel = gui.createPanel("Main Panel", ofxDatGuiPanel::Orientation::VERTICAL);
    panel.setHeaderEnabled(true);
    panel.setPosition(40, 40);         // always position panels manually
    panel.addButton("Hello");
    panel.addToggle("World", false);
}
```

- `createPanel(...)`/`addPanel(...)`: gui creates & owns the panel; destroyed with `gui`. Width/theme are inherited automatically.
- Optional z-order: `gui.setBringToFrontOnInteract(true)` raises the most recently interacted top-level item in draw order without changing manual positions.
- Muting: `gui.setMuteUnfocusedPanels(true)` applies muted theme colors to non-focused panels. Per-panel opt-out: `panel.setPreventMuting(true)` (muting is allowed by default).

## Binding callbacks
Two equivalent styles are available:

- Direct callbacks (lambda or `std::function`): `button->onButtonEvent([](ofxDatGuiButtonEvent e){ ofLog() << e.target->getLabel(); });`
- ofxGui-style listeners: `button->addButtonListener(this, &ofApp::onButtonPressed);` (and `removeButtonListener(...)` to clear). Toggles also support `addToggleListener/removeToggleListener`.

Example:
```cpp
// Lambdas
button->onButtonEvent([](ofxDatGuiButtonEvent e){
    ofLog() << "Clicked: " << e.target->getLabel();
});

// ofxGui-style listeners
button->addButtonListener(this, &ofApp::onButtonPressed);
toggle->addToggleListener(this, &ofApp::onToggleChanged);

void ofApp::onButtonPressed(ofxDatGuiButtonEvent e) {
    ofLogNotice() << "Pressed " << e.target->getLabel();
}

void ofApp::onToggleChanged(ofxDatGuiToggleEvent e) {
    ofLogNotice() << "Toggle is now " << e.checked;
}

// You can also bind a no-arg member function for buttons:
button->addButtonListener(this, &ofApp::onPressedWithoutArgs);
```
