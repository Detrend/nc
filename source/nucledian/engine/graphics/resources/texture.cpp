#include <common.h>
#include <logging.h>
#include <engine/graphics/resources/texture.h>

#include <glad/glad.h>
#include <stb/stb_image.h>

#include <filesystem>

namespace nc
{

//==============================================================================
ResLifetime TextureHandle::get_lifetime() const
{
  return m_lifetime;
}

//==============================================================================
u32 TextureHandle::get_x() const
{
  return m_x;
}

//==============================================================================
u32 TextureHandle::get_y() const
{
  return m_y;
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
const TextureAtlas& TextureHandle::get_atlas() const
{
  return TextureManager::get().get_atlas(m_lifetime);
}

//==============================================================================
u16 TextureHandle::get_texture_id() const
{
  return m_texture_id;
}

//==============================================================================
vec2 TextureHandle::get_pos() const
{
  return vec2(m_x, m_y);
}

//==============================================================================
vec2 TextureHandle::get_size() const
{
  return vec2(m_width, m_height);
}

//==============================================================================
TextureHandle TextureHandle::invalid()
{
  return TextureHandle();
}

//==============================================================================
TextureHandle::TextureHandle(ResLifetime lifetime, u32 x, u32 y, u32 width, u32 height, u16 generation, u16 texture_id)
:
  m_lifetime(lifetime),
  m_generation(generation),
  m_texture_id(texture_id),
  m_x(x),
  m_y(y),
  m_width(width),
  m_height(height)
{}

//==============================================================================
vec2 TextureAtlas::get_size() const
{
  return vec2(width, height);
}

//==============================================================================
TextureManager& TextureManager::get()
{
  if (m_instance == nullptr)
  {
    m_instance = std::unique_ptr<TextureManager>(new TextureManager());
  }

  return *m_instance;
}

//==============================================================================
void TextureManager::load_directory(ResLifetime lifetime, const std::string& path)
{
  for (const auto& entry : std::filesystem::directory_iterator(path))
  {
    if (!entry.is_regular_file())
      continue;

    load(entry.path().string());
  }

  finish_load(lifetime);
}

//==============================================================================
void TextureManager::unload(ResLifetime lifetime)
{
  nc_assert(lifetime == ResLifetime::Game || lifetime == ResLifetime::Level);

  auto& atlas = get_atlas_mut(lifetime);

  glDeleteTextures(1, &atlas.handle);
  atlas.handle = 0;
  atlas.textures.clear();

  m_generation++;
}

//==============================================================================
const TextureAtlas& TextureManager::get_atlas(ResLifetime lifetime) const
{
  nc_assert(lifetime == ResLifetime::Game || lifetime == ResLifetime::Level);

  if (lifetime == ResLifetime::Game)
    return m_game_atlas;
  else
    return m_level_atlas;
}

//==============================================================================
GLuint TextureManager::get_error_texture_handle() const
{
  return m_error_texture;
}

//==============================================================================
const std::vector<TextureHandle>& TextureManager::get_textures() const
{
  return m_textures;
}

//==============================================================================
const TextureHandle& TextureManager::operator[](const std::pair<const std::string&, ResLifetime> pair) const
{
  return get_atlas(pair.second).textures.at(pair.first);
}

//==============================================================================
const TextureHandle& TextureManager::operator[](const std::string& name) const
{
  return operator[](std::make_pair(name, ResLifetime::Game));
}

const TextureHandle& TextureManager::operator[](u16 texture_id) const
{
  nc_assert(texture_id < m_textures.size(), "Texture ID is out of range.");
  return m_textures[texture_id];
}

//==============================================================================
TextureManager::TextureManager()
{
  create_error_texture();
}

//==============================================================================
TextureAtlas& TextureManager::get_atlas_mut(ResLifetime lifetime)
{
  nc_assert(lifetime == ResLifetime::Game || lifetime == ResLifetime::Level);

  if (lifetime == ResLifetime::Game)
    return m_game_atlas;
  else
    return m_level_atlas;
}

//==============================================================================
void TextureManager::create_error_texture()
{
  constexpr u32 channels = 3;

  std::vector<unsigned char> data(ERROR_TEXTURE_SIZE * ERROR_TEXTURE_SIZE * channels);
  // black-magenta checkerboard pattern
  for (u32 y = 0; y < ERROR_TEXTURE_SIZE; ++y)
  {
    for (u32 x = 0; x < ERROR_TEXTURE_SIZE; ++x)
    {
      u32 index = (y * ERROR_TEXTURE_SIZE + x) * channels;
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

  glGenTextures(1, &m_error_texture);
  glBindTexture(GL_TEXTURE_2D, m_error_texture);

  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, ERROR_TEXTURE_SIZE, ERROR_TEXTURE_SIZE, 0, GL_RGB, GL_UNSIGNED_BYTE, data.data());
  glGenerateMipmap(GL_TEXTURE_2D);

  glBindTexture(GL_TEXTURE_2D, 0);
}

//==============================================================================
void nc::TextureManager::load(const std::string& path)
{
  int width, height, channels;
  unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, 0);
  if (data == nullptr)
  {
    nc_crit("Cannot load image \"{}\": {}", path, stbi_failure_reason());
    stbi_image_free(data);
  }

  GLenum format = 0;
  if (channels == 3)
    format = GL_RGB;
  else if (channels == 4)
    format = GL_RGBA;
  else
  {
    nc_crit("Cannot load image \"{}\": {}", path, "Texture format not supported.");
    stbi_image_free(data);
  }

  const size_t last_separator_pos = path.find_last_of("/\\");
  const std::string filename = (last_separator_pos != std::string::npos) ? path.substr(last_separator_pos + 1) : path;
  const size_t first_dot_pos = filename.find('.');
  const std::string name = (first_dot_pos != std::string::npos) ? filename.substr(0, first_dot_pos) : filename;

  m_load_rects.push_back(stbrp_rect
    {
      .id = static_cast<int>(m_load_rects.size()),
      .w = width,
      .h = height,
      .x = 0,
      .y = 0,
      .was_packed = 0,
    });
  m_load_data.push_back(LoadData
    {
      .width = width,
      .height = height,
      .format = format,
      .data = data,
      .name = name,
    });
}

//==============================================================================
void TextureManager::finish_load(ResLifetime lifetime)
{
  static constexpr u16 DEFAULT_LOAD_TARGET_SIZE = 256;

  nc_assert(lifetime == ResLifetime::Game || lifetime == ResLifetime::Level);

  int target_width = DEFAULT_LOAD_TARGET_SIZE;
  int target_height = DEFAULT_LOAD_TARGET_SIZE;

  while (true)
  {
    stbrp_context context;
    std::vector<stbrp_node> nodes(target_width);

    stbrp_init_target(&context, target_width, target_height, nodes.data(), static_cast<int>(nodes.size()));
    stbrp_pack_rects(&context, m_load_rects.data(), static_cast<int>(m_load_rects.size()));

    bool all_packed = true;
    for (const auto& rect : m_load_rects)
    {
      if (rect.was_packed == 0)
      {
        all_packed = false;
        break;
      }
    }
    if (all_packed)
      break;

    if (target_height < target_width)
      target_height <<= 1;
    else
      target_width <<= 1;
  }

  GLuint gl_handle = 0;
  glGenTextures(1, &gl_handle);
  glBindTexture(GL_TEXTURE_2D, gl_handle);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, target_width, target_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  auto& atlas = get_atlas_mut(lifetime);
  atlas.handle = gl_handle;
  atlas.width = target_width;
  atlas.height = target_height;

  for (u32 i = 0; i < m_load_rects.size(); ++i)
  {
    const auto& rect = m_load_rects[i];
    const auto& load_data = m_load_data[i];

    glTexSubImage2D(
      GL_TEXTURE_2D,
      0,
      rect.x,
      rect.y,
      rect.w,
      rect.h,
      load_data.format,
      GL_UNSIGNED_BYTE,
      load_data.data
    );
    stbi_image_free(load_data.data);

    const TextureHandle handle(
      lifetime,
      rect.x,
      rect.y,
      rect.w,
      rect.h,
      m_generation,
      static_cast<u16>(m_textures.size())
    );

    atlas.textures.emplace(load_data.name, handle);
    m_textures.push_back(handle);
  }

  m_load_rects.clear();
  m_load_data.clear();
}

}
