// Project Nuclidean Source File
#pragma once

#include <vector>

namespace nc
{

struct EditorPrimitiveRenderingProperties;
struct EditorPrimitive;

class IEditorPrimitiveRenderingModifier
{
public:
  s64 order = 0;

  virtual void modify_rendering_properties
  (
    EditorPrimitiveRenderingProperties& properties,
    const EditorPrimitive&              primitive
  ) = 0;

  virtual ~IEditorPrimitiveRenderingModifier() {};
};

using RenderModifierList = std::vector<IEditorPrimitiveRenderingModifier*>;

}
