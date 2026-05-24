// Project Nuclidean Source File
#pragma once

#include <buffer.h>
#include <common.h>

namespace nc
{

inline Buffer::Buffer(void* data, u64 bytes_cnt)
: head(data)
, size(bytes_cnt)
{
}

template<typename T>
void Buffer::store(const T& value)
{
  T* ptr = recast<T*>(head);
  *ptr = value;
  head = recast<void*>(ptr + 1);
  size -= sizeof(T);
  nc_assert(size >= 0);
}

template<typename T>
T Buffer::load()
{
  T* ptr   = recast<T*>(head);
  T  value = *ptr;
  head = recast<T*>(ptr + 1);
  size -= sizeof(T);
  nc_assert(size >= 0);
  return value;
}

template<typename T>
void Buffer::store_array(const T* first, u64 cnt)
{
  std::memcpy(head, first, sizeof(T) * cnt);
  size -= sizeof(T) * cnt;
  nc_assert(size >= 0);
}

template<typename T>
void Buffer::load_array(T* first, u64 cnt)
{
  std::memcpy(first, head, sizeof(T) * cnt);
  size -= sizeof(T) * cnt;
  nc_assert(size >= 0);
}

template<typename T>
void Buffer::serialize(T& inout, bool serialize)
{
  if (serialize)
  {
    store<T>(inout);
  }
  else
  {
    inout = load<T>();
  }
}

template<typename T>
void Buffer::serialize_array(T* array, u64 cnt, bool serialize)
{
  if (serialize)
  {
    store_array<T>(array, cnt);
  }
  else
  {
    load_array<T>(array, cnt);
  }
}

}
