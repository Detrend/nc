// Project Nuclidean Source File
#pragma once

#include <ring_buffer.h>

#include <common.h>

namespace nc
{

//==============================================================================
template<typename T, size_t Capacity>
void RingBuffer<T, Capacity>::push_back(const T& value)
{
  nc_assert(m_size < Capacity, "ring buffer full");
  m_data[(m_first + m_size) % Capacity] = value;
  ++m_size;
}

//==============================================================================
template<typename T, size_t Capacity>
const T& RingBuffer<T, Capacity>::front() const
{
  nc_assert(m_size > 0, "ring buffer empty");
  return m_data[m_first];
}

//==============================================================================
template<typename T, size_t Capacity>
void RingBuffer<T, Capacity>::pop_front()
{
  nc_assert(m_size > 0, "ring buffer empty");
  m_first = (m_first + 1) % Capacity;
  --m_size;
}

//==============================================================================
template<typename T, size_t Capacity>
bool RingBuffer<T, Capacity>::empty() const
{
  return m_size == 0;
}

//==============================================================================
template<typename T, size_t Capacity>
void RingBuffer<T, Capacity>::clear()
{
  m_first = 0;
  m_size  = 0;
}

}
