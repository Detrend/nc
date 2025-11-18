#include "sky_box.h"

#include <engine/entity/entity_type_definitions.h>

namespace nc
{

//==============================================================================
EntityType SkyBox::get_type_static()
{
  return EntityTypes::sky_box;
}

//==============================================================================
SkyBox::SkyBox(GLuint texture_handle, f32 exposure, bool use_gamma_correction)
: Entity(VEC3_ZERO, 0.0f, 0.0f)
, texture_handle(texture_handle)
, exposure(exposure)
, use_gamma_correction(use_gamma_correction)
{}

//==============================================================================
GLuint SkyBox::get_texture_handle() const
{
  return texture_handle;
}

}