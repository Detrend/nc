// Project Nuclidean Source File

#include <engine/editor/editor_primitive.h>
#include <engine/graphics/resources/mesh.h>

namespace nc
{

//==================================================================================================
bool EditorPrimitive::is_valid() const
{
  return handle.is_valid();
}

//==================================================================================================
void EditorPrimitive::refresh_gpu_data(std::span<vec2> points, GLenum primitive_type)
{
  // Get rid of the old data
  MeshManager::get().destroy_editor_primitive(handle);

  // Refresh the bbox if any vertices
  if (points.size())
  {
    bbox = aabb2{};
    for (vec2 p : points)
    {
      bbox.insert_point(p);
    }

    // Upload the data
    handle = MeshManager::get().create_editor_primitive(points, primitive_type);
  }
}

//==================================================================================================
EditorPrimitive::~EditorPrimitive()
{
  // It should be ok to call this even if the handle is null
  MeshManager::get().destroy_editor_primitive(handle);
}

}
