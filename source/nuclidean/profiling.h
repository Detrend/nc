// Project Nuclidean Source File
#pragma once

#include <config.h>

#if NC_PROFILING

#include <types.h>           // u32, f32
#include <metaprogramming.h> // NC_TOKENJOIN

#include <vector>
#include <map>
#include <chrono>
#include <array>

namespace nc
{

// Prints the counter data into the specified file. If null then prints to standart output.
void print_runtime_counters(cstr output_path);

// A hierarchical counter that accumulates time spent in a section of a code. Is supposed to work as
// a static variable somewhere in the code or potentially a global one.
struct ProfilingCounter
{
  // We have to store a pointer to the counter that was active before us and track it as a parent.
  // For example, a function A can have a counter and call function B that has another counter
  // inside. Then, the counter inside B will remember the counter in A as a parent. However, if B is
  // then called from C which also has a counter then it has to remember more parents. This is the
  // number of parents we are able to remember.
  static constexpr u64 MAX_PARENT_COUNTERS = 12;

  // Max number of nested counters A->B->C->D->...
  static constexpr u64 NESTED_COUNTER_STACK_SIZE = 16;

  // Each slot tracks the time spent under one distinct parent: parent[i] is the
  // enclosing counter and total_time[i] is the seconds accumulated under it.
  // A slot whose parent is null is unused; a slot whose parent is the counter
  // itself marks a root.
  f64               total_time[MAX_PARENT_COUNTERS]{};
  ProfilingCounter* parent[MAX_PARENT_COUNTERS]    {}; // Set by the scope counter
  cstr              name;

	// Inserts the name of the counter
  ProfilingCounter(cstr label);
};

// Captures the counter at the start of the scope, then measures how much time was spent in the
// scope when it ends and then writes this information into the counter.
struct ScopeProfilingCounter
{
  ScopeProfilingCounter(ProfilingCounter& counter);
  ~ScopeProfilingCounter();

  using ClockType = std::chrono::high_resolution_clock;
  using TimePoint = ClockType::time_point;
  TimePoint         start;
  ProfilingCounter* counter  = nullptr;
  u64               used_idx = 0;
};

// =================================================================================================
// A simple profiler that stores recent delta times of scopes marked by the "NC_SCOPE_PROFILER"
// macro. Expect a performance and memory overhead as the data has to be processed and stored.
// =================================================================================================
class Profiler
{
public:
  // Number of stored samples in the ring buffer.
  static constexpr u64 MAX_SAMPLES = 2048;

  // Returns the profiler singleton instance.
  static Profiler& get();

  template<typename T>
  using RingBuffer = std::array<T, MAX_SAMPLES>;

  struct DataPerScope
  {
    RingBuffer<f32> delta_time;
    RingBuffer<u32> num_calls;
  };
  using ProfilingData = std::map<std::string, DataPerScope, std::less<void>>;

  // Called from the engine on the start of the new frame
  void new_frame(u64 idx, f32 delta);

  // Returns profiling data that can be displayed.
  const ProfilingData& get_profiling_data_for_all_scopes() const;

protected: friend class ScopeProfiler;
  // These two to be called only by the scope profiler
  void push_scope(cstr name);
  void pop_scope();

private:
  struct AccumulatedScopeData
  {
    u32  num_calls        = 0;
    f32  accumulated_time = 0.0f; // in ms
    cstr name             = nullptr;
  };
  using ScopeMap = std::map<u64, AccumulatedScopeData>;

  struct ScopeEntry
  {
    using Clock = std::chrono::high_resolution_clock;
    using Time  = Clock::time_point;

    cstr name;
    Time start_time;
    f32  minus_time;
  };

  ProfilingData           m_data_for_scopes;
  std::vector<ScopeEntry> m_scope_stack;
  ScopeMap                m_scope_map_this_frame;
};

class ScopeProfiler
{
public:
  ScopeProfiler(cstr name);
  ~ScopeProfiler();
};

}

#define NC_SCOPE_PROFILER_IMPL(_name, _line) \
  ::nc::ScopeProfiler NC_TOKENJOIN(NC_TOKENJOIN(__scope_profiler_, _name), _line)(#_name);

#define NC_SCOPE_PROFILER(_name) NC_SCOPE_PROFILER_IMPL(_name, __LINE__)

#define NC_SCOPE_COUNTER(_counter)                                               \
  static ::nc::ProfilingCounter _Counter_ ## _counter (#_counter);               \
  ::nc::ScopeProfilingCounter _ScopeCounter_ ## _counter (_Counter_ ## _counter );

#else

// Empty
#define NC_SCOPE_PROFILER(_name)
#define NC_SCOPE_COUNTER(_counter)

#endif
