#pragma once
#include <engine/entity/entity.h>
#include <engine/entity/entity_types.h>
#include <engine/appearance.h>
#include <transform.h>


namespace nc
{
  enum class EnemyState
  {
    idle = 0,
    chase,
    attack,
    hurt,
    dead,
    count
  };

  class Enemy : public Entity
  {
  public:
    using Base = Entity;
    static EntityType get_type_static();

    Enemy(vec3 position, vec3 facing);
    void init();
    void update();

    // Returns points that create a path
    virtual std::vector<vec3> get_path(const MapSectors& map, vec3 start_pos, vec3 endPos);

    void calculate_wish_velocity(f32 delta_seconds);
    void apply_velocity();
    void damage(int damage);
    void die();
    bool can_attack();

    vec3& get_velocity();

    const Appearance& get_appearance() const;
    Transform         calc_transform() const;

  private:
    vec3 velocity;
    vec3 facing;
    EnemyState state = EnemyState::idle;
    Appearance appear;

    int health;
    int maxHealth;

    bool alive = true;

    float attackDelay = 3.0f;
    float timeRemaining = 3.0f;
  };
};