// Project Nuclidean Source File
#pragma once

#include <types.h>
#include <engine/core/engine_module.h>
#include <engine/core/engine_module_id.h>

namespace nc
{

class DatabaseSystem : public IEngineModule
{
public:
  bool init();
  virtual void on_event(ModuleEvent& event) override;
  static EngineModuleId get_module_id();

private:

};

}
