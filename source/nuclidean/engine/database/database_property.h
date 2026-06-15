// Project Nuclidean Source File
#pragma once

#include <util/struct_to_tie.h>
#include <token.h>

#include <type_traits> // std::false_type/std::true_type
#include <typeindex> // std::type_index

namespace nc
{

class IDatabase;

struct IUnresolvedDbProperty
{
  // Returns the type of the foreign db row this unresolved property expects.
  virtual std::type_index get_row_type_index() const = 0;

  // Resolves the property. IDatabase has rows of the same type as "get_row_type".
  virtual bool resolve(IDatabase& with) = 0;

  // Virtual destructor because C++ is a retarded language and we have to do this everywhere.
  virtual ~IUnresolvedDbProperty(){};
};

template<typename T>
  requires std::is_pointer_v<T> && !std::is_const_v<T>
struct UnresolvedDbProperty : IUnresolvedDbProperty
{
  UnresolvedDbProperty(Token key, T& ref);

  virtual bool resolve(IDatabase& with) override;

  virtual std::type_index get_row_type_index() const override;

  Token           key;
  T&              reference; // to a pointer
  std::type_index row_type;
};

}

#include <engine/database/database_property.inl>
