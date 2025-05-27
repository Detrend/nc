#include <common.h>
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
u32 TextureHandle::get_channels() const
{
  return m_channels;
}

//==============================================================================
TextureHandle TextureHandle::invalid()
{
  return TextureHandle();
}

//==============================================================================
TextureHandle::TextureHandle(ResLifetime lifetime, GLuint gl_handle, u32 width, u32 height, u32 channels)
:
  m_lifetime(lifetime),
  m_gl_handle(gl_handle),
  m_width(width),
  m_height(height),
  m_channels(channels),
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
  std::cout << std::filesystem::current_path() << std::endl;

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
    // TODO: Use error texture.
    // TODO: Log error that texture could not be loaded (stbi_failure_reason()).
    glDeleteTextures(1, &gl_handle);
    return TextureHandle::invalid();
  }

  GLenum format = 0;
  if (channels == 3)
    format = GL_RGB;
  else if (channels == 4)
    format = GL_RGBA;
  else
  {
    // TOOD: Log error with unssported formt
    glDeleteTextures(1, &gl_handle);
    stbi_image_free(data);
    return TextureHandle::invalid();
  }

  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, format, GL_UNSIGNED_BYTE, data);
  glGenerateMipmap(GL_TEXTURE_2D);

  stbi_image_free(data);

  return TextureHandle{
    lifetime,
    gl_handle,
    static_cast<u32>(width),
    static_cast<u32>(height),
    static_cast<u32>(channels)
  };
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
const TextureHandle& TextureManager::get_test_enemy_texture()
{
  return m_test_enemy_texture;
}

//==============================================================================
const TextureHandle& TextureManager::get_test_gun_texture()
{
  return m_test_gun_texture;
}

//==============================================================================
const TextureHandle& TextureManager::get_test_plasma_texture()
{
  return m_test_plasma_texture;
}

//==============================================================================
TextureManager::TextureManager()
:
  m_test_enemy_texture(create(ResLifetime::Game, "content/textures/mff_pepe_walk.png")),
  m_test_gun_texture(create(ResLifetime::Game, "content/textures/math_gun.png")),
  m_test_plasma_texture(create(ResLifetime::Game, "content/textures/plasma_ball.png"))
{}

//==============================================================================
std::vector<TextureHandle>& TextureManager::get_storage(ResLifetime lifetime)
{
  nc_assert(lifetime == ResLifetime::Game || lifetime == ResLifetime::Level);

  if (lifetime == ResLifetime::Level)
    return m_level_textures;
  else
    return m_level_textures;
}

}
