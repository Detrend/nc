// Project Nuclidean Source File
/**
 * Place where to put all common macro or template black magic utilities used throught the codebase
 */
#pragma once

#include <config.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <source_location>
#include <string_view>
#include <utility>

#define NC_INTERNAL_TOKENPASTE2(_a, _b) _a##_b
#define NC_TOKENJOIN(_a, _b) NC_INTERNAL_TOKENPASTE2(_a, _b)
#define NC_TOKENJOIN3(_a, _b, _c) NC_TOKENJOIN(_a, NC_TOKENJOIN(_b, _c))
#define NC_TOKENPASTE(_a) _a



#define STRINGIFY_impl(...) # __VA_ARGS__
#define STRINGIFY(...) STRINGIFY_impl(__VA_ARGS__)

#define ARRAY_LENGTH(...) (sizeof(__VA_ARGS__)/sizeof(*(__VA_ARGS__)))


namespace nc {
  namespace meta {

    consteval std::array<char, 128> source_location_to_string(const std::source_location& loc) {
      std::array<char, 128> result{};
      std::size_t size = 0;
      const auto append = [&result, &size](char value) {
        if (size >= result.size() - 1) {
          throw "source location string exceeds capacity";
        }
        result[size++] = (('a' <= value && value <= 'z') || ('0' <= value && value <= '9') || ('A' <= value && value <= 'Z')) ? value : '_';
      };
      const auto append_number = [&append](std::uint_least32_t value) {
        char digits[std::numeric_limits<std::uint_least32_t>::digits10 + 2]{};
        std::size_t count = 0;
        do {
          digits[count++] = static_cast<char>('0' + (value % 10));
          value /= 10;
        } while (value);
        while (count) {
          append(digits[--count]);
        }
      };

      const char* filename = loc.file_name();
      const char* basename = filename;
      for (const char* character = filename; *character != '\0'; ++character) {
        if (*character == '/' || *character == '\\') {
          basename = character + 1;
        }
      }
      for (const char* character = basename; *character != '\0'; ++character) {
        append(*character);
      }
      append('_');
      append_number(loc.line());
      return result;
    }

    template<auto Value>
    constexpr std::string_view enum_value_name()
    {
#if NC_COMPILER_MSVC
      constexpr std::string_view signature = __FUNCSIG__;
      constexpr std::string_view prefix = "enum_value_name<";
#else
      constexpr std::string_view signature = __PRETTY_FUNCTION__;
      constexpr std::string_view prefix = "Value = ";
#endif

      const std::size_t start = signature.find(prefix) + prefix.size();
      const std::string_view qualified = signature.substr(start, signature.find_first_of("];>", start) - start);
      return qualified.substr(qualified.rfind(':') + 1);
    }

    template<typename Enum, typename Indices = std::make_index_sequence<static_cast<std::size_t>(Enum::count)>>
    inline constexpr auto ENUM_VALUE_ALL_NAMES = nullptr;

    template<typename Enum, std::size_t... Index>
    inline constexpr auto ENUM_VALUE_ALL_NAMES<Enum, std::index_sequence<Index...>> =
      std::array{enum_value_name<static_cast<Enum>(Index)>()...};

    template<typename Enum>
    constexpr std::string_view enum_value_name(Enum value)
    {
      return ENUM_VALUE_ALL_NAMES<Enum>[static_cast<std::size_t>(value)];
    }

    template<typename Enum>
    constexpr bool parse_enum(std::string_view name, Enum& out_value)
    {
      const auto& names = ENUM_VALUE_ALL_NAMES<Enum>;

      for (std::size_t index = 0; index < names.size(); ++index)
      {
        if (names[index] != name)
          continue;

        out_value = static_cast<Enum>(index);
        return true;
      }

      return false;
    }

  } // namespace meta
} // namespace nc
