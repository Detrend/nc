// Project Nucledian Source File
#include <config.h>

#ifdef NC_BENCHMARK
#include <vec.h>
#include <intersect.h>

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
#include <cmath>
#include <immintrin.h>

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

static f32 random_value()
{
  f32 sign  = (std::rand() % 2) == 0 ? 1.0f : -1.0f;
  f32 value = (std::rand() % 4096) / 512.0f;
  return sign * value;
}

template<u64 S, typename RT, RT(*unary_func)(const vec<f32, S>&, const vec<f32, S>&)>
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
      auto result = unary_func(values1[i], values2[i]);
      benchmark::DoNotOptimize(result);
    }
    benchmark::ClobberMemory();
  }

  state.SetItemsProcessed(n * state.iterations());
}

template<u64 S, typename RT, typename AT, RT(*unary_func)(AT, AT, AT, AT)>
static void bm_vectors4(benchmark::State& state)
{
  std::srand(1);

  auto n = state.range(0);

  std::vector<vec<f32, S>> values1;
  std::vector<vec<f32, S>> values2;
  std::vector<vec<f32, S>> values3;
  std::vector<vec<f32, S>> values4;

  for (s64 i = 0; i < n; ++i)
  {
    values1.push_back(random_vector<S>());
    values2.push_back(random_vector<S>());
    values3.push_back(random_vector<S>());
    values4.push_back(random_vector<S>());
  }

  for (auto _ : state)
  {
    for (s64 i = 0; i < n; ++i)
    {
      auto result = unary_func(values1[i], values2[i], values3[i], values4[i]);
      benchmark::DoNotOptimize(result);
    }
    benchmark::ClobberMemory();
  }

  state.SetItemsProcessed(n * state.iterations());
}

//#pragma optimize("", off)
template<typename RT, typename AT, RT(*unary_func)(AT)>
static void bm_single(benchmark::State& state)
{
  std::srand(1);

  auto n = state.range(0);

  std::vector<f32> values;

  for (s64 i = 0; i < n; ++i)
  {
    values.push_back(random_value());
  }

  for (auto _ : state)
  {
    for (s64 i = 0; i < n; ++i)
    {
      auto result = unary_func(values[i]);
      benchmark::DoNotOptimize(result);
    }
    benchmark::ClobberMemory();
  }

  state.SetItemsProcessed(n * state.iterations());
}

constexpr s64 NUM_VALUES = 1<<23;

f32 std_sqrt(f32 number)
{
  return std::sqrt(number);
}

f32 sse_sqrt(f32 number)
{
  sse::reg128 reg = _mm_set1_ps(number);
  reg = _mm_sqrt_ss(reg);
  return reg.f32x;
}

f32 check(f32 number)
{
  auto a = std_sqrt(number);
  auto b = sse_sqrt(number);
  if (a != b)
  {
    volatile int c = static_cast<int>(a + b);
    [[maybe_unused]]auto d = c+1;
  }
  return b;
}

BENCHMARK(bm_single<f32, f32, std_sqrt>)->Arg(NUM_VALUES)->Name("std sqrt");
BENCHMARK(bm_single<f32, f32, sse_sqrt>)->Arg(NUM_VALUES)->Name("sse sqrt");

BENCHMARK(bm_single<f32, f32, sse_sqrt>)->Arg(NUM_VALUES)->Name("sse sqrt");
BENCHMARK(bm_single<f32, f32, std_sqrt>)->Arg(NUM_VALUES)->Name("std sqrt");
BENCHMARK(bm_single<f32, f32, sse_sqrt>)->Arg(NUM_VALUES)->Name("sse sqrt");
BENCHMARK(bm_single<f32, f32, std_sqrt>)->Arg(NUM_VALUES)->Name("std sqrt");

BENCHMARK(bm_single<f32, f32, check>)->Arg(NUM_VALUES)->Name("Check");

//BENCHMARK(bm_vectors4<2, bool, vec2, intersect::sse::point_triangle>)      ->Arg(NUM_VALUES)->Name("(sse)     Point triangle intersection");
//BENCHMARK(bm_vectors4<2, bool, vec2, intersect::point_triangle>)           ->Arg(NUM_VALUES)->Name("(non-sse) Point triangle intersection");

//BENCHMARK(bm_vectors<4, bool, sse::eq>)       ->Arg(NUM_VALUES)->Name("(sse)     vec4 eq");
//BENCHMARK(bm_vectors<4, bool, non_sse::eq>)   ->Arg(NUM_VALUES)->Name("(non-sse) vec4 eq");

//BENCHMARK(bm_vectors<4, vec4, sse::add>)      ->Arg(NUM_VALUES)->Name("(sse)     vec4 add");
//BENCHMARK(bm_vectors<4, vec4, non_sse::add>)  ->Arg(NUM_VALUES)->Name("(non-sse) vec4 add");
//
//BENCHMARK(bm_vectors<4, vec4, sse::sub>)      ->Arg(NUM_VALUES)->Name("(sse)     vec4 sub");
//BENCHMARK(bm_vectors<4, vec4, non_sse::sub>)  ->Arg(NUM_VALUES)->Name("(non-sse) vec4 sub");
//
//BENCHMARK(bm_vectors<4, vec4, sse::mul>)      ->Arg(NUM_VALUES)->Name("(sse)     vec4 mul");
//BENCHMARK(bm_vectors<4, vec4, non_sse::mul>)  ->Arg(NUM_VALUES)->Name("(non-sse) vec4 mul");
//
//BENCHMARK(bm_vectors<4, vec4, sse::div>)      ->Arg(NUM_VALUES)->Name("(sse)     vec4 div");
//BENCHMARK(bm_vectors<4, vec4, non_sse::div>)  ->Arg(NUM_VALUES)->Name("(non-sse) vec4 div");
//
//BENCHMARK(bm_vectors<4, vec4, sse::max>)      ->Arg(NUM_VALUES)->Name("(sse)     vec4 max");
//BENCHMARK(bm_vectors<4, vec4, non_sse::max>)  ->Arg(NUM_VALUES)->Name("(non-sse) vec4 max");
//
//BENCHMARK(bm_vectors<4, vec4, sse::min>)      ->Arg(NUM_VALUES)->Name("(sse)    vec4 min");
//BENCHMARK(bm_vectors<4, vec4, non_sse::min>)  ->Arg(NUM_VALUES)->Name("(non-sse) vec4 min");
//
//BENCHMARK(bm_vectors<4, f32, sse::dot>)       ->Arg(NUM_VALUES)->Name("(sse)    vec4 dot");
//BENCHMARK(bm_vectors<4, f32, non_sse::dot>)   ->Arg(NUM_VALUES)->Name("(non-sse) vec4 dot");
//
//BENCHMARK(bm_vectors<4, vec4, sse::cross>)    ->Arg(NUM_VALUES)->Name("(sse)    vec4 cross");
//BENCHMARK(bm_vectors<4, vec4, non_sse::cross>)->Arg(NUM_VALUES)->Name("(non-sse) vec4 cross");

}


#endif

