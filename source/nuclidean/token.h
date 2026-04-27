// Project Nuclidean Source File
#pragma once
/**
 * This file contains definition of Token - compressed, fixed-size string-like object with value semantics and limited charset, 
 * which should be used instead of std::string whenever applicable throught the game.
 * 
 * Token doesn't allocate anything, is trivially copyiable, comparable (both for `==` and `<`) and hashable.
 * Default parametrization supports only lowercase aphanumeric characters and the '_' char.
 * You can create custom token types with different charsets, by defining a custom token_policy and passing that as the TPermittedChars type parameter to CompositeToken
 *  - see e.g. token_policies::Chars_Default for insipration. Keep in mind, that the more characters are supported, the fewer characters can fit in the Token.
 * 
 * For most cases, the default `Token` alias (declared at the bottom of the file) should be sufficient. Custom token types should be used only with good justification.
 * 
 * You can construct a Token from various types of strings during runtime. If you are constructing from a string literal,
 *  please use the Token::Const("some_text") factory function to ensure a compilation error when the literal is too long or contains unsupported characters.
 * 
 * For interfacing with code that requires passing a C-string, use the `.to_cstring()` function - that will return an std::array suffiecient enought to fit the null-terminated string.
 * There is also .to_cstring_enclosed(), which allows passing additional prefix and postfix string literals.
 *   Use it e.g. like this: Token::Const("cultist_die").to_cstring_enclosed("content/sound/", ".mp3) // returns an std::array<char,> containing "content/sound/cultist_die.mp3\0".
 * The .to_string() function can allocate and thus should be used only when absolutely necessary.
 */
#include<types.h>
#include<string>
#include<string_view>
#include<array>
#include<exception>

#include "math/utils.h"


namespace nc {


  /// <summary>
  /// Namespace containing charset policies that can be used by our Tokens
  /// </summary>
  namespace token_policies 
  {

    // Always use this policy unless there is a very good reason not to
    struct Chars_Default 
    {
      //These are the chars that can be stored in a token
      static constexpr const char chars[] = "abcdefghijklmnopqrstuvwxyz0123456789_"; 

      //These chars can be used to initialize a token, but they will get converted into their equivalent at the same index of the `chars` array
      static constexpr const char alt[]   = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    };

    // Supports additional `.` and ` ` chars needed by our cvars
    struct Chars_CVar 
    {
      static constexpr const char chars[] = "abcdefghijklmnopqrstuvwxyz0123456789_. ";
      static constexpr const char alt[]   = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    };

  } // namespace token_policies




  /// <summary>
  /// Helper functions internally used by Token-related structures.
  /// 
  /// Mainly exists to workaround dumb C++ rules (a static consteval function cannot be called inside the class definition that defines it).
  /// </summary>
  namespace token_helpers 
  {

    static consteval auto make_char_to_code_table(const std::string& chars_list, const std::string& alt_chars_list)
    {
      if (alt_chars_list.size() > chars_list.size()) {
        throw new std::exception("Alt chars list must not be bigger than chars_list!");
      }

      std::array<u8, 256> ret = {};
      for (char t = 0; t < chars_list.size(); ++t) {
        ret[chars_list[t]] = t + 1;
        if (t < alt_chars_list.size()) {
          ret[alt_chars_list[t]] = t + 1;
        }
      }
      return ret;
    }

    static consteval size_t compute_max_token_length(const u64 chars_count) 
    {
      u64 max = ULLONG_MAX;
      size_t ret = 0;
      for (; max > chars_count; max /= chars_count) {
        ++ret;
      }
      return ret;
    }
  } // namespace token_helpers



  /// <summary>
  /// String of typically <= 12 characters in range /_a-z0-9/ (alphanumeric and '_', case-insensitive) compressed into a single 64-bit number.
  /// Very fast to pass around and compare against each other.
  /// Charset can be customized by providing a token_policy. The more chars are supported, the fewer of them can fit inside a single token.
  /// 
  /// All of its operations are supposed to be constexpr-friendly.
  /// </summary>
  template<typename TPermittedChars = token_policies::Chars_Default>
  struct BasicToken 
  {

    static constexpr auto PERMITTED_CHARS = TPermittedChars::chars;

    static constexpr u64 BASE = std::string(PERMITTED_CHARS).size() + 1;

    // a single BasicToken can contain at most this many chars
    static constexpr size_t MAX_LENGTH = token_helpers::compute_max_token_length(BASE);

  private:

    static constexpr auto CHAR_TO_CODE = token_helpers::make_char_to_code_table(TPermittedChars::chars, TPermittedChars::alt);


    static constexpr u64 char_to_code(const char c)
    {
      const u8 ret = CHAR_TO_CODE[c];
      if (!ret) throw std::exception("Unsupported token character!"); //ideally this would be an assert, but those seem to behave problematically in constexpr context
      return ret;
    }

    static constexpr char code_to_char(const u64 t)
    {
      if ((t - 1) >= BASE) throw std::exception("Invalid token code!");
      return PERMITTED_CHARS[static_cast<u8>(t - 1)];
    }


  public:

    constexpr BasicToken() : raw(0) {}


    constexpr BasicToken(const std::string_view& str)
      : raw(0)
    {
      if (str.size() > MAX_LENGTH) throw std::exception("String too big for a token!");

      for (size_t t = str.size(); t-- > 0;) 
      {
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

    constexpr auto operator<=>(const BasicToken&) const = default;


    constexpr char* write_to_buffer(char* buffer) const
    {
      for (u64 t = raw; t > 0; t /= BASE) 
      {
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

  }; // struct BasicToken

  /// <summary>
  /// Fixed-size string of characters in range /_a-z0-9/ (alphanumeric and '_', case-insensitive) compressed into a single 64-bit number.
  /// Very fast to pass around and compare against each other.
  /// Charset can be customized by providing a token_policy. The more chars are supported, the fewer of them can fit inside a single token.
  /// 
  /// All of its operations are supposed to be constexpr-friendly.
  /// 
  /// Max number of chars is `TTokenCount * BasicToken<TPermittedChars>::MAX_LENGTH`
  /// </summary>
  template<size_t TTokenCount, typename TPermittedChars=token_policies::Chars_Default>
  struct CompositeToken 
  {

    using Segment = BasicToken<TPermittedChars>;

    static constexpr size_t MAX_LENGTH = TTokenCount * Segment::MAX_LENGTH;

    constexpr CompositeToken() : storage{} {}
    constexpr CompositeToken(const CompositeToken& other) : storage(other.storage) {}
    constexpr CompositeToken& operator=(const CompositeToken& other)
    {
      this->storage = other.storage;
      return *this;
    }

    constexpr CompositeToken(const Segment& other) : storage{ other } {}

    template<size_t TSize>
    static consteval CompositeToken Const(const char(&literal)[TSize]) 
    {
      return CompositeToken(literal);
    }

    template<typename size_t TOtherTokenCount, typename _sfinae_guard = std::enable_if<(TOtherTokenCount < TTokenCount), char>::type>
    constexpr CompositeToken(const CompositeToken<TOtherTokenCount>& other) : storage {}
    {
      storage = other.get_storage();
      //for (size_t t = 0; t < TOtherTokenCount; ++t) storage[t] = other.get_storage()[t];
    }


    constexpr CompositeToken(const std::string_view& str)
      : storage{}
    {
      if (str.size() > MAX_LENGTH) throw std::exception("String too big for a token!");

      for (size_t t = 0; t < TTokenCount; ++t) 
      {
        const size_t start = t * Segment::MAX_LENGTH;
        if (start >= str.size()) break;
        const size_t count = (start + Segment::MAX_LENGTH >= str.size()) ? (str.size() - start) : Segment::MAX_LENGTH;
        storage[t] = Segment(str.substr(start, count));
      }
    }

    constexpr CompositeToken(cstr c_string)
     : CompositeToken(std::string_view{c_string})
    {}

    constexpr bool operator==(const CompositeToken& other) const
    {
      return storage == other.storage;
    }

    constexpr bool operator!=(const CompositeToken& other) const
    {
      return storage != other.storage;
    }

    constexpr std::strong_ordering operator<=>(const CompositeToken& other) const
    {
      for (size_t t = 0; t < TTokenCount; ++t) 
      {
        auto ret = (storage[t] <=> other.storage[t]);
        if (ret != std::strong_ordering::equal) 
        {
          return ret;
        }
      }
      return std::strong_ordering::equal;
    }


    constexpr const std::array<Segment, TTokenCount>& get_storage() const {
      return storage;
    }

    constexpr char* write_to_buffer(char* buffer) const
    {
        for (size_t t = 0; t < TTokenCount; ++t) 
        {
            char*const end = storage[t].write_to_buffer(buffer);
            const bool is_finished = (end < buffer + Segment::MAX_LENGTH);
            buffer = end;
            if (is_finished) break;
        }
        return buffer;
    }

    constexpr auto to_cstring() const
    {
      std::array<char, MAX_LENGTH + 1> ret{};

      write_to_buffer(ret.data());
      return ret;
    }

    template<size_t TPrefixSize, size_t TPostfixSize>
    constexpr auto to_cstring_enclosed(const char(&prefix)[TPrefixSize], const char(&postfix)[TPostfixSize]) const 
    {
        std::array<char, MAX_LENGTH + (TPrefixSize - 1) + (TPostfixSize - 1) + 1> ret{};
        char* buf = ret.data();

        for (const char* p = prefix; *p;) *(buf++) = *(p++);

        buf = write_to_buffer(buf);

        for (const char* p = postfix; *p;) *(buf++) = *(p++);
        *buf = '\0';

        return ret;
    }

    constexpr std::string to_string() const
    {
      return std::string(to_cstring().data());
    }


  private:
    std::array<Segment, TTokenCount> storage;
  }; // struct CompositeToken

} //namespace nc

  template<typename TPermittedChars>
  struct std::hash<nc::BasicToken<TPermittedChars>>
  {
    std::size_t operator()(const nc::BasicToken<TPermittedChars>& token) const noexcept
    {
      return std::hash<nc::u64>{}(token.get_raw());
    }
  };
  
  template<size_t TTokenCount, typename TPermittedChars>
  struct std::hash<nc::CompositeToken<TTokenCount, TPermittedChars>>
  {
    std::size_t operator()(const nc::CompositeToken<TTokenCount, TPermittedChars>& token) const noexcept
    {
      using hasher = std::hash<nc::CompositeToken<TTokenCount, TPermittedChars>::Segment>;
      const auto& segments(token.get_storage());
      size_t ret = hasher{}(segments[0]);
      for (size_t t = 1; t < TTokenCount; ++t) 
      {
        ret = nc::hash_combine(ret, hasher{}(segments[t]));
      }
      return ret;
    }
  };


namespace nc {
  // Declare our standard token to carry up to 24 chars
  using Token = CompositeToken<2>;
}