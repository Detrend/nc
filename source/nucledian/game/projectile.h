// Project Nucledian Source File
#pragma once

#include <types.h>
#include <engine/entity/entity.h>

#include <engine/appearance.h>

#include <game/weapons_types.h>

namespace nc
{

class Projectile : public Entity
{
public:
  static EntityType get_type_static();

  Projectile(vec3 pos, vec3 dir, EntityID author_id, WeaponType proj_type);

  void update(f32 dt);

  Appearance&       get_appearance();
  const Appearance& get_appearance() const;
  EntityID          get_author_id()  const;

private:
  bool on_entity_hit(const struct CollisionHit& hit);

private:
  EntityID m_author = INVALID_ENTITY_ID;
  vec3     m_velocity;
	u32      m_hit_cnt_remaining;

  Appearance m_appear;
  WeaponType m_type;
};

}
