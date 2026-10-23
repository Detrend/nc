// Project Nuclidean Source File
/**
 * Place where to put all common macro or template black magic utilities used throught the codebase
 */
#pragma once

#define NC_INTERNAL_TOKENPASTE2(_a, _b) _a##_b
#define NC_TOKENJOIN(_a, _b) NC_INTERNAL_TOKENPASTE2(_a, _b)
#define NC_TOKENJOIN3(_a, _b, _c) NC_TOKENJOIN(_a, NC_TOKENJOIN(_b, _c))
#define NC_TOKENPASTE(_a) _a



#define STRINGIFY_impl(...) # __VA_ARGS__
#define STRINGIFY(...) STRINGIFY_impl(__VA_ARGS__)

#define ARRAY_LENGTH(...) (sizeof(__VA_ARGS__)/sizeof(*(__VA_ARGS__)))

namespace nc {
  namespace meta {
    template<typename TFn>
    struct defer_dispose_helper_t {
      defer_dispose_helper_t(TFn&& the_on_destruction) : on_destruction(the_on_destruction) {}
      ~defer_dispose_helper_t() { on_destruction(); }
    private:
      TFn on_destruction;
    };
  }
}

#define defer(...) auto NC_TOKENJOIN(_defer_helper_, __LINE__) = ::nc::meta::defer_dispose_helper_t([&]()->void{ __VA_ARGS__ })

