// Project Nucledian Source File
#pragma once

#include <anim_state_machine.h>

#include <common.h>

#include <algorithm> // std::min, std::fill
#include <iterator>  // std::begin, std::end


namespace nc
{

//==============================================================================
template<u64 NS, auto GOTO, typename ST, typename TT>
AnimFSM<NS, GOTO, ST, TT>::AnimFSM(State start_state)
: state(start_state)
{
  
}

//==============================================================================
template<u64 NS, auto GOTO, typename ST, typename TT>
void AnimFSM<NS, GOTO, ST, TT>::add_trigger
(
  State to_state, f32 on_time, const Trigger& trigger
)
{
  nc_assert(to_state < NS);
  nc_assert(this->state_lengths[to_state] >= on_time);
  this->trigger_times[to_state].push_back(on_time);
  this->trigger_types[to_state].push_back(trigger);
}

//==============================================================================
template<u64 NS, auto GOTO, typename ST, typename TT>
template<typename Functor>
void AnimFSM<NS, GOTO, ST, TT>::update(f32 delta, Functor&& func)
{
  while (delta > 0.0f)
  {
    f32 state_len = this->state_lengths[this->state];

    f32 curr_time = this->time;
    f32 next_time = std::min(curr_time + delta, state_len);

    f32 until_state_end = state_len - curr_time;

    // Check all triggers
    for (u64 i = 0; i < this->trigger_times[this->state].size(); ++i)
    {
      f32 t = this->trigger_times[this->state][i];
      if (curr_time < t && next_time >= t)
      {
        // Activate the trigger
        func
        (
          AnimFSMEvents::trigger,
          this->trigger_types[this->state][i],
          this->state
        );
      }
    }

    if (delta >= until_state_end)
    {
      State curr_state = this->state;
      State next_state = cast<State>(GOTO[curr_state]);

      // Only subtract the amount until the end of the state
      delta -= until_state_end;

      // Has to be called before the functor as the functor might want to
      // change the state.
      this->state = next_state;

      func(AnimFSMEvents::goto_state, Trigger{}, curr_state);
      this->time = 0.0f;
    }
    else
    {
      this->time = next_time;
      delta = 0.0f;
    }

    if (this->state_lengths[this->state] == 0.0f)
    {
      break;
    }
  }
}

//==============================================================================
template<u64 NS, auto GOTO, typename ST, typename TT>
AnimFSM<NS, GOTO, ST, TT>::State AnimFSM<NS, GOTO, ST, TT>::get_state() const
{
  return this->state;
}

//==============================================================================
template<u64 NS, auto GOTO, typename ST, typename TT>
f32 AnimFSM<NS, GOTO, ST, TT>::get_time() const
{
  return this->time;
}

//==============================================================================
template<u64 NS, auto GOTO, typename ST, typename TT>
f32 AnimFSM<NS, GOTO, ST, TT>::get_time_relative() const
{
  f32 state_len = this->state_lengths[this->state];
  return state_len ? (this->get_time() / state_len) : 0.0f;
}

//==============================================================================
template<u64 NS, auto GOTO, typename ST, typename TT>
void AnimFSM<NS, GOTO, ST, TT>::set_state(State new_state)
{
  nc_assert(new_state < NS);
  this->state = new_state;
  this->time  = 0.0f;
}

//==============================================================================
template<u64 NS, auto GOTO, typename ST, typename TT>
void AnimFSM<NS, GOTO, ST, TT>::require_state(State new_state)
{
  nc_assert(new_state < NS);
  if (this->state != new_state)
  {
    this->set_state(new_state);
  }
}

//==============================================================================
template<u64 NS, auto GOTO, typename ST, typename TT>
void AnimFSM<NS, GOTO, ST, TT>::set_state_length(State the_state, f32 len)
{
  nc_assert(the_state < NS);
  this->state_lengths[the_state] = len;
}

//==============================================================================
template<u64 NS, auto GOTO, typename ST, typename TT>
void AnimFSM<NS, GOTO, ST, TT>::clear_lengths()
{
  std::fill
  (
    std::begin(this->state_lengths), std::end(this->state_lengths), 0.0f
  );
}

//==============================================================================
template<u64 NS, auto GOTO, typename ST, typename TT>
void AnimFSM<NS, GOTO, ST, TT>::clear_triggers()
{
  for (auto& lil_vec : trigger_times)
  {
    lil_vec.clear();
  }

  for (auto& lil_vec : trigger_types)
  {
    lil_vec.clear();
  }
}

//==============================================================================
template<u64 NS, auto GOTO, typename ST, typename TT>
void AnimFSM<NS, GOTO, ST, TT>::clear()
{
  this->clear_triggers();
  this->clear_lengths();
}

}
