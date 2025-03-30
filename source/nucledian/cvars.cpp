// Project Nucledian Source File
#include <cvars.h>

namespace nc
{

//==============================================================================
CVars& CVars::get()
{
  static CVars cvars;
  return cvars;
}

//==============================================================================
CVars::CVarList& CVars::get_cvar_list()
{
  static CVarList list;
  return list;
}

//==============================================================================
CVars::CVarRanges& CVars::get_cvar_ranges()
{
  static CVarRanges ranges;
  return ranges;
}

}

