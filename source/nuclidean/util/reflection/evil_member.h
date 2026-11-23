// Project Nuclidean Source File
#pragma once

#include <types.h>
#include <util/reflection/evil_enum.h> // evil::is_identifier_char
#include <util/reflection/struct_to_tie.h>

#include <array>
#include <concepts>    // std::same_as
#include <string_view>
#include <tuple>       // std::get, std::tuple_size_v
#include <type_traits> // std::remove_cvref_t
#include <utility>     // std::index_sequence, std::declval

namespace nc::evil
{

// Never read, only the addresses of its members are used.
template<typename T>
inline const T FAKE_OBJECT{};

template<typename T>
struct MemberPtr
{
  const T* ptr;
};

template<typename T, u64 Idx>
constexpr auto get_member_ptr()
{
  auto& member = std::get<Idx>(struct_to_tie(FAKE_OBJECT<T>));
  return MemberPtr<std::remove_cvref_t<decltype(member)>>{&member};
}

template<auto Ptr>
constexpr std::string_view get_member_ptr_signature()
{
#if defined(_MSC_VER) && !defined(__clang__)
  return __FUNCSIG__;
#else
  return __PRETTY_FUNCTION__;
#endif
}

// The name is the identifier before the last '}':
// MSVC:  ...<struct MemberPtr<float>{const float*:&FAKE_OBJECT<struct Foo>->bar}>(void)
// Clang: ...[Ptr = MemberPtr<float>{&FAKE_OBJECT.bar}]
template<typename T, u64 Idx>
constexpr std::string_view get_member_name()
{
  std::string_view signature = get_member_ptr_signature<get_member_ptr<T, Idx>()>();

  u64 end  = signature.rfind('}');
  u64 from = end;

  while (from >= 1 && is_identifier_char(signature[from-1]))
    from -= 1;

  return signature.substr(from, end - from);
}

struct MemberNameTest
{
  int   number;
  float some_value_2 = 1.0f;
  int*  _Ptr;
};

static_assert(get_member_name<MemberNameTest, 0>() == "number");
static_assert(get_member_name<MemberNameTest, 1>() == "some_value_2");
static_assert(get_member_name<MemberNameTest, 2>() == "_Ptr");

}

namespace nc
{

//==================================================================================================
// 
template<typename T>
concept HasMemberNames = requires { T{}; }
  && !std::same_as<decltype(struct_to_tie(std::declval<T&>())), ErrorType>;

//==================================================================================================
// Names of the members, in the order of their declaration. The indices are the same as in the tuple
// returned by "struct_to_tie".
template<typename T>
  requires HasMemberNames<T>
struct MemberNameTable
{
  static constexpr u64 MEMBER_COUNT = std::tuple_size_v<decltype(struct_to_tie(std::declval<T&>()))>;
  static constexpr u64 INVALID_IDX  = static_cast<u64>(-1);

  using NameList = std::array<std::string_view, MEMBER_COUNT>;

  static constexpr const NameList& get_names()
  {
    return NAMES;
  }

  static constexpr std::string_view get_name(u64 idx)
  {
    return NAMES[idx];
  }

  // Returns INVALID_IDX if there is no such member.
  static constexpr u64 find_index(std::string_view name)
  {
    for (u64 i = 0; i < MEMBER_COUNT; ++i)
    {
      if (NAMES[i] == name)
      {
        return i;
      }
    }

    return INVALID_IDX;
  }

  static constexpr bool name_exists(std::string_view name)
  {
    return find_index(name) != INVALID_IDX;
  }

private:
  template<std::size_t...Indices>
  static constexpr NameList build_names(std::index_sequence<Indices...>)
  {
    return NameList{evil::get_member_name<T, Indices>()...};
  }

  static constexpr NameList NAMES = build_names(std::make_index_sequence<MEMBER_COUNT>{});
};

}
