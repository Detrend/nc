// Project Nucledian Source File
#pragma once

#include <types.h>
#include <config.h>

#include <engine/core/engine.h>

#include <engine/core/engine_module.h>
#include <engine/core/engine_module_id.h>
#include <engine/sound/sound_types.h>

#include <array>
#include <atomic>


namespace nc
{

struct SoundHandle
{
public:
  SoundHandle() = default;

  // Pauses/unpauses the sound
  void set_paused(bool should_be_paused);

  // Stops the sound
  void kill();

  // Changes the volume. Valid values are from 0.0f to 1.0f
  void set_volume(f32 volume01);

  // Checks if the sound is paused
  bool is_paused() const;

  // Returns the volume
  f32  get_volume() const;

  // Checks if the sound handle is valid
  bool is_valid() const;

private:
  using Channel = u8;
  static constexpr Channel INVALID_CHANNEL = static_cast<Channel>(-1);

  friend class SoundSystem;
  SoundHandle(Channel the_channel, u16 the_version);

  Channel channel    = INVALID_CHANNEL;
  u16     generation = 0;
};

// ************************************************************************** //
//                              SOUND SYSTEM                                  //
// ************************************************************************** //
class SoundSystem : public IEngineModule
{
public:
  static SoundSystem&   get();
  static EngineModuleId get_module_id();

  bool init();

  // Sound interface
  void        play_oneshot(SoundID sound);
  SoundHandle play(SoundID sound, f32 volume = 1.0f);
  bool        is_handle_valid(const SoundHandle& handle) const;

  // IEngineModule
  void on_event(ModuleEvent& event) override;

  void set_sound_volume(int step);
  void set_music_volume(int step);

private:
  void terminate();
  void update(f32 delta_seconds);
  void on_channel_finished(int channel);

private:
  struct Helper;
  static constexpr u64 CHANNEL_COUNT = 64;

  using ChannelArray = std::array<u8, CHANNEL_COUNT>;

  f32 soundVoulume = 1;
  f32 musicVolume = 1;

  // This is a queue of channels to free during the next update. The callback
  // that a certain channel stopped playing is usually called from other thread
  // and therefore we have to be careful to avoid race conditions. The queue
  // has two "tracks" that are filled with a list of ids of channels that
  // stopped playing.
  u64          channels_to_free_cnt[2] = {0, 0};
  ChannelArray channels_to_free[2];

  // We want to remain lockless instead of using mutexes and this is our way
  // of doing that.
  std::atomic_bool    some_channel_was_just_stopped = false;
  std::atomic_uint8_t used_track = 0;

  struct ChannelInfo
  {
    bool is_free    = true;
    u16  generation = 0;
  };
  ChannelInfo channels[CHANNEL_COUNT]{};
};

}
