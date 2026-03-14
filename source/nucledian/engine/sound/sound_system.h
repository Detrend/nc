// Project Nucledian Source File
#pragma once

#include <types.h>
#include <config.h>

#include <engine/core/engine.h>

#include <engine/core/engine_module.h>
#include <engine/core/engine_module_id.h>
#include <engine/sound/sound_types.h>
#include <engine/sound/sound_handle.h>

#include <token.h>

#include <array>
#include <atomic>

struct Mix_Music;


namespace nc
{

using SoundLayer = u8;
namespace SoundLayers
{
  enum evalue : SoundLayer
  {
    game,
    ui,
    // - //
    count,
    none = count,
  };
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
  void        play_oneshot(SoundID sound, f32 volume = 1.0f, SoundLayer layer = SoundLayers::game);
  SoundHandle play(SoundID sound, f32 volume = 1.0f, bool loop = false, SoundLayer layer = SoundLayers::game);
  bool        is_handle_valid(const SoundHandle& handle) const;
  
  void        play_music(const Token track_name);


  // IEngineModule
  void on_event(ModuleEvent& event) override;

  void set_sound_volume(int step);
  void set_music_volume(int step);

  void process_sdl_event(const SDL_Event& event);

  void enable_layer(SoundLayer layer, bool enable);
  bool is_layer_enabled(SoundLayer layer) const;

private:
  void terminate();
  void update(f32 delta_seconds);
  void on_channel_finished(int channel);

  bool try_init();
  bool terminate_impl();

private:
  struct Helper;
  static constexpr u64 CHANNEL_COUNT = 64;

  using ChannelArray = std::array<u8, CHANNEL_COUNT>;

  // Might not be working after a device gets disconnected
  u32  device_id  = 0; // Internal SDL device ID

  f32 global_sound_volume = 1.0f;
  f32 global_music_volume = 1.0f;

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
  std::atomic_bool    terminated = true;

  // All
  u8 enabled_layers = (1 << SoundLayers::game) | (1 << SoundLayers::ui);

  Token               current_music_name;
  Mix_Music*          current_music_track;

  struct ChannelInfo
  {
    bool       is_free    = true;
    SoundLayer layer      = SoundLayers::none;
    u16        generation = 0;
  };
  ChannelInfo channels[CHANNEL_COUNT]{};
};

}
