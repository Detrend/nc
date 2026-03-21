# Shader Hot Reload - Implementation Plan

## Overview

Add the ability to hot-reload GLSL shaders at runtime. When a shader source file
changes on disk, it is recompiled in the background; on success the old OpenGL
program handle is replaced transparently; on failure the old shader stays active
and an error is logged. The public API of `Renderer` must not change.

---

## Inventory of files to touch

| File | Change |
|------|--------|
| `source/nucledian/engine/graphics/shaders/*.vert` | Remove `constexpr const char* VERTEX_SOURCE = R"(…)";` wrapper |
| `source/nucledian/engine/graphics/shaders/*.frag` | Remove `constexpr const char* FRAGMENT_SOURCE = R"(…)";` wrapper |
| `source/nucledian/engine/graphics/shaders/*.comp` | Remove `constexpr const char* COMPUTE_SOURCE = R"(…)";` wrapper |
| `source/nucledian/engine/ui/*.vert/.frag` | Same wrapper removal |
| `source/nucledian/engine/graphics/shaders/shaders.h` | Stop `#include`-ing shader files as C headers; instead store file paths |
| `source/nucledian/engine/graphics/resources/shader_program.h/.cpp` | Add file-reading constructor + `try_reload()` method |
| `source/nucledian/engine/graphics/renderer.h` | Remove `const` from shader members; add hot-reload bookkeeping |
| `source/nucledian/engine/graphics/renderer.cpp` | Register shaders + file paths; call hot-reload check in `Renderer::render` |

---

## Step-by-step plan

### Step 1 – Strip the C-string wrappers from all shader source files

Every `.vert`, `.frag`, and `.comp` file currently looks like:

```
constexpr const char* VERTEX_SOURCE = R"(
  <glsl source>
)";
```

Remove the first line (`constexpr const char* … = R"(`) and the closing `)";\`
so the file contains only raw GLSL.  Do this for all shaders under
`shaders/` and `engine/ui/`.

The variable names (`VERTEX_SOURCE`, `FRAGMENT_SOURCE`, `COMPUTE_SOURCE`) will
no longer exist in the files; `shaders.h` currently relies on them via
`#include`—that dependency will be broken and fixed in the next step.

---

### Step 2 – Rework `shaders.h` to hold file paths instead of source strings

Replace each `#include <engine/graphics/shaders/solid.vert>` pair with
`constexpr` path strings, e.g.:

```cpp
namespace solid
{
  inline constexpr const char* VERTEX_FILE   = "shaders/solid.vert";
  inline constexpr const char* FRAGMENT_FILE = "shaders/solid.frag";
  // Uniform<…> constants stay unchanged
}
```

Do the same for every namespace (`billboard`, `gun`, `light`, `pixel_light`,
`sector`, `light_culling`, `ui_button`, `ui_text`, `sky_box`).  Compute shaders
get a single `COMPUTE_FILE` constant instead.

This keeps `shaders.h` as the single source-of-truth for "which files belong to
which program" while decoupling it from compile-time baking.

---

### Step 3 – Add file-reading and reload support to `ShaderProgramHandle`

#### 3a – New constructors / factory functions

Add two static factory methods (or new constructors) that take file paths instead
of source strings:

```cpp
// For vert+frag programs
static ShaderProgramHandle from_files(
    const char* vert_path, const char* frag_path);

// For compute programs
static ShaderProgramHandle from_file(const char* comp_path);
```

Internally these read the files with `std::ifstream`, then call the existing
compilation pipeline.

#### 3b – `try_reload()` method

Add a non-const method:

```cpp
// Returns true if reload succeeded, false on compile/link error (old handle kept).
bool try_reload(std::initializer_list<const char*> source_files);
```

This reads all source files, compiles, links into a *new* `GLuint`, and on
success calls `glDeleteProgram(m_shader_program)` before overwriting the member.
On failure it logs and returns false, leaving `m_shader_program` unchanged.

#### 3c – File-timestamp helper

Add a free helper (can live in an anonymous namespace in `shader_program.cpp`):

```cpp
// Returns the last-write-time of a file, or 0 on error.
std::filesystem::file_time_type get_file_time(const char* path);
```

Uses `std::filesystem::last_write_time` (C++17).

---

### Step 4 – Add hot-reload bookkeeping to `Renderer`

#### 4a – New internal struct

Inside `renderer.h` (private section) add:

```cpp
struct ShaderEntry
{
  ShaderProgramHandle*          handle;      // pointer to the member
  std::vector<std::string>      file_paths;  // all source files for this program
  std::filesystem::file_time_type last_time; // last seen modification time
};
```

#### 4b – Member list and registration helper

Add to `Renderer`:

```cpp
std::vector<ShaderEntry> m_shader_entries;  // not const

void register_shader(ShaderProgramHandle& handle,
                     std::initializer_list<const char*> paths);
```

#### 4c – Remove `const` from shader members

The shader members in `renderer.h` are currently `const ShaderProgramHandle`.
Change them to non-`const` so that `try_reload()` can overwrite the OpenGL
handle in place:

```cpp
ShaderProgramHandle m_solid_material;
ShaderProgramHandle m_billboard_material;
// …etc.
```

The public getter `get_solid_material()` still returns `const ShaderProgramHandle&`,
so external callers are unaffected.

---

### Step 5 – Update `Renderer` constructor

Replace the current inline-source constructor calls:

```cpp
// Before
m_solid_material(shaders::solid::VERTEX_SOURCE, shaders::solid::FRAGMENT_SOURCE)

// After
m_solid_material(ShaderProgramHandle::from_files(
    shaders::solid::VERTEX_FILE, shaders::solid::FRAGMENT_FILE))
```

After construction, call `register_shader` for every program:

```cpp
register_shader(m_solid_material,
    {shaders::solid::VERTEX_FILE, shaders::solid::FRAGMENT_FILE});
register_shader(m_light_culling_shader,
    {shaders::light_culling::COMPUTE_FILE});
// …etc.
```

`register_shader` stores the pointer, path list, and the current file timestamps
into `m_shader_entries`.

---

### Step 6 – Add `check_shader_hot_reload()` and call it from `Renderer::render`

Add a private non-const method:

```cpp
void check_shader_hot_reload();
```

Implementation:

```cpp
void Renderer::check_shader_hot_reload()
{
  for (auto& entry : m_shader_entries)
  {
    // Check whether any file in this entry is newer than recorded
    bool needs_reload = false;
    for (const auto& path : entry.file_paths)
    {
      const auto t = get_file_time(path.c_str());
      if (t > entry.last_time) { needs_reload = true; break; }
    }

    if (!needs_reload) continue;

    // Build initializer_list equivalent for try_reload
    std::vector<const char*> paths;
    for (const auto& p : entry.file_paths) paths.push_back(p.c_str());

    if (entry.handle->try_reload(paths))
    {
      // Update timestamps only on success
      for (const auto& path : entry.file_paths)
        entry.last_time = std::max(entry.last_time, get_file_time(path.c_str()));
      nc_log("Shader reloaded: {}", entry.file_paths[0]);
    }
    else
    {
      nc_log_error("Shader reload failed: {}", entry.file_paths[0]);
    }
  }
}
```

Call it at the **top** of `Renderer::render` (before any draw calls), making
`render` non-const (or keep it `const` and mark `m_shader_entries` as `mutable`
— prefer removing `const` from `render` for clarity).

> Note: `render` is already `const` — it will need to become non-`const`, or
> `m_shader_entries` and the shader members made `mutable`.  Prefer making them
> `mutable` to minimise diff in call-sites.

---

### Step 7 – Shader file path resolution

Shader files currently live inside the source tree. At runtime the executable
needs to find them. Two options:

1. **Relative path from the executable** – copy shaders to a `shaders/` folder
   next to the `.exe` as part of the build/deploy step; paths in `shaders.h`
   are relative (e.g. `"shaders/solid.vert"`).

2. **Source-tree path** – in debug builds only, point directly at the source
   directory (configured via a `#define SHADER_ROOT` set by the build system).

Recommended: use option 1 for all builds (it mirrors what shipping would look
like) and update the project build rules to copy shader files to the output
directory.  The `nucledian.vcxproj` already tracks the shader files—add a
post-build copy step.

---

### Step 8 – Build system update (`nucledian.vcxproj`)

- Shader files no longer need to be compiled into the binary, but they still
  need to be tracked as project items.
- Add a post-build step to copy `source/nucledian/engine/graphics/shaders/*`
  (and ui shaders) to `$(OutDir)shaders/`.
- Remove the include-as-header mechanism (the `.vert`/`.frag` files will no
  longer be `#include`d, so their "Header" item type in the project is fine to
  leave or change to "None").

---

## Summary of new / changed public API

| Symbol | Before | After |
|--------|--------|-------|
| `ShaderProgramHandle(const char* vert, const char* frag)` | takes GLSL source strings | unchanged (kept for any ad-hoc use) |
| `ShaderProgramHandle::from_files(vert_path, frag_path)` | does not exist | new static factory |
| `ShaderProgramHandle::from_file(comp_path)` | does not exist | new static factory |
| `ShaderProgramHandle::try_reload(paths)` | does not exist | new method |
| `Renderer::m_solid_material` (etc.) | `const ShaderProgramHandle` | `mutable ShaderProgramHandle` |
| `Renderer::render(…)` | `const` | stays `const` (members are `mutable`) |
| `shaders::solid::VERTEX_SOURCE` (etc.) | `constexpr const char*` GLSL string | removed |
| `shaders::solid::VERTEX_FILE` (etc.) | does not exist | `constexpr const char*` path |

All other code that calls `m_solid_material.use()` or `set_uniform(…)` is
unaffected.
