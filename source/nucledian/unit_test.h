// Project Nucledian Source File
#pragma once

#include <types.h>
#include <metaprogramming.h>
#include <config.h>

#include <vector>

namespace nc::unit_test
{

struct TestCtx
{
  u64 argument;
};

struct Test
{
  using Signature = bool(*)(TestCtx&);
  Test(Signature function, cstr default_name);

  Test* name(cstr str);
  Test* arg(u64 value);

  cstr      test_name      = nullptr;
  Signature test_function  = nullptr;
  u64       argument_value = 0;
};

std::vector<Test>& get_tests();

}

#define NC_TEST_SUCCESS return true;
#define NC_TEST_FAIL    return false;

#ifdef NC_TESTS

#define NC_UNIT_TEST(_test_func)                                                   \
static inline ::nc::unit_test::Test* NC_TOKENJOIN(_testBase, __LINE__) = []()      \
{                                                                                  \
  auto& test = ::nc::unit_test::get_tests().emplace_back(_test_func, #_test_func); \
  return &test;                                                                    \
}()

#else

#define NC_UNIT_TEST(_dummy)

#endif

