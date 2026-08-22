# graphics-08: wrong sentinel comparison + unguarded render_gun null deref via C-style cast

**Source:** code_review_findings_graphics.txt, "Minor notes"
**File:** source/nuclidean/engine/graphics/renderer.cpp

## Issue 1: `wall.render_data_index == INVALID_SECTOR_ID` -- wrong sentinel
`render_data_index` is a `PortalRenderID` (default `INVALID_PORTAL_RENDER_ID`),
not a `SectorID`. The check against `INVALID_SECTOR_ID` only worked
because both sentinels happen to be `u16(-1)` today; the correct sentinel
(`INVALID_PORTAL_RENDER_ID`) is already used correctly two hundred-ish
lines below at `renderer.cpp:1050`. Same bug family as `physics.cpp:588`
(fixed separately as part of the map/ module).

### Fix
Changed the comparison to `wall.render_data_index ==
INVALID_PORTAL_RENDER_ID`.

## Issue 2: render_gun null-derefs the player through a C-style cast
```
const vec2 player_position = ((Entity*)GameHelpers::get().get_player())->get_position().xz;
```
`GameHelpers::get_player()` can return `nullptr`; this was only saved from
a crash by an invariant maintained in a different file
(`grab_render_gun_props` clears `gun.sprite` whenever there is no player,
and `render_gun` early-outs on an empty sprite) -- fragile, and the
C-style cast to `Entity*` only compiles/works because `Player` derives
from `Entity` via ordinary single inheritance at offset 0; with only a
forward declaration of `Player` in scope, the compiler could not verify
that relationship, so it silently fell back to `reinterpret_cast`
semantics.

### Fix
Added `#include <engine/player/player.h>` (previously only
forward-declared here) and replaced the cast with an explicit null check:
```
Player* player = GameHelpers::get().get_player();
if (!player) return;
const vec2 player_position = player->get_position().xz;
```
This no longer depends on the sprite-clearing invariant holding, and the
now-complete `Player` type means the base-class access is a real,
compiler-checked upcast instead of a raw reinterpret.
