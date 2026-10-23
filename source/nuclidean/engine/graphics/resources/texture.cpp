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

namespace nc
{

//==============================================================================
namespace
{
constexpr cstr ERROR_TEXTURE_NAME       = "__nc_error_texture__";
constexpr u32  ERROR_TEXTURE_ATLAS_SIZE = 64;
constexpr u32  ERROR_TEXTURE_CELL_SIZE  = 16;
}

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
u32 TextureHandle::get_array_idx() const
{
  return m_array_idx;
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
      .atlas_idx = static_cast<float>(get_array_idx())
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
  u32 array_idx,
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
  m_height(height),
  m_array_idx(array_idx)
{}

//==============================================================================
bool TextureHandle::is_valid() const
{
  if (m_lifetime == ResLifetime::Game && m_generation != TextureManager::m_generation)
  {
    return false;
  }

  return m_lifetime != ResLifetime::None && m_texture_id != INVALID_TEXTURE_ID;
}

//==============================================================================
TextureHandle::operator bool() const
{
  return is_valid();
}

//==============================================================================
vec2 TextureAtlasBundle::get_size() const
{
  return vec2(width, height);
}

//==============================================================================
TextureManager& TextureManager::get()
{
  static TextureManager instance;
  return instance;
}

//==============================================================================
void TextureManager::load_directory(ResLifetime lifetime, const std::string& path)
{
  auto _scratch_scope = ScratchAllocator::get().create_scope("TextureManager::load_directory()", 384_mb); // Give scratch a big chunk of memory that surely fits all the textures
  LoadContext ctx; // scratch-allocated stuff lives hear

  bool did_use_packed = false;
  if (lifetime == ResLifetime::Game) { // TODO: make general

    auto path_packed = std::filesystem::path(path + "-packed") / "game";
    std::filesystem::path path_packed_manifest = path_packed / "textures.json";

    if (std::filesystem::exists(path_packed_manifest)) {
      did_use_packed = true;

      this->load_prebaked_atlases(lifetime, ctx, path_packed.string());
    }
  }

  for (const auto& entry : std::filesystem::recursive_directory_iterator(path))
  {
    const std::filesystem::path& entry_path = entry.path();

    if (!entry_path.has_extension())
      continue;

    const std::filesystem::path extension = entry_path.extension();
    if ((! did_use_packed) && (extension == ".png" || extension == ".jpg"))
    {
      const std::string stem = entry_path.stem().string();
      if (stem.ends_with("_normal") || stem.ends_with("_specular") || stem.ends_with("_emissive"))
        continue;

      load_texture(entry_path, ctx);
    }
    // not only cube maps have .hdr extension, but for now we are using HDR textures only for cube maps
    else if (extension == ".hdr")
    {
      load_equirectangular_map(entry_path.string(), lifetime);
    }
  }

  if(! did_use_packed)
    finish_load(lifetime, ctx);
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
const std::vector<TextureHandle>& TextureManager::get_textures() const
{
  return m_textures;
}

//==============================================================================
const TextureHandle& TextureManager::operator[](const std::pair<const std::string&, ResLifetime> pair) const
{
  const TextureMap& textures = get_atlas_bundle(pair.second).textures;

  if (const auto it = textures.find(pair.first); it != textures.end())
    return it->second;

  nc_warn
  (
    "Texture \"{}\" not found in {} atlas. Using error texture.",
    pair.first, pair.second == ResLifetime::Game ? "game" : "level"
  );
  return m_error_texture_handle;
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
static scratch_vector<unsigned char> pad_with_edge_extend(
  const unsigned char* source, int width, int height, u32 channels, u32 gutter)
{
  const u32 padded_width  = cast<u32>(width)  + gutter * 2;
  const u32 padded_height = cast<u32>(height) + gutter * 2;
  scratch_vector<unsigned char> padded(cast<size_t>(padded_width) * padded_height * channels);

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
void TextureManager::load_texture(const std::filesystem::path& path, LoadContext& ctx)
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

  ctx.load_rects.push_back(stbrp_rect
  {
    .id = cast<int>(ctx.load_rects.size()),
    .w = width  + cast<int>(ATLAS_GUTTER) * 2,
    .h = height + cast<int>(ATLAS_GUTTER) * 2,
    .x = 0,
    .y = 0,
    .was_packed = 0,
  });
  ctx.load_data.push_back(LoadData
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
void TextureManager::push_error_texture_load(LoadContext& ctx)
{
  constexpr u32 channels = 3;
  constexpr u32 size     = ERROR_TEXTURE_ATLAS_SIZE;

  auto* data = cast<unsigned char*>(scratch_malloc_safe(cast<size_t>(size) * size * channels, 16));

  for (u32 y = 0; y < size; ++y)
  {
    for (u32 x = 0; x < size; ++x)
    {
      const u32  index   = (y * size + x) * channels;
      const bool magenta = ((y / ERROR_TEXTURE_CELL_SIZE) + (x / ERROR_TEXTURE_CELL_SIZE)) % 2 != 0;
      data[index]     = magenta ? 255 : 0;
      data[index + 1] = 0;
      data[index + 2] = magenta ? 255 : 0;
    }
  }

  ctx.load_rects.push_back(stbrp_rect
  {
    .id = cast<int>(ctx.load_rects.size()),
    .w  = cast<int>(size) + cast<int>(ATLAS_GUTTER) * 2,
    .h  = cast<int>(size) + cast<int>(ATLAS_GUTTER) * 2,
    .x  = 0,
    .y  = 0,
    .was_packed = 0,
  });
  ctx.load_data.push_back(LoadData
  {
    .width             = cast<int>(size),
    .height            = cast<int>(size),
    .diffuse_channels  = cast<int>(channels),
    .normal_channels   = 0,
    .specular_channels = 0,
    .emissive_channels = 0,
    .diffuse_data      = data,
    .normal_data       = nullptr,
    .specular_data     = nullptr,
    .emissive_data     = nullptr,
    .name              = ERROR_TEXTURE_NAME,
  });
}

//==============================================================================

struct TextureAtlasLayoutParameters {
  GLint internal_format = GL_SRGB8_ALPHA8;
  GLenum format = GL_RGBA;
  GLenum data_type = GL_UNSIGNED_BYTE;
  GLint min_filter = GL_LINEAR_MIPMAP_LINEAR; // GL_TEXTURE_MIN_FILTER
  GLint mag_filter = GL_NEAREST;              // GL_TEXTURE_MAG_FILTER
  GLint max_mip_level = ATLAS_MAX_MIP_LEVEL;  // GL_TEXTURE_MAX_LEVEL
  GLint wrap_s = GL_CLAMP_TO_EDGE;        // GL_TEXTURE_WRAP_S
  GLint wrap_t = GL_CLAMP_TO_EDGE;        // GL_TEXTURE_WRAP_T

  void gen_texture(GLuint* const out_handle) const { glGenTextures(1, out_handle);}

  void config_texture_misc_params() const {
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, min_filter);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, mag_filter);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAX_LEVEL, max_mip_level);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, wrap_s);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, wrap_t);
  }

  void config_texture(GLuint handle, ivec3 dims) const {
    glBindTexture(GL_TEXTURE_2D_ARRAY, handle);
    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, internal_format, /*width:*/dims.x, /*height:*/dims.y, /*layers_count:*/dims.z, 0, format, data_type, nullptr);
    config_texture_misc_params();
  }

  void config_texture_compressed(GLuint handle, ivec3 dims, GLenum compressed_internal_format, GLsizei compressed_size_per_layer) const {
    glBindTexture(GL_TEXTURE_2D_ARRAY, handle);
    glCompressedTexImage3D(GL_TEXTURE_2D_ARRAY, 0, compressed_internal_format, dims.x, dims.y, dims.z, 0, compressed_size_per_layer * dims.z, nullptr);
    config_texture_misc_params();
  }

  void define_mipmaps(ivec3 dims) const {
    for (GLint mip = 1; mip <= max_mip_level; ++mip) {
      glTexImage3D(GL_TEXTURE_2D_ARRAY, mip, internal_format, dims.x >> mip, dims.y >> mip, dims.z, 0, format, data_type, nullptr);
    }
  }

  void define_mipmap_compressed(GLint mip, ivec3 dims, GLenum compressed_internal_format, GLsizei compressed_mip_size_per_layer) const {
    glCompressedTexImage3D(GL_TEXTURE_2D_ARRAY, mip, compressed_internal_format, dims.x >> mip, dims.y >> mip, dims.z, 0, compressed_mip_size_per_layer * dims.z, nullptr);
  }

  GLuint gen_and_config_texture(ivec3 dims) const {
    GLuint ret;
    gen_texture(&ret);
    config_texture(ret, dims);
    return ret;
  }

  GLuint gen_and_config_texture_compressed(ivec3 dims, GLenum compressed_internal_format, GLsizei compressed_size) const {
    GLuint ret;
    gen_texture(&ret);
    config_texture_compressed(ret, dims, compressed_internal_format, compressed_size);
    return ret;
  }

};
constexpr TextureAtlasLayoutParameters DIFFUSE_TEXTURE_PARAMS {
  .internal_format = GL_SRGB8_ALPHA8,
  .format = GL_RGBA,
  .data_type = GL_UNSIGNED_BYTE
};
constexpr TextureAtlasLayoutParameters NORMAL_TEXTURE_PARAMS {
  .internal_format = GL_RGB8,
  .format = GL_RGB,
  .data_type = GL_UNSIGNED_BYTE
};
constexpr TextureAtlasLayoutParameters SPECULAR_TEXTURE_PARAMS {
  .internal_format = GL_R8,
  .format = GL_RED,
  .data_type = GL_UNSIGNED_BYTE
};
constexpr TextureAtlasLayoutParameters EMISSIVE_TEXTURE_PARAMS {
  .internal_format = GL_RGB8,
  .format = GL_RGB,
  .data_type = GL_UNSIGNED_BYTE
};

//==============================================================================
void TextureManager::finish_load(ResLifetime lifetime, LoadContext& ctx)
{
  static constexpr u16 DEFAULT_LOAD_TARGET_SIZE = 4096;

  nc_assert(lifetime == ResLifetime::Game || lifetime == ResLifetime::Level);

  if (lifetime == ResLifetime::Game)
    push_error_texture_load(ctx);


  int target_width = DEFAULT_LOAD_TARGET_SIZE;
  int target_height = DEFAULT_LOAD_TARGET_SIZE;
  int layers_count = 0;

  {
    auto _scope = ScratchAllocator::get().create_scope("packing");
    scratch_vector<stbrp_rect> pending_rects_a = ctx.load_rects;
    scratch_vector<stbrp_rect> pending_rects_b;
    pending_rects_b.reserve(ctx.load_rects.size());

    scratch_vector<stbrp_rect>* pending_rects_in = &pending_rects_a;
    scratch_vector<stbrp_rect>* pending_rects_out = &pending_rects_b;

    while (true)
    {
      auto _scope_iteration = ScratchAllocator::get().create_scope("iteration");
      stbrp_context context;
      scratch_vector<stbrp_node> nodes(target_width);

      stbrp_init_target(&context, target_width, target_height, nodes.data(), cast<int>(nodes.size()));
      stbrp_pack_rects(&context, pending_rects_in->data(), cast<int>(pending_rects_in->size()));

      bool all_packed = true;
      for (const auto& rect : *pending_rects_in)
      {
        auto& data = ctx.load_data[rect.id];
        if (rect.was_packed == 0)
        {
          all_packed = false;
          pending_rects_out->emplace_back(rect);
        }
        else {
          data.texture_array_idx = layers_count;
          ctx.load_rects[rect.id] = rect;
        }
      }
      ++layers_count;
      if (all_packed)
        break;

      std::swap(*pending_rects_in, *pending_rects_out);
      pending_rects_out->clear();
    }
  }

  auto& bundle = get_atlas_bundle_mut(lifetime);
  bundle.width = target_width;
  bundle.height = target_height;
  bundle.layers_count = layers_count;

  GLint max_supported_texture_size;
  glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_supported_texture_size);
  nc_expect((target_width <= max_supported_texture_size) && (target_height <= max_supported_texture_size), 
    "Texture atlas size {}x{} exceeds maximum supported texture size {}.", 
    target_width, target_height, max_supported_texture_size
  );

  nc_log("Texture atlas for {} resources: {}x{} (max supported {})", lifetime == ResLifetime::Game ? "game" : "level", target_width, target_height, max_supported_texture_size);

  const ivec3 texture_dims(target_width, target_height, layers_count);
  // diffuse texture atlas
  {
    const auto& params = DIFFUSE_TEXTURE_PARAMS;
    bundle.diffuse_handle = params.gen_and_config_texture(texture_dims);

    for (u32 i = 0; i < ctx.load_rects.size(); ++i)
    {
      auto _scope = ScratchAllocator::get().create_scope("diffuse"); //free the padded data once finished
      const auto& rect = ctx.load_rects[i];
      const auto& load_data = ctx.load_data[rect.id];

      const scratch_vector<unsigned char> padded = pad_with_edge_extend(
        load_data.diffuse_data, load_data.width, load_data.height,
        cast<u32>(load_data.diffuse_channels), ATLAS_GUTTER);

      glTexSubImage3D(
        GL_TEXTURE_2D_ARRAY,
        0,
        rect.x,
        rect.y,
        load_data.texture_array_idx,
        rect.w,
        rect.h,
        1,
        gl_format_from_channels(load_data.diffuse_channels),
        params.data_type,
        padded.data()
      );
      stbi_image_free(load_data.diffuse_data);

      const TextureHandle handle(
        lifetime,
        rect.x + ATLAS_GUTTER,
        rect.y + ATLAS_GUTTER,
        load_data.width,
        load_data.height,
        load_data.texture_array_idx,
        m_generation,
        cast<TextureID>(m_textures.size())
      );

      bundle.textures.emplace(load_data.name, handle);
      m_textures.push_back(handle);

      if (load_data.name == ERROR_TEXTURE_NAME)
        m_error_texture_handle = handle;
    }

    glGenerateMipmap(GL_TEXTURE_2D_ARRAY);
  }

  // normal texture atlas
  {
    const auto& params = NORMAL_TEXTURE_PARAMS;
    bundle.normal_handle = params.gen_and_config_texture(texture_dims);

    {
      std::vector<unsigned char> identity(cast<size_t>(target_width* target_height * layers_count * 3)); // too big for scratch :(
      for (size_t texel = 0; texel < identity.size(); texel += 3)
      {
        identity[texel] = 128;
        identity[texel + 1] = 128;
        identity[texel + 2] = 255;
      }
      glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, 0, target_width, target_height, layers_count, params.format, params.data_type, identity.data());
    }

    for (u32 i = 0; i < ctx.load_rects.size(); ++i)
    {
      auto _scope = ScratchAllocator::get().create_scope("normal"); //free the padded data once finished
      const auto& rect = ctx.load_rects[i];
      const auto& load_data = ctx.load_data[rect.id];

      if (load_data.normal_data == nullptr)
        continue;

      const scratch_vector<unsigned char> padded = pad_with_edge_extend(
        load_data.normal_data, load_data.width, load_data.height,
        cast<u32>(load_data.normal_channels), ATLAS_GUTTER);

      glTexSubImage3D(
        GL_TEXTURE_2D_ARRAY,
        0,
        rect.x,
        rect.y,
        load_data.texture_array_idx,
        rect.w,
        rect.h,
        1,
        gl_format_from_channels(load_data.normal_channels),
        params.data_type,
        padded.data()
      );
      stbi_image_free(load_data.normal_data);
    }

    glGenerateMipmap(GL_TEXTURE_2D_ARRAY);
  }

  // specular texture atlas
  {
    const auto& params = SPECULAR_TEXTURE_PARAMS;
    bundle.specular_handle = params.gen_and_config_texture(texture_dims);

    {
      const std::vector<unsigned char> zero(cast<size_t>(target_width* target_height * layers_count), 0);
      glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, 0, target_width, target_height, layers_count, GL_RED, GL_UNSIGNED_BYTE, zero.data());
    }

    for (u32 i = 0; i < ctx.load_rects.size(); ++i)
    {
      auto _scope = ScratchAllocator::get().create_scope("specular"); //free the padded data once finished
      const auto& rect = ctx.load_rects[i];
      const auto& load_data = ctx.load_data[rect.id];

      if (load_data.specular_data == nullptr)
        continue;

      const scratch_vector<unsigned char> padded = pad_with_edge_extend(
        load_data.specular_data, load_data.width, load_data.height,
        cast<u32>(load_data.specular_channels), ATLAS_GUTTER);

      glTexSubImage3D(
        GL_TEXTURE_2D_ARRAY,
        0,
        rect.x,
        rect.y,
        load_data.texture_array_idx,
        rect.w,
        rect.h,
        1,
        gl_format_from_channels(load_data.specular_channels),
        params.data_type,
        padded.data()
      );
      stbi_image_free(load_data.specular_data);
    }

    glGenerateMipmap(GL_TEXTURE_2D_ARRAY);
  }

  // emissive texture atlas
  {
    const auto& params = EMISSIVE_TEXTURE_PARAMS;
    bundle.emissive_handle = params.gen_and_config_texture(texture_dims);

    for (u32 i = 0; i < ctx.load_rects.size(); ++i)
    {
      auto _scope = ScratchAllocator::get().create_scope("emissive"); //free the padded data once finished
      const auto& rect = ctx.load_rects[i];
      const auto& load_data = ctx.load_data[rect.id];

      if (load_data.emissive_data == nullptr)
        continue;

      const scratch_vector<unsigned char> padded = pad_with_edge_extend(
        load_data.emissive_data, load_data.width, load_data.height,
        cast<u32>(load_data.emissive_channels), ATLAS_GUTTER);

      glTexSubImage3D(
        GL_TEXTURE_2D_ARRAY,
        0,
        rect.x,
        rect.y,
        load_data.texture_array_idx,
        rect.w,
        rect.h,
        1,
        gl_format_from_channels(load_data.emissive_channels),
        params.data_type,
        padded.data()
      );
      stbi_image_free(load_data.emissive_data);
    }
    glGenerateMipmap(GL_TEXTURE_2D_ARRAY);
  }

  ctx.load_rects.clear();
  ctx.load_data.clear();
}


//==============================================================================

static u32 read_u24_le(const void*const bytes) {
  return static_cast<u32>(static_cast<const u8*>(bytes)[0])
    | (static_cast<u32>(static_cast<const u8*>(bytes)[1]) << 8)
    | (static_cast<u32>(static_cast<const u8*>(bytes)[2]) << 16)
    ;
}

struct AstcImgInfo
{
  u8 block_width, block_height, block_depth;
  u32 width, height, depth;
  GLenum format;
  size_t payload_size;
  unsigned char* data;
};


static bool load_astc(const std::filesystem::path& img_path, const FunctionReference<void(const AstcImgInfo&)>& fn) 
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

  AstcImgInfo info;

  info.block_width = static_cast<u8>(buf[4]);
  info.block_height = static_cast<u8>(buf[5]);
  info.block_depth = static_cast<u8>(buf[6]);
  info.width = read_u24_le(buf + 7);
  info.height = read_u24_le(buf + 10);
  info.depth = read_u24_le(buf + 13);
  nc_expect((info.block_depth == 1) && (info.depth == 1) && (info.width != 0) && (info.height != 0));

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

  GLenum gl_format = 0;
  for (const auto& format : ASTC_FORMATS) {
    if (format.block_width == info.block_width && format.block_height == info.block_height) {
      gl_format = format.gl_format_srgb;
      break;
    }
  }
  nc_expect(gl_format);

  const size_t blocks_x = (info.width + info.block_width - 1) / info.block_width;
  const size_t blocks_y = (info.height + info.block_height - 1) / info.block_height;
  const size_t expected_payload_size = blocks_x * blocks_y * 16;
  const size_t payload_size = buffer.size() - ASTC_HEADER_SIZE;
  nc_expect(payload_size == expected_payload_size);
  nc_expect(payload_size < std::numeric_limits<GLsizei>::max());

  info.format = gl_format;
  info.payload_size = payload_size;
  info.data = buf + ASTC_HEADER_SIZE;
  fn(info);

  return true;
}

void TextureManager::load_prebaked_atlases(ResLifetime lifetime, [[maybe_unused]]LoadContext& ctx, const std::string& path)
{
  auto& bundle = get_atlas_bundle_mut(lifetime);

  std::filesystem::path dir_path(path);
  std::filesystem::path cfg_path(dir_path / "textures.json");
  std::ifstream f(cfg_path);
  nc_assert(f.is_open(), "No atlas at path '{0}'!", std::filesystem::absolute(cfg_path).string());
  auto data_js = nlohmann::json::parse(f);
  auto dims_js = data_js["dims"];
  ivec3 atlas_dims(dims_js[0], dims_js[1], dims_js[2]);

  bundle.width = atlas_dims.x;
  bundle.height = atlas_dims.y;
  bundle.layers_count = atlas_dims.z;

  // Setup data for individual rectangles
  for (const auto& image_js : data_js["img"]) {
    const auto pos_js = image_js["pos"];
    const auto dim_js = image_js["dim"];
    const std::string name = std::string(image_js["name"]);
    const int layer = image_js["layer"];
    const ivec2 pos{ pos_js[0], pos_js[1] };
    const ivec2 dim{ dim_js[0], dim_js[1] };

    const TextureHandle handle(
      lifetime,
      pos.x,
      pos.y,
      dim.x,
      dim.y,
      layer,
      m_generation,
      cast<TextureID>(m_textures.size())
    );
    bundle.textures.emplace(name, handle);
    m_textures.emplace_back(handle);

    if (name == ERROR_TEXTURE_NAME)
      m_error_texture_handle = handle;
  }


  // Load the texture atlas pages
  auto load_page = [&](const std::string& name, const TextureAtlasLayoutParameters& params, GLuint& handle)->void {
      handle = params.gen_and_config_texture(atlas_dims);
      params.define_mipmaps(atlas_dims);

      for (GLint mip = 0; mip <= params.max_mip_level; ++mip) {
        for (GLuint layer = 0; layer < static_cast<GLuint>(atlas_dims.z); ++layer) {
          auto _sc = ScratchAllocator::get().create_scope();
          std::filesystem::path img_path = dir_path / std::format("{}_{}_mip{}.png", name, layer, mip);
          int x, y, channels;
          stbi_uc*const img = stbi_load(img_path.string().c_str(), &x, &y, &channels, 0);
          nc_expect(img);
          defer(stbi_image_free(img););
          nc_expect(x == (atlas_dims.x >> mip));
          nc_expect(y == (atlas_dims.y >> mip));
          
          glTexSubImage3D(GL_TEXTURE_2D_ARRAY, mip, 0, 0, layer, x, y, 1, params.format, params.data_type, img);
        }
      }
    };

  std::string compressed_extension = "astc";
  auto load_page_compressed = [&](const std::string& name, const TextureAtlasLayoutParameters& params, GLuint& handle)->void {

    bool was_texture_generated = false;

    for (GLint mip = 0; mip <= params.max_mip_level; ++mip) {
      bool was_mip_generated = false;
      for (GLuint layer = 0; layer < static_cast<GLuint>(atlas_dims.z); ++layer) {
        auto _sc = ScratchAllocator::get().create_scope();

        std::filesystem::path img_path = dir_path / std::format("{}_{}_mip{}.{}", name, layer, mip, compressed_extension);

        load_astc(img_path, [&](const AstcImgInfo& info)->void {
          nc_expect(info.width  == static_cast<u32>(atlas_dims.x >> mip));
          nc_expect(info.height == static_cast<u32>(atlas_dims.y >> mip));

          if (!was_texture_generated) {
            was_texture_generated = true;
            was_mip_generated = true;
            nc_assert(mip == 0);
            handle = params.gen_and_config_texture_compressed(atlas_dims, info.format, static_cast<GLsizei>(info.payload_size));
          }
          if (!was_mip_generated) {
            was_mip_generated = true;
            nc_assert(was_texture_generated && (mip > 0));
            params.define_mipmap_compressed(mip, atlas_dims, info.format, static_cast<GLsizei>(info.payload_size));
          }

          nc_expect(was_texture_generated && was_mip_generated);
          glCompressedTexSubImage3D(GL_TEXTURE_2D_ARRAY, mip, 0, 0, layer, atlas_dims.x >> mip, atlas_dims.y >> mip, 1, info.format, info.payload_size, info.data);
          //glTexSubImage3D(GL_TEXTURE_2D_ARRAY, mip, 0, 0, layer, x, y, 1, params.format, params.data_type, img);
        });

      }
    }
  };

  static_cast<void>(load_page);
  load_page_compressed("diffuse", DIFFUSE_TEXTURE_PARAMS, bundle.diffuse_handle);
  load_page_compressed("normal", NORMAL_TEXTURE_PARAMS, bundle.normal_handle);
  load_page_compressed("specular", SPECULAR_TEXTURE_PARAMS, bundle.specular_handle);
  load_page_compressed("emissive", EMISSIVE_TEXTURE_PARAMS, bundle.emissive_handle);
}


//==============================================================================

void TextureManager::dump_atlases(ResLifetime lifetime, const std::string& path)
{
  if (GraphicsSystem::get().get_backend() != RenderBackend::OpenGL) {
    nc_crit("Dumping textures can only be done on Open GL!");
    return;
  }

  nc_log("Dumping atlases to {}", path);
  std::filesystem::create_directories(path);
  
  auto _scope = ScratchAllocator::get().create_scope("dump_atlases", 512_mb);
  const TextureAtlasBundle& atlas_bundle = get_atlas_bundle(lifetime);

  auto dump_texture = [&](GLuint texture_handle, const std::string &name, int channels_count, const TextureAtlasLayoutParameters& params)->void {
    auto _sc = ScratchAllocator::get().create_scope();
    const size_t pixel_count_per_page = static_cast<size_t>(atlas_bundle.width) * atlas_bundle.height;
    const size_t pixel_count = pixel_count_per_page * atlas_bundle.layers_count;
    const size_t size_in_bytes_per_page = pixel_count_per_page * channels_count;
    const size_t size_in_bytes = pixel_count * channels_count;
    unsigned char* buffer = static_cast<unsigned char*>(scratch_malloc(size_in_bytes, 16));
    defer(scratch_delete(buffer););
    nc_assert(buffer);

    for (GLint mip = 0; mip <= params.max_mip_level; ++mip) {
      const GLsizei current_size_in_bytes = static_cast<GLsizei>(size_in_bytes) >> (2 * mip);
      const GLsizei current_size_in_bytes_per_page = static_cast<GLsizei>(size_in_bytes_per_page) >> (2 * mip);
      glGetTextureImage(texture_handle, mip, params.format, params.data_type, current_size_in_bytes, buffer);

      for (u32 i = 0; i < atlas_bundle.layers_count; ++i) {
        unsigned char* start = buffer + (i * current_size_in_bytes_per_page);
        auto save_path = scratch_format("{}/{}_{}_mip{}.png", path, name, i, mip);
        nc_log("Dumping to {}", save_path);
        stbi_write_png(save_path.c_str(), atlas_bundle.width >> mip, atlas_bundle.height >> mip, channels_count, start, 0);
      }
    }
  };
  

  auto root = nlohmann::json::object();
  root["dims"] = nlohmann::json::array({ atlas_bundle.width, atlas_bundle.height, atlas_bundle.layers_count });
  auto images = nlohmann::json::array();
  for (const auto& texture_kv : atlas_bundle.textures) {
    auto item = nlohmann::json::object();
    item["name"] = std::string(texture_kv.first);
    item["pos"] = nlohmann::json::array({ texture_kv.second.get_x(), texture_kv.second.get_y() });
    item["dim"] = nlohmann::json::array({ texture_kv.second.get_width(), texture_kv.second.get_height() });
    item["layer"] = texture_kv.second.get_array_idx();
    images.emplace_back(item);
  }
  root["img"] = images;
  
  {
    auto save_path = scratch_format("{}/textures.json", path);
    nc_log("Dumping to {}", save_path);
    std::ofstream(save_path.c_str()) << root.dump();
  }

  dump_texture(atlas_bundle.diffuse_handle, "diffuse", 4, DIFFUSE_TEXTURE_PARAMS);
  dump_texture(atlas_bundle.normal_handle, "normal", 3, NORMAL_TEXTURE_PARAMS);
  dump_texture(atlas_bundle.specular_handle, "specular", 1, SPECULAR_TEXTURE_PARAMS);
  dump_texture(atlas_bundle.emissive_handle, "emissive", 3, EMISSIVE_TEXTURE_PARAMS);
}






} // namespace nc
