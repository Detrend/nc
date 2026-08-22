# ui-05: settings.cfg parsing could crash the game at startup

**Source:** code_review_findings_ui.txt, finding 5
**File:** source/nuclidean/engine/ui/ui_menu_page.cpp (`OptionsPage::load_settings`, `OptionsPage::save_settings`)

## Issue
```
auto data = nlohmann::json::parse(f);   // OUTSIDE the try block
try { ... } catch (int) {}              // dead handler (again)
```
`nlohmann::json::parse` throws `json::parse_error` on malformed input, and
it was called BEFORE the `try`; the `catch` clause wouldn't have caught it
anyway since it only catches `int` (same dead pattern as `load_json_map`,
fixed separately as `game-03`). `load_settings` runs at `post_init`, so a
corrupt or hand-edited `settings.cfg` aborted the game at boot with no
message. Also, `save_settings` warned on `ofstream` open failure and then
wrote to the failed stream anyway (a silent no-op, but confusing).

## Fix
- Moved `nlohmann::json::parse(f)` inside the `try` block.
- Changed `catch (int) {}` to `catch (const nlohmann::json::exception& e)`
  that logs `e.what()` via `nc_warn` -- a corrupt settings file now logs a
  clear warning and falls back to defaults instead of crashing at boot.
- Added a `return;` after `save_settings`'s open-failure warning, so it no
  longer writes to a stream that's already known to have failed to open.
