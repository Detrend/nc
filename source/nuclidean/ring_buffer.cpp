// Project Nuclidean Source File
#include <ring_buffer.h>

#include <unit_test.h>

#if NC_TESTS
// CMD args:
// -unit_test -test_filter=RingBuffer.*
namespace nc
{

static constexpr u32 CAPACITY = 4;
using TestBuffer = RingBuffer<u32, CAPACITY>;

//==============================================================================
static void push_sequence(TestBuffer& buffer, u32 first_value, u32 count)
{
  for (u32 i = 0; i < count; ++i)
    buffer.push_back(first_value + i);
}

//==============================================================================
// Pops count elements and checks they come out as first_value, first_value + 1, ...
static bool pop_sequence(TestBuffer& buffer, u32 first_value, u32 count)
{
  for (u32 i = 0; i < count; ++i)
  {
    if (buffer.empty() || buffer.front() != first_value + i)
      return false;
    buffer.pop_front();
  }
  return true;
}

//==============================================================================
static bool ring_buffer_fill_to_capacity_test(unit_test::TestCtx& /*ctx*/)
{
  // start the full buffer at every slot so its contents wrap around the end of the storage
  for (u32 start = 0; start < CAPACITY; ++start)
  {
    TestBuffer buffer;
    push_sequence(buffer, 0, start);
    NC_TEST_ASSERT(pop_sequence(buffer, 0, start));

    push_sequence(buffer, start, CAPACITY);
    NC_TEST_ASSERT(pop_sequence(buffer, start, CAPACITY));
    NC_TEST_ASSERT(buffer.empty());
  }

  NC_TEST_SUCCESS;
}
NC_UNIT_TEST(ring_buffer_fill_to_capacity_test)->name("RingBuffer Fill To Capacity");

//==============================================================================
static bool ring_buffer_wrap_around_test(unit_test::TestCtx& /*ctx*/)
{
  TestBuffer buffer;
  u32        oldest = 0;
  NC_TEST_ASSERT(buffer.empty());

  // keep the buffer nearly full while the oldest element goes around the storage several times
  push_sequence(buffer, 0, CAPACITY - 1);
  for (u32 value = CAPACITY - 1; value < CAPACITY * 4; ++value)
  {
    buffer.push_back(value);
    NC_TEST_ASSERT(buffer.front() == oldest);
    buffer.pop_front();
    ++oldest;
  }

  // drain what is left
  NC_TEST_ASSERT(pop_sequence(buffer, oldest, CAPACITY - 1));
  NC_TEST_ASSERT(buffer.empty());

  NC_TEST_SUCCESS;
}
NC_UNIT_TEST(ring_buffer_wrap_around_test)->name("RingBuffer Wrap Around");

//==============================================================================
static bool ring_buffer_clear_test(unit_test::TestCtx& /*ctx*/)
{
  TestBuffer buffer;

  // leave the oldest element away from the first slot before clearing
  push_sequence(buffer, 0, 3);
  NC_TEST_ASSERT(pop_sequence(buffer, 0, 1));
  buffer.clear();
  NC_TEST_ASSERT(buffer.empty());

  // full capacity is usable again and nothing from before the clear comes back
  push_sequence(buffer, 100, CAPACITY);
  NC_TEST_ASSERT(pop_sequence(buffer, 100, CAPACITY));
  NC_TEST_ASSERT(buffer.empty());

  NC_TEST_SUCCESS;
}
NC_UNIT_TEST(ring_buffer_clear_test)->name("RingBuffer Clear");

}
#endif
