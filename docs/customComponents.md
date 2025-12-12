# Adding Custom Components

This project separates containers (own children) and leaves (self-contained widgets). Here’s how to add a new/experimental component without breaking the current setup.

## 1) Decide container vs leaf
- **Leaf**: derives from `ofxDatGuiComponent`; draws and handles its own input; owns no child components.
- **Container**: derives from `ofxDatGuiContainer`; owns children via `unique_ptr`; implements `layoutChildren()`.

## 2) Place the header
- Add the header in `src/components/YourThing.h` (or a subfolder you control).
- Keep includes consistent (e.g., `#include "ofxDatGuiYourThing.h"`); avoid temporary indirection layers.

## 3) Wire into the addon
- Include your header: `#include "ofxDatGuiYourThing.h"` (or whatever name you choose).
- To attach to panels/folders, add a helper in `ofxDatGuiPanel`/`ofxDatGuiFolder` that:
  - constructs the widget,
  - calls `setRoot`, `setTheme`, `setWidth`,
  - stores it via `addChild`/`emplaceChild` (containers) or returns it for manual placement.
- For manual insertion, you can `auto* w = new YourThing(...); w->setRoot(panel->getRoot()); w->setTheme(...); w->setWidth(...); panel->emplaceChild(std::unique_ptr<ofxDatGuiComponent>(w));`

## 4) Theme/layout/input basics
- In `setTheme`, call `setComponentStyle(theme)` and pick up any widget-specific colors/sizing.
- In `setWidth`, recompute your own geometry; don’t move siblings (containers handle layout).
- Respect `setOpacity`/`applyMutedPalette` if you want muting to work.
- For draggable/anchored behavior, follow existing patterns (see `ofxDatGuiPanel` for anchor handling).

## 5) Events
- Use `ofxDatGuiInteractiveObject` callbacks (`onButtonEvent`, `onToggleEvent`, etc.) as needed.
- Emit `ofxDatGuiInternalEvent` if the parent/container needs to react (e.g., visibility changes).

## 6) Types (optional)
- If you need a new `ofxDatGuiType`, add it to `core/ofxDatGuiConstants.h` and set `mType` in your component.

## 7) Docs and tracking (optional)
- Mention notable additions in `docs/internalGuiLayout/containers.md` or `leafComponents.md`.
- If it’s experimental/legacy, note it in `docs/tracking/internalLayoutsChecklist.md`.
