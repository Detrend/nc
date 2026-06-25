// Project Nuclidean Source File
#pragma once

#include <engine/core/engine_module.h>
#include <engine/core/engine_module_id.h>

#include <memory>

namespace nc
{

class Editor;

class EditorSystem : public IEngineModule
{
public:
  static EngineModuleId get_module_id();
  static EditorSystem&  get();

  bool init();
  void on_event(ModuleEvent& event) override;

  Editor* get_editor();
  bool    has_editor();

private:
  std::unique_ptr<Editor> m_editor;
};

}
