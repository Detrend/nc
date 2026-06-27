// Project Nuclidean Source File
#pragma once

#include <types.h>
#include <config.h>

#if NC_EDITOR

#include <memory> // std::unique_ptr

namespace nc
{

class Editor
{
public:
  static Editor* get();

  Editor();
  ~Editor();
  Editor(const Editor&)            = delete;
  Editor& operator=(const Editor&) = delete;

  void init();
  void terminate();
  void update(f32 delta);
  void render();
  void on_window_resized(u32 width, u32 height);

private:
  struct EditorImpl;
  std::unique_ptr<EditorImpl> m_impl;
};

}

#endif // #if NC_EDITOR
