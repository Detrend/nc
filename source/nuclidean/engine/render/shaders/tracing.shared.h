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
  // Surface texturing.
  wall_segments,
  textures,
};

enum class TracingUniform
{
  context,
};

enum class TracingTexture
{
  atlas,
};

enum class TracingImage
{
  out_color,
};

#ifdef __SLANG__
// Each type is in its own descriptor set, so Slang reports overlaps only within a type. GL ignores the sets.
#define BUFFER(type, name)  [[vk::binding(TracingBuffer::name, 0)]] type name
#define UNIFORM(type, name) [[vk::binding(TracingUniform::name, 1)]] type name
#define TEXTURE(type, name) [[vk::binding(TracingTexture::name, 2)]] type name
#define IMAGE(type, name)   [[vk::binding(TracingImage::name, 3)]] type name
#endif

// Width and height of the pixel block traced by one work group.
static const u32 TRACING_GROUP_SIZE = 8;

struct Texture
{
  // Position of the top left texel of the texture within the atlas.
  uvec2 position;
  // Size of the texture in texels.
  uvec2 size;
};

struct Surface
{
  // Offset of the texture on the surface, in meters.
  vec2 offset;
  // INVALID if the surface is hidden, otherwise index to the textures buffer.
  u32 texture_id;
  u32 _padding;
};

struct WallSegment
{
  Surface surface;
  // Height where the segment ends, it begins where the previous one ends.
  f32 end_height;
  u32 _padding0;
  u32 _padding1;
  u32 _padding2;
};

struct Wall
{
  // First position. Second position is position of the next wall.
  vec2 position;
  // INVALID if wall is not portal, otherwise index to portals buffer.
  u32 portal_id;
  // Length of the sector walls preceding this one, so the texture continues across corners.
  f32 cumulative_length;
  // Index of the first wall segment within the wall segments buffer.
  u32 segments_offset;
  // Number of segments of the wall.
  u32 segments_count;
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
  // Floor height the sector starts with, textures of lifts move by how far the floor moved from it.
  f32 default_floor_y;
  // Ceiling height the sector starts with, textures of doors move by how far the ceiling moved from it.
  f32 default_ceil_y;
  Surface floor;
  Surface ceiling;
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
