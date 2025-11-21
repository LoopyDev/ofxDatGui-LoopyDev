# ofxDatGui-LoopyDev Refactor Roadmap ?

Bringing this addon up to modern, concise C++ with:

- **One clear GUI root**
- Proper **ownership**
- Predictable **input handling**
- Clean **state binding** via `ofParameter`

Use this checklist as you refactor the addon. Tick things off as you go.

---

## ?? Goals

- [ ] Have **one root GUI** per app (`ofxDatGui` as �GuiRoot�).
- [ ] Introduce a clean **Container vs Leaf** hierarchy.
- [ ] Remove fragile **global/static state** for input where possible.
- [ ] Use **RAII / `std::unique_ptr`** for component ownership.
- [ ] Keep the public API familiar but make internals sane and modern.
- [ ] Restructure components to optionally use **`ofParameter` and `ofParameterGroup`** for state and bindings.

---

## Phase 0 � Safety Net & Minimal Example

- [x] Create a **minimal test app** (e.g. `example-refractor`):
  - [x] One `ofxDatGui` instance.
  - [x] A few folders, sliders, toggles.
  - [x] Basic hover / press / drag / expand-collapse behaviour.
  - [x] A simple �page� concept (switching between groups/panels).
- [x] Use this app to quickly spot breaks after each refactor step.

---

## Phase 1 � Introduce `ofxDatGuiContainer` (Common Container Base)

**Goal:** unify all �things that own children� behind a single base class.

- [ ] Add `ofxDatGuiContainer`:
  - [ ] `src/core/ofxDatGuiContainer.h` (and `.cpp`):
    - [ ] `class ofxDatGuiContainer : public ofxDatGuiComponent`
    - [ ] `std::vector<std::unique_ptr<ofxDatGuiComponent>> children;`
    - [ ] `template<typename T, typename... Args> T* addChild(Args&&... args);`
    - [ ] `void update(bool parentEnabled) override;`
    - [ ] `void draw() override;`
    - [ ] `virtual void layoutChildren() = 0;`
    - [ ] `virtual void onChildrenChanged() { layoutChildren(); }`
- [ ] Make **containers inherit from it**:
  - [ ] `ofxDatGuiPanel : public ofxDatGuiContainer`
  - [ ] `ofxDatGuiFolder : public ofxDatGuiContainer`
- [ ] Refactor **existing child management**:
  - [ ] Replace ad-hoc `attachItem(...)`-style child vectors in `Panel` with `addChild<>()` + `children`.
  - [ ] Same for `Folder` (or at least route it through `ofxDatGuiContainer` internally).
  - [ ] Move generic �update children / draw children� logic into `ofxDatGuiContainer`.
- [ ] Keep `ofxDatGui`�s `items` vector unchanged for now (raw pointers are fine here temporarily).

---

## Phase 2 � Per-Root Mouse Capture & Root Back-Pointers

**Goal:** input and mouse capture are managed per root GUI, not via global statics.

- [ ] Add a **root pointer** to `ofxDatGuiComponent`:
  - [ ] `ofxDatGui* root = nullptr;`
  - [ ] `void setRoot(ofxDatGui* r);`
  - [ ] `ofxDatGui* getRoot() const;`
- [ ] Ensure the root pointer is propagated:
  - [ ] In `ofxDatGui::addXxx(...)`, after creating a component, call `component->setRoot(this);`
  - [ ] In `ofxDatGuiContainer::addChild`, propagate the root to children:
    - [ ] When the container�s root changes, update all children recursively.
- [ ] Add **mouse capture owner** to `ofxDatGui`:
  - [ ] In `ofxDatGui`:
    - [ ] `ofxDatGuiComponent* mouseCaptureOwner = nullptr;`
    - [ ] `void setMouseCapture(ofxDatGuiComponent* c);`
    - [ ] `ofxDatGuiComponent* getMouseCapture() const;`
- [ ] Remove global `sPressOwner` from `ofxDatGuiComponent`:
  - [ ] Delete the static variable and its uses.
  - [ ] Replace logic such as:
    ```cpp
    if (mousePressedThisFrame && hit && sPressOwner == nullptr) {
        sPressOwner = this;
        onMousePress();
    }
    ```
    with:
    ```cpp
    auto* root = getRoot();
    if (mousePressedThisFrame && hit && root && root->getMouseCapture() == nullptr) {
        root->setMouseCapture(this);
        onMousePress();
    }
    ```
  - [ ] For drag/release:
    - [ ] Only the component that equals `root->getMouseCapture()` handles drag.
    - [ ] On mouse release, clear it: `root->setMouseCapture(nullptr);`
- [ ] **De-fang multi-GUI focus switching**:
  - [ ] Find the block in `ofxDatGui::update()` that scans `mGuis` on mouse press to change `mActiveGui`.
  - [ ] Remove or disable it (comment out / guard with a flag).
  - [ ] Commit to �one gui per window� as the normal usage.

---

## Phase 3 � Move to `std::unique_ptr` Ownership

**Goal:** clear ownership; no leaks; fewer dangling pointers.

### 3.1 Root GUI item ownership

- [ ] Change `ofxDatGui::items` from raw pointers to `std::unique_ptr`:
  - [ ] `using ComponentPtr = std::unique_ptr<ofxDatGuiComponent>;`
  - [ ] `std::vector<ComponentPtr> items;`
- [ ] Update all `addXxx(...)` functions:
  - [ ] Use `std::make_unique<T>(...)` to allocate.
  - [ ] Store in `items` via `emplace_back(std::move(ptr));`
  - [ ] Call `setRoot(this)` and theme setup on the raw pointer.
  - [ ] Return raw non-owning pointer to user code for convenience.
- [ ] Update loops:
  - [ ] Replace `for (int i=0; i<items.size(); ++i) items[i]->update(...);`
    with `for (auto& c : items) c->update(...);`, etc.
- [ ] Remove manual deletes in `ofxDatGui` destructor (RAII will handle it).

### 3.2 Container child ownership

- [ ] In `ofxDatGuiContainer`, confirm:
  - [ ] `std::vector<std::unique_ptr<ofxDatGuiComponent>> children;`
  - [ ] `addChild<T>` returns raw pointer, but stores `unique_ptr`.
- [ ] Ensure children are updated/drawn via RAII (no manual deletes).

---

## Phase 4 � Theme & Layout Responsibilities

**Goal:** theme and layout are explicit, not magical.

- [ ] Make `ofxDatGui` own the theme:
  - [ ] `std::unique_ptr<ofxDatGuiTheme> theme;`
  - [ ] `void setTheme(std::unique_ptr<ofxDatGuiTheme> t);`
  - [ ] Possibly keep a convenience `setTheme(const ofxDatGuiTheme& t)` that copies.
- [ ] Implement theme propagation:
  - [ ] Add `applyThemeRecursive()` on root that:
    - [ ] Applies theme to root.
    - [ ] Walks through all `items` and all container children, calling `component->applyTheme(theme.get());`
- [ ] Reduce or remove static/global theme access where possible:
  - [ ] Prefer root-owned theme passed down / referenced.
- [ ] Layout:
  - [ ] Ensure `ofxDatGuiContainer::layoutChildren()` is called:
    - [ ] On `onChildrenChanged()`.
    - [ ] On resize / anchor changes if relevant.
  - [ ] Each container subclass implements its own layout (H/V, etc.) cleanly.

---

## Phase 5 � API Polish & �One Root, Many Panels� Pattern

**Goal:** the public API matches how you actually want to use the addon.

- [ ] Decide on the **canonical usage** pattern in examples/README:
  - [ ] One `ofxDatGui` instance as root.
  - [ ] Multiple `ofxDatGuiPanel` instances for top bar, sidebars, pages.
  - [ ] Folders and widgets inside panels.
- [ ] Add convenience methods to `ofxDatGuiPanel`:
  - [ ] `ofxDatGuiSlider* addSlider(const std::string& label, float min, float max, float value);`
  - [ ] `ofxDatGuiToggle* addToggle(const std::string& label, bool enabled);`
  - [ ] `ofxDatGuiButton* addButton(const std::string& label);`
  - [ ] Internally: use `addChild<>()` + proper ownership.
- [ ] Update / add examples that:
  - [ ] Show a **top navigation bar panel** with radio-group �pages�.
  - [ ] Show multiple panels for different UI sections.
  - [ ] Demonstrate clean interaction (no double-click, no weird capture issues).
- [ ] Update the README to:
  - [ ] Explain the root/container/leaf hierarchy.
  - [ ] Recommend �one gui, many panels� as the default usage.
  - [ ] Mark multi-root setups as �advanced / legacy� if still supported.

---

## Phase 6 � `ofParameter` / `ofParameterGroup` Integration

**Goal:** link widgets to actual app state via `ofParameter`, reduce manual sync code, and allow swapping GUI front-ends without touching logic.

### 6.1 Add `ofParameter`-backed constructors / attach methods

- [ ] For **scalar components**:
  - [ ] `ofxDatGuiSlider`:
    - [ ] Add constructor or factory: `ofxDatGuiSlider(ofParameter<float>& param);`
    - [ ] Or method: `void bind(ofParameter<float>& param);`
  - [ ] `ofxDatGuiToggle`:
    - [ ] `ofxDatGuiToggle(ofParameter<bool>& param);` / `bind(ofParameter<bool>& param);`
  - [ ] `ofxDatGuiDropdown`, `ofxDatGuiTextInput`, etc.:
    - [ ] Equivalent `ofParameter<T>` or `ofParameter<std::string>` bindings where sensible.
- [ ] Internal logic:
  - [ ] Store a pointer/reference to the bound `ofParameter` (non-owning).
  - [ ] On GUI change ? update parameter.
  - [ ] Optionally listen to parameter `.addListener` ? update GUI when external code changes the value.

### 6.2 Panel / folder convenience for parameter groups

- [ ] Add overloads to panels/folders like:
  - [ ] `ofxDatGuiSlider* addSlider(ofParameter<float>& param);`
  - [ ] `ofxDatGuiToggle* addToggle(ofParameter<bool>& param);`
- [ ] Introduce a helper for `ofParameterGroup`:
  - [ ] Accept an `ofParameterGroup&` and auto-create folders/controls:
    - [ ] For each `ofParameter<float>` ? slider
    - [ ] For each `ofParameter<bool>`  ? toggle
    - [ ] For nested `ofParameterGroup` ? nested folder
  - [ ] This can be opt-in (e.g. `addParameters(ofParameterGroup& group)`).

### 6.3 Ensure lifetime & listener safety

- [ ] Make sure widgets don�t outlive their `ofParameter`s:
  - [ ] Document that parameters must exist as long as the GUI does.
  - [ ] (Optionally) use weak pointers / explicit `unbind()` if you want to be extra safe.
- [ ] When destroying a widget:
  - [ ] Remove any parameter listeners to avoid callbacks into dead GUI components.

### 6.4 Examples with `ofParameter`

- [ ] Add a new `example-parameters`:
  - [ ] `ofParameterGroup guiParams;`
  - [ ] `ofParameter<float> rocketX{"Rocket X", 0, -1000, 1000};`
  - [ ] `ofParameter<bool> wireframe{"Wireframe", true};`
  - [ ] Build the GUI from the parameter group.
  - [ ] Use the parameters directly in `draw()` / `update()` (no manual copy from widgets).
- [ ] Show both styles:
  - [ ] �Classic� style (manual event handlers).
  - [ ] `ofParameter` style (state-driven, less glue code).

---

## After Refactor � Nice-to-Haves (Optional)

If you still have energy:

- [ ] Add `std::function`-based callbacks in addition to / instead of global event listeners.
- [ ] Add scoped connection helpers for `ofEvents` to avoid dangling event handlers.
- [ ] Wrap example usage in a small helper API (e.g. �builder� functions) for even less boilerplate.

---

You can drop this into `REFACTOR_ROADMAP.md` or merge into your main `README.md` and tick things off as you go, e.g.:

- [x] Phase 0 Safety Net & Minimal Example (done 2025-01-XX)
- [ ] Phase 1 � Container base (done 2025-11-21)
- [ ] Phase 2 � Mouse capture per root
- [ ] Phase 6 � ofParameter integration
