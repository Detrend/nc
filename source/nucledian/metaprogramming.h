/**
 * Place where to put all common macro or template black magic utilities used throught the codebase
 */
#pragma once



#define STRINGIFY_impl(...) # __VA_ARGS__
#define STRINGIFY(...) STRINGIFY_impl(__VA_ARGS__)

#define ARRAY_LENGTH(...) (sizeof(__VA_ARGS__)/sizeof(*(__VA_ARGS)))