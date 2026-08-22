# graphics-04: Texture lookup by name threw with no fallback -- one bad sprite name aborted the game

**Source:** code_review_findings_graphics.txt, finding 4
**File:** source/nuclidean/engine/graphics/resources/texture.cpp (`TextureManager::operator[](name, lifetime)`)

## Issue
`TextureManager::operator[]` used `unordered_map::at()`, which throws
`std::out_of_range` for unknown names, and nothing up the stack catches
it. Sprite names are built dynamically all over the code
(`std::format("{}_{}", ...)` for projectile/particle/enemy animation
frames, `"_dl"`/`"_ur"` suffixes for dir8 sprites) -- one typo in a level
file, a missing frame on disk, or a frame-index bug elsewhere becomes an
instant `terminate()` instead of a visibly-wrong texture.

## Fix
Replaced `.at()` with `.find()` plus an explicit miss path: on a missing
name, logs via `nc_crit` (name + which atlas) and returns a reference to
an existing texture already in that same atlas bundle
(`bundle.textures.begin()->second`) instead of throwing.

This is a smaller fix than "return the dedicated magenta error texture"
that the review suggested as the ideal outcome: `create_error_texture()`
builds a **standalone** GL texture (`m_error_texture`, a raw `GLuint`) that
is never baked into the shared texture atlas, whereas `TextureHandle` (the
type this function returns) only describes a *sub-rectangle within* that
shared atlas -- there is no atlas rectangle for the error texture to
reference. Wiring the error texture into the atlas for real would mean
reserving space for it during atlas packing (`finish_load`, which uses
`stb_rect_pack`) and deciding whether it belongs in the Game bundle, the
Level bundle, or both, and how that survives `unload()`/reload -- real
architecture work, not a one-line fix. The fallback used here (an existing,
already-valid texture, loudly logged) delivers the actual asked-for
outcome -- "one bad sprite name should not crash the game" -- without that
larger change. Making it a true visible-in-game magenta checker texture is
left for the operator if wanted; see NEEDS_USER_INPUT.md.
