#pragma once
#include <engine/entities.h>
#include <engine/player/map_object.h>


namespace nc
{
  class Enemy : public MapObject
  {
  public:
    Enemy();
    Enemy(vec3 position);
    void init();
    void update();
    void get_wish_velocity(f32 delta_seconds);
    void check_for_collision(MapObject collider);
    void apply_velocity();
  private:
    u32 m_entity_index = 0; //this will be overwritten anyway
    vec3 velocity;
  };
};