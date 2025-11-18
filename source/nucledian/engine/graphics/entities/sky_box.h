#pragma once

#include <types.h>

#include <engine/entity/entity.h>
#include <engine/entity/entity_types.h>

#include <engine/graphics/gl_types.h>

class EquirectangularMapHandle;

namespace nc
{

class SkyBox : public Entity
{
public:
  using Base = Entity;
  static EntityType get_type_static();

  SkyBox(GLuint texture_handle, f32 exposure = 1.0f, bool use_gamma_correction = false);

  GLuint get_texture_handle() const;

  f32  exposure             = 1.0f;
  bool use_gamma_correction = false;

private:
  GLuint texture_handle;
};

}
