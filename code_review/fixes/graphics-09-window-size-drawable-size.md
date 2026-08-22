# graphics-09: window-size check used SDL_GetWindowSize instead of SDL_GL_GetDrawableSize

**Source:** code_review_findings_graphics.txt, "Minor notes" (own TODO already present nearby)
**File:** source/nuclidean/engine/graphics/graphics_system.cpp (`GraphicsSystem::render`)

## Issue
`SDL_GetWindowSize` returns the window size in screen coordinates
("points"), which differs from the actual framebuffer size in pixels on
high-DPI displays. This size feeds directly into `m_renderer->
on_window_resized(...)`, which sizes the G-buffer -- on a high-DPI
display, the G-buffer would be sized wrong relative to the actual
drawable surface.

## Fix
Swapped `SDL_GetWindowSize` for `SDL_GL_GetDrawableSize`, which returns
the drawable size in pixels that the GL context actually renders into.
