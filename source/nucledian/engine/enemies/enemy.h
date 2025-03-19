#pragma once
#include <engine/entities.h>


namespace nc
{
  class Enemy
  {
  public:
    Enemy();
    Enemy(vec3 position);
    void Init();
    void Update();
  private:
    vec3 m_position;
    u32 m_entity_index = -1; //this will be overwritten anyway
  };
};