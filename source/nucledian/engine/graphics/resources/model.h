#pragma once

#include <engine/graphics/resources/mesh.h>
#include <engine/graphics/resources/material.h>
#include <engine/graphics/resources/res_lifetime.h>

#include <string_view>
#include <unordered_map>

namespace nc
{

// Collection of data needed to render an object.
struct Model
{
  Mesh mesh;
  Material material;
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
  // TODO: register model
  // TODO: get model
  template<ResLifetime lifetime>
  void unload();

  Model& get(const ModelHandle& handle);

private:
  std::vector<Model>& get_storage(ResLifetime lifetime);

  std::vector<Model> m_level_models;
  std::vector<Model> m_game_models;
};

}

#include <engine/graphics/resources/model.inl>