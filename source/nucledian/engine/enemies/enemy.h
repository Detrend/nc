#pragma once
#include <engine/entities.h>
#include <engine/player/map_object.h>

#include <engine/entity/entity_types.h>


namespace nc
{
  enum EnemyState
  {
    idle = 0,
    chase,
    attack,
    hurt,
    dead,
    count
  };

  class Enemy : public MapObject
  {
  public:
    using Base = MapObject;
    static EntityType get_type_static();

    Enemy(vec3 position, vec3 facing);
    void init();
    void update();
    void get_wish_velocity(f32 delta_seconds);
    void check_for_collision(const MapObject& collider);
    void apply_velocity();
    void damage(int damage);
    void die();
    bool can_attack();
  private:
    u32 m_entity_index = 0; //this will be overwritten anyway
    vec3 velocity;
    vec3 facing;
    EnemyState state = idle;

    int health;
    int maxHealth;

    bool alive = true;

    float attackDelay = 3.0f;
    float timeRemaining = 3.0f;
  };
};