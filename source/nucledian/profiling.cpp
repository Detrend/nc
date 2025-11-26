// Project Nucledian Source File
#include <profiling.h>

#ifdef NC_PROFILING

#include <common.h>
#include <string>
#include <algorithm>
#include <string_view>

namespace nc
{

//==============================================================================
constexpr bool is_power_of_2(u64 num)
{
  return (num & (num - 1)) == 0;
}

//==============================================================================
void Profiler::new_frame(u64 idx, f32 /*dt*/)
{
  static_assert(is_power_of_2(MAX_SAMPLES), "Max samples must be a power of 2!");
  static_assert(MAX_SAMPLES > 0);

  u64 idx_in_ring = idx & (MAX_SAMPLES - 1);

  // Remove the data, but keep the entry so it remains in the plot
  for (auto&[key, data] : m_scope_map_this_frame)
  {
    auto it = m_data_for_scopes.find(std::string_view{data.name});
    if (it == m_data_for_scopes.end())
    {
      auto[new_it, ok] = m_data_for_scopes.insert({data.name, {}});
      nc_assert(ok);
      it = new_it;

      // Reset deltas to 0
      std::fill
      (
        it->second.delta_time.begin(), it->second.delta_time.end(), 0.0f
      );

      // Reset number of calls as well
      std::fill
      (
        it->second.num_calls.begin(), it->second.num_calls.end(), 0_u32
      );
    }

    it->second.delta_time[idx_in_ring] = data.accumulated_time;
    it->second.num_calls[idx_in_ring]  = data.num_calls;
  }

  m_scope_map_this_frame.clear();
}

//==============================================================================
void Profiler::push_scope(cstr name)
{
  m_scope_stack.push_back(ScopeEntry{name, ScopeEntry::Clock::now()});
}

//==============================================================================
void Profiler::pop_scope()
{
  namespace sch = std::chrono;

  nc_assert(m_scope_stack.size());

  const ScopeEntry& top = m_scope_stack.back();
  f32 time = sch::duration_cast<sch::microseconds>
  (
    ScopeEntry::Clock::now() - top.start_time
  ).count() * 0.001f;

  // Update the data
  AccumulatedScopeData& data = m_scope_map_this_frame[recast<u64>(top.name)];
  data.num_calls        += 1;
  data.accumulated_time += time;
  data.name              = top.name;

  m_scope_stack.pop_back();
}

//==============================================================================
const Profiler::ProfilingData& Profiler::get_profiling_data_for_all_scopes() const
{
  return m_data_for_scopes;
}

//==============================================================================
/*static*/ Profiler& Profiler::get()
{
  static Profiler profiler;
  return profiler;
}

//==============================================================================
ScopeProfiler::ScopeProfiler(cstr name)
{
  Profiler::get().push_scope(name);
}

//==============================================================================
ScopeProfiler::~ScopeProfiler()
{
  Profiler::get().pop_scope();
}

}

#endif
