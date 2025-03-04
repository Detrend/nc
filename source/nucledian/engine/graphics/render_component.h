#pragma once

#include <types.h>
#include <temp_math.h>
#include <engine/graphics/resources/model.h>

#include <vector>

namespace nc
{

// Component which enables entities to be rendered in the scene.
struct RenderComponent
{
  ModelHandle model_handle;
  f32 scale;
  f32 y_offset;
  color model_color;
};

/**
 * Temporary global render component array. This array is only temporary until neccesary features are implemented.
 * 
 * In future each level sector will store indices of all entities within the secor. Render system will these ids from
 * visible sectors. Entity system will enable to obtain components by id, render system will use it to obtain
 * RenderComponent instances.
 */
inline std::vector<RenderComponent> g_render_components;

/**
 * Temporary helper function for creating instances of render components within g_render_components array. For furher
 * information see description of g_render_components.
 */
u32 create_render_component(f32 scale, const color& color, f32 y_offset = 0.0f);

}