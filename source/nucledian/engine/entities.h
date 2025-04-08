#pragma once

#include <types.h>
#include <temp_math.h>
#include <engine/graphics/resources/model.h>

#include <vector>

namespace nc
{

// Component which enables entities to be rendered in the scene.
struct Appearance
{
  f32 rotation;
  f32 y_offset;
  Model model;
  vec3 scale;
  color color;
};

// Component holding in-world position of entity.
using Position = vec3;

/**
 * Temporary global appearance component array. This array is only temporary until necessary features are implemented.
 * 
 * In future each level sector will store indices of all entities within the sector. Render system will these ids from
 * visible sectors. Entity system will enable to obtain components by id, render system will use it to obtain
 * RenderComponent instances.
 */
inline std::vector<Appearance> g_appearance_components;

/**
 * Temporary global position array. This array is only temporary until necessary features are implemented.
 */
inline std::vector<vec3> m_position_components;

/**
 * Temporary helper function for creating instances of render components within g_appearance_components array. For
 * more information see description of g_appearance_components.
 */
u32 create_entity(const Position& position, f32 scale, const color& color, f32 rotation = 0.0f, f32 y_offset = 0.0f);

/**
 * Temporary helper function for creating instances of render components within g_appearance_components array. For
 * more information see description of g_appearance_components.
 */
u32 create_entity(
  const Position& position, 
  const vec3& scale, 
  const color& color, 
  f32 rotation = 0.0f, 
  f32 y_offset = 0.0f
);

/**
 * Temporary helper function for creating instances of render components within g_appearance_components array. For
 * more information see description of g_appearance_components.
 */
u32 create_entity(
  const Position& position, 
  const Model& model, 
  const vec3& scale, 
  const color& color, 
  f32 rotation = 0.0f, 
  f32 y_offset = 0.0f
);


}