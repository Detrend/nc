#include <engine/enemies/enemy.h>
#include <engine/core/engine.h>
#include <engine/player/thing_system.h>
#include <engine/player/player.h>

namespace nc 
{
Enemy::Enemy()
{
  position = vec3(0, 0, 0);
  Init();

  /*collision = true;
  width = 0.5f;
  height = 0.5f;*/
}

Enemy::Enemy(vec3 position)
{
  position = position;
  Init();

  /*collision = true;
  width = 0.5f;
  height = 0.5f;*/
}

void Enemy::Init()
{
  m_entity_index = create_entity(position, 1, colors::GREEN);
}

void Enemy::Update()
{
  vec3 target_pos = get_engine().get_module<ThingSystem>().get_player()->get_position();
  vec3 target_dir = normalize_or_zero(target_pos - position);

  position += target_dir * 1.0f * 0.001f;

  m_position_components[m_entity_index] = position;
}



}


