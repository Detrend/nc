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
