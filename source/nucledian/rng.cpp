// Project Nucledian Source File

#include <rng.h>
#include <common.h>
#include <array>

namespace nc
{

constexpr u64 PRIME             = 281_u64;
constexpr u64 NUM_PRECALCULATED = 256_u64;
using PrecalculatedDist = std::array<f32, NUM_PRECALCULATED>;

//==============================================================================
// Returns a precalculated uniform distribution of the interval [0, 1].
// Each number is represented only once.
constexpr PrecalculatedDist precalculate_distribution()
{
  PrecalculatedDist retvals;
  constexpr f32 DENOM = 1.0f / cast<f32>(NUM_PRECALCULATED-1);

  u64 accum = 0;
  for (u64 i = 0; i < NUM_PRECALCULATED; ++i)
  {
    accum = (accum + PRIME) % NUM_PRECALCULATED;
    retvals[i] = accum * DENOM;
  }

  return retvals;
}

//==============================================================================
constexpr PrecalculatedDist PRECALC_VALUES = precalculate_distribution();

//==============================================================================
f32 Rng::next(f32 min, f32 max)
{
  // Calculate a sample in [0, 1] range.
  // We overflow and then wrap back up again
  f32 sample = PRECALC_VALUES[m_state];
  m_state += 1;

  // And finally do a lerp and return
  return (1.0f - sample) * min + sample * max;
}

//==============================================================================
void Rng::seed(u8 new_seed)
{
  m_state = new_seed;
}

}
