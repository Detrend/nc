# ui-06: save-list pagination off-by-one

**Source:** code_review_findings_ui.txt, finding 6
**File:** source/nuclidean/engine/ui/ui_menu_page.cpp (`LoadGamePage::page_up`)

## Issue
```
page = min(page + 1, (s32)load_game_buttons.size() / PAGE_SIZE);
```
With a save count that is an exact multiple of `PAGE_SIZE` (e.g. 8 saves,
`PAGE_SIZE` 8), `size/PAGE_SIZE == 1` allowed page 1, which displayed
entries `[8, 16)` -- an empty page.

## Fix
Changed the last-page bound to `(size - 1) / PAGE_SIZE`, matching the fix
given directly in the review. Verified this also handles the empty-list
case (`size == 0`) correctly without a separate guard: `(0 - 1) /
PAGE_SIZE == -1 / PAGE_SIZE == 0` (C++ integer division truncates toward
zero), so `page` stays clamped to 0.
