// Project Nucledian Source File
#pragma once

#include <types.h>

namespace nc
{

using AnimFSMEvent = u8;
namespace AnimFSMEvents
{
  enum evalue : AnimFSMEvent
  {
    trigger,    // We encountered a trigger
    goto_state, // Animation ended and we went from one state to another
  };
}

// Animation state machine. Keeps track of the current state and time.
// Each state has a given length (can be changed) and transitions to the go-to
// state if the time exceeds it's length.
// While the number of states and the go-to states are static, the state lengths
// are dynamic and can be changed. States can have 0 length.
// The states themselves can also contain triggers (can be added and removed on
// the runtime) that have a time associated with them and get triggered when the
// internal time exceeds the trigger-time.
template
<
  u64      NumStates,        // Number of states the state machine has
  auto     GoToStatesArray,  // Array of size NumStates with go-to states
  typename StateType   = u8,
  typename TriggerType = u8
>
class AnimFSM
{
public:
  using State   = u8;
  using Trigger = u8;

  // Requires a start state
  AnimFSM(State start_state);

  // Updates the state machine - moves the time ahead and transitions to other
  // states.
  // Accepts a callback function that gets called on encountering a trigger or
  // if a state ends. The state machine can be modified from within the callback
  // function - calling "set_state" or adding new triggers is ok.
  // The signature of the callback function is:
  //   void(AnimFSMEvents::evalue, Trigger, State).
  template<typename Functor>
  void update(f32 delta, Functor&& func);

  // Returns the current state
  State get_state() const;

  // Returns current time for a state in interval [0, state_len].
  f32 get_time() const;

  // Returns a time relative to the length of a state in interval [0, 1].
  f32 get_time_relative() const;

  // Adds a new trigger onto the given state in the given time point.
  void add_trigger(State to_state, f32 on_time, const Trigger& trigger);

  // Resets the state machine and starts from the given state
  void set_state(State new_state);

  // Sets a length for a current state
  void set_state_length(State the_state, f32 len);

  // Resets lengths of all states to 0
  void clear_lengths();

  // Removes all triggers
  void clear_triggers();

  void clear();

private:
  f32                  state_lengths[NumStates]{}; // Lengths of states
  std::vector<f32>     trigger_times[NumStates]{}; // Timings of triggers
  std::vector<Trigger> trigger_types[NumStates]{}; // Types of triggers
  State                state;                      // The current state
  f32                  time = 0.0f;                // Time within the state
};

}

#include <anim_state_machine.inl>
