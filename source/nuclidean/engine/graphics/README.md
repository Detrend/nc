# Graphics System

Graphics system is an engine module responsible for rendering. The module class itself (`GraphicsSystem`) is the central class which connects all rendering components together. Main responsibilities of `GraphicsSystem` include creation of sector meshes (`create_sector_meshes`) and rendering (`render`). `GraphicsSystem` just queries visible sectors, the rendering itself is delegated to `Renderer`.

## Renderer

`Renderer` contains the main rendering logic. Project is using deferred rendering with the following G-buffers:
- **position** - World space position.
- **stitched_position** - Position in the stitched space (more info in light rendering section).
- **normal** - World space normal.
- **stitched_normal** - Normal in the stitched space (more info in light rendering section).
- **albedo**
- **sector** - Sector IDs.
(There are too many G-buffers, some will get merged together in the near future.)

Whole rendering starts by calling `render`. Whole rendering process is divided into 3 passes:
1. **Geometry Pass** (`do_geometry_pass`) - Geometry pass is responsible for rendering data into G-buffers. The rendering order is following:
   1. **render sectors** (`render_sectors`) - Renders geometry of visible level sectors.
   2. **render entities** (`render_entities`) - Renders billboards for visible entities (enemies, props, etc.).
   3. **render portals** (`render_portals`) - More info in portal rendering section.
   4. **render gun** (`render_gun`)
   5. **render sky box** (`render_sky_box`)
2. **Light Culling Pass** (`do_light_culling_pass`) - Light culling pass is a simple culling compute shader. More info in light rendering.
3. **Lighting Pass** (`do_lighting_pass`) - Uses data from G-buffers to render the frame.

### Portal Rendering

Portal rendering is recursive. `render_portals` starts with precomputed sector visibility tree. This tree is traversed and for each visited node `render_portal` is called. Rendering a portal is divided into three steps:
1. `render_portal_to_stencil` - Renders a quad representing the portal to the stencil buffer, this ensures that following steps render only to the portal quad.
2. `render_portal_to_depth`
3. `render_portal_to_color` - This renders geometry from the portal's view. Specifically the following methods are called:
   1. `render_sectors`
   2. `render_entities`
   3. `render_sky_box`

The view matrix is updated as we traverse through the portal tree. To go from source portal to destination portal we multiply following matrices:
1. world space -> source portal’s local space
2. 180° rotation around the up (Y) axis
3. destination portal's local space -> world space

### Light Rendering

#### Light Culling

All culling is done in `light_culling` compute shader. The process is following:
1. Screen is divided into tiles.
2. For each tile the sub-frustum is computed.
3. Each tile stores all lights intersecting with its sub-frustum.
4. Results are stored in `light_indices` and `tile_data` ssbo buffers.

#### Point & Directional Lights

Point lights are collected during *Geometry Pass*. Then during `update_ssbos` (called before *Lighting Pass*) the collected point lights and all directional lights are uploaded to GPU. In `light.frag` (called during *Lighting Pass*) first all directional lights are rendered. Then the list of point lights is obtained and rendered from tile data (computed during *Light Culling Pass*). Phong shading with slight modifications is used for lighting calculations.

##### Stitched Space

From the light rendering perspective portals (partially) don't exist. This allows calculating lights more easily. Stitched space is composed out of all world space sector pieces stitched together as if portals don't exist. So sectors connected through a portal that could be really far from each other in world space are right next to each other in stitched space.

#### Shadows

Before a point light is rendered, `is_in_shadow` is used to check if a pixel is in shadow. The shadow check process is following:
1. Compute ray from pixel's position to light's position.
2. current_sector <- pixel's sector
3. Go through all sector walls
4.     If the ray collides with a wall in 2d (top-down view)
5.         If the wall is not a portal return true, otherwise go to 6.
6. If ray at intersection position collides with sector ceiling or floor return true
7. current_sector <- sector we enter by traversing intersected portal
8. Repeat 3. - 7. until we enter light's sector, then return false. 

## Shaders

(Current approach limits features like syntax highlighting and hot-reload which is why it will be reworked in the near future.) 
All shaders are currently compiled together with C++ as strings. Each shader source is defined inside "header" file. All shader "headers" are then included in `shaders.h` - each has its own namespace that lists all related uniforms. The actual shader program is then represented by `ShaderProgramHandle` class (`resources/shader_program.h`).

## Resources

Rendering system currently has the following types of resources:
- Mesh - Meshes are defined manually in `mesh.cpp`.
- Texture
- Shader Program
- Model - (pair mesh + shader program, mostly obsolete and will be probably dropped in near future)