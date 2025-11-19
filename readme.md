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
### New components!
- Horizontal button bar/radio group (it's a bit messy right now but it works, expect changes)
- Cubic Bezier
- Radio Group
- Curve Editor

![ofxDatGui-LoopyDev](https://loopydev.co.uk/img/software/ofxDatGui-LoopyDev/gallery/1.webp "ofxDatGui-LoopyDev")  

Temp:
Restructuring horizontally stacked gui components. To add a horizontally stacked GUI panel:

	gui = new ofxDatGui(ofxDatGuiAnchor::TOP_LEFT);
	gui->setWidth(ofGetWidth());
	auto * row = gui->addPanel(ofxDatGuiPanel::Orientation::HORIZONTAL);
	row->attachItem(new ofxDatGuiButton("Play"));
	row->attachItem(new ofxDatGuiButton("Pause"));
	row->attachItem(new ofxDatGuiButton("Stop"));
	row->attachItem(new ofxDatGuiButton("Okeyyeh"));