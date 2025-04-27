// Project Nucledian Source File
#pragma once

#include <types.h>
#include <transform.h>
#include <math/vector.h>
#include <engine/graphics/resources/model.h>

#include <vector>

namespace nc
{

// Component which enables entities to be rendered in the scene.
struct Appearance
{
  // Color modulation of the model.
  color4 color;
  Model model;
  // Model transform (applied after the transform of the entity).
  Transform transform;
};

/**
 * Temporary global appearance component array. This array is only temporary until necessary features are implemented.
 * 
 * In future each level sector will store indices of all entities within the sector. Render system will these ids from
 * visible sectors. Entity system will enable to obtain components by id, render system will use it to obtain
 * RenderComponent instances.
 */
inline std::vector<Appearance> g_appearance_components;

/**
 * Temporary global transform array. This array is only temporary until necessary features are implemented.
 */
inline std::vector<Transform> g_transform_components;

/**
 * @brief Creates an entity with cube model. 
 * 
 * Temporary helper function for creating instances of render components within g_appearance_components array. For
 * more information see description of g_appearance_components.
 */
u32 create_entity(const Transform& transform, const color4& color, const Transform& model_transform = Transform());
/**
 * Temporary helper function for creating instances of render components within g_appearance_components array. For
 * more information see description of g_appearance_components.
 */
u32 create_entity(
  const Transform& transform,
  const color4& color,
  const Model& model,
  const Transform& model_transform = Transform()
);

}