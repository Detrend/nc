#pragma once

#include "sound_system.h"

namespace nc {
	struct SoundResources {
#define SOUND_RESOURCES_PREFIX ".\\art\\sounds\\"

		static inline constexpr SoundResource PlayerFootsteps = SOUND_RESOURCES_PREFIX "166508__yoyodaman234__concrete-footstep-2.wav";

#undef SOUND_RESOURCES_PREFIX
	};
}