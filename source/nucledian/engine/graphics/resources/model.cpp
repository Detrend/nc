#include <engine/graphics/resources/model.h>

#include <common.h>
#include <engine/core/engine.h>
#include <engine/graphics/graphics_system.h>

namespace nc
{

//==============================================================================
bool ModelHandle::is_valid() const
{
  if (m_lifetime == ResLifetime::Game && m_generation != ModelManager::generation)
  {
    return false;
  }

  return m_lifetime != ResLifetime::None;
}

//==============================================================================
ModelHandle::operator bool() const
{
  return this->is_valid();
}

//==============================================================================
Model& ModelHandle::operator*() const
{
  return get_engine().get_module<GraphicsSystem>().get_model_manager().get(*this);
}

//==============================================================================
Model* ModelHandle::operator->() const
{
  return &get_engine().get_module<GraphicsSystem>().get_model_manager().get(*this);
}

//==============================================================================
bool ModelHandle::operator==(const ModelHandle& other)
{
  return m_lifetime == other.m_lifetime && m_model_id == other.m_model_id;
}

//==============================================================================
ModelHandle ModelHandle::invalid()
{
  return ModelHandle();
}

//==============================================================================
ModelHandle::ModelHandle(u32 model_id, ResLifetime lifetime, u16 generation)
  : m_generation(generation), m_lifetime(lifetime), m_model_id(model_id) { }

//==============================================================================
Model& ModelManager::get(const ModelHandle& handle)
{
  NC_ASSERT(handle, "Inavlid handle.");
  NC_ASSERT(handle.m_lifetime == ResLifetime::Level || handle.m_lifetime == ResLifetime::Game, "Inavlid lifetime.");

  auto& storage = get_storage(handle.m_lifetime);
  NC_ASSERT(handle.m_model_id < storage.size(), "Model index greater than model storage size.");

  return storage[handle.m_model_id];
}

//==============================================================================
std::vector<Model>& ModelManager::get_storage(ResLifetime lifetime)
{
  NC_ASSERT(lifetime == ResLifetime::Level || lifetime == ResLifetime::Game, "Invalid lifetime.");

  if (lifetime == ResLifetime::Level)
  {
    return m_level_models;
  }
  else
  {
    return m_game_models;
  }
}

}
