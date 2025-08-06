// Project Nucledian Source File
#pragma once

#include <memory> // std::allocator

namespace nc
{

template<typename T, u64 Cnt>
struct StackData
{
  T    m_data[Cnt];
  bool m_used = false;
};

template<typename T, u64 Cnt>
class StackAllocator : public std::allocator<T>
{
public:
  using pointer   = T*;
  using size_type = std::size_t;

  template<typename U>
  struct rebind
  {
    using other = StackAllocator<U, Cnt>;
  };

  template<typename U, u64 OtherCnt>
  StackAllocator(const StackAllocator<U, OtherCnt>&)  noexcept;

  StackAllocator(StackData<T, Cnt>* d)                noexcept;

  StackAllocator(const StackAllocator<T, Cnt>& other) noexcept;

  StackAllocator(StackAllocator<T, Cnt>&& other)      noexcept;

  T* allocate(u64 n);

  void deallocate(T* ptr, u64 n);

private:
  StackData<T, Cnt>* m_data = nullptr;
};

}

#include <stack_allocator.inl>
