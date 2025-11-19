// Project Nucledian Source File
#pragma once

// MR says:
// Access basic map types without including the whole map system

#include <types.h>

namespace nc
{

using SectorID  = u16; // absolute indexing of sectors
using WallID    = u16; // absolute indexing
using WallRelID = u8;  // indexing relative to the sector, DO WE NEED THIS?
using TriggerID = u16; // absolute indexing of trigger data for walls and sectors
using PortalRenderID = u16; // absolute indexing of portals render data
using ActivatorID    = u16; // absolute indexing of activators

constexpr auto INVALID_SECTOR_ID        = static_cast<SectorID>(-1);
constexpr auto INVALID_WALL_ID          = static_cast<WallID>(-1);
constexpr auto INVALID_WALL_REL_ID      = static_cast<WallRelID>(-1);
constexpr auto INVALID_PORTAL_RENDER_ID = static_cast<PortalRenderID>(-1);
constexpr auto INVALID_TRIGGER_ID       = static_cast<TriggerID>(-1);
constexpr auto INVALID_ACTIVATOR_ID     = static_cast<ActivatorID>(-1);

constexpr auto MAX_WALLS_PER_SECTOR = static_cast<u64>(WallRelID(~0)-1);
constexpr auto MAX_SECTORS          = static_cast<u64>(SectorID(~0)-1);
constexpr auto MAX_WALLS            = static_cast<u64>(WallID(~0)-1);
constexpr auto MAX_TRIGGERS         = static_cast<u64>(TriggerID(~0)-1);
constexpr auto MAX_ACTIVATORS       = static_cast<u64>(ActivatorID(~0)-1);

}

