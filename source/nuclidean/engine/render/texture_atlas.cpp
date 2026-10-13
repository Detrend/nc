// Project Nuclidean Source File
#include <engine/render/texture_atlas.h>

#include <common.h>
#include <logging.h>

#include <stb/stb_image.h>
#include <stb/stb_rect_pack.h>

#include <algorithm>
#include <cmath>
#include <optional>

namespace nc::render
{

// Magenta and black checkerboard used in place of missing textures.
static constexpr TextureID ERROR_TEXTURE_ID   = 0;
static constexpr u32       ERROR_TEXTURE_SIZE = 64;
static constexpr u32       ERROR_CELL_SIZE    = 16;

// Every OpenGL 4.3 GPU supports textures at least this large.
static constexpr u32       MAX_ATLAS_HEIGHT   = 16384;

namespace
{
// Decoded texture waiting to be packed into the atlas.
struct Image
{
  std::string     name;
  uvec2           size;
  std::vector<u8> texels;
};
}

//==============================================================================
static Image make_error_image()
{
  Image image{.name = "__nc_error_texture__", .size = uvec2(ERROR_TEXTURE_SIZE, ERROR_TEXTURE_SIZE)};
  image.texels.reserve(cast<size_t>(ERROR_TEXTURE_SIZE) * ERROR_TEXTURE_SIZE * 4);
  for (u32 y = 0; y < ERROR_TEXTURE_SIZE; ++y)
  {
    for (u32 x = 0; x < ERROR_TEXTURE_SIZE; ++x)
    {
      const u8 magenta = ((x / ERROR_CELL_SIZE + y / ERROR_CELL_SIZE) % 2) ? 255 : 0;
      image.texels.insert(image.texels.end(), {magenta, 0, magenta, 255});
    }
  }
  return image;
}

//==============================================================================
static std::optional<Image> load_image(const std::filesystem::path& path)
{
  s32 width = 0;
  s32 height = 0;
  s32 channels = 0;
  u8* const texels = stbi_load(path.string().c_str(), &width, &height, &channels, 4);
  if (!texels)
  {
    nc_crit("[render][texture atlas] cannot load texture \"{}\": {}", path.string(), stbi_failure_reason());
    return std::nullopt;
  }

  Image image
  {
    .name   = path.stem().string(),
    .size   = uvec2(width, height),
    .texels = std::vector<u8>(texels, texels + cast<size_t>(width) * cast<size_t>(height) * 4),
  };
  stbi_image_free(texels);
  return image;
}

//==============================================================================
void TextureAtlas::load_directory(const std::filesystem::path& directory)
{
  std::vector<Image> images;
  images.push_back(make_error_image());

  for (const std::filesystem::directory_entry& entry : std::filesystem::recursive_directory_iterator(directory))
  {
    const std::filesystem::path& path = entry.path();
    const std::string stem = path.stem().string();

    if (path.extension() != ".png" && path.extension() != ".jpg")
      continue;

    // normal, specular and emissive maps belong to the texture without the suffix
    if (stem.ends_with("_normal") || stem.ends_with("_specular") || stem.ends_with("_emissive"))
      continue;

    if (std::optional<Image> image = load_image(path))
      images.push_back(std::move(*image));
  }

  // wide enough for a roughly square atlas, the height is cropped to the packed textures afterwards
  u64 area = 0;
  u32 width = 0;
  for (const Image& image : images)
  {
    area += cast<u64>(image.size.x) * image.size.y;
    width = std::max(width, image.size.x);
  }
  width = std::max(width, cast<u32>(std::ceil(std::sqrt(cast<f64>(area)))));

  std::vector<stbrp_rect> rects;
  rects.reserve(images.size());
  for (const Image& image : images)
    rects.push_back(stbrp_rect{.w = cast<s32>(image.size.x), .h = cast<s32>(image.size.y)});

  std::vector<stbrp_node> nodes(width);
  stbrp_context context;
  stbrp_init_target(&context, cast<s32>(width), cast<s32>(MAX_ATLAS_HEIGHT), nodes.data(), cast<s32>(nodes.size()));
  if (!stbrp_pack_rects(&context, rects.data(), cast<s32>(rects.size())))
  {
    nc_crit("[render][texture atlas] textures in \"{}\" do not fit into the atlas", directory.string());
    return;
  }

  u32 height = 0;
  for (const stbrp_rect& rect : rects)
    height = std::max(height, cast<u32>(rect.y + rect.h));

  m_size = uvec2(width, height);
  m_texels.assign(cast<size_t>(width) * height * 4, 0);
  m_textures.clear();
  m_texture_ids.clear();

  for (size_t i = 0; i < images.size(); ++i)
  {
    const Image& image = images[i];
    const uvec2 position(rects[i].x, rects[i].y);

    for (u32 row = 0; row < image.size.y; ++row)
    {
      const u8* const source = image.texels.data() + cast<size_t>(row) * image.size.x * 4;
      u8* const destination = m_texels.data() + (cast<size_t>(position.y + row) * width + position.x) * 4;
      std::copy_n(source, image.size.x * 4, destination);
    }

    m_texture_ids.emplace(image.name, cast<TextureID>(m_textures.size()));
    m_textures.push_back(Texture{.position = position, .size = image.size});
  }
}

//==============================================================================
TextureID TextureAtlas::get_texture_id(const std::string& name) const
{
  if (const auto it = m_texture_ids.find(name); it != m_texture_ids.end())
    return it->second;

  nc_warn("[render][texture atlas] texture \"{}\" not found, using the error texture", name);
  return ERROR_TEXTURE_ID;
}

//==============================================================================
void TextureAtlas::free_gpu_data()
{
  m_texels = std::vector<u8>();
  m_textures = std::vector<Texture>();
}

}
