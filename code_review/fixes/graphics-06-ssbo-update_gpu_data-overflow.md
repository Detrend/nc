# graphics-06: SSBOBuffer::update_gpu_data silently drops data past capacity + push_back(T&&) copies instead of moving

**Source:** code_review_findings_graphics.txt, finding 5
**File:** source/nuclidean/engine/graphics/ssbo_buffer.inl

## Issue 1: update_gpu_data has no capacity clamp
`push_back`/`extend` grow the CPU staging vector with no capacity check,
and `update_gpu_data` called `glBufferSubData` with the full staging size
at offset `m_gpu_size`. Once `m_gpu_size + m_buffer.size()` exceeds the
fixed GPU capacity (e.g. more than `MAX_VISIBLE_POINT_LIGHTS` = 1024 light
sightings in a portal-heavy scene, or a map with more than `MAX_SECTORS`
walls), the upload generates `GL_INVALID_VALUE` and is discarded --
lights/walls silently vanish with no assert or log.
`update_gpu_data_with` already clamps correctly; `update_gpu_data` did not.

### Fix
Added the same clamp `update_gpu_data_with` already uses: compute
`num_additions = min(m_capacity - min(m_gpu_size, m_capacity),
m_buffer.size())`, upload only that many, and log via `nc_warn` when
items had to be dropped (previously: nothing). `m_gpu_size`/`m_size` are
now updated by `+= num_additions` instead of unconditionally set to the
old `m_size` (which would have been wrong once clamping can make fewer
items land on the GPU than were pushed).

## Issue 2: push_back(T&&) copies instead of moving
```
size_t SSBOBuffer<T>::push_back(T&& value)
{
  m_buffer.push_back(value);   // value is a named rvalue ref -> an lvalue here
  ...
```
`value` is itself an lvalue expression inside the function body, so this
called the copy overload, not a move.

### Fix
Changed to `m_buffer.push_back(std::move(value));`. Added
`#include <utility>` for `std::move`.

## Not changed (see NEEDS_USER_INPUT.md)
`SSBOBuffer` still has no destructor (the GL buffer created in its
non-default constructor leaks) and a default-constructed instance still
has handle 0 (binding/updating it would silently fail). The review itself
flags why this needs care rather than a quick patch: `renderer.cpp` does
`m_textures_ssbo = SSBOBuffer<TextureGPU>(...)` (assignment, not just
construction), so adding a destructor without also correctly implementing
move semantics (and deciding what copy should mean, if anything) would
double-free/leak. That's a rule-of-five design decision, not a
self-contained bug fix.
