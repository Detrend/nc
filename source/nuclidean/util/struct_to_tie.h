// Project Nuclidean Source File
#pragma once

#include <tuple>

namespace nc
{

template<typename T>
concept CanBeBrokenInto_1 = requires(T& t)
{
  [](T& obj) { auto&[a] = obj; }(t);
};

template<typename T>
concept CanBeBrokenInto_2 = requires(T& t)
{
  [](T& obj) { auto&[a, b] = obj; }(t);
};

template<typename T>
concept CanBeBrokenInto_3 = requires(T& t)
{
  [](T& obj) { auto&[a, b, c] = obj; }(t);
};

template<typename T>
concept CanBeBrokenInto_4 = requires(T& t)
{
  [](T& obj) { auto&[a, b, c, d] = obj; }(t);
};

template<typename T>
concept CanBeBrokenInto_5 = requires(T& t)
{
  [](T& obj) { auto&[a, b, c, d, e] = obj; }(t);
};

template<typename T>
concept CanBeBrokenInto_6 = requires(T& t)
{
  [](T& obj) { auto&[a, b, c, d, e, f] = obj; }(t);
};

template<typename T>
concept CanBeBrokenInto_7 = requires(T& t)
{
  [](T& obj) { auto&[a, b, c, d, e, f, g] = obj; }(t);
};

template<typename T>
concept CanBeBrokenInto_8 = requires(T& t)
{
  [](T& obj) { auto&[a, b, c, d, e, f, g, h] = obj; }(t);
};

template<typename T>
concept CanBeBrokenInto_9 = requires(T& t)
{
  [](T& obj) { auto&[a, b, c, d, e, f, g, h, i] = obj; }(t);
};

template<typename T>
concept CanBeBrokenInto_10 = requires(T& t)
{
  [](T& obj) { auto&[a, b, c, d, e, f, g, h, i, j] = obj; }(t);
};

template<typename T>
concept CanBeBrokenInto_11 = requires(T& t)
{
  [](T& obj) { auto&[a, b, c, d, e, f, g, h, i, j, k] = obj; }(t);
};

template<typename T>
concept CanBeBrokenInto_12 = requires(T& t)
{
  [](T& obj) { auto&[a, b, c, d, e, f, g, h, i, j, k, l] = obj; }(t);
};

template<typename T>
concept CanBeBrokenInto_13 = requires(T& t)
{
  [](T& obj) { auto&[a, b, c, d, e, f, g, h, i, j, k, l, m] = obj; }(t);
};

template<typename T>
concept CanBeBrokenInto_14 = requires(T& t)
{
  [](T& obj) { auto&[a, b, c, d, e, f, g, h, i, j, k, l, m, n] = obj; }(t);
};

template<typename T>
concept CanBeBrokenInto_15 = requires(T& t)
{
  [](T& obj) { auto&[a, b, c, d, e, f, g, h, i, j, k, l, m, n, o] = obj; }(t);
};

template<typename T>
concept CanBeBrokenInto_16 = requires(T& t)
{
  [](T& obj) { auto&[a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p] = obj; }(t);
};

template<typename T>
concept CanBeBrokenInto_17 = requires(T& t)
{
  [](T& obj) { auto&[a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q] = obj; }(t);
};

template<typename T>
concept CanBeBrokenInto_18 = requires(T& t)
{
  [](T& obj) { auto&[a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r] = obj; }(t);
};

template<typename T>
concept CanBeBrokenInto_19 = requires(T& t)
{
  [](T& obj) { auto&[a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s] = obj; }(t);
};

template<typename T>
concept CanBeBrokenInto_20 = requires(T& t)
{
  [](T& obj) { auto&[a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t] = obj; }(t);
};

template<typename T>
concept CanBeBrokenInto_21 = requires(T& t)
{
  [](T& obj) { auto&[a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t, u] = obj; }(t);
};

template<typename T>
concept CanBeBrokenInto_22 = requires(T& t)
{
  [](T& obj) { auto&[a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t, u, v] = obj; }(t);
};

template<typename T>
concept CanBeBrokenInto_23 = requires(T& t)
{
  [](T& obj) { auto&[a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t, u, v, w] = obj; }(t);
};

template<typename T>
concept CanBeBrokenInto_24 = requires(T& t)
{
  [](T& obj) { auto&[a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t, u, v, w, x] = obj; }(t);
};

// The type returned when conversion to std::tie is impossible
struct ErrorType {};

// Converts all struct members into a std::tie.
// Supports structs with up-to 24 members.
template<typename Type>
auto struct_to_tie(Type& value)
{
  using T = std::decay_t<Type>;

  if constexpr(CanBeBrokenInto_24<T>)
  {
    auto&[a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t, u, v, w, x] = value; return std::tie(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t, u, v, w, x);
  }
  else if constexpr(CanBeBrokenInto_23<T>)
  {
    auto&[a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t, u, v, w] = value; return std::tie(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t, u, v, w);
  }
  else if constexpr(CanBeBrokenInto_22<T>)
  {
    auto&[a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t, u, v] = value; return std::tie(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t, u, v);
  }
  else if constexpr(CanBeBrokenInto_21<T>)
  {
    auto&[a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t, u] = value; return std::tie(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t, u);
  }
  else if constexpr(CanBeBrokenInto_20<T>)
  {
    auto&[a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t] = value; return std::tie(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t);
  }
  else if constexpr(CanBeBrokenInto_19<T>)
  {
    auto&[a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s] = value; return std::tie(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s);
  }
  else if constexpr(CanBeBrokenInto_18<T>)
  {
    auto&[a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r] = value; return std::tie(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r);
  }
  else if constexpr(CanBeBrokenInto_17<T>)
  {
    auto&[a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q] = value; return std::tie(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q);
  }
  else if constexpr(CanBeBrokenInto_16<T>)
  {
    auto&[a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p] = value; return std::tie(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p);
  }
  else if constexpr(CanBeBrokenInto_15<T>)
  {
    auto&[a, b, c, d, e, f, g, h, i, j, k, l, m, n, o] = value; return std::tie(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o);
  }
  else if constexpr(CanBeBrokenInto_14<T>)
  {
    auto&[a, b, c, d, e, f, g, h, i, j, k, l, m, n] = value; return std::tie(a, b, c, d, e, f, g, h, i, j, k, l, m, n);
  }
  else if constexpr(CanBeBrokenInto_13<T>)
  {
    auto&[a, b, c, d, e, f, g, h, i, j, k, l, m] = value; return std::tie(a, b, c, d, e, f, g, h, i, j, k, l, m);
  }
  else if constexpr(CanBeBrokenInto_12<T>)
  {
    auto&[a, b, c, d, e, f, g, h, i, j, k, l] = value; return std::tie(a, b, c, d, e, f, g, h, i, j, k, l);
  }
  else if constexpr(CanBeBrokenInto_11<T>)
  {
    auto&[a, b, c, d, e, f, g, h, i, j, k] = value; return std::tie(a, b, c, d, e, f, g, h, i, j, k);
  }
  else if constexpr(CanBeBrokenInto_10<T>)
  {
    auto&[a, b, c, d, e, f, g, h, i, j] = value; return std::tie(a, b, c, d, e, f, g, h, i, j);
  }
  else if constexpr(CanBeBrokenInto_9<T>)
  {
    auto&[a, b, c, d, e, f, g, h, i] = value; return std::tie(a, b, c, d, e, f, g, h, i);
  }
  else if constexpr(CanBeBrokenInto_8<T>)
  {
    auto&[a, b, c, d, e, f, g, h] = value; return std::tie(a, b, c, d, e, f, g, h);
  }
  else if constexpr(CanBeBrokenInto_7<T>)
  {
    auto&[a, b, c, d, e, f, g] = value; return std::tie(a, b, c, d, e, f, g);
  }
  else if constexpr(CanBeBrokenInto_6<T>)
  {
    auto&[a, b, c, d, e, f] = value; return std::tie(a, b, c, d, e, f);
  }
  else if constexpr(CanBeBrokenInto_5<T>)
  {
    auto&[a, b, c, d, e] = value; return std::tie(a, b, c, d, e);
  }
  else if constexpr(CanBeBrokenInto_4<T>)
  {
    auto&[a, b, c, d] = value; return std::tie(a, b, c, d);
  }
  else if constexpr(CanBeBrokenInto_3<T>)
  {
    auto&[a, b, c] = value; return std::tie(a, b, c);
  }
  else if constexpr(CanBeBrokenInto_2<T>)
  {
    auto&[a, b] = value; return std::tie(a, b);
  }
  else if constexpr(CanBeBrokenInto_1<T>)
  {
    auto&[a] = value; return std::tie(a);
  }
  else
  {
    return ErrorType{};
  }
}

}
