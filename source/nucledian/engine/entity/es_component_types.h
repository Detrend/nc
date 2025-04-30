// Project Nucledian Source File
#pragma once

#include <engine/entity/es_types.h>

namespace nc
{
  
namespace Components
{
  enum evalue : ComponentID
  {
    entity_data = 0,
    render,
    physics,
    ai,
    // 
    count,
  };
}

namespace ComponentMaskTypes
{
  enum evalue : ComponentMask
  {
    entity_data = 1 << Components::entity_data,
    render      = 1 << Components::render,
    physics     = 1 << Components::physics,
    ai          = 1 << Components::ai,
  };
}

}

