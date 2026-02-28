// Project Nucledian Source File

#include <engine/player/save_types.h>

#include <common.h>

#include <array>
#include <cstring> // std::memcpy

namespace nc
{

using Header = std::array<char, 8>;
constexpr Header DEFAULT_SAVE_HEADER = std::to_array("NC_SAVE");

struct SaveGameStructure
{
  Header       header = DEFAULT_SAVE_HEADER;
  SaveGameData data;
};

//==============================================================================
void serialize_save_game_to_bytes
(
  const SaveGameData& data,
  std::vector<byte>&  bytes_out
  )
{
  nc_assert(bytes_out.empty());
  u64 required_size = sizeof(SaveGameStructure);
  bytes_out.resize(required_size);

  SaveGameStructure save;
  save.data = data;

  // Save byte-by-byte
  byte* bytes = reinterpret_cast<byte*>(&save);
  for (u64 i = 0; i < required_size; ++i)
  {
    bytes_out[i] = bytes[i];
  }
}

//==============================================================================
bool deserialize_save_game_from_bytes
(
  SaveGameData&            data_out,
  const std::vector<byte>& input
)
{
  u64 required_size = sizeof(SaveGameStructure);
  if (input.size() != required_size)
  {
    return false;
  }
  
  SaveGameStructure save;
  byte* bytes = reinterpret_cast<byte*>(&save);
  for (u64 i = 0; i < required_size; ++i)
  {
    bytes[i] = input[i];
  }

  if (save.header != DEFAULT_SAVE_HEADER)
  {
    return false;
  }

  // Everything ok
  data_out = save.data;
  return true;
}

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
  DemoDataHeader&             header_out,
  std::vector<DemoDataFrame>& frames_out,
  const byte*                 bytes_start,
  u64                         bytes_cnt
)
{
  if (bytes_cnt < sizeof(DemoDataHeader))
  {
    // Size is retarded apparently
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
    // What the fuck
    return false;
  }

  // Memcpy the fuck out of it
  frames_out.resize(header_out.num_frames);
  std::memcpy
  (
    frames_out.data(), bytes_start + sizeof(DemoDataHeader),
    sizeof(DemoDataFrame) * header_out.num_frames
  );

  // All good on the demo front
  return true;
}

}
