// Project Nuclidean Source File

#include <engine/player/save_types.h>

#include <common.h>

#include <array>
#include <cstring> // std::memcpy
#include <filesystem>
#include <fstream>

namespace nc
{

using Header = std::array<char, 8>;
constexpr Header DEFAULT_SAVE_HEADER = std::to_array("NC_SAVE");

struct SaveGameStructure
{
  Header       header = DEFAULT_SAVE_HEADER;
  SaveGameHeader data;
};

//==============================================================================
u64 calc_size_for_demo_to_bytes(const DemoDataHeader& header)
{
  return sizeof(DemoDataHeader) + header.num_frames * sizeof(DemoDataFrame);
}

//==============================================================================
void save_demo_to_bytes
(
  const DemoDataHeader& header,
  const DemoDataFrame*  frames,
  byte*                 bytes_out
)
{
  // Copy header
  std::memcpy(bytes_out, &header, sizeof(DemoDataHeader));

  // Copy frames
  std::memcpy
  (
    bytes_out + sizeof(DemoDataHeader), frames,
    header.num_frames * sizeof(DemoDataFrame)
  );
}

//==============================================================================
bool load_demo_from_bytes
(
  DemoDataHeader& header_out,
  DemoDataFrames& frames_out,
  const byte*     bytes_start,
  u64             bytes_cnt
)
{
  if (bytes_cnt < sizeof(DemoDataHeader))
  {
    // Reading something we are not supposed to..
    return false;
  }

  std::memcpy(&header_out, bytes_start, sizeof(DemoDataHeader));
  bool header_mismatch = std::memcmp
  (
    header_out.signature,
    DemoDataHeader::SIGNATURE,
    DemoDataHeader::SIGNATURE_SIZE
  );

  if (header_mismatch)
  {
    // Header does not match
    return false;
  }

  if (header_out.version != CURRENT_GAME_VERSION)
  {
    // Versions do not match
    return false;
  }

  u64 required_size
    = header_out.num_frames * sizeof(DemoDataFrame) + sizeof(DemoDataHeader);

  if (required_size != bytes_cnt)
  {
    // What?
    return false;
  }

  // Memcpy it
  frames_out.resize(header_out.num_frames);
  std::memcpy
  (
    frames_out.data(), bytes_start + sizeof(DemoDataHeader),
    sizeof(DemoDataFrame) * header_out.num_frames
  );

  // All good on the demo front
  return true;
}

//==============================================================================
static bool load_demo_from_file_into_bytes
(
  const std::string& demo_name,
  std::vector<u8>&   out
)
{
  namespace fs = std::filesystem;

  fs::path demo_dir = DEMO_DIR_RELATIVE;
  if (!fs::exists(demo_dir) || !fs::is_directory(demo_dir))
  {
    return false;
  }

  fs::path full_path = demo_dir / demo_name;
  if (!fs::exists(full_path) || !fs::is_regular_file(full_path))
  {
    return false;
  }

  std::ifstream in(full_path, std::ios::binary);
  if (!in)
  {
    return false;
  }

  in.seekg(0, std::ios::end);
  std::streamsize size = in.tellg();
  in.seekg(0, std::ios::beg);

  out.resize(size);

  if (!in.read(recast<char*>(out.data()), size))
  {
    return false;
  }

  return true;
}

//==============================================================================
void save_bytes_to_file
(
  const std::string& path,
  void*              data,
  u64                size
)
{
  std::ofstream out(path, std::ios::binary);
  nc_assert(out);

  out.write(recast<cstr>(data), cast<std::streamsize>(size));
  out.close();
}

//==============================================================================
void* load_bytes_from_file
(
  const std::string& path,
  u64&               size
)
{
  std::ifstream in(path, std::ios::binary);
  if (!in)
  {
    size = 0;
    return nullptr;
  }

  in.seekg(0, std::ios::end);
  size = cast<u64>(in.tellg());
  in.seekg(0, std::ios::beg);

  void* data = std::malloc(size);
  nc_assert(data);

  if (!in.read(recast<char*>(data), cast<std::streamsize>(size)))
  {
    std::free(data);
    size = 0;
    return nullptr;
  }

  return data;
}

//==============================================================================
bool load_demo_from_file
(
  const std::string&   file,
  LevelName&           level_name_out,
  LevelTransitionData& transition_out,
  DemoDataFrames&      frames_out
)
{
  std::vector<u8> bytes;
  if (!load_demo_from_file_into_bytes(file, bytes))
  {
    return false;
  }

  DemoDataHeader header;
  if (!load_demo_from_bytes(header, frames_out, bytes.data(), bytes.size()))
  {
    return false;
  }

  level_name_out = header.level_name;
  transition_out = header.transition_data;

  return true;
}

//==============================================================================
std::vector<std::string> list_available_save_files()
{
  namespace fs = std::filesystem;

  std::vector<std::string> result;
  fs::path save_dir = SAVE_DIR_RELATIVE;

  if (!fs::exists(save_dir) || !fs::is_directory(save_dir))
  {
    return result;
  }

  for (auto& entry : fs::directory_iterator(save_dir))
  {
    if (entry.is_regular_file())
    {
      auto path = entry.path();
      if (path.extension() == SAVE_FILE_SUFFIX)
      {
        result.push_back(path.filename().string());
      }
    }
  }

  return result;
}

//==============================================================================
std::vector<std::string> list_available_demo_files()
{
  namespace fs = std::filesystem;

  std::vector<std::string> result;
  fs::path demo_dir = DEMO_DIR_RELATIVE;

  if (!fs::exists(demo_dir) || !fs::is_directory(demo_dir))
  {
    return result;
  }

  for (auto &entry : fs::directory_iterator(demo_dir))
  {
    if (entry.is_regular_file())
    {
      auto path = entry.path();
      if (path.extension() == DEMO_FILE_SUFFIX)
      {
        result.push_back(path.filename().string());
      }
    }
  }

  return result;
}

//==============================================================================
void save_demo_to_file
(
  const std::string&   name,
  LevelName            lvl_name,
  const DemoDataFrame* frames,
  u64                  frames_cnt
)
{
  namespace fs = std::filesystem;

  DemoDataHeader header;

  // Set the file signature
  std::memcpy
  (
    header.signature, DemoDataHeader::SIGNATURE, DemoDataHeader::SIGNATURE_SIZE
  );

  // Set the level name
  header.level_name = lvl_name;
  header.version    = CURRENT_GAME_VERSION;
  header.num_frames = frames_cnt;

  std::vector<byte> bytes(calc_size_for_demo_to_bytes(header));

  // Demo to bytes
  save_demo_to_bytes(header, frames, bytes.data());

  // Create the directory for demos if it does not exist
  if (!fs::exists(DEMO_DIR_RELATIVE))
  {
    fs::create_directory(DEMO_DIR_RELATIVE);
  }

  // Append the demo dir to the path
  std::string final_path = std::format
  (
    "{}/{}{}", DEMO_DIR_RELATIVE, name, DEMO_FILE_SUFFIX
  );

  // Bytes to a file
  save_bytes_to_file(final_path, bytes.data(), bytes.size());
}

}
