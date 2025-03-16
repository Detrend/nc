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
  Mesh mesh = Mesh::invalid();
  Material material = Material::invalid();
};

// Light-weight handle around model.
class ModelHandle
{
public:
  friend class ModelManager;

  friend struct std::hash<ModelHandle>;
  bool operator==(const ModelHandle& other);

  bool is_valid() const;
  operator bool() const;
  
  Model& operator*() const;
  Model* operator->() const;

  // Get instance of invalid model handle.
  static ModelHandle invalid();

private:
  ModelHandle() {}
  ModelHandle(u32 model_id, ResLifetime lifetime, u16 generation);

  u16           m_generation = 0;
  ResLifetime   m_lifetime   = ResLifetime::None;
  u32           m_model_id   = 0;
};

class ModelManager
{
public:
  friend class ModelHandle;

  template<ResLifetime lifetime>
  ModelHandle add(const Mesh& mesh, const Material& material);

  template<ResLifetime lifetime>
  void unload();

  Model& get(const ModelHandle& handle);

private:
  static inline u16 generation = 0;

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