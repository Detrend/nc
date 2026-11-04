// Project Nuclidean Source File
#pragma once

#include <engine/graphics/render_proxy.h>
#include <engine/graphics/resources/mesh.h>
#include <engine/map/map_system.h>

#include <span>
#include <vector>

namespace nc
{

struct SegmentDynState
{
  SegmentID          segment;
  WallSegmentDynData data;
};

class SectorsRenderProxy : public IRenderProxy
{
public:
  explicit SectorsRenderProxy(const MapSectors& map);

  void get_render_elements(RenderElementCollector& collector) const override;

  const MapSectors& get_map() const;

  void build_meshes();
  const MeshHandle& get_and_update_mesh(SectorID sector);

  void update_sector
  (
    SectorID                         sector,
    const SectorDynData&             sector_data,
    std::span<const SegmentDynState> segments
  );

  bool                  consume_full_upload_request();
  std::vector<SectorID> consume_changed_heights();

private:
  MapSectors              m_map;
  std::vector<MeshHandle> m_sector_meshes;
  std::vector<bool>       m_dirty_sectors;
  std::vector<SectorID>   m_changed_heights;
  bool                    m_full_upload_requested = true;
};

}
