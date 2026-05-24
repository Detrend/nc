// Project Nuclidean Source File
#pragma once

namespace nc
{

class Buffer
{
public:
  Buffer(void* data, u64 bytes_cnt);

  template<typename T>
  void store(const T& value);

  template<typename T>
  T load();

  template<typename T>
  void store_array(const T* first, u64 cnt);

  template<typename T>
  void load_array(T* first, u64 cnt);

  template<typename T>
  void serialize(T& inout, bool serialize);

  template<typename T>
  void serialize_array(T* array, u64 cnt, bool serialize);

  u64  get_remaining_buffer_size() const { return size; }

private:
  void* head = nullptr;
  u64   size = 0;
};

}

#include <buffer.inl>
