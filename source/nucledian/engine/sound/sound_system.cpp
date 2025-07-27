// Project Nucledian Source File
#include <common.h>
#include <config.h>
#include <cvars.h>
#include <intersect.h>

#include <math/utils.h>
#include <math/lingebra.h>

#include <engine/sound/sound_system.h>

#include <engine/core/engine.h>
#include <engine/core/engine_module_types.h>
#include <engine/core/module_event.h>

#include <engine/graphics/gizmo.h>
#include <engine/graphics/resources/res_lifetime.h>

#include <engine/input/input_system.h>
#include <engine/map/map_system.h>
#include <engine/entity/entity_system.h>
#include <engine/player/thing_system.h>

#include <glad/glad.h>
#include <SDL2/include/SDL.h>
#include <SDL_mixer/include/SDL_mixer.h>

#include <engine/sound/sound_resources.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <map>
#include <numbers>
#include <set>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>

namespace nc
{

//==============================================================================
Mix_Chunk* SoundSystem::loaded_chunks[TOTAL_SOUND_CNT]{};

//==============================================================================
EngineModuleId SoundSystem::get_module_id()
{
  return EngineModule::sound_system;
}

//==============================================================================
/*static*/ SoundSystem& SoundSystem::get()
{
  return get_engine().get_module<SoundSystem>();
}

//==============================================================================
bool SoundSystem::is_valid(const SoundHandle& handle) const
{
  if (handle.channel < 0)
  {
    return false;
  }

  bool free  = channels[handle.channel].is_free;
  bool okgen = channels[handle.channel].generation == handle.version;

  return !free && okgen;
}

//==============================================================================
struct SoundSystem::Helper
{

//==============================================================================
static void SDLCALL on_channel_finished(int channel)
{
  SoundSystem& self = get_engine().get_module<SoundSystem>();
  self.on_channel_finished(channel);
}

};

//==============================================================================
bool SoundSystem::init()
{
  //Initialize SDL_mixer
  if (Mix_OpenAudio(MIX_DEFAULT_FREQUENCY, MIX_DEFAULT_FORMAT, MIX_DEFAULT_CHANNELS, 2048) < 0)
  {
    nc_expect(false, "SDL_mixer could not initialize! SDL_mixer Error: {0}\n", Mix_GetError());
    return false;
  }

  int ret = Mix_AllocateChannels(CHANNEL_COUNT);
  bool ok = static_cast<u64>(ret) >= CHANNEL_COUNT;

  if (!ok)
  {
    nc_assert(false, "Could not allocate that many channels?");
    nc_log("SDL mixer failed to allocate {} channels", CHANNEL_COUNT);
    return false;
  }

  // Set up the defaults
  channels_to_free[0].fill(SoundHandle::INVALID_CHANNEL);
  channels_to_free[1].fill(SoundHandle::INVALID_CHANNEL);

  // Setup a callback
  Mix_ChannelFinished(SoundSystem::Helper::on_channel_finished);

  // Load sounds into chunks
  for (SoundID id = 0; id < TOTAL_SOUND_CNT; ++id)
  {
    cstr path = SOUND_FILES[id];
    Mix_Chunk* chunk = Mix_LoadWAV(path);
    if (!chunk)
    {
      nc_warn("Failed to load sound \"{}\"", path);
    }

    loaded_chunks[id] = chunk;
  }

  //background_music = Mix_LoadMUS("..\\art\\sounds\\166508__yoyodaman234__concrete-footstep-2.wav");
  //nc_expect(background_music, "Failed to load music file! '{}'", Mix_GetError());
  //
  //Mix_PlayMusic(background_music, -1);

  return true;
}

//==============================================================================
void SoundSystem::on_event(ModuleEvent& event)
{
    switch (event.type)
    {
    case ModuleEventType::game_update:
    {
        this->update(event.update.dt);
        break;
    }

    case ModuleEventType::terminate:
    {
        this->terminate();
        break;
    }
    }
}

//==============================================================================
void SoundSystem::play_oneshot(SoundID sound)
{
  play(sound);
}

//==============================================================================
SoundHandle SoundSystem::play(SoundID sound)
{
  using Channel = SoundHandle::Channel;

  nc_assert(sound < TOTAL_SOUND_CNT);

  Mix_Chunk* chunk = loaded_chunks[sound];
  if (!chunk)
  {
    // Do not assert here, this is a file problem and not a code problem.
    // We do not want to crash the whole game anyone forgets to submit a sound.
    return SoundHandle{SoundHandle::INVALID_CHANNEL, 0};
  }

  // Find free channel
  Channel free_channel_idx = SoundHandle::INVALID_CHANNEL;
  for (u64 i = 0; i < CHANNEL_COUNT; ++i)
  {
    if (channels[i].is_free)
    {
      free_channel_idx = static_cast<Channel>(i);
      break;
    }
  }

  if (free_channel_idx == SoundHandle::INVALID_CHANNEL)
  {
    // No free channel
    return SoundHandle{SoundHandle::INVALID_CHANNEL, 0};
  }

  // First, we have to set up the channel
  channels[free_channel_idx].is_free    = false;
  channels[free_channel_idx].generation += 1;

  // Then play the sound
  [[maybe_unused]] int chosen = Mix_PlayChannel(free_channel_idx, chunk, 0);
  nc_assert(static_cast<Channel>(chosen) == free_channel_idx);

  return SoundHandle{free_channel_idx, channels[free_channel_idx].generation};
}

//==============================================================================
void SoundSystem::terminate()
{
  Mix_Quit();
}

//==============================================================================
void SoundSystem::update([[maybe_unused]] f32 delta_seconds)
{
  bool expected = false;
  while (some_channel_was_just_stopped.compare_exchange_strong(expected, true) == false)
  {
    // Spin and wait until the audio thread does not stop freeing the channels
  }

  // We gained the lock, lets quickly swap the currently used track

  // Swap the tracks
  u8 free_track = used_track;
  used_track = !used_track;

  // And release the lock
  some_channel_was_just_stopped.store(false);

  // Now do the freeing of the free track.
  for (u64 i = 0; i < channels_to_free_cnt[free_track]; ++i)
  {
    channels[channels_to_free[free_track][i]].is_free = true;
    channels_to_free[free_track][i] = SoundHandle::INVALID_CHANNEL; // set an invalid value for a check
  }
  channels_to_free_cnt[free_track] = 0; // reset
}

//==============================================================================
void SoundSystem::on_channel_finished(int channel)
{
  nc_assert(channel >= 0 && channel < CHANNEL_COUNT);

  bool expected = false;
  while (some_channel_was_just_stopped.compare_exchange_strong(expected, true) == false)
  {
    // Spin and wait until it is not true
  }

  // We have the guarantee that to_use does not change under our hands
  u64 idx = channels_to_free_cnt[used_track];

  // Check if it is an invalid value
  nc_assert(channels_to_free[used_track][idx] == SoundHandle::INVALID_CHANNEL);

  channels_to_free[used_track][idx] = static_cast<u8>(channel);
  channels_to_free_cnt[used_track] += 1;

  // Free the lock
  some_channel_was_just_stopped.store(false);
}

//==============================================================================
void SoundHandle::set_paused(bool should_be_paused)
{
  nc_assert(check_is_valid());

  if(should_be_paused)
  {
    Mix_Pause(this->channel);
  }
  else
  {
    Mix_Resume(this->channel);
  }
}

//==============================================================================
void SoundHandle::kill()
{
  nc_assert(check_is_valid());
  Mix_HaltChannel(this->channel);
}

//==============================================================================
static int volume_from01(float volume01) {
    return static_cast<int>(clamp(volume01, 0.0f, 1.0f) * MIX_MAX_VOLUME); 
}
static float volume_to01(int mixer_volume) {
    return clamp(static_cast<float>(mixer_volume) / MIX_MAX_VOLUME, 0.0f, 1.0f);
}

//==============================================================================
void SoundHandle::set_volume(float volume01)
{
  if (check_is_valid())
  {
    [[maybe_unused]] int ret = Mix_Volume(channel, volume_from01(volume01));
    nc_assert(ret >= 0);
  }
}

//==============================================================================
bool SoundHandle::is_paused() const
{
    nc_assert(check_is_valid());
    const int ret = Mix_Paused(this->channel);
    if (ret == 1) return true;
    if (ret == 0) return false;
    nc_assert(false, "invalid paused value '{}'", ret);
    return false;
}

//==============================================================================
bool SoundHandle::is_playing() const
{
  nc_assert(check_is_valid());
  return Mix_Playing(this->channel);
}

//==============================================================================
float SoundHandle::get_volume()
{
  nc_assert(check_is_valid());

  if (check_is_valid())
  {
    const int ret = Mix_Volume(this->channel, -1);
    nc_assert(ret >= 0);
    return volume_to01(ret);
  }

  return 0.0f;
}

//==============================================================================
bool SoundHandle::check_is_valid() const
{
  SoundSystem& ss = get_engine().get_module<SoundSystem>();
  return ss.is_valid(*this);
}

}
