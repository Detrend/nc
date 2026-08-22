# graphics-03: light_test is a null global -- enabling the light_debug cvar crashes instantly

**Source:** code_review_findings_graphics.txt, finding 3
**File:** source/nuclidean/engine/graphics/graphics_system.cpp (`GraphicsSystem::handle_light_debug`)

## Issue
```
PointLight* light_test;              // never assigned anywhere
void GraphicsSystem::handle_light_debug()
{ ... vec3 pos = light_test->get_position(); ... }
```
Grepped the whole codebase and confirmed there is no assignment to
`light_test` anywhere. Flipping the `light_debug` cvar in the debug window
dereferences a null pointer on the very next frame the window is drawn
(debug-build-only code path, but a straightforward crash the moment
someone tries to use this feature).

## Fix
Added a null check right after `ImGui::Begin`: if `light_test` is null,
shows a "No light selected for debugging." message and returns (calling
`ImGui::End()` itself first, to keep the Begin/End pairing correct, since
the early return skips the function's original trailing `ImGui::End()`).
This is the minimal fix the review itself suggested (`if (!light_test)
return;`) -- actually picking a light from the registry to debug by
default would need a policy decision (which light? first found? closest to
the player?) so that part is left alone.
