// Project Nucledian Source File
#pragma once

#include <types.h>
#include <common.h>
#include <math/lingebra.h>

#include <engine/sound/sound_system.h>

#include <SDL_mixer/include/SDL_mixer.h>

namespace nc::sound_helpers
{

//==============================================================================
static int volume_from01(f32 volume01)
{
  return static_cast<int>(clamp(volume01, 0.0f, 1.0f) * MIX_MAX_VOLUME); 
}

//==============================================================================
static f32 volume_to01(int mixer_volume)
{
  return clamp(static_cast<f32>(mixer_volume) / MIX_MAX_VOLUME, 0.0f, 1.0f);
}

}

namespace nc
{

//==============================================================================
SoundHandle::SoundHandle(SoundHandle::Channel ch, u16 gen)
: channel(ch)
, generation(gen)
{}

//==============================================================================
void SoundHandle::set_paused(bool should_be_paused)
{
  nc_assert(is_valid());

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
  nc_assert(is_valid());
  Mix_HaltChannel(this->channel);
}

//==============================================================================
void SoundHandle::set_volume(f32 volume01)
{
  if (is_valid())
  {
    [[maybe_unused]] int ret = Mix_Volume
    (
      channel, sound_helpers::volume_from01(volume01)
    );
    nc_assert(ret >= 0);
  }
}

//==============================================================================
bool SoundHandle::is_paused() const
{
    nc_assert(is_valid());
    const int ret = Mix_Paused(this->channel);
    if (ret == 1) return true;
    if (ret == 0) return false;
    nc_assert(false, "invalid paused value '{}'", ret);
    return false;
}

//==============================================================================
float SoundHandle::get_volume() const
{
  nc_assert(is_valid());

  if (is_valid())
  {
    const int ret = Mix_Volume(this->channel, -1);
    nc_assert(ret >= 0);
    return sound_helpers::volume_to01(ret);
  }

  return 0.0f;
}

//==============================================================================
bool SoundHandle::is_valid() const
{
  SoundSystem& ss = get_engine().get_module<SoundSystem>();
  return ss.is_handle_valid(*this);
}

}
