#include <engine/core/engine_module.h>
#include <engine/core/engine_module_id.h>
#include <engine/core/module_event.h>
#include <engine/ui/ui_texture.h>


namespace nc
{
  struct ModuleEvent;

  class UserInterfaceSystem : public IEngineModule
  {
  public:
    static EngineModuleId get_module_id();
    static UserInterfaceSystem& get();
    void on_event(ModuleEvent& event) override;
    bool init();
    ~UserInterfaceSystem();

  private:
    void init_shaders();
    void gather_player_info();
    void draw();

    std::vector<GuiTexture> ui_elements;

    int display_health = 0;

    unsigned int VBO;
    unsigned int VAO;

    unsigned int shader_program;

  };
}