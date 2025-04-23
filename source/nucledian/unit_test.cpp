// Project Nucledian Source File
#include <unit_test.h>

namespace nc::unit_test
{
  
//==============================================================================
Test::Test(Signature function, cstr default_name)
: test_name(default_name)
, test_function(function)
{

}

//==============================================================================
Test* Test::name(cstr str)
{
  test_name = str;
  return this;
}

//==============================================================================
Test* Test::arg(u64 value)
{
  argument_value = value;
  return this;
}

//==============================================================================
std::vector<Test>& get_tests()
{
  static std::vector<Test> tests;
  return tests;
}

}

