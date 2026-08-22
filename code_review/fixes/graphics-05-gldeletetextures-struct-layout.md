# graphics-05: glDeleteTextures(4, &bundle.diffuse_handle) relied on struct layout

**Source:** code_review_findings_graphics.txt, finding 6
**File:** source/nuclidean/engine/graphics/resources/texture.cpp (`TextureManager::unload`)

## Issue
`glDeleteTextures(4, &bundle.diffuse_handle)` deleted four textures through
a pointer to the first of four adjacent `TextureAtlasBundle` members. This
only works while `diffuse_handle`/`normal_handle`/`specular_handle`/
`emissive_handle` stay contiguous and in exactly that declaration order --
inserting or reordering a member would turn this into deleting garbage
handles with no compiler diagnostic.

## Fix
Built an explicit local `GLuint handles[4]` array from the four named
members and passed that to `glDeleteTextures` instead. No struct layout
changes needed (didn't touch `TextureAtlasBundle` itself, since its
members are read by name elsewhere in the renderer) -- just removed the
"deletion order tracks declaration order" coupling at this one call site.
