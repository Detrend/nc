/**
 * Place where to put all common macro or template black magic utilities used throught the codebase
 */
#pragma once

#define NC_INTERNAL_TOKENPASTE2(_a, _b) _a##_b
#define NC_TOKENJOIN(_a, _b) NC_INTERNAL_TOKENPASTE2(_a, _b)
#define NC_TOKENPASTE(_a) _a



#define STRINGIFY_impl(...) # __VA_ARGS__
#define STRINGIFY(...) STRINGIFY_impl(__VA_ARGS__)

#define ARRAY_LENGTH(...) (sizeof(__VA_ARGS__)/sizeof(*(__VA_ARGS__)))