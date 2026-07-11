// Project Nuclidean Source File
#pragma once

#include <string_view>
#include <type_traits>

namespace nc::evil
{

constexpr std::size_t strlen_compile_time(const char* str)
{
  return str[0] == 0 ? 0 : (1 + strlen_compile_time(str+1));
}

static_assert(strlen_compile_time("hello") == 5);
static_assert(strlen_compile_time("hell")  == 4);

constexpr bool is_identifier_char(char c)
{
  return (c >= 'a' && c <= 'z')
    || (c >= 'A' && c <= 'Z')
    || (c >= '0' && c <= '9')
    || (c == '_');
}

constexpr std::size_t find_enum_name_end_offset(const char* full_name, std::size_t len)
{
  while (!is_identifier_char(full_name[len]) && len >= 1)
    len -= 1;
  return len;
}

constexpr std::size_t find_enum_name_start_offset(const char* full_name, std::size_t len)
{
  std::size_t from = find_enum_name_end_offset(full_name, len);
  while (is_identifier_char(full_name[from]) && from >= 1)
    from -= 1;
  return from+1;
}

template<auto E>
constexpr std::string_view get_enum_item_name()
{
#if defined(_MSC_VER)
  constexpr const char* name = __FUNCSIG__;
  std::size_t len = strlen_compile_time(name) - 7;
#elif defined(__clang__)
  constexpr const char* name = __PRETTY_FUNCTION__;
  std::size_t len = strlen_compile_time(name) - 0;
#endif
  return std::string_view{name + find_enum_name_start_offset(name, len), name + find_enum_name_end_offset(name, len)+1};
}

}

namespace nc
{

//==================================================================================================
template<typename EnumType, u64 Idx>
constexpr void fill_elements_from_this_to_zero(std::array<std::string_view, 256>& output)
{
  output[Idx] = evil::get_enum_item_name<cast<EnumType>(Idx)>();
  if constexpr (Idx != 0)
  {
    fill_elements_from_this_to_zero<EnumType, Idx-1>(output);
  }
}

//==================================================================================================
template<typename EnumType>
constexpr std::array<std::string_view, 256> build_string_table()
{
  std::array<std::string_view, 256> table;
  fill_elements_from_this_to_zero<EnumType, 255>(table);
  return table;
}

//==================================================================================================
template<typename EnumType>
struct EnumNameTable
{
  using BoolList = std::array<bool, 256>;
  using NameList = std::array<std::string_view, 256>;
  using NameMap  = std::map<std::string_view, EnumType>;

  static NameMap build_name_map()
  {
    const auto& names = get_names(); 
    NameMap map;

    for (u64 i = 0; i < 256; ++i)
    {
      if (EnumType eval = cast<EnumType>(i); item_exists(eval))
      {
        map[names[i]] = eval;
      }
    }

    return map;
  }

  static const NameMap& get_name_map()
  {
    static NameMap map = build_name_map();
    return map;
  }

  static const NameList& get_names()
  {
    static NameList names = build_string_table<EnumType>();
    return names;
  }

  static std::string_view get_name_for_value(EnumType value)
  {
    return get_names()[cast<u8>(value)];
  }

  static bool item_exists(EnumType value)
  {
    return get_name_for_value(value)[0] != '0';
  }

  static bool name_exists(std::string_view name)
  {
    return get_name_map().contains(name);
  }

  static EnumType get_value_for_name(std::string_view name)
  {
    nc_assert(name_exists(name));
    return get_name_map().at(name);
  }
};

}
