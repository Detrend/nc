#pragma once

#include<types.h>
#include<string>
#include<string_view>
#include<array>
#include<exception>


namespace nc {



/// <summary>
/// String of <= 12 characters in range /_a-z0-9/ (alphanumeric and '_', case-insensitive) compressed into a single 64-bit number.
/// Very fast to pass around and compare against each other.
/// 
/// All of its operations are supposed to be constexpr-friendly.
/// </summary>
struct Token {

private:

  static constexpr u64 ALPHA_START = 1; // Start from 1 so that getting at 0 code can mean end of string
  static constexpr u64 NUMBERS_START = ALPHA_START + 'z' - 'a' + 1;
  static constexpr u64 SPECIAL_START = NUMBERS_START + ('9' - '0') + 1; // only 1 special character that we support is '_'

  static constexpr u64 MAX_CHAR_CODE = SPECIAL_START;
  static constexpr u64 BASE = MAX_CHAR_CODE + 1; // We interpret the u64 backing field as a string of digits encoded in this base


  static constexpr u64 char_to_code(const char c)
  {
    if ('a' <= c && c <= 'z') return ALPHA_START + (c - 'a');
    if ('A' <= c && c <= 'Z') return ALPHA_START + (c - 'A');
    if ('0' <= c && c <= '9') return NUMBERS_START + (c - '0');
    if (c == '_') return SPECIAL_START;
    throw std::exception("Unsupported token character!"); //ideally this would be an assert, but those seem to behave problematically in constexpr context
  }

  static constexpr char code_to_char(const u64 t)
  {
    if (ALPHA_START <= t && t < NUMBERS_START) return static_cast<char>(t - ALPHA_START + 'a');
    if (NUMBERS_START <= t && t < SPECIAL_START) return static_cast<char>(t - NUMBERS_START + '0');
    if (SPECIAL_START == t) return '_';
    throw std::exception("Invalid token code!");
  }


public:

  // a single Token can contain at most this many chars
  static constexpr size_t MAX_LENGTH = 12;


  constexpr Token() : raw(0) {}


  constexpr Token(const std::string_view& str)
    : raw(0)
  {
    if (str.size() > MAX_LENGTH) throw std::exception("String too big for a token!");

    for (size_t t = str.size(); t-- > 0;) {
      raw = raw * BASE + char_to_code(str[t]);
    }
  }

  constexpr Token(const Token &other) : raw(other.raw){}
  constexpr Token& operator=(const Token& other) 
  {
    this->raw = other.raw;
    return *this;
  }

  constexpr bool operator==(const Token other) const
  {
    return get_raw() == other.get_raw();
  }

  constexpr bool operator!=(const Token other) const
  {
    return get_raw() != other.get_raw();
  }

  constexpr std::string to_string() const
  {
    std::string ret;
    for (u64 t = raw; t > 0; t /= BASE) {
      ret.push_back(code_to_char(t % BASE));
    }
    return ret;
  }


  constexpr auto to_cstring() const
  {
    std::array<char, MAX_LENGTH + 1> ret{};

    size_t idx = 0;
    for (u64 t = raw; t > 0; t /= BASE) {
      ret[idx++] = code_to_char(t % BASE);
    }
    ret[idx] = '\0';

    return ret;
  }

  constexpr u64 get_raw() const
  {
    return raw;
  }



private:
  u64 raw;

};

}