// Project Nuclidean Source File
#pragma once

#include <types.h>

#include <concepts>    // std::same_as
#include <cstddef>     // std::size_t
#include <tuple>       // std::tie
#include <type_traits> // std::is_aggregate_v, std::decay_t
#include <utility>     // std::index_sequence

namespace nc
{

// The type returned when conversion to std::tie is impossible
struct ErrorType {};

constexpr u64 STRUCT_TO_TIE_MAX_MEMBERS = 24;

}

namespace nc::detail
{

// Convertible to any type, allows us to deduct from how many items can a struct be initialized
template<typename Owner>
struct AnyMember
{
  template<typename T>
    requires (!std::same_as<T, Owner>)
  constexpr operator T() const;
};

template<typename T, typename Indices>
struct IsInitializableWith;

template<typename T, std::size_t...Indices>
struct IsInitializableWith<T, std::index_sequence<Indices...>>
{
  static constexpr bool value = requires { T{(static_cast<void>(Indices), AnyMember<T>{})...}; };
};

template<typename T, u64 COUNT>
constexpr bool HAS_AT_LEAST_MEMBERS
  = IsInitializableWith<T, std::make_index_sequence<static_cast<std::size_t>(COUNT)>>::value;

template<typename T, u64 COUNT = STRUCT_TO_TIE_MAX_MEMBERS>
constexpr u64 count_struct_members()
{
  if constexpr (COUNT == 0 || HAS_AT_LEAST_MEMBERS<T, COUNT>)
  {
    return COUNT;
  }
  else
  {
    return count_struct_members<T, COUNT-1>();
  }
}

template<typename T>
constexpr u64 get_tie_size()
{
  if constexpr (!std::is_aggregate_v<T> || std::is_array_v<T>)
  {
    return 0;
  }
  else if constexpr (HAS_AT_LEAST_MEMBERS<T, STRUCT_TO_TIE_MAX_MEMBERS + 1>)
  {
    return 0;
  }
  else
  {
    return count_struct_members<T>();
  }
}

}

namespace nc
{

// Converts all struct members into a std::tie.
// Supports structs with up-to 24 members.
template<typename Type>
constexpr auto struct_to_tie(Type& value)
{
  using T = std::decay_t<Type>;

  [[maybe_unused]] constexpr u64 member_count = detail::get_tie_size<T>();

  if constexpr (member_count == 1)
  {
    auto&[a] = value; return std::tie(a);
  }
  else if constexpr (member_count == 2)
  {
    auto&[a, b] = value; return std::tie(a, b);
  }
  else if constexpr (member_count == 3)
  {
    auto&[a, b, c] = value; return std::tie(a, b, c);
  }
  else if constexpr (member_count == 4)
  {
    auto&[a, b, c, d] = value; return std::tie(a, b, c, d);
  }
  else if constexpr (member_count == 5)
  {
    auto&[a, b, c, d, e] = value; return std::tie(a, b, c, d, e);
  }
  else if constexpr (member_count == 6)
  {
    auto&[a, b, c, d, e, f] = value; return std::tie(a, b, c, d, e, f);
  }
  else if constexpr (member_count == 7)
  {
    auto&[a, b, c, d, e, f, g] = value; return std::tie(a, b, c, d, e, f, g);
  }
  else if constexpr (member_count == 8)
  {
    auto&[a, b, c, d, e, f, g, h] = value; return std::tie(a, b, c, d, e, f, g, h);
  }
  else if constexpr (member_count == 9)
  {
    auto&[a, b, c, d, e, f, g, h, i] = value; return std::tie(a, b, c, d, e, f, g, h, i);
  }
  else if constexpr (member_count == 10)
  {
    auto&[a, b, c, d, e, f, g, h, i, j] = value; return std::tie(a, b, c, d, e, f, g, h, i, j);
  }
  else if constexpr (member_count == 11)
  {
    auto&[a, b, c, d, e, f, g, h, i, j, k] = value; return std::tie(a, b, c, d, e, f, g, h, i, j, k);
  }
  else if constexpr (member_count == 12)
  {
    auto&[a, b, c, d, e, f, g, h, i, j, k, l] = value; return std::tie(a, b, c, d, e, f, g, h, i, j, k, l);
  }
  else if constexpr (member_count == 13)
  {
    auto&[a, b, c, d, e, f, g, h, i, j, k, l, m] = value; return std::tie(a, b, c, d, e, f, g, h, i, j, k, l, m);
  }
  else if constexpr (member_count == 14)
  {
    auto&[a, b, c, d, e, f, g, h, i, j, k, l, m, n] = value; return std::tie(a, b, c, d, e, f, g, h, i, j, k, l, m, n);
  }
  else if constexpr (member_count == 15)
  {
    auto&[a, b, c, d, e, f, g, h, i, j, k, l, m, n, o] = value; return std::tie(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o);
  }
  else if constexpr (member_count == 16)
  {
    auto&[a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p] = value; return std::tie(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p);
  }
  else if constexpr (member_count == 17)
  {
    auto&[a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q] = value; return std::tie(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q);
  }
  else if constexpr (member_count == 18)
  {
    auto&[a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r] = value; return std::tie(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r);
  }
  else if constexpr (member_count == 19)
  {
    auto&[a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s] = value; return std::tie(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s);
  }
  else if constexpr (member_count == 20)
  {
    auto&[a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t] = value; return std::tie(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t);
  }
  else if constexpr (member_count == 21)
  {
    auto&[a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t, u] = value; return std::tie(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t, u);
  }
  else if constexpr (member_count == 22)
  {
    auto&[a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t, u, v] = value; return std::tie(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t, u, v);
  }
  else if constexpr (member_count == 23)
  {
    auto&[a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t, u, v, w] = value; return std::tie(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t, u, v, w);
  }
  else if constexpr (member_count == 24)
  {
    auto&[a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t, u, v, w, x] = value; return std::tie(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t, u, v, w, x);
  }
  else
  {
    return ErrorType{};
  }
}

}
