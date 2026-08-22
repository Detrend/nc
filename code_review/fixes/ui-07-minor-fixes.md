# ui-07: pragma once in .cpp files + screen flash drawn every frame even when invisible

**Source:** code_review_findings_ui.txt, "Minor notes"
**Files:** source/nuclidean/engine/ui/ui_menu_page.cpp,
           source/nuclidean/engine/ui/ui_button.cpp,
           source/nuclidean/engine/ui/ui_screen_effect.cpp

## Issue 1: `#pragma once` in .cpp files
`ui_menu_page.cpp:2` and `ui_button.cpp:2` had a meaningless `#pragma
once` (same pattern separately noted/fixed in the sound layer as
`sound-07`) -- suggests these were once headers; invites confusion.
Removed both.

## Issue 2: full-screen flash quad drawn every frame even when fully faded
`UiScreenEffect::draw()` always issued a full-screen blended draw call,
even when both the damage-flash and pickup-flash alphas were 0 (i.e.
nothing visible) -- free fill-rate cost every frame most of the time.

### Fix
Added an early `return` at the top of `draw()` when both effects are
fully faded (`(MAX_DMG_FLASH_DURATION - time_since_last_dmg) == 0` and the
equivalent pickup check), skipping the shader bind/uniform
setup/`glDrawArrays` entirely. Pure performance change -- when either
effect is active, behavior is identical to before.
