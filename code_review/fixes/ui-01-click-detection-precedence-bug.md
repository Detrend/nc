# ui-01: click detection had an operator-precedence bug, repeated in six places

**Source:** code_review_findings_ui.txt, finding 1
**File:** source/nuclidean/engine/ui/ui_menu_page.cpp (6 occurrences)

## Issue
```
if (!prev_mouse & SDL_BUTTON(1) && cur_mouse & SDL_BUTTON(1))
```
`!` binds tighter than `&`, so this was actually `(!prev_mouse) & 1` --
true only when NO mouse button at all was pressed last frame, not
specifically "left button not pressed last frame". It only worked by
accident for the common case because `SDL_BUTTON(1) == 1`: hold the right
(or middle) button while left-clicking a menu button and the click was
silently swallowed.

## Fix
Changed all 6 occurrences (identical text, fixed with one `replace_all`)
to `!(prev_mouse & SDL_BUTTON(1)) && (cur_mouse & SDL_BUTTON(1))` --
explicit parentheses so `!` applies to the whole "was left button
pressed" expression, matching the intended "left button specifically not
pressed last frame, pressed this frame" edge-detection.
