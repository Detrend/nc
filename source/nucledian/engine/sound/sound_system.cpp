// Project Nucledian Source File
#include <common.h>
#include <config.h>
#include <cvars.h>
#include <intersect.h>

#include <engine/sound/sound_system.h>

#include <engine/core/engine.h>
#include <engine/core/engine_module_types.h>
#include <engine/core/module_event.h>

#include <engine/graphics/debug/gizmo.h>
#include <engine/graphics/resources/res_lifetime.h>

#include <engine/input/input_system.h>
#include <engine/map/map_system.h>
#include <engine/entity/entity_system.h>
#include <engine/game/game_system.h>

#include <glad/glad.h>
#include <SDL2/include/SDL.h>
#include <SDL_mixer/include/SDL_mixer.h>

#include <engine/sound/sound_resources.h>

namespace nc
{

//==============================================================================
// Maps to sound IDs from the Sounds::evalue enum to SDL_mixer chunks, which
// store the actual sound data uncompressed.
// This is used internally only by the sound system and no one else. Since I did
// not want to unnecessarily forward declare the "Mix_Chunk" in the header file,
// it ended up here.
static Mix_Chunk* g_loaded_sound_chunks[TOTAL_SOUND_CNT]{};

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
bool SoundSystem::is_handle_valid(const SoundHandle& handle) const
{
  if (handle.channel == SoundHandle::INVALID_CHANNEL)
  {
    return false;
  }

  bool free  = channels[handle.channel].is_free;
  bool okgen = channels[handle.channel].generation == handle.generation;

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
  if (!try_init())
  {
    nc_warn("Failed to initialize sound during startup.");
  }

  // Go on, do not kill the engine. The sound will not be working, but we are ok
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
void SoundSystem::set_sound_volume(int step)
{
  global_sound_volume = (1.0f / 9.0f * step) * (1.0f / 9.0f * step);
}

//==============================================================================
void SoundSystem::set_music_volume(int step)
{
  global_music_volume = (1.0f / 9.0f * step) * (1.0f / 9.0f * step);
  if (!terminated)
  {
    Mix_VolumeMusic((int)(128.0f * global_music_volume));
  }
}

//==============================================================================
void SoundSystem::play_oneshot(SoundID sound, f32 volume, SoundLayer layer)
{
  play(sound, volume, false, layer);
}

//==============================================================================
void SoundSystem::play_music(const Token track_name)
{
  if (terminated)
  {
    return;
  }

  
  if ((track_name == this->current_music_name) && Mix_PlayingMusic()) {
      return;
  }
  current_music_name = track_name;

  Mix_Music* const og_music_track = this->current_music_track;

  const auto path = track_name.to_cstring_enclosed(NC_SOUND_DIRECTORY_CSTR, NC_MUSIC_TYPE);
  this->current_music_track = Mix_LoadMUS(path.data());
  if (this->current_music_track)
  {
    Mix_PlayMusic(this->current_music_track, -1);
    if (og_music_track) {
        Mix_FreeMusic(og_music_track);
    }
  }
  else
  {
    nc_warn("Failed to load music \"{}\"", path.data());
  }
}

//==============================================================================
SoundHandle SoundSystem::play
(
  SoundID sound, f32 volume, bool loop, SoundLayer layer
)
{
  using Channel = SoundHandle::Channel;

  if (terminated)
  {
    return SoundHandle{SoundHandle::INVALID_CHANNEL, 0};
  }

  if (!this->is_layer_enabled(layer))
  {
    return SoundHandle{SoundHandle::INVALID_CHANNEL, 0};
  }

  nc_assert(sound < TOTAL_SOUND_CNT);
  nc_assert(volume >= 0.0f);

  if (volume == 0.0f)
  {
    // Do not play the sound at all if the volume is 0
    return SoundHandle{SoundHandle::INVALID_CHANNEL, 0};
  }

  Mix_Chunk* chunk = g_loaded_sound_chunks[sound];
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
  int do_loop = loop ? -1 : 0;
  [[maybe_unused]] int chosen = Mix_PlayChannel(free_channel_idx, chunk, do_loop);
  nc_assert(static_cast<Channel>(chosen) == free_channel_idx);

  SoundHandle handle{free_channel_idx, channels[free_channel_idx].generation};

  // We have to set the volume each time explicitly because we do not want the
  // sound to have the old volume of the channel.
  handle.set_volume(volume * global_sound_volume);

  return handle;
}

//==============================================================================
void SoundSystem::terminate()
{
  if (!terminated)
  {
    terminate_impl();
  }
}

//==============================================================================
void SoundSystem::update([[maybe_unused]] f32 delta_seconds)
{
  if (terminated)
  {
    return;
  }

  bool expected = false;
  while (!some_channel_was_just_stopped.compare_exchange_strong(expected, true))
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

    // Set an invalid value for a check
    channels_to_free[free_track][i] = SoundHandle::INVALID_CHANNEL; 
  }
  channels_to_free_cnt[free_track] = 0; // reset

  // Handle the game layer
  this->enable_layer(SoundLayers::game, get_engine().is_level_sound_enabled());
}

//==============================================================================
bool SoundSystem::is_layer_enabled(SoundLayer layer) const
{
  return !!(this->enabled_layers & (1 << layer));
}

//==============================================================================
void SoundSystem::enable_layer(SoundLayer layer, bool enable)
{
  if (this->is_layer_enabled(layer) == enable)
  {
    // Do nothing, already set
    return;
  }

  if (!enable)
  {
    // Turn off the sounds already playing in this layer..
    for (u64 i = 0; i < CHANNEL_COUNT; ++i)
    {
      if (this->channels[i].layer == layer && !this->channels[i].is_free)
      {
        // The callback to "on_channel_finished" will clean it up
        Mix_HaltChannel(cast<int>(i));
      }
    }
  }

  if (enable)
  {
    this->enabled_layers |= (1 << layer);
  }
  else
  {
    this->enabled_layers &= ~(1 << layer);
  }
}

//==============================================================================
void SoundSystem::process_sdl_event(const SDL_Event& event)
{
  if (terminated)
  {
    bool added = event.type == SDL_AUDIODEVICEADDED;
    if (added && event.adevice.iscapture == 0)
    {
      // Try init
      if (!try_init())
      {
        nc_log("Failed to initialize sound system after adding a new device.");
      }
    }
  }
  else
  {
    bool removed = event.type == SDL_AUDIODEVICEREMOVED;
    if (removed && !event.adevice.iscapture && event.adevice.which == device_id)
    {
      // Our audio device got removed, terminate
      terminate_impl();
      nc_log("Sound system terminated after removing the device.");

      // And then try to initialize again with another device
      if (!try_init())
      {
        nc_log("Sound system failed to initialize with other device after primary device removal");
      }
    }
  }
}

//==============================================================================
void SoundSystem::on_channel_finished(int channel)
{
  nc_assert(channel >= 0 && channel < cast<int>(CHANNEL_COUNT));

  bool expected = false;
  while (!some_channel_was_just_stopped.compare_exchange_strong(expected, true))
  {
    // Spin and wait until it is not true
  }

  if (terminated)
  {
    // This might rarely happen
    return;
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
bool SoundSystem::try_init()
{
  nc_assert(terminated == true);

  int retval = Nucledian_Mix_OpenAudioDevice
  (
    MIX_DEFAULT_FREQUENCY, MIX_DEFAULT_FORMAT, MIX_DEFAULT_CHANNELS, 2048,
    nullptr, SDL_AUDIO_ALLOW_FREQUENCY_CHANGE | SDL_AUDIO_ALLOW_CHANNELS_CHANGE,
    &device_id
  );

  if (retval < 0)
  {
    device_id = 0;
    nc_warn("SDL_mixer could not initialize! SDL_mixer Error: {0}\n", Mix_GetError());
    return false;
  }

  int ret = Mix_AllocateChannels(CHANNEL_COUNT);
  bool ok = static_cast<u64>(ret) >= CHANNEL_COUNT;

  if (!ok)
  {
    nc_assert(false, "Could not allocate that many channels?");
    nc_warn("SDL mixer failed to allocate {} channels", CHANNEL_COUNT);
    return false;
  }

  terminated = false;

  // Set up the defaults for the freeing tracks
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

    g_loaded_sound_chunks[id] = chunk;
  }

  return true;
}

//==============================================================================
bool SoundSystem::terminate_impl()
{
  nc_assert(terminated == false);

  bool expected = false;
  while (!some_channel_was_just_stopped.compare_exchange_strong(expected, true))
  {
    // Spin and wait until the audio thread does not stop freeing the channels
  }

  // Store terminated
  terminated.store(true);

  // Unregister the callback
  Mix_ChannelFinished(nullptr);

  // Release the lock
  some_channel_was_just_stopped.store(false);

  // Free up the sound chunks
  for (Mix_Chunk*& chunk : g_loaded_sound_chunks)
  {
    Mix_FreeChunk(chunk);
    chunk = nullptr;
  }

  // Quit the mixer
  Mix_Quit();

  return true;
}

}
