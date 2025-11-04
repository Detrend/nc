// Project Nucledian Source File
#include <game/entity_attachment_manager.h>

#include <config.h>
#include <common.h>

#include <engine/entity/entity_system.h>

#include <stack_vector.h>

#include <algorithm> // std::transform
#include <iterator>  // std::back_inserter

namespace nc
{

//==============================================================================
EntityAttachment::EntityAttachment(EntityRegistry& reg)
: m_registry(reg)
{
  
}

//==============================================================================
void EntityAttachment::on_entity_move(EntityID id, vec3 p, f32, f32)
{
  auto it = m_child_mapping.find(id);
  if (it == m_child_mapping.end())
  {
    return;
  }

  // Move all children
  for (const Attachment& attach : it->second)
  {
    if (attach.type & EntityAttachmentFlags::copy_position)
    {
      Entity* entity = m_registry.get_entity(attach.entity);
      nc_assert(entity);

      entity->set_position(p + UP_DIR * attach.offset);
    }
  }
}

//==============================================================================
void EntityAttachment::on_entity_garbaged(EntityID id)
{
  this->detach_entity(id);

  auto it = m_child_mapping.find(id);
  if (it == m_child_mapping.end())
  {
    return;
  }

  const AttachmentList& attach_list = it->second;
  StackVector<EntityID, 16> kill_list;
  for (const Attachment& attach : attach_list)
  {
    if (attach.type & EntityAttachmentFlags::kill_on_death)
    {
      kill_list.push_back(attach.entity);
    }

    // This will remove it from the child mapping as well. The child mapping is
    // destroyed after the last entity is removed from it.
    this->detach_entity(attach.entity);
  }

  // Now, the child mapping should be dead
  nc_assert(!m_child_mapping.contains(id));

  // Then kill all the entities that should die with us
  for (EntityID to_kill : kill_list)
  {
    m_registry.destroy_entity(to_kill);
  }
}

//==============================================================================
void EntityAttachment::on_entity_destroy(EntityID)
{
  // Do nothing
}

//==============================================================================
void EntityAttachment::on_entity_create(EntityID, vec3, f32, f32)
{
  // Do nothing
}

//==============================================================================
void EntityAttachment::attach_entity
(
  EntityID             child_id,
  EntityID             parent_id,
  EntityAttachmentType attachment_type
)
{
  // Check if the entity does not have a parent already
  nc_assert(this->get_entity_parent(child_id) == INVALID_ENTITY_ID);

  // Check if they both exist
  nc_assert(m_registry.get_entity(child_id)  != nullptr);
  nc_assert(m_registry.get_entity(parent_id) != nullptr);

#ifdef NC_ASSERTS
  // Check if this would not create a cyclic hierarchy
  EntityID parent = this->get_entity_parent(parent_id);
  while (parent != INVALID_ENTITY_ID)
  {
    nc_assert(parent != child_id, "CYCLE!");
    parent = this->get_entity_parent(parent);
  }
#endif

  // Insert into the child list
  AttachmentList& attachments = m_child_mapping[parent_id];
  attachments.push_back(Attachment
  {
    .entity = child_id,
    .offset = 0.0f,
    .type   = attachment_type
  });

  // Insert into the parent list
  m_parent_mapping[child_id] = parent_id;
}

//==============================================================================
void EntityAttachment::detach_entity(EntityID id)
{
  nc_assert(m_registry.get_entity(id) != nullptr, "Entity does not exist.");

  auto it_to_parent_mapping = m_parent_mapping.find(id);
  if (it_to_parent_mapping == m_parent_mapping.end())
  {
    // Not attached
    return;
  }

  EntityID parent = it_to_parent_mapping->second;
  nc_assert(m_child_mapping.contains(parent));

  auto it_to_child_mapping = m_child_mapping.find(parent);
  AttachmentList& list = it_to_child_mapping->second;

  [[maybe_unused]]u64 erased_cnt = std::erase_if
  (
    list,
    [id](const Attachment& attachment)
    {
      return attachment.entity == id;
    }
  );

  nc_assert(erased_cnt == 1);

  m_parent_mapping.erase(it_to_parent_mapping);

  if (!list.size())
  {
    m_child_mapping.erase(it_to_child_mapping);
  }
}

//==============================================================================
void EntityAttachment::change_attachment(EntityID id, EntityAttachmentType type)
{
  auto it = m_parent_mapping.find(id);
  if (it == m_parent_mapping.end())
  {
    return;
  }

  AttachmentList& list = m_child_mapping[it->second];
  for (Attachment& attachment : list)
  {
    if (attachment.entity == id)
    {
      attachment.type = type;
      return;
    }
  }

  nc_assert(false);
}

//==============================================================================
EntityID EntityAttachment::get_entity_parent(EntityID id) const
{
  if (auto it = m_parent_mapping.find(id); it != m_parent_mapping.end())
  {
    return it->second;
  }

  return INVALID_ENTITY_ID;
}

//==============================================================================
const EntityAttachment::AttachmentList* EntityAttachment::get_entity_children
(
  EntityID id
)
const
{
  if (auto it = m_child_mapping.find(id); it != m_child_mapping.end())
  {
    return &it->second;
  }

  return nullptr;
}

}
