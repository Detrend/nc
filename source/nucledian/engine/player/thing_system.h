#include <engine/core/engine_module_id.h>
#include <engine/core/engine_module.h>
#include <engine/player/player.h>

namespace nc
{
  struct ModuleEvent;

  class ThingSystem : public IEngineModule
  {
  public:
    static EngineModuleId get_module_id();
    static ThingSystem& get();

    bool init();
    void on_event(ModuleEvent& event) override;

    Player* get_player();

  private:
    Player player;
  };
}