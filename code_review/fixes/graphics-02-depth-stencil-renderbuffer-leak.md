# graphics-02: depth-stencil renderbuffer leaked on every window resize

**Source:** code_review_findings_graphics.txt, finding 2
**File:** source/nuclidean/engine/graphics/renderer.cpp/.h (`create_renderbuffers`/`destroy_renderbuffers`)

## Issue
`create_renderbuffers` generated a local `depth_stencil_buffer`
renderbuffer that was never stored anywhere; `destroy_renderbuffers`
deletes the FBO and the six g-buffer textures but had no member to
reference this renderbuffer, so it could never delete it.
`Renderer::on_window_resized` calls `destroy_renderbuffers()` then
`create_renderbuffers()` on every resize, so every resize leaked a
`width x height` `DEPTH24_STENCIL8` buffer (~8 MB at 1080p). The window is
resizable in editor mode, where interactive resizing fires this
repeatedly.

## Fix
- Added a `GLuint m_g_depth_stencil = 0;` member to `Renderer` (next to
  the other g-buffer handles).
- `create_renderbuffers` now writes directly into `m_g_depth_stencil`
  instead of a local variable.
- `destroy_renderbuffers` now deletes it via `glDeleteRenderbuffers(1,
  &m_g_depth_stencil)` and resets it to 0, inside the same `if (m_g_buffer)`
  guard as the other g-buffer cleanup (with matching
  `nc_assert(glIsRenderbuffer(...))` sanity check, mirroring the pattern
  already used for the other g-buffer resources in this function).
