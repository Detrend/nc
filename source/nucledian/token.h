#pragma once

#include<types.h>
#include<string>
#include<string_view>
#include<array>
#include<exception>

#include "math/utils.h"

namespace nc {



  /// <summary>
  /// String of <= 12 characters in range /_a-z0-9/ (alphanumeric and '_', case-insensitive) compressed into a single 64-bit number.
  /// Very fast to pass around and compare against each other.
  /// 
  /// All of its operations are supposed to be constexpr-friendly.
  /// </summary>
  struct BasicToken {

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

    static constexpr const char PERMITTED_CHARS[] = "abcdefghijklmnopqrstuvwxyz0123456789_";

    // a single BasicToken can contain at most this many chars
    static constexpr size_t MAX_LENGTH = 12;


    constexpr BasicToken() : raw(0) {}


    constexpr BasicToken(const std::string_view& str)
      : raw(0)
    {
      if (str.size() > MAX_LENGTH) throw std::exception("String too big for a token!");

      for (size_t t = str.size(); t-- > 0;) {
        raw = raw * BASE + char_to_code(str[t]);
      }
    }

    constexpr BasicToken(const BasicToken& other) : raw(other.raw) {}
    constexpr BasicToken& operator=(const BasicToken& other)
    {
      this->raw = other.raw;
      return *this;
    }

    constexpr bool operator==(const BasicToken other) const
    {
      return get_raw() == other.get_raw();
    }

    constexpr bool operator!=(const BasicToken other) const
    {
      return get_raw() != other.get_raw();
    }


    constexpr char* write_to_buffer(char* buffer) const
    {
      for (u64 t = raw; t > 0; t /= BASE) {
        *(buffer++) = code_to_char(t % BASE);
      }
      *buffer = '\0';

      return buffer;
    }

    constexpr auto to_cstring() const
    {
      std::array<char, MAX_LENGTH + 1> ret{};

      write_to_buffer(ret.data());

      return ret;
    }

    constexpr std::string to_string() const
    {
      return std::string(to_cstring().data());
    }

    constexpr u64 get_raw() const
    {
      return raw;
    }



  private:
    u64 raw;

  };



  /// <summary>
  /// Fixed-size string of characters in range /_a-z0-9/ (alphanumeric and '_', case-insensitive) compressed into a single 64-bit number.
  /// Very fast to pass around and compare against each other.
  /// 
  /// All of its operations are supposed to be constexpr-friendly.
  /// 
  /// Max number of chars is `TTokenCount * BasicToken::MAX_LENGTH`
  /// </summary>
  template<size_t TTokenCount>
  struct CompositeToken {

    static constexpr size_t MAX_LENGTH = TTokenCount * BasicToken::MAX_LENGTH;

    constexpr CompositeToken() : storage{} {}
    constexpr CompositeToken(const CompositeToken& other) : storage(other.storage) {}
    constexpr CompositeToken& operator=(const CompositeToken& other)
    {
      this->storage = other.storage;
      return *this;
    }

    constexpr CompositeToken(const BasicToken& other) : storage{ other } {}


    template<typename size_t TOtherTokenCount, typename _sfinae_guard = std::enable_if<(TOtherTokenCount < TTokenCount), char>::type>
    constexpr CompositeToken(const CompositeToken<TOtherTokenCount>& other) : storage{ }
    {
      for (size_t t = 0; t < TOtherTokenCount; ++t) storage[t] = other.get_storage()[t];
    }


    constexpr CompositeToken(const std::string_view& str)
      : storage{}
    {
      if (str.size() > MAX_LENGTH) throw std::exception("String too big for a token!");

      for (size_t t = 0; t < TTokenCount; ++t) {
        const size_t start = t * BasicToken::MAX_LENGTH;
        if (start >= str.size()) break;
        const size_t count = (start + BasicToken::MAX_LENGTH >= str.size()) ? (str.size() - start) : BasicToken::MAX_LENGTH;
        storage[t] = BasicToken(str.substr(start, count));
      }
    }


    constexpr bool operator==(const CompositeToken other) const
    {
      return storage == other.storage;
    }

    constexpr bool operator!=(const CompositeToken other) const
    {
      return storage != other.storage;
    }

    constexpr const std::array<BasicToken, TTokenCount>& get_storage() const {
      return storage;
    }

    constexpr auto to_cstring() const
    {
      std::array<char, MAX_LENGTH + 1> ret{};

      for (size_t t = 0; t < TTokenCount; ++t) {
        char* const start = ret.data() + t * BasicToken::MAX_LENGTH;
        const auto end = storage[t].write_to_buffer(start);
        if (end < start + BasicToken::MAX_LENGTH)
          break;
      }
      return ret;
    }

    constexpr std::string to_string() const
    {
      return std::string(to_cstring().data());
    }




  private:
    std::array<BasicToken, TTokenCount> storage;
  };

}
  template<>
  struct std::hash<nc::BasicToken>
  {
    std::size_t operator()(const nc::BasicToken& token) const noexcept
    {
      return std::hash<nc::u64>{}(token.get_raw());
    }
  };
  
  template<size_t TTokenCount>
  struct std::hash<nc::CompositeToken<TTokenCount>>
  {
    std::size_t operator()(const nc::CompositeToken<TTokenCount>& token) const noexcept
    {
      using hasher = std::hash<nc::BasicToken>;
      const auto& segments(token.get_storage());
      size_t ret = hasher{}(segments[0]);
      for (size_t t = 1; t < TTokenCount; ++t) {
        ret = nc::hash_combine(ret, hasher{}(segments[t]));
      }
      return ret;
    }
  };


namespace nc {
  // Declare our standard token to carry up to 24 chars
  using Token = CompositeToken<2>;


}