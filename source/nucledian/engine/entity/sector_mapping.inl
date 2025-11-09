// Project Nucledian Source File
#pragma once

namespace nc
{

//==============================================================================
template<typename EntityT, typename F>
void SectorMapping::for_each_in_sector(SectorID sector, F&& func) const
{
  u64 cnt = this->sectors_to_entities.entities[sector].size();
  for (u64 i = 0; i < cnt; ++i)
  {
    EntityID id = this->sectors_to_entities.entities[sector][i];
    mat4     tr = this->sectors_to_entities.transforms[sector][i];
    if (id.type == EntityT::get_type_static())
    {
      func(id, tr);
    }
  }
}

//==============================================================================
template<typename F>
void SectorMapping::for_each_in_sector(SectorID sector, F&& func) const
{
  u64 cnt = this->sectors_to_entities.entities[sector].size();
  for (u64 i = 0; i < cnt; ++i)
  {
    EntityID id = this->sectors_to_entities.entities[sector][i];
    mat4     tr = this->sectors_to_entities.transforms[sector][i];
    func(id, tr);
  }
}

}
