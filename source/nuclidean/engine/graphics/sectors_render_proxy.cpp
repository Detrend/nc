// Project Nuclidean Source File
#include <engine/graphics/sectors_render_proxy.h>

#include <common.h>

#include <utility>
#include <algorithm>

namespace nc
{

//==============================================================================
SectorsRenderProxy::SectorsRenderProxy(const MapSectors& map)
: m_map(map)
{
}

//==============================================================================
void SectorsRenderProxy::get_render_elements(RenderElementCollector& /*collector*/) const
{
}

//==============================================================================
const MapSectors& SectorsRenderProxy::get_map() const
{
  return m_map;
}

//==============================================================================
void SectorsRenderProxy::build_meshes()
{
  MeshManager&     mesh_manager = MeshManager::get();
  std::vector<f32> vertices;

  m_sector_meshes.clear();
  m_sector_meshes.reserve(m_map.sectors.size());
  m_dirty_sectors.assign(m_map.sectors.size(), false);

  for (SectorID sector_id = 0; sector_id < m_map.sectors.size(); ++sector_id)
  {
    vertices.clear();
    m_map.sector_to_vertices(sector_id, vertices);

    m_sector_meshes.push_back(mesh_manager.create_sector
    (
      ResLifetime::Level,
      vertices.data(),
      cast<u32>(vertices.size())
    ));
  }

  m_full_upload_requested = true;
}

//==============================================================================
const MeshHandle& SectorsRenderProxy::get_and_update_mesh(SectorID sector)
{
  nc_assert(sector < m_sector_meshes.size());

  if (m_dirty_sectors[sector])
  {
    std::vector<f32> vertices;
    m_map.sector_to_vertices(sector, vertices);

    MeshManager::get().recreate_sector
    (
      m_sector_meshes[sector], vertices.data(), cast<u32>(vertices.size())
    );

    m_dirty_sectors[sector] = false;
  }

  return m_sector_meshes[sector];
}

//==============================================================================
void SectorsRenderProxy::update_sector
(
  SectorID                         sector,
  const SectorDynData&             sector_data,
  std::span<const SegmentDynState> segments
)
{
  nc_assert(m_map.is_valid_sector_id(sector));

  m_map.sectors_dynamic[sector] = sector_data;

  for (const SegmentDynState& segment : segments)
  {
    nc_assert(segment.segment < m_map.wall_segments_dynamic.size());
    m_map.wall_segments_dynamic[segment.segment] = segment.data;
  }

  m_dirty_sectors[sector] = true;

  m_map.for_each_portal_of_sector(sector, [&](WallID wall)
  {
    const SectorID neighbor = m_map.walls[wall].portal_sector_id;
    nc_assert(m_map.is_valid_sector_id(neighbor));
    m_dirty_sectors[neighbor] = true;
  });

  if (std::ranges::find(m_changed_heights, sector) == m_changed_heights.end())
  {
    m_changed_heights.push_back(sector);
  }
}

//==============================================================================
bool SectorsRenderProxy::consume_full_upload_request()
{
  return std::exchange(m_full_upload_requested, false);
}

//==============================================================================
std::vector<SectorID> SectorsRenderProxy::consume_changed_heights()
{
  std::vector<SectorID> changed;
  changed.swap(m_changed_heights);
  return changed;
}

}
