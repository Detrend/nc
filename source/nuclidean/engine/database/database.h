// Project Nuclidean Source File
#pragma once

#include <types.h>
#include <token.h>
#include <common.h>
#include <util/struct_to_tie.h>

#include <engine/database/database_base.h>
#include <math/vector.h>

#include <map>
#include <unordered_map>
#include <string>
#include <vector>
#include <typeindex>   // std::type_index
#include <type_traits> // std::same_as
#include <concepts>    // std::same_as
#include <memory>      // std::unique_ptr

namespace nc
{

// Concept for checking if the 
template<typename T>
concept IsDbRow = requires(T& val)
{
  !std::same_as<decltype(struct_to_tie(val)), ErrorType>;
};

// Supports both JSON and binary data.
template<typename RowType>
  requires IsDbRow<RowType>
class Database : public IDatabase
{
public:
  using KeyType = Token;

  // Initializes the database. Does not load any data yet.
  Database(Token name) : m_db_name(name) {}

  // Returns pointer to the entry if it exists.
  const RowType* try_get(const KeyType& key) const;

  // Same as above but asserts if the key does not exist.
  const RowType& get(const KeyType& key) const;

  // IDatabase
  virtual std::type_index get_row_type_index() const override;

  virtual void resolve_with(IDatabase& other_db) override;

  Token get_type() const override;
  bool  add_or_patch_row_from_file(const std::string& file_path, std::string& error) override;
  // ~IDatabase

private:
  // Unique ptr required because we don't want the row to move around in the memory.
  using RowPtr  = std::unique_ptr<RowType>;
  using DataMap = std::unordered_map<KeyType, RowPtr>;

  Token   m_db_name;
  DataMap m_data;
};


// By default most of the stuff is forbidden.
template<typename T>
struct PropertyTypeSupported : std::false_type {};

// Basic integral types are allowed.
template<typename T>
  requires std::is_integral_v<T>
struct PropertyTypeSupported<T> : std::true_type {};

// Bool is ok as well.
template<>
struct PropertyTypeSupported<bool> : std::true_type {};

// Not supporting doubles, would get converted to f32 anyway.
template<>
struct PropertyTypeSupported<f32> : std::true_type {};

// Support for tokens
template<>
struct PropertyTypeSupported<Token> : std::true_type {};

// Support for vectors
template<u64 N>
struct PropertyTypeSupported<vec<f32, N>> : std::true_type {};

// Support for having another row as a property.
template<typename T>
  requires std::is_pointer_v<T>
    && std::is_class_v<std::remove_pointer_t<T>>
      && IsDbRow<std::remove_pointer_t<T>>
struct PropertyTypeSupported<T> : std::true_type {};

// Vector of properties is ok as well.
template<typename T>
  requires PropertyTypeSupported<T>::value
struct PropertyTypeSupported<std::vector<T>> : std::true_type {};

template<u64 Len>
struct CompileTimeString
{
  char str[Len];
  constexpr CompileTimeString(const char(&input)[Len])
  {
    for (u64 i = 0; i < Len; ++i)
    {
      str[i] = input[i];
    }
  }
};

template<typename Type, CompileTimeString Name>
  requires PropertyTypeSupported<Type>::value
struct DbCol
{
  Type value;

  // Const reference that can be passed to a function without doing a copy
  operator const Type&()
  {
    return value;
  }

  Type& operator=(const Type& input)
  {
    value = input;
  }
};

}
