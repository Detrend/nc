// Project Nuclidean Source File
#pragma once
#ifdef __SLANG__
#include "types.slangh"
using namespace nc::render;
#else
#include <types.h>
#endif

namespace nc::render
{

enum class TracingImage
{
  out_color,
};

#ifdef __SLANG__
// Each type is in its own descriptor set, so Slang reports overlaps only within a type. GL ignores the sets.
#define IMAGE(type, name) [[vk::binding(TracingImage::name, 3)]] type name
#endif

// Width and height of the pixel block traced by one work group.
static const u32 TRACING_GROUP_SIZE = 8;

}
