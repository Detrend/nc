# graphics-01: level sector meshes are never freed -- leak on every level load, restart, and death

**Source:** code_review_findings_graphics.txt, finding 1
**File:** source/nuclidean/engine/graphics/graphics_system.cpp (`GraphicsSystem::on_event`)

## Issue
`create_sector_meshes()` runs on every `after_map_rebuild` and creates
fresh `ResLifetime::Level` meshes (VAO+VBO per sector).
`MeshManager::unload(Level)` existed but was never called anywhere -- the
only `unload` call in the codebase was `unload(Game)` at shutdown. Every
level change leaked all sector meshes of the previous level; since dying
triggers a level restart, every player death leaked a full level's worth
of GPU geometry.

## Fix
Added a `before_map_rebuild` case to `GraphicsSystem::on_event` that calls
`MeshManager::get().unload(ResLifetime::Level)` -- exactly the "natural
place" the review pointed at (the event already existed and nobody
listened to it). `unload()` is safe to call when nothing is loaded yet
(empty storage, `glDelete*` with count 0 is a no-op), so this is also safe
on the very first level load at startup.

Not touched: `TextureManager::unload`, which the review notes has its own
separate TODOs about not actually freeing everything -- that's a distinct,
larger piece of unfinished work, not a one-line "call the existing
function" fix like this one.
