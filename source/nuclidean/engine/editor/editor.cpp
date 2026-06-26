// Project Nuclidean Source File
#pragma once

#include <engine/editor/editor.h>
#include <engine/input/input_system.h>
#include <engine/editor/editor_system.h>

#include <imgui/imgui.h>

namespace nc
{

//==================================================================================================
/*static*/ Editor* Editor::get()
{
  return EditorSystem::get().get_editor();
}

//==================================================================================================
void Editor::init()
{
  InputSystem::get().lock_player_input(InputLockLayers::editor, true);
}

//==================================================================================================
void Editor::terminate()
{
  InputSystem::get().lock_player_input(InputLockLayers::editor, false);
}

//==================================================================================================
void Editor::update(f32 /*delta*/)
{
  
}

//==================================================================================================
void Editor::render()
{
  if (ImGui::BeginMainMenuBar())
  {
    if (ImGui::BeginMenu("File"))
    {
      ImGui::MenuItem("Save Map",    "Ctrl+S");
      ImGui::MenuItem("Save Map As", "Ctrl+Shift+S");
      ImGui::MenuItem("Open Map",    "Ctrl+O");

      ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Edit"))
    {
      ImGui::EndMenu();
    }

    ImGui::EndMainMenuBar();
  }
}

}
