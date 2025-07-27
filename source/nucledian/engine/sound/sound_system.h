// Project Nucledian Source File
#pragma once

#include <types.h>
#include <config.h>
#include <transform.h>
#include <math/vector.h>

#include <engine/core/engine.h>

#include <engine/core/engine_module.h>
#include <engine/core/engine_module_id.h>
#include <engine/graphics/resources/model.h>
#include <engine/sound/sound_types.h>

#include <array>
#include <unordered_set>
#include <tuple>
#include <atomic>

struct Mix_Music;
struct Mix_Chunk;

namespace nc
{

using SoundResource = cstr;

struct SoundHandle
{
public:
  SoundHandle() = default;

  void set_paused(const bool should_be_paused);
  void kill();
  void set_volume(float volume01);

  bool is_paused() const;
  bool is_playing() const;
  f32  get_volume();

  bool check_is_valid(void) const;

private:
  friend class SoundSystem;
  using Channel = u8;
  static constexpr Channel INVALID_CHANNEL = static_cast<Channel>(-1);

  SoundHandle(Channel the_channel, u16 the_version)
    : channel(the_channel)
    , version(the_version)
  {
  }

  Channel channel = INVALID_CHANNEL;
  u16     version = 0;
};

class SoundSystem : public IEngineModule
{
public:
  static SoundSystem&   get();
  static EngineModuleId get_module_id();

  bool init();


  // Sound interface
  void        play_oneshot(SoundID sound);
  SoundHandle play(SoundID sound);
  bool        is_valid(const SoundHandle& handle) const;

  // IEngineModule
  void on_event(ModuleEvent& event) override;

private:
  void terminate();
  void update(f32 delta_seconds);
  void on_channel_finished(int channel);

private:
  struct Helper;
  static constexpr u64 CHANNEL_COUNT = 64;

  // Maps to sound IDs from the Sounds::evalue enum
  static Mix_Chunk* loaded_chunks[];

  using ChannelArray = std::array<u8, CHANNEL_COUNT>;

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
