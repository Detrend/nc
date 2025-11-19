// Project Nucledian Source File
#pragma once

#include <math/vector.h>

#include <string>

namespace nc
{

struct Appearance
{
  enum class SpriteMode : u8
  {
    // Texture is the name of the referenced sprite, always facing the camera.
    mono,
    // 8 directional sprite.
    // Names of the sprites are "[sprite]_[d/u/l/r/dl/dr/ul/ur].png"
    // Correct sprite from among the 8 ones is chosen based on the "direction"
    // member of the struct.
    dir8,
  };

  enum class PivotMode : u8
  {
    centered, // the position of the entity is the center of the billboard
    bottom,   // the position is the bottom(y) center(x)
  };

  enum class ScalingMode : u8
  {
    fixed,        // the size remains the same
    texture_size, // the size is multiplied by the texture resolution
  };

  enum class RotationMode : u8
  {
    only_horizontal, // The billboard rotates only around the up/down axis
    full,            // Faces fully to the camera
  };

  // Depending on the sprite mode, the "sprite" field can mean 2 things:
  // mono: It is the name of the texture
  // dir8: It is a prefix of the texture onto which a directional suffix is
  //       appended which is calculated from the "direction" field and rotation
  //       of the camera.
  std::string  sprite;
  vec3         direction    = VEC3_ZERO;
  f32          scale        = 1.0f;
  SpriteMode   mode     : 1 = SpriteMode::mono;
  PivotMode    pivot    : 1 = PivotMode::centered;
  ScalingMode  scaling  : 1 = ScalingMode::texture_size;
  RotationMode rotation : 1 = RotationMode::only_horizontal;
};

}
