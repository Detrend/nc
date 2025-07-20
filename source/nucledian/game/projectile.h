// Project Nucledian Source File

#include <types.h>
#include <engine/entity/entity.h>

#include <engine/appearance.h>
#include <transform.h>

namespace nc
{

class Projectile : public Entity
{
public:
  static EntityType get_type_static();

  Projectile(vec3 pos, vec3 dir, f32 size, bool player_projectile);

  void update(f32 dt);

  Appearance&       get_appearance();
  const Appearance& get_appearance() const;
  Transform         calc_transform() const;

private:
  bool m_player_authored = false;
  vec3 m_velocity;
	u32  m_hit_cnt_remaining = 5;

  Appearance m_appear;
};

}
