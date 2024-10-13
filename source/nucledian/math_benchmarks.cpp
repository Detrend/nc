// Project Nucledian Source File
#include <config.h>

#ifdef NC_BENCHMARK
#include <vec.h>

namespace nc::sse
{
#include <vector_maths_declarations.h>
}

namespace nc::non_sse
{
#include <vector_maths_declarations.h>
}

#include <benchmark/benchmark.h>

#include <vector>
#include <cstdlib>

namespace nc
{

template<u64 S>
static vec<f32, S> random_vector()
{
  vec<f32, S> result;

  for (u64 i = 0; i < S; ++i)
  {
    f32 sign  = (std::rand() % 2) == 0 ? 1.0f : -1.0f;
    f32 value = (std::rand() % 4096) / 512.0f;
    result[i] = sign * value;
  }

  return result;
}

template<u64 S, typename RT, RT(*binary_func)(const vec<f32, S>&, const vec<f32, S>&)>
static void bm_vectors(benchmark::State& state)
{
  std::srand(1);

  auto n = state.range(0);

  std::vector<vec<f32, S>> values1;
  std::vector<vec<f32, S>> values2;

  for (s64 i = 0; i < n; ++i)
  {
    values1.push_back(random_vector<S>());
    values2.push_back(random_vector<S>());
  }

  for (auto _ : state)
  {
    for (s64 i = 0; i < n; ++i)
    {
      auto result = binary_func(values1[i], values2[i]);
      benchmark::DoNotOptimize(result);
    }
    benchmark::ClobberMemory();
  }

  state.SetItemsProcessed(n * state.iterations());
}

constexpr s64 NUM_VALUES = 1<<17;

BENCHMARK(bm_vectors<4, bool, sse::eq>)       ->Arg(NUM_VALUES)->Name("(sse)     vec4 eq");
BENCHMARK(bm_vectors<4, bool, non_sse::eq>)   ->Arg(NUM_VALUES)->Name("(non-sse) vec4 eq");

BENCHMARK(bm_vectors<4, vec4, sse::add>)      ->Arg(NUM_VALUES)->Name("(sse)     vec4 add");
BENCHMARK(bm_vectors<4, vec4, non_sse::add>)  ->Arg(NUM_VALUES)->Name("(non-sse) vec4 add");

BENCHMARK(bm_vectors<4, vec4, sse::sub>)      ->Arg(NUM_VALUES)->Name("(sse)     vec4 sub");
BENCHMARK(bm_vectors<4, vec4, non_sse::sub>)  ->Arg(NUM_VALUES)->Name("(non-sse) vec4 sub");

BENCHMARK(bm_vectors<4, vec4, sse::mul>)      ->Arg(NUM_VALUES)->Name("(sse)     vec4 mul");
BENCHMARK(bm_vectors<4, vec4, non_sse::mul>)  ->Arg(NUM_VALUES)->Name("(non-sse) vec4 mul");

BENCHMARK(bm_vectors<4, vec4, sse::div>)      ->Arg(NUM_VALUES)->Name("(sse)     vec4 div");
BENCHMARK(bm_vectors<4, vec4, non_sse::div>)  ->Arg(NUM_VALUES)->Name("(non-sse) vec4 div");

BENCHMARK(bm_vectors<4, vec4, sse::max>)      ->Arg(NUM_VALUES)->Name("(sse)     vec4 max");
BENCHMARK(bm_vectors<4, vec4, non_sse::max>)  ->Arg(NUM_VALUES)->Name("(non-sse) vec4 max");

BENCHMARK(bm_vectors<4, vec4, sse::min>)      ->Arg(NUM_VALUES)->Name("(sse)    vec4 min");
BENCHMARK(bm_vectors<4, vec4, non_sse::min>)  ->Arg(NUM_VALUES)->Name("(non-sse) vec4 min");

BENCHMARK(bm_vectors<4, f32, sse::dot>)       ->Arg(NUM_VALUES)->Name("(sse)    vec4 dot");
BENCHMARK(bm_vectors<4, f32, non_sse::dot>)   ->Arg(NUM_VALUES)->Name("(non-sse) vec4 dot");

BENCHMARK(bm_vectors<4, vec4, sse::cross>)    ->Arg(NUM_VALUES)->Name("(sse)    vec4 cross");
BENCHMARK(bm_vectors<4, vec4, non_sse::cross>)->Arg(NUM_VALUES)->Name("(non-sse) vec4 cross");

}


#endif

