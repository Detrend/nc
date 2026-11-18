// Project Nuclidean Source File
/**
 * Place where to put all common macro or template black magic utilities used throught the codebase
 */
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <source_location>

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


  } // namespace meta
} // namespace nc
