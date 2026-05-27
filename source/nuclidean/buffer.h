// Project Nuclidean Source File
#pragma once

namespace nc
{

enum class SerializationType : u8
{
  serialize,
  deserialize,
  counting_bytes,
};

class Buffer
{
public:
  // Only for counting
  Buffer();

  // For serialization and deserialization
  Buffer(void* data, u64 bytes_cnt, SerializationType type);

  template<typename T>
  void serialize(T& inout);

  template<typename T>
  void serialize_array(T* array, u64 cnt);

  u64  get_remaining_buffer_size() const { return size; }
  u64  get_counted_size()          const { return size; }

  bool is_deserializing() const { return type == SerializationType::deserialize;    }
  bool is_serializing()   const { return type == SerializationType::serialize;      }
  bool is_counting()      const { return type == SerializationType::counting_bytes; }

private:
  template<typename T>
  void store(const T& value);

  template<typename T>
  T load();

  template<typename T>
  void store_array(const T* first, u64 cnt);

  template<typename T>
  void load_array(T* first, u64 cnt);

private:
  void*             head = nullptr;
  u64               size = 0;
  SerializationType type = SerializationType::serialize;
};

}

#include <buffer.inl>
