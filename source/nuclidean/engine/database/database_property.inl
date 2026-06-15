// Project Nuclidean Source File

#include <common.h> // nc_assert
#include <engine/database/database_property.h>
#include <engine/database/database.h>

namespace nc
{

//==================================================================================================
template<typename T>
  requires std::is_pointer_v<T> && !std::is_const_v<T>
UnresolvedDbProperty<T>::UnresolvedDbProperty(Token key, T& ref)
: key(key)
, reference(ref)
, row_type(typeid(std::remove_pointer_t<T>))
{

}

//==================================================================================================
template<typename T>
  requires std::is_pointer_v<T> && !std::is_const_v<T>
bool UnresolvedDbProperty<T>::resolve(IDatabase& with)
{
  using NoPtrT = std::remove_pointer_t<T>;

  // Check if the type matches
  nc_assert(with.get_row_type_index() == row_type);

  // Can be safely casted
  Database<NoPtrT>* typed_db = static_cast<Database<NoPtrT>*>(&with);
  const NoPtrT* value = typed_db->try_get(key);
  if (!value)
  {
    // Key not present
    reference = nullptr;
    return false;
  }

  // All ok
  reference = const_cast<NoPtrT*>(value);
  return true;
}

//==================================================================================================
template<typename T>
  requires std::is_pointer_v<T> && !std::is_const_v<T>
std::type_index UnresolvedDbProperty<T>::get_row_type_index() const
{
  return row_type;
}

}
