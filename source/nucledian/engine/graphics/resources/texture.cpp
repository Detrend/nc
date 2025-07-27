#include <common.h>
#include <logging.h>
#include <engine/graphics/resources/texture.h>

#include <glad/glad.h>
#include <stb_image/stb_image.h>

#include <filesystem>

namespace nc
{

//==============================================================================
ResLifetime TextureHandle::get_lifetime() const
{
  return m_lifetime;
}

//==============================================================================
GLuint TextureHandle::get_gl_handle() const
{
  return m_gl_handle;
}

//==============================================================================
u32 TextureHandle::get_width() const
{
  return m_width;
}

//==============================================================================
u32 TextureHandle::get_height() const
{
  return m_height;
}

//==============================================================================
TextureHandle TextureHandle::invalid()
{
  return TextureHandle();
}

//==============================================================================
const TextureHandle& TextureHandle::error()
{
  return TextureManager::instance().get_error_texture();
}

//==============================================================================
TextureHandle::TextureHandle(ResLifetime lifetime, GLuint gl_handle, u32 width, u32 height)
:
  m_lifetime(lifetime),
  m_gl_handle(gl_handle),
  m_width(width),
  m_height(height),
  m_generation(TextureManager::m_generation)
{}

//==============================================================================
TextureManager& TextureManager::instance()
{
  if (m_instance == nullptr)
  {
    m_instance = std::unique_ptr<TextureManager>(new TextureManager());
  }

  return *m_instance;
}

//==============================================================================
TextureHandle nc::TextureManager::create(ResLifetime lifetime, const std::string& path)
{
  GLuint gl_handle = 0;
  glGenTextures(1, &gl_handle);
  glBindTexture(GL_TEXTURE_2D, gl_handle);

  // Using nearest filters for better retro style.
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  int width = 0, height = 0, channels = 0;
  unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, 0);
  
  if (data == nullptr)
  {
    nc_crit("Cannot load image \"{}\": {}", path, stbi_failure_reason());
    glBindTexture(GL_TEXTURE_2D, 0);
    glDeleteTextures(1, &gl_handle);
    return TextureHandle::error();
  }

  GLenum format = 0;
  if (channels == 3)
    format = GL_RGB;
  else if (channels == 4)
    format = GL_RGBA;
  else
  {
    nc_crit("Cannot load image \"{}\": {}", path, "Texture format not supported.");
    glBindTexture(GL_TEXTURE_2D, 0);
    glDeleteTextures(1, &gl_handle);
    stbi_image_free(data);
    return TextureHandle::error();
  }

  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, format, GL_UNSIGNED_BYTE, data);
  glGenerateMipmap(GL_TEXTURE_2D);

  glBindTexture(GL_TEXTURE_2D, 0);
  stbi_image_free(data);

  return TextureHandle(lifetime, gl_handle, static_cast<u32>(width), static_cast<u32>(height));
}

//==============================================================================
void TextureManager::unload(ResLifetime lifetime)
{
  auto& storage = this->get_storage(lifetime);
  std::vector<GLuint> textures_to_delete;

  for (auto& texture : storage)
  {
    if (texture.m_gl_handle != 0)
    {
      textures_to_delete.push_back(texture.m_gl_handle);
      texture.m_gl_handle = 0;
    }
  }

  glDeleteTextures(static_cast<GLsizei>(textures_to_delete.size()), textures_to_delete.data());
  storage.clear();
  m_generation++;
}

//==============================================================================
const TextureHandle& TextureManager::get_error_texture() const
{
  return m_error_texture;
}

//==============================================================================
const TextureHandle& TextureManager::get_test_enemy_texture() const
{
  return m_test_enemy_texture;
}

//==============================================================================
const TextureHandle& TextureManager::get_test_gun_texture() const
{
  return m_test_gun_texture;
}

//==============================================================================
const TextureHandle& TextureManager::get_test_gun2_texture() const
{
  return m_test_gun2_texture;
}

//==============================================================================
const TextureHandle& TextureManager::get_test_plasma_texture() const
{
  return m_test_plasma_texture;
}

//==============================================================================
TextureManager::TextureManager()
:
  m_error_texture(create_error_texture())
{}

//==============================================================================
void TextureManager::init()
{
  m_test_enemy_texture = create(ResLifetime::Game, "content/textures/mff_pepe_walk.png");
  m_test_gun_texture = create(ResLifetime::Game, "content/textures/math_gun.png");
  m_test_gun2_texture = create(ResLifetime::Game, "content/textures/mage_gun.png");
  m_test_plasma_texture = create(ResLifetime::Game, "content/textures/plasma_ball.png");
}

//==============================================================================
std::vector<TextureHandle>& TextureManager::get_storage(ResLifetime lifetime)
{
  nc_assert(lifetime == ResLifetime::Game || lifetime == ResLifetime::Level);

  if (lifetime == ResLifetime::Level)
    return m_level_textures;
  else
    return m_level_textures;
}

//==============================================================================
TextureHandle TextureManager::create_error_texture()
{
  constexpr u32 size = 1024;
  constexpr u32 channels = 3;

  std::vector<unsigned char> data(size * size * channels);
  // black-magenta checkerboard pattern
  for (u32 y = 0; y < size; ++y)
  {
    for (u32 x = 0; x < size; ++x)
    {
      u32 index = (y * size + x) * channels;
      if ((y / 64 + x / 64) % 2 == 0)
      {
        // black
        data[index]     = 0; // R
        data[index + 1] = 0; // G
        data[index + 2] = 0; // B
      }
      else
      {
        // magenta
        data[index]     = 255; // R
        data[index + 1] = 0;   // G
        data[index + 2] = 255; // B
      }
    }
  }

  GLuint gl_handle = 0;
  glGenTextures(1, &gl_handle);
  glBindTexture(GL_TEXTURE_2D, gl_handle);

  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size, size, 0, GL_RGB, GL_UNSIGNED_BYTE, data.data());
  glGenerateMipmap(GL_TEXTURE_2D);

  glBindTexture(GL_TEXTURE_2D, 0);

  return TextureHandle{ResLifetime::Game, gl_handle, size, size};
}

}
