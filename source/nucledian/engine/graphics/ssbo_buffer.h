#pragma once

#include <types.h>

#include <engine\graphics\gl_types.h>

#include <vector>

namespace nc
{

template <typename T>
class SSBOBuffer
{
public:
  SSBOBuffer() {}
  explicit SSBOBuffer(size_t capacity);

  void resize(size_t new_capacity);
  void clear();
  void push_back(T&& value);
  void extend(std::vector<T>&& elements);
  void update_gpu_data(bool reset_capacity = false);
  template <typename UnaryOp>
  void update_gpu_data(UnaryOp op, bool reset_capacity = false);

  void bind(GLuint index) const;

  // Total SSBO size.
  size_t capacity() const;
  // Number of items on GPU.
  size_t gpu_size() const;
  // Number of items on GPU + number of items in buffer.
  size_t size() const;

  // Total SSBO size.
  u32 capacity_u32() const;
  // Number of items on GPU.
  u32 gpu_size_u32() const;
  // Number of items on GPU + number of items in buffer.
  u32 size_u32() const;

private:
  // Total SSBO size.
  size_t m_capacity = 0;
  // Number of items on GPU.
  size_t m_gpu_size = 0;
  // Number of items on GPU + number of items in buffer.
  size_t m_size     = 0;
  GLuint m_handle   = 0;

  std::vector<T> buffer;
};

}

#include <engine/graphics/ssbo_buffer.inl>