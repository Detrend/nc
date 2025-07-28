#pragma once

#include <types.h>
#include <math/vector.h>

#include <engine/graphics/gl_types.h>
#include <engine/graphics/resources/res_lifetime.h>

#include <stb/stb_rect_pack.h>

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace nc
{

struct TextureAtlas;

class TextureHandle
{
public:
  friend class TextureManager;

  static TextureHandle invalid();

  ResLifetime get_lifetime() const;
  u32 get_x() const;
  u32 get_y() const;
  u32 get_width() const;
  u32 get_height() const;
  const TextureAtlas& get_atlas() const;

  vec2 get_pos() const;
  vec2 get_size() const;

private:
  TextureHandle() {}
  TextureHandle(ResLifetime lifetime, u32 x, u32 y, u32 width, u32 height);

  ResLifetime m_lifetime   = ResLifetime::None;
  u16         m_generation = 0;
  u32         m_x          = 0;
  u32         m_y          = 0;
  u32         m_width      = 0;
  u32         m_height     = 0;
};

using TextureMap = std::unordered_map<std::string, TextureHandle>;

struct TextureAtlas
{
  GLuint     handle = 0;
  u32        width = 0;
  u32        height = 0;
  TextureMap textures;

  vec2 get_size() const;
};

class TextureManager
{
public:
  friend class TextureHandle;

  static TextureManager& get();

  // Begin loading of multiple textures of a specified lifetime.
  void begin_load(ResLifetime lifetime);
  /**
   * Loads texture from the file. You must call TextureManager::begin_load before loading any texture of a specified
   * lifetime. After loading all texture of a specified lifetime, you must call TextureManager::finih_load. Mixing of
   * lifetimes is not allowed, i.e. you cannot load textures of different lifetimes in the same begin_load/finish_load.
   */
  void load(ResLifetime lifetime, const std::string& path);
  // Finishes loading of multiple textures of a specified lifetime.
  void finish_load(ResLifetime lifetime);
  void load_directory(ResLifetime lifetime, const std::string& path);
  // Unloads all textures with specified lifetime.
  void unload(ResLifetime lifetime);

  const TextureAtlas& get_atlas(ResLifetime lifetime) const;
  GLuint get_error_texture_handle() const;

  const TextureHandle& operator[](const std::pair<const std::string&, ResLifetime> pair);
  const TextureHandle& operator[](const std::string& name);

  constexpr static u32 ERROR_TEXTURE_SIZE = 1024;

private:
  struct LoadData;

  inline static std::unique_ptr<TextureManager> m_instance = nullptr;
  inline static u16 m_generation = 0;

  TextureManager();

  TextureAtlas& get_atlas_mut(ResLifetime lifetime);
  void create_error_texture();

  TextureAtlas m_game_atlas;
  TextureAtlas m_level_atlas;

  GLuint m_error_texture = 0;

  // Lifetime of the current batch load operation, or ResLifetime::None if no load is active.
  ResLifetime m_load_lifetime = ResLifetime::None;
  /**
   * During a load operation, each loaded texture's width and height are added here to be processed by stb_rect_pack
   * in TextureManager::finish_load.
   */
  std::vector<stbrp_rect> m_load_rects;
  // Stores pixel data and metadata for textures which are currently being loaded.
  std::vector<LoadData> m_load_data;

  // Holds the pixel data and properties of a single image which is currently being loaded from disk.
  struct LoadData
  {
    // The width of the image in pixels.
    int                  width  = 0;
    // The height of the image in pixels.
    int                  height = 0;
    // The OpenGL texture format (e.g., GL_RGB, GL_RGBA).
    GLenum               format = GL_RGB;
    // A pointer to the pixel data.
    unsigned char*       data   = nullptr;
    // Name of the texture.
    std::string          name   = "";
  };
};

}
