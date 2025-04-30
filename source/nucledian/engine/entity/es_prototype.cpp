// Project Nucledian Source File

#include <engine/entity/es_prototype.h>
#include <iostream>

#include <engine/entity/es_system.h>
#include <engine/entity/es_controller.h>
#include <engine/entity/es_component_def.h>

namespace nc
{

struct PlayerTest : public EntityControllerHelper<PlayerTest, RenderComp, PhysComp>
{
  virtual void on_collision(const EntityID&) override
  {
    // do something
  }

  virtual void on_render() override
  {
    
  }

  bool bool_value = true;
  int  int_value  = 11;

  virtual ~PlayerTest(){};
};

int test_compilation()
{
  EntitySystem es;
  const auto id1 = es.create_entity_pure<RenderComp>();
  const auto id2 = es.create_entity_pure<RenderComp, AIComp>();
  const auto id3 = es.create_entity_pure<RenderComp, AIComp, PhysComp>();

  const auto p = es.create_entity_controlled<PlayerTest>();

  if (auto* c = es.get_component<RenderComp>(id1))
  {
    c->num = 1;
  }

  if (auto* c = es.get_component<RenderComp>(id2))
  {
    c->num = 4;
  }

  if (auto* c = es.get_component<RenderComp>(id3))
  {
    c->num = 12;
  }

  es.destroy_entity(p);

  es.for_each_entity<RenderComp>([&](RenderComp& rend)
  {
    std::cout << rend.num << std::endl;
  });

  es.destroy_entity(id1);
  es.destroy_entity(id2);
  es.destroy_entity(id3);

  return 0;
}

static int brruuh = test_compilation();

}

