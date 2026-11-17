// Project Nuclidean Source File
#include <config.h>

#if NC_TESTS

#include <metaprogramming.h>
#include <unit_test.h>

#include <engine/input/game_input.h>

#include <array>
#include <string_view>

namespace nc
{

enum class MetaTestEnum : u8
{
  first,
  middle_value,
  last,
  // - //
  count
};

//==============================================================================
static bool meta_enum_value_name_of_template_argument(unit_test::TestCtx& /*ctx*/)
{
  NC_TEST_ASSERT(meta::enum_value_name<MetaTestEnum::first>() == "first");
  NC_TEST_ASSERT(meta::enum_value_name<MetaTestEnum::middle_value>() == "middle_value");
  NC_TEST_ASSERT(meta::enum_value_name<MetaTestEnum::last>() == "last");
  NC_TEST_ASSERT(meta::enum_value_name<PlayerKeyInputs::forward>() == "forward");
  NC_TEST_ASSERT(meta::enum_value_name<PlayerKeyInputs::weapon_3>() == "weapon_3");

  // first_weapon, declared after count, shares this value
  NC_TEST_ASSERT(meta::enum_value_name<PlayerKeyInputs::weapon_0>() == "weapon_0");

  NC_TEST_SUCCESS;
}
NC_UNIT_TEST(meta_enum_value_name_of_template_argument);

//==============================================================================
static bool meta_enum_value_all_names(unit_test::TestCtx& /*ctx*/)
{
  constexpr std::array<std::string_view, 3> EXPECTED_NAMES{"first", "middle_value", "last"};
  NC_TEST_ASSERT(meta::ENUM_VALUE_ALL_NAMES<MetaTestEnum> == EXPECTED_NAMES);

  const auto& key_names = meta::ENUM_VALUE_ALL_NAMES<PlayerKeyInputs::values>;
  NC_TEST_ASSERT(key_names.size() == PlayerKeyInputs::count);
  NC_TEST_ASSERT(key_names[PlayerKeyInputs::forward] == "forward");
  NC_TEST_ASSERT(key_names[PlayerKeyInputs::weapon_0] == "weapon_0");
  NC_TEST_ASSERT(key_names[PlayerKeyInputs::weapon_3] == "weapon_3");

  NC_TEST_SUCCESS;
}
NC_UNIT_TEST(meta_enum_value_all_names);

//==============================================================================
static bool meta_enum_value_name(unit_test::TestCtx& /*ctx*/)
{
  NC_TEST_ASSERT(meta::enum_value_name(MetaTestEnum::first) == "first");
  NC_TEST_ASSERT(meta::enum_value_name(MetaTestEnum::middle_value) == "middle_value");
  NC_TEST_ASSERT(meta::enum_value_name(MetaTestEnum::last) == "last");
  NC_TEST_ASSERT(meta::enum_value_name(PlayerKeyInputs::forward) == "forward");
  NC_TEST_ASSERT(meta::enum_value_name(PlayerKeyInputs::weapon_0) == "weapon_0");

  NC_TEST_SUCCESS;
}
NC_UNIT_TEST(meta_enum_value_name);

//==============================================================================
static bool meta_parse_enum(unit_test::TestCtx& /*ctx*/)
{
  MetaTestEnum value = MetaTestEnum::first;
  NC_TEST_ASSERT(meta::parse_enum("last", value) && value == MetaTestEnum::last);
  NC_TEST_ASSERT(meta::parse_enum("first", value) && value == MetaTestEnum::first);

  PlayerKeyInputs::values key = PlayerKeyInputs::forward;
  NC_TEST_ASSERT(meta::parse_enum("weapon_3", key) && key == PlayerKeyInputs::weapon_3);

  // count marks the end of the enum, scripts must not accept it as a value
  NC_TEST_ASSERT(!meta::parse_enum("count", value));
  NC_TEST_ASSERT(!meta::parse_enum("", value));
  NC_TEST_ASSERT(!meta::parse_enum("las", value));
  NC_TEST_ASSERT(!meta::parse_enum("last_", value));

  NC_TEST_SUCCESS;
}
NC_UNIT_TEST(meta_parse_enum);

}

#endif
