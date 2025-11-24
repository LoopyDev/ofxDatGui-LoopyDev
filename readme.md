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

    // 1) Dynamic/gui-owned panel (lifetime tied to gui)
    auto& dyn = gui.createPanel("Dynamic Panel", ofxDatGuiPanel::Orientation::VERTICAL);
    dyn.setHeaderEnabled(true);
    dyn.setPosition(40, 40);         // always position panels manually
    dyn.addButton("Hello");
    dyn.addToggle("World", false);

    // 2) Stack-owned panel you attach to the gui
    stackPanel.setHeaderEnabled(true);
    stackPanel.addButton("External A");
    stackPanel.addButton("External B");

    // Attach hands theme/width/ownership to the gui while you keep the object alive.
    gui.attachPanel(stackPanel, "Attached Panel", ofxDatGuiPanel::Orientation::HORIZONTAL);
    stackPanel.setPosition(40, 200);
    stackPanel.setWidth(480, 0.35f);
}
```

- `createPanel(...)`/`addPanel(...)`: gui creates & owns the panel; destroyed with `gui`. Width/theme are inherited automatically.
- `attachPanel(panel, ...)`: plug in a stack-owned panel; caller keeps it alive. `attachPanel` will set the label/orientation/theme/width to match the gui, but you still position it manually.
- If you keep a stack-owned panel *detached* (not attached to a gui), set its theme yourself (`panel.setTheme(ofxDatGuiComponent::getTheme())`), and call `panel.update()` / `panel.draw()` manually.
