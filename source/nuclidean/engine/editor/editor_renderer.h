// Project Nuclidean Source File
#pragma once

#include <engine/graphics/shaders/uniform.h>
#include <engine/graphics/shaders/shaders.h>
#include <engine/graphics/resources/shader_program.h>

#include <engine/editor/editor_primitive.h>

#include <math/matrix.h> // mat3

namespace nc
{

class IEditorPrimitiveRenderingModifier;

class EditorRenderer
{
public:
  EditorRenderer();

  void render
  (
    mat3                                          projection,
    const std::span<EditorPrimitivePtr>           primitives,
    std::span<IEditorPrimitiveRenderingModifier*> modifiers
  );

private:
  ShaderProgramHandle grid_rendering;
};

}
