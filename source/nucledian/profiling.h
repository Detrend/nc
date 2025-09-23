// Project Nucledian Source File
#pragma once

#include <config.h>

#ifdef NC_PROFILING

#include <types.h>           // u32, f32
#include <metaprogramming.h> // NC_TOKENJOIN

#include <vector>
#include <map>
#include <chrono>
#include <array>

namespace nc
{

// =============================================================================
// A simple profiler that stores recent delta times of scopes marked by the
// "NC_SCOPE_PROFILER" macro.
// Expect a small performance and memory overhead as the data has to be
// processed and stored.
// =============================================================================
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

#else

// Empty
#define NC_SCOPE_PROFILER(_name)

#endif
