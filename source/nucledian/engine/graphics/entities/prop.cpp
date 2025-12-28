// Project Nucledian Source File
#include <engine/graphics/prop.h>

#include <engine/entity/entity_type_definitions.h>

namespace nc
{

//==============================================================================
/*static*/ EntityType Prop::get_type_static()
{
  return EntityTypes::prop;
}

//==============================================================================
SectorSnapType Prop::get_snap_type() const
{
  return m_snap;
}

//==============================================================================
Prop::Prop
(
  vec3              pos,
  f32               rad,
  f32               height,
  const Appearance& appear,
  SectorSnapType    snap
)
: Entity(pos, rad, height)
, m_appear(appear)
, m_snap(snap)
{

}

//==============================================================================
const Appearance& Prop::get_appearance() const
{
  return const_cast<Prop*>(this)->get_appearance();
}

//==============================================================================
Appearance& Prop::get_appearance()
{
  return m_appear;
}

}
