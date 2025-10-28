// Project Nucledian Source File
#pragma once

#include <common.h>

#ifdef NC_TESTS
#include <unit_test.h>
#include <engine/map/map_system.h>
#include <engine/entity/sector_mapping.h>
#include <engine/entity/entity_system.h>

namespace nc::entity_test
{ 

class TestPlayerEntity : public Entity
{
public:
  static EntityType get_type_static()
  {
    return EntityTypes::player;
  }

  inline static s64 alive_cnt = 0;

  TestPlayerEntity() : Entity(vec3{0}, 0, 0)
  {
    ++alive_cnt;
  }

  ~TestPlayerEntity()
  {
    --alive_cnt;
  }
};

//==============================================================================
bool entity_system_test([[maybe_unused]]unit_test::TestCtx& ctx)
{
  MapSectors     map;
  SectorMapping  mapping(map);
  EntityRegistry es;
  es.add_listener(&mapping);

  NC_TEST_ASSERT(TestPlayerEntity::alive_cnt == 0);

  auto* e1 = es.create_entity<TestPlayerEntity>();
  auto* e2 = es.create_entity<TestPlayerEntity>();
  auto* e3 = es.create_entity<TestPlayerEntity>();

  NC_TEST_ASSERT(TestPlayerEntity::alive_cnt == 3);
  NC_TEST_ASSERT(e1->get_type() == EntityTypes::player);
  NC_TEST_ASSERT(e2->get_type() == EntityTypes::player);
  NC_TEST_ASSERT(e3->get_type() == EntityTypes::player);

  s64 counter = 0;
  bool fail = false;
  es.for_each(EntityTypeFlags::player, [&](Entity& ent)
  {
    if (ent.get_type() != EntityTypes::player)
    {
      fail = true;
      return;
    }

    counter += 1;
  });
  NC_TEST_ASSERT(fail == false);
  NC_TEST_ASSERT(counter == TestPlayerEntity::alive_cnt);

  es.destroy_entity(e1->get_id());
  es.destroy_entity(e2->get_id());
  es.destroy_entity(e3->get_id());

  NC_TEST_ASSERT(TestPlayerEntity::alive_cnt == 0);

  NC_TEST_SUCCESS;
}

}

NC_UNIT_TEST(nc::entity_test::entity_system_test)->name("Basic entity system test");

#endif

