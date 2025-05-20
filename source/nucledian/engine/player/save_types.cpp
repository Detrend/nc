// Project Nucledian Source File

#include <engine/player/save_types.h>

#include <common.h>

#include <array>

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

}
