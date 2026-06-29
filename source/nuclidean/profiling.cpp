// Project Nuclidean Source File
#include <profiling.h>

#if NC_PROFILING

#include <common.h>
#include <string>
#include <algorithm>
#include <string_view>
#include <chrono>
#include <fstream>
#include <iterator>
#include <numeric>

namespace nc
{

//==============================================================================
std::vector<ProfilingCounter*>& get_all_profiling_counters()
{
  static std::vector<ProfilingCounter*> counters;
  return counters;
}

//==============================================================================
ProfilingCounter::ProfilingCounter(cstr label)
: name(label)
{
  for (u64 i = 0; i < MAX_PARENT_COUNTERS; ++i)
  {
    total_time[i] = 0.0;
    parent[i]     = nullptr;
  }

  get_all_profiling_counters().push_back(this);
}

//==============================================================================
static ProfilingCounter* counter_stack[ProfilingCounter::NESTED_COUNTER_STACK_SIZE]{};
static u64               counter_stack_it = 0;

//==============================================================================
ScopeProfilingCounter::ScopeProfilingCounter(ProfilingCounter& ref)
: counter(&ref)
, start(ClockType::now())
{
  // The enclosing scope is the parent; the outermost scope is its own parent.
  ProfilingCounter* the_parent = counter_stack_it ? counter_stack[counter_stack_it - 1] : &ref;

  // Find the slot already assigned to this parent, or the first free one, so
  // that all the time spent under the same parent accumulates together. If we
  // run out of slots, lump the overflow into the last one.
  used_idx = 0;
  for (; used_idx < ProfilingCounter::MAX_PARENT_COUNTERS - 1; ++used_idx)
  {
    if (ref.parent[used_idx] == the_parent || ref.parent[used_idx] == nullptr)
    {
      break;
    }
  }
  ref.parent[used_idx] = the_parent;

  counter_stack[counter_stack_it] = &ref;
  counter_stack_it += 1;
}

//==============================================================================
ScopeProfilingCounter::~ScopeProfilingCounter()
{
  auto   now     = std::chrono::high_resolution_clock::now();
  double seconds = std::chrono::duration_cast<std::chrono::microseconds>(now - start).count() / 1'000'000.0;
  counter->total_time[used_idx] += seconds;
  counter_stack_it -= 1;
}


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

//==============================================================================
static f64 counter_total_time(ProfilingCounter* counter)
{
  return std::accumulate(std::begin(counter->total_time), std::end(counter->total_time), 0.0);
}

//==============================================================================
// Prints an aligned table of all runtime counters laid out as a tree following
// their parent links. Because a counter can run under several parents and keeps
// a separate timing for each, it is printed once under every parent it ran in,
// showing the time spent there and that time's share of the parent's total.
// Child rows are indented one level to the right and siblings are sorted
// against each other within their layer.
void print_runtime_counters(cstr output_path)
{
  constexpr u64 INDENT        = 2; // spaces of indentation per tree level
  constexpr s32 TIME_WIDTH    = 12; // width of the "Time (s)" column
  constexpr s32 PERCENT_WIDTH = 11; // width of the "% of parent" column

  // An edge of the call tree: a child counter and the time it spent under the
  // parent it hangs from.
  struct Edge
  {
    ProfilingCounter* counter = nullptr;
    f64               time    = 0.0;
  };

  // Group every counter under each of its parents. A slot whose parent is the
  // counter itself marks a root; a null parent is an unused slot.
  std::map<ProfilingCounter*, std::vector<Edge>> children;
  std::vector<Edge> roots;
  for (ProfilingCounter* counter : get_all_profiling_counters())
  {
    for (u64 i = 0; i < ProfilingCounter::MAX_PARENT_COUNTERS; ++i)
    {
      ProfilingCounter* parent = counter->parent[i];
      if (parent == nullptr)
      {
        continue;
      }
      else if (parent == counter)
      {
        roots.push_back({counter, counter->total_time[i]});
      }
      else
      {
        children[parent].push_back({counter, counter->total_time[i]});
      }
    }
  }

  // Sort siblings against each other so every layer is ordered on its own.
  auto by_time = [](const Edge& a, const Edge& b) { return a.time > b.time; };
  std::sort(roots.begin(), roots.end(), by_time);
  for (auto& entry : children)
  {
    std::sort(entry.second.begin(), entry.second.end(), by_time);
  }

  // A single printed line of the table.
  struct Row
  {
    std::string label;      // name, already indented for its depth
    f64         time;       // seconds spent under the parent
    f64         percentage; // share of the parent's total time
  };

  std::vector<Row> rows;
  u64 name_width = std::string_view{"Counter"}.size();
  std::vector<ProfilingCounter*> path; // counters currently being expanded, to break cycles

  // Emit the row for one counter, then recurse into its (already sorted)
  // children. The denominator for a child's percentage is the parent's total
  // time across all of its own parents.
  auto build = [&](auto&& self, const Edge& edge, f64 parent_total, u64 depth) -> void
  {
    const f64 percentage = parent_total > 0.0 ? 100.0 * edge.time / parent_total : 0.0;
    std::string label = std::string(depth * INDENT, ' ') + edge.counter->name;
    name_width = std::max(name_width, label.size());
    rows.push_back({std::move(label), edge.time, percentage});

    // Stop if this counter is already an ancestor on the current path so that
    // recursive or mutually-recursive counters cannot loop forever.
    if (std::find(path.begin(), path.end(), edge.counter) != path.end())
    {
      return;
    }

    if (auto it = children.find(edge.counter); it != children.end())
    {
      const f64 total = counter_total_time(edge.counter);
      path.push_back(edge.counter);
      for (const Edge& child : it->second)
      {
        self(self, child, total, depth + 1);
      }

      path.pop_back();
    }
  };

  for (const Edge& root : roots)
  {
    build(build, root, root.time, 0);
  }

  // Determine where do we want to output the stuff.
  std::ofstream file_output;
  if (output_path)
  {
    file_output.open(output_path, std::ios_base::out);
  }

  std::ostream& output = (output_path && file_output.is_open()) ? file_output : std::cout;

  u64 table_width = name_width + TIME_WIDTH + PERCENT_WIDTH + 4;

  output << std::string(table_width, '=') << std::endl;
  output << "Total runtime counters" << std::endl;

  // Header row + separator.
  output << std::format("{:<{}}  {:>{}}  {:>{}}",
                           "Counter",     name_width,
                           "Time (s)",    TIME_WIDTH,
                           "% of parent", PERCENT_WIDTH) << std::endl;

  output << std::string(table_width, '-') << std::endl;

  for (const Row& row : rows)
  {
    // The percentage column reserves one character for the trailing '%'.
    output << std::format("{:<{}}  {:>{}.3f}  {:>{}.3f}%",
                             row.label,      name_width,
                             row.time,       TIME_WIDTH,
                             row.percentage, PERCENT_WIDTH - 1) << std::endl;
  }

  output << std::string(table_width, '=') << std::endl;
}

}

#endif
