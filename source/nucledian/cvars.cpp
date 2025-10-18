// Project Nucledian Source File
#include <cvars.h>

namespace nc
{

//==============================================================================
CVarList& CVars::get_cvar_list()
{
  static CVarList list;
  return list;
}

//==============================================================================
CVarRanges& CVars::get_cvar_ranges()
{
  static CVarRanges ranges;
  return ranges;
}

}

