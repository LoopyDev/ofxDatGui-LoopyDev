# Internal Layouts Checklist (Container vs Leaf)

Use this to confirm the container/leaf split is clean across the addon.

## Audit child-owning classes
- [ ] Verify `ofxDatGuiGroup` inherits `ofxDatGuiContainer` and owns children via `unique_ptr`.
- [ ] Verify `ofxDatGuiScrollView` inherits `ofxDatGuiContainer`; no raw child vectors/deletes.
- [ ] Verify `ofxDatGuiMatrix` (the matrix itself) inherits `ofxDatGuiContainer`; cells are leaves.
- [ ] Scan for any other list/grid/wrapper components still managing children manually and migrate to `ofxDatGuiContainer`.

## Enforce the rule in code/docs
- [ ] Add/keep a short note in `ofxDatGuiContainer.h` stating: classes that own children must derive from `ofxDatGuiContainer`.
- [ ] Add/keep a note in `ofxDatGuiComponent.h` that leaves do not own children; use containers for ownership/layout.

## Layout & lifecycle
- [ ] Each container implements `layoutChildren()` and calls it on width/anchor/children changes; leaves do not manage siblings.
- [ ] Propagation (root/theme/width/opacity) flows through containers; leaves only handle their own draw/input.

## Examples & docs
- [ ] Add a brief pointer in README or docs index to `docs/internalGuiLayout/containers.md` and `leafComponents.md`.

## Close-out
- [ ] Once all child-owning classes are containers and no leaves manage children, tick “Introduce a clean Container vs Leaf hierarchy” in the roadmap.

### Audit status (2025-02-05)
- ofxDatGuiGroup: still inherits `ofxDatGuiButton`, not `ofxDatGuiContainer`; owns children via legacy `children` + `ownedChildren`. Needs conversion.
- ofxDatGuiScrollView: inherits `ofxDatGuiComponent`; manages items via raw pointers and manual delete. Needs container refactor.
- ofxDatGuiMatrix: inherits `ofxDatGuiComponent`; owns internal button structs (non-components). Decide if promoting to container or keep as leaf with internal buttons.
- Docs pointer in README: not added yet.
