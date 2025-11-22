# ofxDatGui-LoopyDev Refactor Roadmap ?

Bringing this addon up to modern, concise C++ with:

- **One clear GUI root**
- Proper **ownership**
- Predictable **input handling**
- Clean **state binding** via `ofParameter`

Use this checklist as you refactor the addon. Tick things off as you go.

---

## ?? Goals

- [ ] Have **one root GUI** per app (`ofxDatGui` as ï¿½GuiRootï¿½).
- [ ] Introduce a clean **Container vs Leaf** hierarchy.
- [ ] Remove fragile **global/static state** for input where possible.
- [ ] Use **RAII / `std::unique_ptr`** for component ownership.
- [ ] Keep the public API familiar but make internals sane and modern.
- [ ] Restructure components to optionally use **`ofParameter` and `ofParameterGroup`** for state and bindings.

---

## Phase 0 ï¿½ Safety Net & Minimal Example

- [x] Create a **minimal test app** (e.g. `example-refractor`):
  - [x] One `ofxDatGui` instance.
  - [x] A few folders, sliders, toggles.
  - [x] Basic hover / press / drag / expand-collapse behaviour.
  - [x] A simple ï¿½pageï¿½ concept (switching between groups/panels).
- [x] Use this app to quickly spot breaks after each refactor step.

---

## Phase 1 ï¿½ Introduce `ofxDatGuiContainer` (Common Container Base)

**Goal:** unify all ï¿½things that own childrenï¿½ behind a single base class.

- [x] Add `ofxDatGuiContainer`:
  - [x] `src/core/ofxDatGuiContainer.h` (and `.cpp`):
    - [x] `class ofxDatGuiContainer : public ofxDatGuiComponent`
    - [x] `std::vector<std::unique_ptr<ofxDatGuiComponent>> children;`
    - [x] `template<typename T, typename... Args> T* addChild(Args&&... args);`
    - [x] `void update(bool parentEnabled) override;`
    - [x] `void draw() override;`
    - [x] `virtual void layoutChildren() = 0;`
    - [x] `virtual void onChildrenChanged() { layoutChildren(); }`
- [x] Make **containers inherit from it**:
  - [x] `ofxDatGuiPanel : public ofxDatGuiContainer`
  - [x] `ofxDatGuiFolder : public ofxDatGuiContainer`
- [x] Refactor **existing child management**:
  - [x] Replace ad-hoc `attachItem(...)`-style child vectors in `Panel` with `addChild<>()` + `children`.
  - [x] Same for `Folder` (or at least route it through `ofxDatGuiContainer` internally).
  - [x] Move generic ï¿½update children / draw childrenï¿½ logic into `ofxDatGuiContainer`.
- [ ] Keep `ofxDatGui`ï¿½s `items` vector unchanged for now (raw pointers are fine here temporarily).

---

## Phase 2 ï¿½ Per-Root Mouse Capture & Root Back-Pointers

**Goal:** input and mouse capture are managed per root GUI, not via global statics.

- [x] Add a **root pointer** to `ofxDatGuiComponent`:
  - [x] `ofxDatGui* root = nullptr;`
  - [x] `void setRoot(ofxDatGui* r);`
  - [x] `ofxDatGui* getRoot() const;`
- [x] Ensure the root pointer is propagated:
  - [x] In `ofxDatGui::addXxx(...)`, after creating a component, call `component->setRoot(this);`
  - [x] In `ofxDatGuiContainer::addChild`, propagate the root to children:
    - [x] When the container’s root changes, update all children recursively.
- [x] Add **mouse capture owner** to `ofxDatGui`:
  - [x] In `ofxDatGui`:
    - [x] `ofxDatGuiComponent* mouseCaptureOwner = nullptr;`
    - [x] `void setMouseCapture(ofxDatGuiComponent* c);`
    - [x] `ofxDatGuiComponent* getMouseCapture() const;`
- [x] Remove global `sPressOwner` from `ofxDatGuiComponent`:
  - [x] Delete the static variable and its uses.
  - [x] Replace logic such as:
    ```cpp
    auto* root = getRoot();
    if (mousePressedThisFrame && hit && root && root->getMouseCapture() == nullptr) {
        root->setMouseCapture(this);
        onMousePress();
    }
    ```
  - [x] For drag/release:
    - [x] Only the component that equals `root->getMouseCapture()` handles drag.
    - [x] On mouse release, clear it: `root->setMouseCapture(nullptr);`
- [x] **De-fang multi-GUI focus switching**:
  - [x] Find the block in `ofxDatGui::update()` that scans `mGuis` on mouse press to change `mActiveGui`.
  - [x] Remove or disable it (comment out / guard with a flag).
  - [x] Commit to ???one gui per window??? as the normal usage.

---

## Phase 3 ï¿½ Move to `std::unique_ptr` Ownership

**Goal:** clear ownership; no leaks; fewer dangling pointers.

### 3.1 Root GUI item ownership

- [x] Change `ofxDatGui::items` from raw pointers to `std::unique_ptr`:
  - [x] `using ComponentPtr = std::unique_ptr<ofxDatGuiComponent>;`
  - [x] `std::vector<ComponentPtr> items;`
- [ ] Update all `addXxx(...)` functions:
  - [x] Use `std::make_unique<T>(...)` to allocate.
  - [x] Store in `items` via `emplace_back(std::move(ptr));`
  - [x] Call `setRoot(this)` and theme setup on the raw pointer.
  - [x] Return raw non-owning pointer to user code for convenience.
- [ ] Update loops:
  - [x] Replace `for (int i=0; i<items.size(); ++i) items[i]->update(...);`
    with `for (auto& c : items) c->update(...);`, etc.
- [x] Remove manual deletes in `ofxDatGui` destructor (RAII will handle it).

### 3.2 Container child ownership

- [x] `std::vector<std::unique_ptr<ofxDatGuiComponent>> children;` (in `ofxDatGuiContainer`).
- [x] `addChild<T>` returns raw pointer, but stores `unique_ptr`.
- [x] Stop using `ofxDatGuiComponent::children` (raw) for traversal; route recursive logic through container-owned children.
- [x] Migrate legacy containers like `ofxDatGuiGroup` off raw `children` (and remove manual deletes in its destructor).
- [x] Ensure mouse-down/theme/width propagation walks RAII children rather than raw vectors.


- [ ] In `ofxDatGuiContainer`, confirm:
  - [x] `std::vector<std::unique_ptr<ofxDatGuiComponent>> children;`
  - [x] `addChild<T>` returns raw pointer, but stores `unique_ptr`.
- [x] Ensure children are updated/drawn via RAII (no manual deletes).

---

## Phase 4 ï¿½ Theme & Layout Responsibilities

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

## Phase 5 ï¿½ API Polish & ï¿½One Root, Many Panelsï¿½ Pattern

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
  - [ ] Show a **top navigation bar panel** with radio-group ï¿½pagesï¿½.
  - [ ] Show multiple panels for different UI sections.
  - [ ] Demonstrate clean interaction (no double-click, no weird capture issues).
- [ ] Update the README to:
  - [ ] Explain the root/container/leaf hierarchy.
  - [ ] Recommend ï¿½one gui, many panelsï¿½ as the default usage.
  - [ ] Mark multi-root setups as ï¿½advanced / legacyï¿½ if still supported.

---

## Phase 6 ï¿½ `ofParameter` / `ofParameterGroup` Integration

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

- [ ] Make sure widgets donï¿½t outlive their `ofParameter`s:
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
  - [ ] ï¿½Classicï¿½ style (manual event handlers).
  - [ ] `ofParameter` style (state-driven, less glue code).

---

## After Refactor ï¿½ Nice-to-Haves (Optional)

If you still have energy:

- [ ] Add `std::function`-based callbacks in addition to / instead of global event listeners.
- [ ] Add scoped connection helpers for `ofEvents` to avoid dangling event handlers.
- [ ] Wrap example usage in a small helper API (e.g. ï¿½builderï¿½ functions) for even less boilerplate.

---

You can drop this into `REFACTOR_ROADMAP.md` or merge into your main `README.md` and tick things off as you go, e.g.:

- [x] Phase 0 Safety Net & Minimal Example (done 2025-01-XX)
- [ ] Phase 1 ï¿½ Container base (done 2025-11-21)
- [x] Phase 2 ? Mouse capture per root
- [ ] Phase 6 ï¿½ ofParameter integration
