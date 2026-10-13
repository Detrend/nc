// Project Nuclidean Source File
#pragma once

#include <types.h>
#include <math/vector.h>
#include <engine/graphics/resources/texture_id.h>
#include <engine/render/shaders/tracing.shared.h>

#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

namespace nc::render
{

// Textures packed into a single image, so shaders reach all of them through a single sampler.
class TextureAtlas
{
public:
  // Loads all textures from the directory and its subdirectories and packs them into the atlas.
  void load_directory(const std::filesystem::path& directory);

  // Returns the error texture if there is no texture with the name.
  TextureID get_texture_id(const std::string& name) const;

  // Frees the texels and placements once they are uploaded to the GPU, keeps the names for lookups.
  void free_gpu_data();

  // Atlas size in texels.
  uvec2 get_size() const { return m_size; }
  // Texels of the whole atlas for upload to the GPU, 4 bytes RGBA in sRGB each, row by row from the top.
  const std::vector<u8>& get_texels() const { return m_texels; }
  // Placement of each texture within the atlas, indexed by TextureID.
  const std::vector<Texture>& get_textures() const { return m_textures; }

private:
  // Atlas size in texels.
  uvec2                                      m_size = uvec2(0, 0);
  // RGBA texels in sRGB, row by row.
  std::vector<u8>                            m_texels;
  // Placement of each texture within the atlas, indexed by TextureID.
  std::vector<Texture>                       m_textures;
  // TextureID of each texture by its name.
  std::unordered_map<std::string, TextureID> m_texture_ids;
};

}
