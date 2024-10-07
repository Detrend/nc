// Project Nucledian Source File
#pragma once

namespace nc
{

struct ModuleEvent;
  
class IEngineModule
{
public:
  // static EngineModuleId get_module_id();
  virtual void on_event(ModuleEvent& event) = 0;
  virtual ~IEngineModule(){};
};

}

