# Containers

A *container* owns and lays out other GUI components. It derives from `ofxDatGuiContainer`, which provides:

- `std::vector<std::unique_ptr<ofxDatGuiComponent>> children;`
- `addChild<T>(...)` / `emplaceChild(...)` to take ownership
- `layoutChildren()` (pure virtual) where subclasses position children
- Recursive theme/width/root propagation and child traversal helpers

## Current containers
- `ofxDatGuiPanel` (top-level pane; root holds panels only)
- `ofxDatGuiFolder` (expand/collapse group)
- `ofxDatGuiGroup` (legacy; should still behave as a container)
- `ofxDatGuiScrollView` (wraps child items and manages viewport)
- `ofxDatGuiMatrix` (matrix owns its button cells)

## Responsibilities
- Own children via `unique_ptr`; never leak or share ownership.
- Implement `layoutChildren()` to place children when widths change, children added, or anchors update.
- Forward `setRoot`, `setTheme`, `setWidth`, `update`, and `draw` through to children (base class already does this; override only when needed).
- Do not perform leaf-level input handling for children; let children handle their own input once layout is set.

## When to create a container
- You need to own other widgets and decide their placement.
- You need expand/collapse, scrolling, or multi-row layouts composed of existing widgets.
- You need to wrap a collection (e.g., matrix/grid, list) and manage its children’s lifecycle.

## Anti-patterns to avoid
- Storing raw child pointers outside `children`.
- Manual deletes of children (RAII handles this).
- Letting leaves manage their own sublists instead of using `ofxDatGuiContainer`.

## Checklist for new containers
- [ ] Derive from `ofxDatGuiContainer`.
- [ ] Keep all owned components in `children` via `addChild`/`emplaceChild`.
- [ ] Implement `layoutChildren()`; call it when children change or sizes change.
- [ ] Use `forEachChild` instead of accessing `children` directly from outside.
- [ ] Propagate root/theme/width changes by delegating to the base class unless you have a specific reason not to.
