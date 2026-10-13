// Project Nuclidean Source File
#pragma once
#ifdef __SLANG__
#include "types.slangh"
#include "constants.shared.h"
using namespace nc::render;
#else
#include <types.h>
#include <math/vector.h>
#include <engine/render/shaders/constants.shared.h>
#endif

namespace nc::render
{

enum class TracingBuffer
{
  // Map geometry.
  walls,
  sectors,
  portals,
};

enum class TracingUniform
{
  context,
};

enum class TracingImage
{
  out_color,
};

#ifdef __SLANG__
// Each type is in its own descriptor set, so Slang reports overlaps only within a type. GL ignores the sets.
#define BUFFER(type, name)  [[vk::binding(TracingBuffer::name, 0)]] type name
#define UNIFORM(type, name) [[vk::binding(TracingUniform::name, 1)]] type name
#define IMAGE(type, name)   [[vk::binding(TracingImage::name, 3)]] type name
#endif

// Width and height of the pixel block traced by one work group.
static const u32 TRACING_GROUP_SIZE = 8;

struct Wall
{
  // First position. Second position is position of the next wall.
  vec2 position;
  // INVALID if wall is not portal, otherwise index to portals buffer.
  u32 portal_id;
  u32 _padding;
};

struct Sector
{
  // Index of first sector wall within the walls buffer.
  u32 walls_offset;
  // Number of walls within sector.
  u32 walls_count;
  // Height of the sector floor.
  f32 floor_y;
  // Height of the sector ceiling.
  f32 ceil_y;
};

struct Context
{
  // Position of the camera.
  vec3 camera_position;
  // Sector in which camera is currently located.
  u32 camera_sector_id;
  // Direction in which camera is looking.
  vec3 camera_direction;
  u32 _padding;
};

struct Portal
{
  // Translation applied to object going through the portal.
  vec3 translation;
  // Rotation along Y axis applied to object going through the portal.
  f32 rotation;
  // Destination sector.
  u32 destination_id;
  u32 _padding0;
  u32 _padding1;
  u32 _padding2;
};

}
