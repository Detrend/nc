// Project Nucledian Source File
#pragma once

#include <types.h>

namespace nc
{

// Deterministic random number generator that samples the values from a
// precomputed distribution.
// Supports 256 distincs values after which it wraps from the start once again.
class Rng
{
public:
  f32  next(f32 min, f32 max); // Calculates next random float
  void seed(u8 new_state);     // Sets a custom seed

  u8 next(); // Calculates next random 8 bit unsigned integer

private:
  u8 m_state = 0;
};

}
