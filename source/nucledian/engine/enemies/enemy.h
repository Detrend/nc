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
  private:
    u32 m_entity_index = -1; //this will be overwritten anyway
  };
};