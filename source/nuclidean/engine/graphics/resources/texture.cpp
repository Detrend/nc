// Project Nuclidean Source File
#include <engine/graphics/resources/texture.h>

#include <common.h>
#include <logging.h>

#include <glad/glad.h>
#include <stb/stb_image.h>

#include <algorithm>
#include <array>
#include <vector>

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
const TextureAtlasBundle& TextureHandle::get_atlas_bundle() const
{
  return TextureManager::get().get_atlas_bundle(m_lifetime);
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
TextureGPU TextureHandle::get_gpu_data() const
{
    return TextureGPU
    {
      .pos = get_pos(),
      .size = get_size(),
      .in_game_atlas = (get_lifetime() == ResLifetime::Game ? 1.0f : 0.0f),
    };
}

//==============================================================================
TextureHandle TextureHandle::invalid()
{
  return TextureHandle();
}

//==============================================================================
TextureHandle::TextureHandle
(
  ResLifetime lifetime, 
  u32 x, 
  u32 y, 
  u32 width, 
  u32 height, 
  u16 generation, 
  TextureID texture_id
)
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
vec2 TextureAtlasBundle::get_size() const
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
  for (const auto& entry : std::filesystem::recursive_directory_iterator(path))
  {
    const std::filesystem::path& entry_path = entry.path();

    if (!entry_path.has_extension())
      continue;

    const std::filesystem::path extension = entry_path.extension();
    if (extension == ".png" || extension == ".jpg")
    {
      const std::string stem = entry_path.stem().string();
      if (stem.ends_with("_normal") || stem.ends_with("_specular") || stem.ends_with("_emissive"))
        continue;

      load_texture(entry_path);
    }
    // not only cube maps have .hdr extension, but for now we are using HDR textures only for cube maps
    else if (extension == ".hdr")
    {
      load_equirectangular_map(entry_path.string(), lifetime);
    }
  }

  finish_load(lifetime);
}

//==============================================================================
void TextureManager::unload(ResLifetime lifetime)
{
  nc_assert(lifetime == ResLifetime::Game || lifetime == ResLifetime::Level);

  auto& bundle = get_atlas_bundle_mut(lifetime);

  glDeleteTextures(4, &bundle.diffuse_handle);
  bundle.diffuse_handle = 0;
  bundle.normal_handle = 0;
  bundle.specular_handle = 0;
  bundle.emissive_handle = 0;
  bundle.textures.clear();

  m_generation++;

  // TODO: delete texture from m_textures
  // TODO: delete sky boxes
}

//==============================================================================
const TextureAtlasBundle& TextureManager::get_atlas_bundle(ResLifetime lifetime) const
{
  nc_assert(lifetime == ResLifetime::Game || lifetime == ResLifetime::Level);

  if (lifetime == ResLifetime::Game)
    return m_game_atlas_bundle;
  else
    return m_level_atlas_bundle;
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
  return get_atlas_bundle(pair.second).textures.at(pair.first);
}

//==============================================================================
const TextureHandle& TextureManager::operator[](const std::string& name) const
{
  return operator[](std::make_pair(name, ResLifetime::Game));
}

//==============================================================================
const TextureHandle& TextureManager::operator[](u16 texture_id) const
{
  nc_assert(texture_id < m_textures.size(), "Texture ID is out of range.");
  return m_textures[texture_id];
}

//==============================================================================
GLuint TextureManager::get_equirectangular_map(const std::string& name, ResLifetime lifetime) const
{
  return get_equirectangular_maps(lifetime).at(name);
}

//==============================================================================
TextureManager::TextureManager()
{
  create_error_texture();
}

//==============================================================================
TextureAtlasBundle& TextureManager::get_atlas_bundle_mut(ResLifetime lifetime)
{
  nc_assert(lifetime == ResLifetime::Game || lifetime == ResLifetime::Level);

  if (lifetime == ResLifetime::Game)
    return m_game_atlas_bundle;
  else
    return m_level_atlas_bundle;
}

//==============================================================================
TextureManager::EquirectangularMapMap& TextureManager::get_equirectangular_maps(ResLifetime lifetime)
{
  nc_assert(lifetime == ResLifetime::Game || lifetime == ResLifetime::Level);

  if (lifetime == ResLifetime::Game)
    return m_game_equirectangular_maps;
  else
    return m_level_equirectangular_maps;
}

//==============================================================================
const TextureManager::EquirectangularMapMap& TextureManager::get_equirectangular_maps(ResLifetime lifetime) const
{
  nc_assert(lifetime == ResLifetime::Game || lifetime == ResLifetime::Level);

  if (lifetime == ResLifetime::Game)
    return m_game_equirectangular_maps;
  else
    return m_level_equirectangular_maps;
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
std::string TextureManager::get_name(const std::string& path) const
{
  const size_t last_separator_pos = path.find_last_of("/\\");
  const std::string filename = (last_separator_pos != std::string::npos) ? path.substr(last_separator_pos + 1) : path;
  const size_t first_dot_pos = filename.find('.');
  const std::string name = (first_dot_pos != std::string::npos) ? filename.substr(0, first_dot_pos) : filename;

  return name;
}

//==============================================================================
static GLenum gl_format_from_channels(int channels)
{
  switch (channels)
  {
    case 1:  return GL_RED;
    case 2:  return GL_RG;
    case 3:  return GL_RGB;
    case 4:  return GL_RGBA;
    default: return 0;
  }
}

//==============================================================================
static constexpr u32 ATLAS_MAX_MIP_LEVEL = 4;
static constexpr u32 ATLAS_GUTTER = 1u << ATLAS_MAX_MIP_LEVEL;

//==============================================================================
static std::vector<unsigned char> pad_with_edge_extend(
  const unsigned char* source, int width, int height, u32 channels, u32 gutter)
{
  const u32 padded_width  = cast<u32>(width)  + gutter * 2;
  const u32 padded_height = cast<u32>(height) + gutter * 2;
  std::vector<unsigned char> padded(cast<size_t>(padded_width) * padded_height * channels);

  for (u32 y = 0; y < padded_height; ++y)
  {
    const int source_y = std::clamp(cast<int>(y) - cast<int>(gutter), 0, height - 1);
    for (u32 x = 0; x < padded_width; ++x)
    {
      const int source_x = std::clamp(cast<int>(x) - cast<int>(gutter), 0, width - 1);
      const unsigned char* src = source + (cast<size_t>(source_y) * width + source_x) * channels;
      unsigned char* dst = padded.data() + (cast<size_t>(y) * padded_width + x) * channels;
      for (u32 c = 0; c < channels; ++c)
        dst[c] = src[c];
    }
  }

  return padded;
}

//==============================================================================
void TextureManager::load_texture(const std::filesystem::path& path)
{
  // TODO: use error texture when loading fails

  const std::string path_string = path.string();
  const std::string path_stem = path.stem().string();
  const std::string path_extension = path.extension().string();

  int width, height, channels;
  unsigned char* diffuse_data = stbi_load(path_string.c_str(), &width, &height, &channels, 0);
  if (diffuse_data == nullptr)
  {
    nc_crit("Cannot load texture \"{}\": {}", path_string, stbi_failure_reason());
    return;
  }

  const std::filesystem::path parent = path.parent_path();
  const std::array<std::string, 3> texture_paths
  {
    (parent / (path_stem + "_normal"   + path_extension)).string(),
    (parent / (path_stem + "_specular" + path_extension)).string(),
    (parent / (path_stem + "_emissive" + path_extension)).string(),
  };
  std::array<unsigned char*, 3> texture_data{};
  std::array<int, 3> texture_channels{};
  for (size_t i = 0; i < texture_paths.size(); ++i)
  {
    if (!std::filesystem::exists(texture_paths[i]))
      continue;

    int aux_width, aux_height, aux_channels;
    texture_data[i] = stbi_load(texture_paths[i].c_str(), &aux_width, &aux_height, &aux_channels, 0);
    if (texture_data[i] == nullptr)
    {
      nc_crit("Cannot load texture \"{}\": {}", texture_paths[i], stbi_failure_reason());
      continue;
    }
    if (aux_width != width || aux_height != height)
    {
      nc_crit(
        "Auxiliary texture \"{}\" dimensions ({}x{}) do not match base \"{}\" ({}x{}). Skipping.",
        texture_paths[i], aux_width, aux_height, path_string, width, height);
      stbi_image_free(texture_data[i]);
      texture_data[i] = nullptr;
      continue;
    }
    texture_channels[i] = aux_channels;
  }

  const GLenum diffuse_format = gl_format_from_channels(channels);
  if (diffuse_format != GL_RGB && diffuse_format != GL_RGBA)
  {
    nc_crit("Cannot load image \"{}\": {}", path_string, "Texture format not supported.");
    stbi_image_free(diffuse_data);
    for (unsigned char* data : texture_data)
      stbi_image_free(data);
    return;
  }

  m_load_rects.push_back(stbrp_rect
  {
    .id = cast<int>(m_load_rects.size()),
    .w = width  + cast<int>(ATLAS_GUTTER) * 2,
    .h = height + cast<int>(ATLAS_GUTTER) * 2,
    .x = 0,
    .y = 0,
    .was_packed = 0,
  });
  m_load_data.push_back(LoadData
  {
    .width = width,
    .height = height,
    .diffuse_channels  = channels,
    .normal_channels   = texture_channels[0],
    .specular_channels = texture_channels[1],
    .emissive_channels = texture_channels[2],
    .diffuse_data = diffuse_data,
    .normal_data = texture_data[0],
    .specular_data = texture_data[1],
    .emissive_data = texture_data[2],
    .name = get_name(path_string),
  });
}

//==============================================================================
void TextureManager::load_equirectangular_map(const std::string& path, ResLifetime lifetime)
{
  // load equirectangular map texture
  int width, height, channels;
  float* data = stbi_loadf(path.c_str(), &width, &height, &channels, 3);
  if (data == nullptr)
    nc_crit("Cannot load cube map \"{}\": {}", path, stbi_failure_reason());

  GLuint handle;
  glGenTextures(1, &handle);
  glBindTexture(GL_TEXTURE_2D, handle);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, data);
  
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  stbi_image_free(data);

  EquirectangularMapMap& maps = get_equirectangular_maps(lifetime);
  maps.insert({ get_name(path), handle });
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

    stbrp_init_target(&context, target_width, target_height, nodes.data(), cast<int>(nodes.size()));
    stbrp_pack_rects(&context, m_load_rects.data(), cast<int>(m_load_rects.size()));

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

  auto& bundle = get_atlas_bundle_mut(lifetime);
  bundle.width = target_width;
  bundle.height = target_height;

  // diffuse texture atlas
  {
    glGenTextures(1, &bundle.diffuse_handle);
    glBindTexture(GL_TEXTURE_2D, bundle.diffuse_handle);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, target_width, target_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, ATLAS_MAX_MIP_LEVEL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    for (u32 i = 0; i < m_load_rects.size(); ++i)
    {
      const auto& rect = m_load_rects[i];
      const auto& load_data = m_load_data[i];

      const std::vector<unsigned char> padded = pad_with_edge_extend(
        load_data.diffuse_data, load_data.width, load_data.height,
        cast<u32>(load_data.diffuse_channels), ATLAS_GUTTER);

      glTexSubImage2D(
        GL_TEXTURE_2D,
        0,
        rect.x,
        rect.y,
        rect.w,
        rect.h,
        gl_format_from_channels(load_data.diffuse_channels),
        GL_UNSIGNED_BYTE,
        padded.data()
      );
      stbi_image_free(load_data.diffuse_data);

      const TextureHandle handle(
        lifetime,
        rect.x + ATLAS_GUTTER,
        rect.y + ATLAS_GUTTER,
        load_data.width,
        load_data.height,
        m_generation,
        cast<TextureID>(m_textures.size())
      );

      bundle.textures.emplace(load_data.name, handle);
      m_textures.push_back(handle);
    }

    glGenerateMipmap(GL_TEXTURE_2D);
  }

  // normal texture atlas
  {
    glGenTextures(1, &bundle.normal_handle);
    glBindTexture(GL_TEXTURE_2D, bundle.normal_handle);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, target_width, target_height, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, ATLAS_MAX_MIP_LEVEL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    std::vector<unsigned char> identity(cast<size_t>(target_width * target_height * 3));
    for (size_t texel = 0; texel < identity.size(); texel += 3)
    {
      identity[texel]     = 128;
      identity[texel + 1] = 128;
      identity[texel + 2] = 255;
    }
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, target_width, target_height, GL_RGB, GL_UNSIGNED_BYTE, identity.data());

    for (u32 i = 0; i < m_load_rects.size(); ++i)
    {
      const auto& rect = m_load_rects[i];
      const auto& load_data = m_load_data[i];

      if (load_data.normal_data == nullptr)
        continue;

      const std::vector<unsigned char> padded = pad_with_edge_extend(
        load_data.normal_data, load_data.width, load_data.height,
        cast<u32>(load_data.normal_channels), ATLAS_GUTTER);

      glTexSubImage2D(
        GL_TEXTURE_2D,
        0,
        rect.x,
        rect.y,
        rect.w,
        rect.h,
        gl_format_from_channels(load_data.normal_channels),
        GL_UNSIGNED_BYTE,
        padded.data()
      );
      stbi_image_free(load_data.normal_data);
    }

    glGenerateMipmap(GL_TEXTURE_2D);
  }

  // specular texture atlas
  {
    glGenTextures(1, &bundle.specular_handle);
    glBindTexture(GL_TEXTURE_2D, bundle.specular_handle);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, target_width, target_height, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, ATLAS_MAX_MIP_LEVEL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    const std::vector<unsigned char> zero(cast<size_t>(target_width * target_height), 0);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, target_width, target_height, GL_RED, GL_UNSIGNED_BYTE, zero.data());

    for (u32 i = 0; i < m_load_rects.size(); ++i)
    {
      const auto& rect = m_load_rects[i];
      const auto& load_data = m_load_data[i];

      if (load_data.specular_data == nullptr)
        continue;

      const std::vector<unsigned char> padded = pad_with_edge_extend(
        load_data.specular_data, load_data.width, load_data.height,
        cast<u32>(load_data.specular_channels), ATLAS_GUTTER);

      glTexSubImage2D(
        GL_TEXTURE_2D,
        0,
        rect.x,
        rect.y,
        rect.w,
        rect.h,
        gl_format_from_channels(load_data.specular_channels),
        GL_UNSIGNED_BYTE,
        padded.data()
      );
      stbi_image_free(load_data.specular_data);
    }

    glGenerateMipmap(GL_TEXTURE_2D);
  }

  // emissive texture atlas
  {
    glGenTextures(1, &bundle.emissive_handle);
    glBindTexture(GL_TEXTURE_2D, bundle.emissive_handle);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, target_width, target_height, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    for (u32 i = 0; i < m_load_rects.size(); ++i)
    {
      const auto& rect = m_load_rects[i];
      const auto& load_data = m_load_data[i];

      if (load_data.emissive_data == nullptr)
        continue;

      const std::vector<unsigned char> padded = pad_with_edge_extend(
        load_data.emissive_data, load_data.width, load_data.height,
        cast<u32>(load_data.emissive_channels), ATLAS_GUTTER);

      glTexSubImage2D(
        GL_TEXTURE_2D,
        0,
        rect.x,
        rect.y,
        rect.w,
        rect.h,
        gl_format_from_channels(load_data.emissive_channels),
        GL_UNSIGNED_BYTE,
        padded.data()
      );
      stbi_image_free(load_data.emissive_data);
    }
  }

  m_load_rects.clear();
  m_load_data.clear();
}

}
