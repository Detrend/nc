// Project Nucledian Source File
#pragma once

#include <types.h>

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

}
