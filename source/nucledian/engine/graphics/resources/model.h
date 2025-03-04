#pragma once

#include <types.h>
#include <engine/graphics/resources/mesh.h>
#include <engine/graphics/resources/material.h>
#include <engine/graphics/resources/res_lifetime.h>

#include <vector>
#include <functional>
#include <cstdint>

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

  friend bool operator==(const ModelHandle& lhs, const ModelHandle& rhs);
  friend struct std::hash<ModelHandle>;

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

bool operator==(const ModelHandle& lhs, const ModelHandle& rhs);

class ModelManager
{
public:
  template<ResLifetime lifetime>
  ModelHandle add(const Mesh& mesh, const Material& material);

  template<ResLifetime lifetime>
  void unload();

  Model& get(const ModelHandle& handle);

private:
  std::vector<Model>& get_storage(ResLifetime lifetime);

  template<ResLifetime lifetime>
  std::vector<Model>& get_storage();

  std::vector<Model> m_level_models;
  std::vector<Model> m_game_models;
};

}

namespace std
{
  template<>
  struct hash<nc::ModelHandle>
  {
    size_t operator()(const nc::ModelHandle& handle) const;
  };
}

#include <engine/graphics/resources/model.inl>