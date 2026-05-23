// Project Nuclidean Source File
#include <engine/graphics/entities/sky_box.h>
#include <engine/entity/entity_type_definitions.h>

namespace nc
{

//==============================================================================
EntityType SkyBox::get_type_static()
{
  return EntityTypes::sky_box;
}

//==============================================================================
void SkyBox::init(GLuint in_texture_handle, f32 in_exposure, bool in_use_gamma_correction)
{
  Entity::init(VEC3_ZERO, 0.0f, 0.0f);
  this->texture_handle       = in_texture_handle;
  this->exposure             = in_exposure;
  this->use_gamma_correction = in_use_gamma_correction;
}

//==============================================================================
GLuint SkyBox::get_texture_handle() const
{
  return texture_handle;
}

}