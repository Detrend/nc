// Project Nuclidean Source File
#pragma once

#include <buffer.h>
#include <common.h>

namespace nc
{

//==============================================================================
inline Buffer::Buffer(void* data, u64 bytes_cnt, SerializationType type)
: head(data)
, size(bytes_cnt)
, type(type)
{
}

//==============================================================================
inline Buffer::Buffer()
: head(nullptr)
, size(0)
, type(SerializationType::counting_bytes)
{
  
}

//==============================================================================
template<typename T>
void Buffer::store(const T& value)
{
  T* ptr = recast<T*>(head);
  *ptr = value;
  head = recast<void*>(ptr + 1);
  size -= sizeof(T);
  nc_assert(size >= 0);
}

//==============================================================================
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

//==============================================================================
template<typename T>
void Buffer::store_array(const T* first, u64 cnt)
{
  std::memcpy(head, first, sizeof(T) * cnt);
  size -= sizeof(T) * cnt;
  head = recast<T*>(head) + cnt;
  nc_assert(size >= 0);
}

//==============================================================================
template<typename T>
void Buffer::load_array(T* first, u64 cnt)
{
  std::memcpy(first, head, sizeof(T) * cnt);
  size -= sizeof(T) * cnt;
  head = recast<T*>(head) + cnt;
  nc_assert(size >= 0);
}

//==============================================================================
template<typename T>
void Buffer::serialize(T& inout)
{
  if (is_serializing())
  {
    store<T>(inout);
  }
  else if (is_deserializing())
  {
    inout = load<T>();
  }
  else if (is_counting())
  {
    size += sizeof(T);
  }
}

//==============================================================================
template<typename T>
void Buffer::serialize_array(T* array, u64 cnt)
{
  if (is_serializing())
  {
    store_array<T>(array, cnt);
  }
  else if (is_deserializing())
  {
    load_array<T>(array, cnt);
  }
  else if (is_counting())
  {
    size += sizeof(T) * cnt;
  }
}

}
