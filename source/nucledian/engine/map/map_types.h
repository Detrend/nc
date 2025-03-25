// Project Nucledian Source File
#pragma once

// MR says:
// Access basic map types without including the whole map system

#include <types.h>

namespace nc
{

using SectorID  = u16; // absolute indexing of sectors
using WallID    = u16; // absolute indexing
using WallRelID = u8;  // indexing relative to the sector
using PortalID  = u16; // absolute indexing
using TextureID = u16; // unused for now

constexpr auto INVALID_SECTOR_ID   = static_cast<SectorID>(-1);
constexpr auto INVALID_WALL_ID     = static_cast<WallID>(-1);
constexpr auto INVALID_WALL_REL_ID = static_cast<WallRelID>(-1);
constexpr auto INVALID_PORTAL_ID   = static_cast<PortalID>(-1);
constexpr auto INVALID_TEXTURE_ID  = static_cast<TextureID>(-1);

constexpr auto MAX_WALLS_PER_SECTOR = static_cast<u64>(WallRelID(~0)-1);
constexpr auto MAX_SECTORS          = static_cast<u64>(SectorID(~0)-1);
constexpr auto MAX_WALLS            = static_cast<u64>(WallID(~0)-1);
constexpr auto MAX_PORTALS          = static_cast<u64>(PortalID(~0)-1);

}

