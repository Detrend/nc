// Project Nucledian Source File
#include <common.h>

#include <engine/core/engine.h>
#include <engine/core/engine_module.h>
#include <engine/core/module_event.h>
#include <engine/core/is_engine_module.h>
#include <engine/core/engine_module_types.h>

#include <engine/graphics/graphics_system.h>
#include <engine/editor/editor_module.h>

#include <ranges>

namespace nc
{

static Engine* g_engine = nullptr;

//==============================================================================
Engine& get_engine()
{
  NC_ASSERT(g_engine);
  return *g_engine;
}

//==============================================================================
int init_engine_and_run_game([[maybe_unused]] const std::vector<std::string>& args)
{
  // create instance of the engine
  g_engine = new Engine();

  if (!g_engine->init())
  {
    // failed to init, end it here
    // do something here
    delete g_engine;
    return 1;
  }

  g_engine->run();

  // graceful exit
  g_engine->terminate();

  delete g_engine;
  return 0;
}

//==============================================================================
void Engine::send_event(ModuleEvent& event)
{
  for (u64 i = 0; i < m_modules.size(); ++i)
  {
    const EngineModuleMask mask = 1<<i;
    if (m_modules[i] && (mask & event.modules))
    {
      m_modules[i]->on_event(event);
    }
  }
}

//==============================================================================
void Engine::send_event(ModuleEvent&& event)
{
  auto& ref = event;
  this->send_event(ref);
}

//==============================================================================
bool Engine::init()
{
  using namespace EngineModule;

  // init the modules here..
  auto graphics = std::make_unique<GraphicsSystem>();
  if (!graphics || !graphics->init())
  {
    return false;
  }
  m_module_init_order.push_back(graphics.get());
  m_modules[graphics_system] = std::move(graphics);

  // this is an ugly hack
  auto g = (GraphicsSystem*) m_modules[graphics_system].get();

  auto editor = std::make_unique<EditorSystem>();
  if (editor->init(g->get_window(), g->get_gl_context()))
  {
    m_module_init_order.push_back(editor.get());
    m_modules[editor_system] = std::move(editor);
  }

  return true;
}

//==============================================================================
void Engine::run()
{
  while (!this->should_quit())
  {
    // pump messages
    this->get_module<GraphicsSystem>().update_window_and_pump_messages();

    // update
    this->send_event(ModuleEvent
    {
      .type = ModuleEventType::game_update,
    });

    // cleanup
    this->send_event(ModuleEvent
    {
      .type = ModuleEventType::cleanup,
    });
  }
}

//==============================================================================
void Engine::terminate()
{
  this->send_event(ModuleEvent
  {
    .type = ModuleEventType::pre_terminate
  });

  auto terminate_event = ModuleEvent
  {
    .type = ModuleEventType::terminate,
  };

  // terminate modules in the opposite order of initialization
  for (auto* module : m_module_init_order | std::views::reverse)
  {
    NC_ASSERT(module);
    module->on_event(terminate_event);
  }
}

//==============================================================================
void Engine::request_quit()
{
  m_should_quit = true;
}

//==============================================================================
bool Engine::should_quit()
{
  return m_should_quit;
}

}

