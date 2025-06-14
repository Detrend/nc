#pragma once

#include <engine/graphics/resources/res_lifetime.h>
#include <engine/graphics/gl_types.h>
#include <types.h>

#include <vector>
#include <memory>
#include <string>

namespace nc
{

class TextureHandle
{
public:
  friend class TextureManager;

  ResLifetime get_lifetime() const;
  /**
   * Get OpenGL handle (used in openGL function calls).
   */
  GLuint get_gl_handle() const;
  u32 get_width() const;
  u32 get_height() const;
  /**
   * Number of channels in each pixel.
   */
  u32 get_channels() const;

  /**
   * Gets an invalid texture handle.
   */
  static TextureHandle invalid();

private:
  TextureHandle() {}
  TextureHandle(ResLifetime lifetime, GLuint gl_handle, u32 width, u32 height, u32 channels);

  ResLifetime m_lifetime   = ResLifetime::None;
  u16         m_generation = 0;
  /**
   * OpenGL handle (used in openGL function calls).
   */
  GLuint      m_gl_handle  = 0;
  u32         m_width      = 0;
  u32         m_height     = 0;
  /**
   * Number of channels in each pixel.
   */
  u32         m_channels   = 0;
};

class TextureManager
{
public:
  friend class TextureHandle;

  static TextureManager& instance();

  /**
   * Loads texture from the file.
   */
  TextureHandle create(ResLifetime lifetime, const std::string& path);

  /**
   * Unloads all textures with specified lifetime.
   */
  void unload(ResLifetime lifetime);

  const TextureHandle& get_test_enemy_texture();
  const TextureHandle& get_test_gun_texture();
  const TextureHandle& get_test_plasma_texture();

private:
  inline static std::unique_ptr<TextureManager> m_instance = nullptr;
  TextureManager();

  static inline u16 m_generation = 0;

  std::vector<TextureHandle>& get_storage(ResLifetime lifetime);

  std::vector<TextureHandle> m_level_textures;
  std::vector<TextureHandle> m_game_textures;

  TextureHandle m_test_enemy_texture = TextureHandle::invalid();
  TextureHandle m_test_gun_texture = TextureHandle::invalid();
  TextureHandle m_test_plasma_texture = TextureHandle::invalid();
};

}
