#pragma once

#include <temp_math.h>
#include <engine/graphics/resources/mesh.h>
#include <engine/graphics/resources/material.h>
#include <engine/graphics/resources/res_lifetime.h>

#include <vector>

namespace nc
{

// Collection of data needed to render an object.
struct Model
{
  Material material;
  Mesh mesh;
  mat4 transform;
};

// Light-weight handle around model.
class ModelHandle
{
public:
  friend class ModelManager;

  // Determine if model is valid. This does not consider models deleted by unloading.
  operator bool() const;
  Model& operator*() const;
  Model* operator->() const;

  // Get instance of invalid model handle.
  static ModelHandle invalid();

private:
  static ModelHandle m_invalid;

  ModelHandle() {}
  ModelHandle(u32 model_id, ResLifetime lifetime);

  bool          m_is_valid = false;
  ResLifetime   m_lifetime = ResLifetime::None;
  u32           m_model_id = 0;
};

class ModelManager
{
public:
  template<ResLifetime lifetime>
  ModelHandle add(const Mesh& mesh, const Material& material, const mat4& transform);

  template<ResLifetime lifetime>
  void unload();

  Model& get(const ModelHandle& handle);

private:
  using ModelMap = std::unordered_map<std::string_view, Model>;

  std::vector<Model>& get_storage(ResLifetime lifetime);

  template<ResLifetime lifetime>
  std::vector<Model>& get_storage();

  std::vector<Model> m_level_models;
  std::vector<Model> m_game_models;
};

}

#include <engine/graphics/resources/model.inl>