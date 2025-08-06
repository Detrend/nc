// Project Nucledian Source File
#pragma once

#include <types.h>

#include <stack_allocator.h>

#include <vector>

namespace nc
{

// Same as std::vector, but first "Cnt" elements are on the stack together with
// the container. If the size exceeds the "Cnt" then the remaining elements are
// then allocated on the heap as with a normal std::vector.
template<typename T, u64 Cnt>
class StackVector
: public StackData<T, Cnt>        // The ordering of bases is important as they
, public StackAllocator<T, Cnt>   // have to initialize in this order.
, public std::vector<T, StackAllocator<T, Cnt>>
{
public:
  using Data      = StackData<T, Cnt>;
  using Allocator = StackAllocator<T, Cnt>;
  using Vector    = std::vector<T, Allocator>;

  StackVector(const StackVector&)            = delete;
  StackVector(StackVector&&)                 = delete;
  StackVector& operator=(const StackVector&) = delete;
  StackVector& operator=(StackVector&&)      = delete;

  StackVector()
  : Allocator(static_cast<Data*>(this))
  , Vector(*static_cast<Allocator*>(this))
  {
    this->reserve(Cnt);
  }
};

}
