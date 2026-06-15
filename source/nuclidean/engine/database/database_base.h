// Project Nuclidean Source File
#pragma once

#include <token.h>

#include <typeindex>
#include <string>
#include <vector>
#include <memory>
#include <map>

namespace nc
{

struct IUnresolvedDbProperty;

class IDatabase
{
public:
  using DbList = std::vector<IDatabase*>;

  IDatabase();
  virtual ~IDatabase();

  // Returns the "suffix" of the supported filetype.
  // If loading from JSON then the filename will be in the format "filename.suffix.json".
  // If loading from binary then the filename will be in the format "filename.suffix.nce".
  virtual Token get_type() const = 0;

  // Called if the suffix matches. Should return true if the loading was succesful, false otherwise.
  virtual bool add_or_patch_row_from_file(const std::string& file_path, std::string& error) = 0;

  // Returns runtime information of the row type.
  virtual std::type_index get_row_type_index() const = 0;

  // Tries resolving the unresolved properties from the rows of the given database.
  virtual void resolve_with(IDatabase& other) = 0;

  // Internal use, do not call this.
  void push_unresolved_property(std::unique_ptr<IUnresolvedDbProperty>&& prop);

  // Helpers for querying the list of databases, you will probably never need this.
  static const DbList& get_db_list();
  static       DbList& get_db_list_mut();

protected:
  using PropertyPtr          = std::unique_ptr<IUnresolvedDbProperty>;
  using PropertyList         = std::vector<PropertyPtr>;
  using UnresolvedProperties = std::map<std::type_index, PropertyList>;

  UnresolvedProperties m_unresolved;
};

}
