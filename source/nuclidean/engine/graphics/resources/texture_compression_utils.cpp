// Project Nuclidean Source File
#include <engine/graphics/resources/texture.h>

#include <common.h>
#include <logging.h>

#include <glad/glad.h>
#include <stb/stb_image.h>
#include <stb/stb_image_write.h>
#include <json/json.hpp>
#include <engine/graphics/graphics_system.h>

#include <algorithm>
#include <array>
#include <cstdlib>
#include <vector>
#include <fstream>
#include "texture_compression_utils.h"

namespace nc
{
  namespace texture_util
  {

    static u32 read_u24_le(const void* const bytes) {
      return static_cast<u32>(reinterpret_cast<const u8*>(bytes)[0])
        | (static_cast<u32>(reinterpret_cast<const u8*>(bytes)[1]) << 8)
        | (static_cast<u32>(reinterpret_cast<const u8*>(bytes)[2]) << 16)
        ;
    }

    static u32 read_u32_le(const void* const bytes) {
      return read_u24_le(bytes) | (static_cast<u32>(reinterpret_cast<const u8*>(bytes)[3]) << 24);
    }



    bool load_astc(const std::filesystem::path& img_path, const FunctionReference<void(const CompressedImgInfo&)>& fn)
    {
      std::ifstream f(img_path, std::ios::binary | std::ios::ate);
      nc_expect(f, "Could not open ASTC file {}", img_path.string());
      const size_t file_size = static_cast<size_t>(f.tellg());
      constexpr size_t ASTC_HEADER_SIZE = 16;
      nc_expect(file_size > ASTC_HEADER_SIZE, "File {} has only {}B", img_path.string(), file_size);
      f.seekg(0, std::ios::beg);
      scratch_vector<char> buffer(file_size);
      nc_expect(f.read(buffer.data(), buffer.size()), "Failed to read file {}", img_path.string());
      unsigned char* buf = reinterpret_cast<unsigned char*>(buffer.data());
      nc_expect(buf[0] == 0x13 && buf[1] == 0xAB && buf[2] == 0xA1 && buf[3] == 0x5C, "Invalid ASTC file magic");

      CompressedImgInfo info;

      const u8 block_width = static_cast<u8>(buf[4]);
      const u8 block_height = static_cast<u8>(buf[5]);
      const u8 block_depth = static_cast<u8>(buf[6]);
      info.width = read_u24_le(buf + 7);
      info.height = read_u24_le(buf + 10);
      const u32 depth = read_u24_le(buf + 13);
      nc_expect((block_depth == 1) && (depth == 1) && (info.width != 0) && (info.height != 0));

      // ASTC RGBA LDR format enums from GL_KHR_texture_compression_astc_ldr.
      constexpr struct {
        u8 block_width;
        u8 block_height;
        GLenum gl_format;
        GLenum gl_format_srgb;
      } ASTC_FORMATS[] = {
          {  4,  4, GL_COMPRESSED_RGBA_ASTC_4x4   , GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4   },
          {  5,  4, GL_COMPRESSED_RGBA_ASTC_5x4   , GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x4   },
          {  5,  5, GL_COMPRESSED_RGBA_ASTC_5x5   , GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5   },
          {  6,  5, GL_COMPRESSED_RGBA_ASTC_6x5   , GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x5   },
          {  6,  6, GL_COMPRESSED_RGBA_ASTC_6x6   , GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6   },
          {  8,  5, GL_COMPRESSED_RGBA_ASTC_8x5   , GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x5   },
          {  8,  6, GL_COMPRESSED_RGBA_ASTC_8x6   , GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x6   },
          {  8,  8, GL_COMPRESSED_RGBA_ASTC_8x8   , GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x8   },
          { 10,  5, GL_COMPRESSED_RGBA_ASTC_10x5  , GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x5  },
          { 10,  6, GL_COMPRESSED_RGBA_ASTC_10x6  , GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x6  },
          { 10,  8, GL_COMPRESSED_RGBA_ASTC_10x8  , GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x8  },
          { 10, 10, GL_COMPRESSED_RGBA_ASTC_10x10 , GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x10 },
          { 12, 10, GL_COMPRESSED_RGBA_ASTC_12x10 , GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x10 },
          { 12, 12, GL_COMPRESSED_RGBA_ASTC_12x12 , GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x12 },
      };

      info.format = info.format_srgb = 0;
      for (const auto& format : ASTC_FORMATS) {
        if (format.block_width == block_width && format.block_height == block_height) {
          info.format = format.gl_format;
          info.format_srgb = format.gl_format_srgb;
          break;
        }
      }
      nc_expect(info.format);
      nc_expect(info.format_srgb);

      const size_t blocks_x = (info.width + block_width - 1) / block_width;
      const size_t blocks_y = (info.height + block_height - 1) / block_height;
      const size_t expected_payload_size = blocks_x * blocks_y * 16;
      const size_t payload_size = buffer.size() - ASTC_HEADER_SIZE;
      nc_expect(payload_size == expected_payload_size);
      nc_expect(payload_size < std::numeric_limits<GLsizei>::max());

      info.payload_size = payload_size;
      info.data = buf + ASTC_HEADER_SIZE;
      fn(info);

      return true;
    }

    
    bool load_s3tc(const std::filesystem::path& img_path, GLenum expected_format, const FunctionReference<void(const CompressedImgInfo&)>& fn)
    {
      std::ifstream f(img_path, std::ios::binary | std::ios::ate);
      nc_expect(f, "Could not open S3TC file {}", img_path.string());
      const size_t file_size = static_cast<size_t>(f.tellg());
      constexpr size_t DDS_HEADER_SIZE = 128;
      nc_expect(file_size > DDS_HEADER_SIZE, "File {} has only {}B", img_path.string(), file_size);
      f.seekg(0, std::ios::beg);
      scratch_vector<char> buffer(file_size);
      nc_expect(f.read(buffer.data(), buffer.size()), "Failed to read file {}", img_path.string());
      unsigned char* buf = reinterpret_cast<unsigned char*>(buffer.data());

      nc_expect(buf[0] == 'D' && buf[1] == 'D' && buf[2] == 'S' && buf[3] == ' ', "Invalid DDS file magic");
      nc_expect(read_u32_le(buf + 4) == 124, "Invalid DDS header size");
      nc_expect(read_u32_le(buf + 76) == 32, "Invalid DDS pixel format header size");
      nc_expect((read_u32_le(buf + 80) & 0x4) != 0, "DDS file does not specify a FOURCC format");
      const u32 fourcc = read_u32_le(buf + 84);

      size_t block_size = 0;
      if (expected_format == 0x8C4F) { // GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT
        nc_expect(fourcc == read_u32_le("DXT5"), "DDS file is not DXT5-compressed");
        nc_expect(fourcc == static_cast<u32>('5TXD'), "DDS file is not DXT5-compressed");
        block_size = 16;
      }
      else if (expected_format == 0x83F0) { // GL_COMPRESSED_RGB_S3TC_DXT1_EXT
        nc_expect(fourcc == read_u32_le("DXT1"), "DDS file is not DXT1-compressed");
        nc_expect(fourcc == static_cast<u32>('1TXD'), "DDS file is not DXT5-compressed");
        block_size = 8;
      }
      else if (expected_format == 0x8DBB) { // GL_COMPRESSED_RED_RGTC1
        nc_expect(fourcc == read_u32_le("ATI1") || fourcc == read_u32_le("BC4U"), "DDS file is not BC4-compressed");
        nc_expect(fourcc == static_cast<u32>('1ITA'), "DDS file is not DXT5-compressed");
        block_size = 8;
      }
      else {
        nc_crit("Unsupported S3TC format {}", expected_format);
        return false;
      }

      CompressedImgInfo info;
      info.height = read_u32_le(buf + 12);
      info.width = read_u32_le(buf + 16);
      nc_expect(info.width != 0 && info.height != 0);

      const size_t blocks_x = (info.width + 3) / 4;
      const size_t blocks_y = (info.height + 3) / 4;
      const size_t expected_payload_size = blocks_x * blocks_y * block_size;
      const size_t payload_size = buffer.size() - DDS_HEADER_SIZE;
      nc_expect(payload_size == expected_payload_size);
      nc_expect(payload_size < std::numeric_limits<GLsizei>::max());

      info.format = expected_format;
      info.payload_size = payload_size;
      info.data = buf + DDS_HEADER_SIZE;
      fn(info);

      return true;
    }


  }
} // namespace nc
