// Project Nuclidean Source File
#pragma once

#include <types.h>

#include <array>
#include <cstddef>

namespace nc
{

// Ring buffer with a fixed capacity.
template<typename T, size_t Capacity>
class RingBuffer
{
public:
  void     push_back(const T& value);
  const T& front() const;
  void     pop_front();
  bool     empty() const;
  void     clear();

private:
  std::array<T, Capacity> m_data{};
  // index of the oldest element
  size_t                  m_first = 0;
  size_t                  m_size  = 0;
};

}

#include <ring_buffer.inl>
