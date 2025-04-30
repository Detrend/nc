// Project Nucledian Source File
#pragma once

#include <engine/entity/es_component_types.h>
#include <engine/entity/es_component.h>

#include <math/vector.h>

namespace nc
{
struct IEntityController;
}

namespace nc
{


struct IRenderIface
{
  virtual void on_render()
  {
    
  }

  RenderComp* get_render()
  {
    return nullptr;
  }

  virtual ~IRenderIface(){};
};

struct RenderComp : ComponentHelper<Components::render, IRenderIface>
{
  u64 num = 0;

  inline const AIComp* get_ai() const
  {
    return static_cast<AIComp*>(pool->get_component_id(parent_id, Components::ai));
  }

  inline u64 get_num() const
  {
    return num;
  }
};

struct PhysIface
{
  virtual void on_collision(const EntityID& /*other*/)
  {
    
  }

  PhysComp* get_physics()
  {
    return nullptr;
  }

  virtual ~PhysIface(){};
};


struct PhysComp : ComponentHelper<Components::physics, PhysIface>
{
  
};

struct AIIface
{
  virtual ~AIIface(){};
};


struct AIComp : ComponentHelper<Components::ai, AIIface>
{
  
};

}

