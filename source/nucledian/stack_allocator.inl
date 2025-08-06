// Project Nucledian Source File
#pragma once

namespace nc
{

//==============================================================================
template<typename T, u64 Cnt>
T* StackAllocator<T, Cnt>::allocate(u64 n)
{
  if (!m_data || m_data->m_used || n > Cnt)
  {
    return std::allocator<T>::allocate(n);
  }
  else
  {
    m_data->m_used = true;
    return m_data->m_data;
  }
}

//==============================================================================
template<typename T, u64 Cnt>
void StackAllocator<T, Cnt>::deallocate(T* ptr, u64 n)
{
  if (!m_data || ptr != m_data->m_data)
  {
    std::allocator<T>::deallocate(ptr, n);
  }
  else
  {
    m_data->m_used = false;
  }
}

//==============================================================================
template<typename T, u64 Cnt>
StackAllocator<T, Cnt>::StackAllocator(StackData<T, Cnt>* d) noexcept
: m_data(d)
{

}

//==============================================================================
template<typename T, u64 Cnt>
template<typename U, u64 OtherCnt>
StackAllocator<T, Cnt>::StackAllocator(const StackAllocator<U, OtherCnt>&) noexcept
: m_data(nullptr)
{

}

//==============================================================================
template<typename T, u64 Cnt>
StackAllocator<T, Cnt>::StackAllocator(const StackAllocator<T, Cnt>& other) noexcept
: m_data(other.m_data)
{

}

//==============================================================================
template<typename T, u64 Cnt>
StackAllocator<T, Cnt>::StackAllocator(StackAllocator<T, Cnt>&& other) noexcept
: m_data(other.m_data)
{
  other.m_data = nullptr;
}

}
