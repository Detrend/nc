// Project Nuclidean Source File
#pragma once

#include <config.h>
#include <types.h>
#include <math/vector.h>

#include <engine/graphics/gl_types.h>
#include <engine/graphics/resources/res_lifetime.h>
#include <engine/graphics/resources/texture_id.h>
#include <memory.h>

#include <filesystem>
#include <string>

#include <stb/stb_rect_pack.h>

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace nc
{
  namespace texture_util
  {

    struct CompressedImgInfo
    {
      u32 width, height;
      GLenum format, format_srgb;
      size_t payload_size;
      unsigned char* data;
    };

    bool load_astc(const std::filesystem::path& img_path, const FunctionReference<void(const CompressedImgInfo&)>& fn);
    bool load_s3tc(const std::filesystem::path& img_path, GLenum expected_format, const FunctionReference<void(const CompressedImgInfo&)>& fn);

  }
}
