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
  // Returns the "suffix" of the supported filetype.
  // If loading from JSON then the filename will be in the format "filename.suffix.json".
  // If loading from binary then the filename will be in the format "filename.suffix.nce".
  virtual Token get_type() const = 0;

  // Called if the suffix matches. Should return true if the loading was succesful, false otherwise.
  virtual bool add_or_patch_row_from_file(const std::string& file_path, std::string& error) = 0;
};

template<typename T>
static std::vector<cstr>& get_db_rows_mut();

template<typename T>
static const std::vector<cstr>& get_db_rows();

#define DB_ROW(_type, _name)                  \
  get_db_rows_mut<_type>().push_back(#_name); \
  _type _name;

// Implement
template<typename T>
auto to_tuple();

// Implement
template<typename T>
using to_tuple_t = decltype(to_tuple<T>());

// Supports both JSON and binary data.
template<typename RowType>
  requires IsDbRow<RowType>
class EntityDatabase
{
public:
  using KeyType = Token;

  // Returns pointer to the entry if it exists.
  const RowType* try_get(const KeyType& key) const;

  // Same as above but asserts if the key does not exist.
  const RowType& get(const KeyType& key) const;

  // IDatabase
  Token get_type() const override;
  bool  add_or_patch_row_from_file(const std::string& file_path, std::string& error) override;
  // ~IDatabase

private:
  std::unordered_map<KeyType, RowType> m_data;
};

}

#include <database.inl>
