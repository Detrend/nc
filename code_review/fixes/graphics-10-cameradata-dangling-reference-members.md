# graphics-10: CameraData reference members are a dangling-reference footgun

**Source:** code_review_findings_graphics.txt, "Minor notes"
**File:** source/nuclidean/engine/graphics/renderer.h (`Renderer::CameraData`)

## Issue
`CameraData` held `const mat4&`/`const vec3&` members that are bound to
temporaries at several construction sites, e.g. `renderer.cpp:1256`:
`.portal_dest_to_src = camera.portal_dest_to_src * portal.dest_to_src`
(the product is a temporary `mat4`). This is safe today only via
aggregate reference lifetime-extension (a real but easy-to-accidentally-
lose C++ guarantee: it holds for a freshly aggregate-initialized named
local, but not if a `CameraData` is ever copied, or constructed via a
function's return value instead of an in-place aggregate-init).

## Fix
Changed `position`, `view`, and `portal_dest_to_src` from reference members
to value members (`vec3`/`mat4` -- 12/64 bytes, cheap to copy). Every
existing construction site (`renderer.cpp:182`, `1053`, `1251`, `1270`)
uses designated-initializer syntax (`.field = value`) that works
identically whether the field is a reference or a value, so **no call
sites needed to change**.

`vis_tree` (`const VisibilityTree&`) was deliberately left as a reference:
`VisibilityTree` contains `std::vector<SectorFrustum>` and a recursive
`std::vector<VisibilityTree> children` -- storing it by value would
deep-copy the whole visibility subtree at every portal hit, every frame
(portal recursion constructs a new `CameraData` per traversed portal).
Every construction site binds `vis_tree` to an already-existing tree node
owned by the caller of `Renderer::render` (which outlives the whole
render call), not to a temporary, so this one does not have the same
dangling-reference risk as the other three -- changing it to a value would
trade a non-issue for a real performance regression, which is why it
stayed a reference.
