// Project Nucledian Source File
#pragma once

#include <types.h>
#include <engine/entity/entity_system_listener.h>

#include <vector>
#include <unordered_map>

namespace nc
{

class EntityRegistry;

using EntityAttachmentType = u8;
namespace EntityAttachmentFlags
{
  enum evalue : EntityAttachmentType
  {
    copy_position = 1 << 0,  // entity moves with it's parent
    kill_on_death = 1 << 1,  // entity dies when the parent dies

    all  = copy_position | kill_on_death,
    none = 0,
  };
};

class EntityAttachment : public IEntityListener
{
public:
  struct Attachment
  {
    EntityID             entity = INVALID_ENTITY_ID;
    f32                  offset = 0.0f;
    EntityAttachmentType type   = EntityAttachmentFlags::none;
  };
  using AttachmentList = std::vector<Attachment>;

  EntityAttachment(EntityRegistry& registry);

  // IEntityListener
  virtual void on_entity_move(EntityID id, vec3 pos, f32 r, f32 h)   override;
  virtual void on_entity_garbaged(EntityID id)                       override;
  virtual void on_entity_destroy(EntityID id)                        override;
  virtual void on_entity_create(EntityID id, vec3 pos, f32 r, f32 h) override;
  //~IEntityListener

  // Attaches entity to another entity.
  void attach_entity
  (
    EntityID             child,
    EntityID             parent,
    EntityAttachmentType type = EntityAttachmentFlags::all
  );

  // Changes the attachment type of the given entity. Does nothing if the entity
  // is not attached.
  void change_attachment(EntityID id, EntityAttachmentType new_type);

  // Detaches entity from its parent.
  void detach_entity(EntityID id);

  EntityID get_entity_parent(EntityID id) const;

  const AttachmentList* get_entity_children(EntityID id) const;

private:
  using ChildMapping  = std::unordered_map<EntityID, AttachmentList>;
  using ParentMapping = std::unordered_map<EntityID, EntityID>;

  EntityRegistry& m_registry;
  ChildMapping    m_child_mapping;
  ParentMapping   m_parent_mapping;
};

}
