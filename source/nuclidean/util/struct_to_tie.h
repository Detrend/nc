// Project Nuclidean Source File
#pragma once

namespace nc
{

template<typename T, typename...Args>
concept IsConstructibleFrom = requires(Args&&...args)
{
  T{args...};
};

// The type returned when conversion to std::tie is impossible
struct ErrorType {};

// Can be constructed from any type
struct AnyType
{
  template<typename T> constexpr operator T() {};
};

// Converts all struct members into a std::tie.
// Supports structs with up-to 16 members.
template<typename Type>
auto struct_to_tie(Type& value)
{
  using AT = AnyType;
  using T  = std::decay_t<Type>;

  if constexpr(IsConstructibleFrom<T, AT, AT, AT, AT, AT, AT, AT, AT, AT, AT, AT, AT, AT, AT, AT, AT>) 
  {
    auto&[a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p] = value; return std::tie(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p);
  }
  else if constexpr(IsConstructibleFrom<T, AT, AT, AT, AT, AT, AT, AT, AT, AT, AT, AT, AT, AT, AT, AT>)
  {
    auto&[a, b, c, d, e, f, g, h, i, j, k, l, m, n, o] = value; return std::tie(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o);
  }
  else if constexpr(IsConstructibleFrom<T, AT, AT, AT, AT, AT, AT, AT, AT, AT, AT, AT, AT, AT, AT>)
  {
    auto&[a, b, c, d, e, f, g, h, i, j, k, l, m, n] = value; return std::tie(a, b, c, d, e, f, g, h, i, j, k, l, m, n);
  }
  else if constexpr(IsConstructibleFrom<T, AT, AT, AT, AT, AT, AT, AT, AT, AT, AT, AT, AT, AT>)
  {
    auto&[a, b, c, d, e, f, g, h, i, j, k, l, m] = value; return std::tie(a, b, c, d, e, f, g, h, i, j, k, l, m);
  }
  else if constexpr(IsConstructibleFrom<T, AT, AT, AT, AT, AT, AT, AT, AT, AT, AT, AT, AT>)
  {
    auto&[a, b, c, d, e, f, g, h, i, j, k, l] = value; return std::tie(a, b, c, d, e, f, g, h, i, j, k, l);
  }
  else if constexpr(IsConstructibleFrom<T, AT, AT, AT, AT, AT, AT, AT, AT, AT, AT, AT>)
  {
    auto&[a, b, c, d, e, f, g, h, i, j, k] = value; return std::tie(a, b, c, d, e, f, g, h, i, j, k);
  }
  else if constexpr(IsConstructibleFrom<T, AT, AT, AT, AT, AT, AT, AT, AT, AT, AT>)
  {
    auto&[a, b, c, d, e, f, g, h, i, j] = value; return std::tie(a, b, c, d, e, f, g, h, i, j);
  }
  else if constexpr(IsConstructibleFrom<T, AT, AT, AT, AT, AT, AT, AT, AT, AT>)
  {
    auto&[a, b, c, d, e, f, g, h, i] = value; return std::tie(a, b, c, d, e, f, g, h, i);
  }
  else if constexpr(IsConstructibleFrom<T, AT, AT, AT, AT, AT, AT, AT, AT>)
  {
    auto&[a, b, c, d, e, f, g, h] = value; return std::tie(a, b, c, d, e, f, g, h);
  }
  else if constexpr(IsConstructibleFrom<T, AT, AT, AT, AT, AT, AT, AT>)
  {
    auto&[a, b, c, d, e, f, g] = value; return std::tie(a, b, c, d, e, f, g);
  }
  else if constexpr(IsConstructibleFrom<T, AT, AT, AT, AT, AT, AT>)
  {
    auto&[a, b, c, d, e, f] = value; return std::tie(a, b, c, d, e, f);
  }
  else if constexpr(IsConstructibleFrom<T, AT, AT, AT, AT, AT>)
  {
    auto&[a, b, c, d, e] = value; return std::tie(a, b, c, d, e);
  }
  else if constexpr(IsConstructibleFrom<T, AT, AT, AT, AT>)
  {
    auto&[a, b, c, d] = value; return std::tie(a, b, c, d);
  }
  else if constexpr(IsConstructibleFrom<T, AT, AT, AT>)
  {
    auto&[a, b, c] = value; return std::tie(a, b, c);
  }
  else if constexpr(IsConstructibleFrom<T, AT, AT>)
  {
    auto&[a, b] = value; return std::tie(a, b);
  }
  else if constexpr(IsConstructibleFrom<T, AT>)
  {
    auto&[a] = value; return std::tie(a);
  }
  else
    return ErrorType{};
}

}
