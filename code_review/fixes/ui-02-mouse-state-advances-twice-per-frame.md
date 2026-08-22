# ui-02: mouse edge-detection state advanced twice per frame -> occasional lost clicks

**Source:** code_review_findings_ui.txt, finding 2
**Files:** source/nuclidean/engine/ui/ui_menu_manager.cpp, ui_menu_manager.h

## Issue
`get_normalized_mouse_pos()` had a hidden side effect: it shifted
`prev_mousestate <- cur_mousestate <- SDL_GetMouseState()`. It was called
once from `update()` (correct, feeding the click edge-detection used by
the current page's buttons) and again from `draw_cursor()` during
rendering (just to position the cursor sprite). A button press landing
between `update()` and `draw()` got copied into `prev_mousestate` before
the next `update()` ever saw it as "new" -- the press/release edge was
consumed by the render pass and the click never fired.

## Fix
Split the function:
- `advance_mouse_state()` (new): does the `prev <- cur <- fresh SDL read`
  state shift. Called exactly once per frame, from `update()` (both the
  `is_transition` early-return branch and the main path).
- `get_normalized_mouse_pos()`: now a pure position query -- reads the
  current mouse position via `SDL_GetMouseState` for the return value, but
  no longer touches `prev_mousestate`/`cur_mousestate`. `draw_cursor()`
  still calls this (for cursor placement) but no longer perturbs click
  state as a side effect.
