# Leaf Components

A *leaf* is any widget that does not own other GUI components. It draws itself and handles its own input/state but never manages a child list.

## Examples
- Buttons, toggles, labels
- Sliders, value plotters, wave monitors
- Text inputs, dropdowns, color pickers
- 2d pads, FRM monitors, matrix cells (the matrix itself is a container)
- Complex widgets such as curve editors or bezier editors, as long as they do not own other GUI components

## Responsibilities
- No child ownership: do not store or manage `children` beyond read-only traversal provided by base helpers.
- Handle your own drawing and input; containers only position you.
- Respect theme/width/opacity propagated from parents/root.
- If you expose expandable/collapsible UI, implement it internally without adding child components; otherwise promote it to a container.

## Do / Don’t
- Do: implement input/draw/state for the widget itself.
- Do: let containers handle layout; leaf widgets should not reposition siblings.
- Don’t: create or own other `ofxDatGuiComponent` instances. If you need composition, refactor into a container.
- Don’t: manage lifetime of sibling components.

## When to stay a leaf vs become a container
- Stay a leaf if the widget is self-contained, even if visually large or interactive.
- Become a container if you need to aggregate other widgets (lists, grids, scrollable stacks, nested sections).

## Quick checklist for leaves
- [ ] No owned child components.
- [ ] Implements draw/input/state for itself only.
- [ ] Accepts layout/size from its parent container.
- [ ] Applies theme/width/opacity via base `setTheme`/`setWidth`/`setOpacity` overrides.
