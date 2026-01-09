#include <engine/graphics/ssbo_buffer.h>

#include <glad/glad.h>

namespace nc
{

//==============================================================================
template<typename T>
inline SSBOBuffer<T>::SSBOBuffer(size_t capacity)
  : m_capacity(capacity)
{
  glGenBuffers(1, &m_handle);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_handle);
  glBufferData(GL_SHADER_STORAGE_BUFFER, capacity * sizeof(T), nullptr, GL_DYNAMIC_DRAW);
}

//==============================================================================
template<typename T>
inline void SSBOBuffer<T>::resize(size_t new_capacity)
{
  m_capacity = new_capacity;
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_handle);
  glBufferData(GL_SHADER_STORAGE_BUFFER, m_capacity * sizeof(T), nullptr, GL_DYNAMIC_DRAW);
}

//==============================================================================
template<typename T>
inline void SSBOBuffer<T>::clear()
{
  m_gpu_size = 0;
  m_size = 0;
  buffer.clear();
}

//==============================================================================
template<typename T>
inline size_t SSBOBuffer<T>::push_back(T&& value)
{
  buffer.push_back(value);
  m_size++;

  return m_size - 1;
}

//==============================================================================
template<typename T>
inline void SSBOBuffer<T>::extend(std::vector<T>&& elements)
{
  buffer.insert
  (
    buffer.end(), 
    std::make_move_iterator(elements.begin()), 
    std::make_move_iterator(elements.end())
  );
  m_size += elements.size();
}

//==============================================================================
template<typename T>
inline void SSBOBuffer<T>::update_gpu_data(bool reset_capacity)
{
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_handle);
  glBufferSubData
  (
    GL_SHADER_STORAGE_BUFFER,
    m_gpu_size * sizeof(T),
    buffer.size() * sizeof(T),
    buffer.data()
  );

  m_gpu_size = m_size;
  buffer.clear();
  if (reset_capacity)
  {
    buffer.shrink_to_fit();
  }
}

//==============================================================================
template<typename T>
template<typename UnaryOp>
inline void SSBOBuffer<T>::update_gpu_data(UnaryOp op, bool reset_capacity)
{
  for (T& element : buffer)
  {
    op(element);
  }

  update_gpu_data(reset_capacity);
}

//==============================================================================
template<typename T>
inline void SSBOBuffer<T>::update_gpu_data_with(const std::vector<T>& elements)
{
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_handle);
  glBufferSubData
  (
    GL_SHADER_STORAGE_BUFFER,
    m_gpu_size * sizeof(T),
    elements.size() * sizeof(T),
    elements.data()
  );

  m_size += elements.size();
  m_gpu_size = m_size;;
}

//==============================================================================
template<typename T>
inline T& SSBOBuffer<T>::get_buffer_item(size_t index)
{
  return buffer[index];
}

//==============================================================================
template<typename T>
inline void SSBOBuffer<T>::bind(GLuint index) const
{
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, index, m_handle);
}

//==============================================================================
template<typename T>
inline size_t SSBOBuffer<T>::capacity() const
{
  return m_capacity;
}

//==============================================================================
template<typename T>
inline size_t SSBOBuffer<T>::gpu_size() const
{
  return m_gpu_size;
}

//==============================================================================
template<typename T>
inline size_t SSBOBuffer<T>::size() const
{
  return m_size;
}

//==============================================================================
template<typename T>
inline u32 SSBOBuffer<T>::capacity_u32() const
{
  return static_cast<u32>(capacity());
}

//==============================================================================
template<typename T>
inline u32 SSBOBuffer<T>::gpu_size_u32() const
{
  return static_cast<u32>(gpu_size());
}

//==============================================================================
template<typename T>
inline u32 SSBOBuffer<T>::size_u32() const
{
  return static_cast<u32>(size());
}

}