# graphics-07: render_sky_box draws even when no SkyBox entity exists

**Source:** code_review_findings_graphics.txt, finding 7
**File:** source/nuclidean/engine/graphics/renderer.cpp (`Renderer::render_sky_box`)

## Issue
The `for_each<SkyBox>` loop bound the texture and set uniforms, but the
draw call was unconditionally issued AFTER the loop regardless of whether
it ran. With zero skyboxes in a level, the cube was drawn with whatever
texture happened to already be bound to `GL_TEXTURE0` and stale
exposure/gamma uniforms from a previous frame/level -- garbage where the
sky should be on levels without a skybox.

## Fix
Added a `has_sky_box` flag set inside the `for_each<SkyBox>` callback and
gated the `glBindVertexArray`/`glDrawArrays` call on it, so nothing is
drawn when there is no `SkyBox` entity.

Not changed: "with multiple skyboxes only the last one survives, silently"
-- left as-is (noted with a comment) since picking a policy for multiple
skyboxes (warn? use the first? blend?) is a design decision, not part of
this specific "draws garbage with zero skyboxes" bug.
