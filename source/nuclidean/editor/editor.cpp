// Project Nuclidean Source File
#pragma once

#include <editor/editor.h>

#if NC_EDITOR

#include <common.h>

#include <editor/editor_impl.h>
#include <engine/input/input_system.h>
#include <editor/editor_system.h>

#include <algorithm> // std::sort

namespace nc
{

//==================================================================================================
/*static*/ Editor* Editor::get()
{
  return EditorSystem::get().get_editor();
}

//==================================================================================================
Editor::Editor()  = default;
Editor::~Editor() = default;

//==================================================================================================
void Editor::init()
{
  InputSystem::get().lock_player_input(InputLockLayers::editor, true);
  nc_assert(!m_impl);
  m_impl = std::make_unique<EditorImpl>();
  m_impl->init();
}

//==================================================================================================
void Editor::terminate()
{
  nc_assert(m_impl);
  m_impl.reset();
  InputSystem::get().lock_player_input(InputLockLayers::editor, false);
}

//==================================================================================================
void Editor::update(f32 delta)
{
  m_impl->update(delta);
}

//==================================================================================================
void Editor::render()
{
  RenderList         primitives;
  RenderModifierList modifiers;

  for (u64 i = 0; i < EditorImpl::NUM_GRIDS; ++i)
  {
    if (m_impl->grids[i]->handle.is_valid())
    {
      primitives.push_back(m_impl->grids[i]->shared_from_this());
    }
  }

  std::visit([&](auto& tool_type)
  {
    tool_type.get_render_data(*m_impl, primitives);
    tool_type.get_modifiers(*m_impl, modifiers);
  },
  m_impl->tool);

  for (auto&[id, object] : m_impl->objects_render)
  {
    std::visit([&](auto& casted_type)
    {
      casted_type.get_render_data(primitives);
    }, object);
  }

  // Sort the primitives by their order
  std::sort(primitives.begin(), primitives.end(), [](const auto& a, const auto& b)
  {
    return a->order < b->order;
  });

  // Sort the modifiers by their order
  std::sort(modifiers.begin(), modifiers.end(), [](const auto* a, const auto* b)
  {
    return a->order < b->order;
  });

  mat3 projection = m_impl->calc_view_matrix();
  m_impl->renderer.render(projection, primitives, modifiers);
}

//==================================================================================================
void Editor::on_window_resized(u32 width, u32 height)
{
  m_impl->aspect      = cast<f32>(width) / height;
  m_impl->screen_size = ivec2{width, height};
}

}

#endif // #if NC_EDITOR
