// Project Nuclidean Source File
#pragma once

#include <types.h>
#include <token.h>
#include <common.h>

#include <unordered_map>
#include <string>
#include <vector>

namespace nc
{

class IDatabase
{
public:
  using DbList = std::vector<IDatabase*>;

  IDatabase();
  ~IDatabase();

  // Returns the "suffix" of the supported filetype.
  // If loading from JSON then the filename will be in the format "filename.suffix.json".
  // If loading from binary then the filename will be in the format "filename.suffix.nce".
  virtual Token get_type() const = 0;

  // Called if the suffix matches. Should return true if the loading was succesful, false otherwise.
  virtual bool add_or_patch_row_from_file(const std::string& file_path, std::string& error) = 0;

  static const DbList& get_db_list();

private:
  static DbList& get_db_list_mut();
};

// Implement
template<typename T>
using to_tuple_t = decltype(to_tuple<T>());

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
struct DbCol
{
  static constexpr cstr col_name = Name.str;
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

template<typename T>
concept IsDbRow = true;

// Supports both JSON and binary data.
template<typename RowType>
  requires IsDbRow<RowType>
class EntityDatabase : public IDatabase
{
public:
  using KeyType = Token;

  // Initializes the database. Does not load any data yet.
  EntityDatabase(Token name)
  : m_db_name(name)
  {

  }

  // Returns pointer to the entry if it exists.
  const RowType* try_get(const KeyType& key) const;

  // Same as above but asserts if the key does not exist.
  const RowType& get(const KeyType& key) const;

  // IDatabase
  Token get_type() const override;
  bool  add_or_patch_row_from_file(const std::string& file_path, std::string& error) override;
  // ~IDatabase

private:
  Token                                m_db_name;
  std::unordered_map<KeyType, RowType> m_data;
};

}
