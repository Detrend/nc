// Project Nuclidean Source File
#pragma once

#include <config.h>

#if NC_EDITOR

#include <editor/editor_level.h>

namespace nc
{

//==================================================================================================
template<typename T, typename F>
void EditorLevel::for_each_object_of_type(F&& lambda)
{
  for (auto&[id, obj] : objects)
  {
    if (T* typed = std::get_if<T>(&obj))
    {
      if constexpr (requires (bool cont){cont = lambda(id, *typed);})
      {
        if (!lambda(id, *typed))
        {
          return;
        }
      }
      else if constexpr (requires {lambda(id, *typed);})
      {
        lambda(id, *typed);
      }
      else if constexpr (requires (bool cont){cont = lambda(*typed);})
      {
        if (!lambda(*typed))
        {
          return;
        }
      }
      else
      {
        lambda(*typed);
      }
    }
  }
}

//==================================================================================================
template<typename T, typename...Args>
T& EditorLevel::create_object(EditorID id, Args&&...arguments)
{
  nc_assert(!objects.contains(id));

  auto[it, ok] = objects.emplace
  (
    std::piecewise_construct, std::forward_as_tuple(id),
    std::forward_as_tuple(std::in_place_type<T>, std::forward<Args>(arguments)...)
  );

  nc_assert(ok);
  T& ref = std::get<T>(it->second);

  if constexpr (requires {this->on_object_created(id, ref);})
  {
    this->on_object_created(id, ref);
  }

  return ref;
}

//==================================================================================================
template<typename T>
T* EditorLevel::try_get_object(EditorID id)
{
  auto it = objects.find(id);
  if (it == objects.end())
  {
    return nullptr;
  }

  return std::get_if<T>(&it->second);
}

//==================================================================================================
template<typename T>
T& EditorLevel::get_object(EditorID id)
{
  T* object = this->try_get_object<T>(id);
  nc_assert(object);
  return *object;
}

//==================================================================================================
template<typename T>
const T& EditorLevel::get_object(EditorID id) const
{
  return const_cast<EditorLevel*>(this)->get_object<T>(id);
}

}

#endif // #if NC_EDITOR
