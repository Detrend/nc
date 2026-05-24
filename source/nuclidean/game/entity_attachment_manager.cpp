// Project Nuclidean Source File
#include <game/entity_attachment_manager.h>

#include <config.h>
#include <common.h>

#include <engine/entity/entity_system.h>

#include <stack_vector.h>
#include <buffer.h>

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
  // Move all children
  for (const Attachment& attach : m_attachments)
  {
    if (attach.parent == id && (attach.type & EntityAttachmentFlags::copy_position))
    {
      Entity* entity = m_registry.get_entity(attach.child);
      nc_assert(entity);

      entity->set_position(p + UP_DIR * attach.offset);
    }
  }
}

//==============================================================================
void EntityAttachment::on_entity_garbaged(EntityID id)
{
  this->detach_entity(id);

  StackVector<EntityID, 16> kill_list;

  // Remove 
  std::erase_if(m_attachments, [&](const Attachment& attach)
  {
    if (attach.parent == id)
    {
      if (attach.type & EntityAttachmentFlags::kill_on_death)
      {
        kill_list.push_back(attach.child);
      }

      return true;
    }

    return false;
  });

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

#if NC_ASSERTS
  static u64 max_attachments_for_warning = 32;
  if (m_attachments.size() > max_attachments_for_warning)
  {
    nc_warn("More than {} entity attachments in parallel! Too many attachments can cause performance issues!", max_attachments_for_warning);
    max_attachments_for_warning *= 2; // Double so that we do not emit a warning every frame
  }
#endif

#if NC_ASSERTS
  // Check if this would not create a cyclic hierarchy
  EntityID parent = this->get_entity_parent(parent_id);
  while (parent != INVALID_ENTITY_ID)
  {
    nc_assert(parent != child_id, "CYCLE!");
    parent = this->get_entity_parent(parent);
  }
#endif

  m_attachments.push_back(Attachment
  {
    .parent = parent_id,
    .child  = child_id,
    .type   = attachment_type,
  });
}

//==============================================================================
void EntityAttachment::detach_entity(EntityID id)
{
  std::erase_if(m_attachments, [&](const Attachment& attach)
  {
    return attach.child == id;
  });
}

//==============================================================================
void EntityAttachment::change_attachment(EntityID id, EntityAttachmentType type)
{
  auto it = std::find_if(m_attachments.begin(), m_attachments.end(), [&](const Attachment& attach)
  {
    return attach.child == id;
  });

  if (it != m_attachments.end())
  {
    it->type = type;
  }
}

//==============================================================================
EntityID EntityAttachment::get_entity_parent(EntityID id) const
{
  auto it = std::find_if(m_attachments.begin(), m_attachments.end(), [&](const Attachment& attach)
  {
    return attach.child == id;
  });

  return it == m_attachments.end() ? INVALID_ENTITY_ID : it->parent;
}

//==============================================================================
void EntityAttachment::serialize(Buffer& buffer)
{
  u64 size = m_attachments.size();
  buffer.serialize<u64>(size);

  if (buffer.is_deserializing())
  {
    m_attachments.resize(size);
  }

  buffer.serialize_array<Attachment>(m_attachments.data(), size);
}

}
