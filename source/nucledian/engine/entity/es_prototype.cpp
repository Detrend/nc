#include <engine/entity/es_system.h>
#include <engine/entity/es_types.h>

#include <math/vector.h>

namespace nc
{

struct Actor : Entity
{
  static constexpr EntityType TYPE = EntityTypes::actor;
  vec3 position;
  vec3 direction;
};

struct EnemyEntity : public Actor
{ 
  static constexpr EntityType TYPE = EntityTypes::enemy;
};

struct PlayerEntity : public Actor
{ 
  static constexpr EntityType TYPE = EntityTypes::player;
  
};
  
int test_func()
{
  EntitySystem es;
  auto* p = es.create_entity<PlayerEntity>();
  auto* e = es.create_entity<EnemyEntity>();
  auto* a = es.create_entity<Actor>();

  es.for_each_entity<PlayerEntity>([&](PlayerEntity& p)
  {
    p.id.padd[0] = 15;
  });

  es.destroy_entity(p->id);
  es.destroy_entity(e->id);
  es.destroy_entity(a->id);

  return 0;
}

static int run_test_func = test_func();

}

