// Project Nuclidean Source File
#pragma once

#include <types.h>
#include <token.h>
#include <common.h>
#include <util/struct_to_tie.h>

#include <unordered_map>
#include <string>
#include <vector>
#include <typeinfo>

namespace nc
{

struct IUnresolvedProperty
{
  virtual bool try_resolve(class IDatabase& with) = 0;
  virtual ~IUnresolvedProperty(){};
};

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

  virtual const std::type_info& get_row_type_info() const = 0;

  virtual void resolve_with(IDatabase& other) = 0;

  static const DbList& get_db_list();
  static       DbList& get_db_list_mut();

  void push_unresolved_property(std::unique_ptr<IUnresolvedProperty>&& prop);

protected:
  using UnresolvedProperties = std::vector<std::unique_ptr<IUnresolvedProperty>>;
  UnresolvedProperties m_unresolved;
};

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

template<typename T>
concept IsDbRow = requires(T& val)
{
  !std::same_as<decltype(struct_to_tie(val)), ErrorType>;
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

// Support for having another row as a property.
template<typename T>
  requires std::is_class_v<T> && IsDbRow<T>
struct PropertyTypeSupported<T> : std::true_type {};

// Vector of properties is ok as well.
template<typename T>
  requires PropertyTypeSupported<T>::value
struct PropertyTypeSupported<std::vector<T>> : std::true_type {};

template<typename Type, CompileTimeString Name>
  requires PropertyTypeSupported<Type>::value
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

// Supports both JSON and binary data.
template<typename RowType>
  requires IsDbRow<RowType>
class EntityDatabase : public IDatabase
{
public:
  using KeyType = Token;

  // Initializes the database. Does not load any data yet.
  EntityDatabase(Token name) : m_db_name(name) {}

  // Returns pointer to the entry if it exists.
  const RowType* try_get(const KeyType& key) const;

  // Same as above but asserts if the key does not exist.
  const RowType& get(const KeyType& key) const;

  // IDatabase
  virtual const std::type_info& get_row_type_info() const override
  {
    return typeid(RowType);
  }

  virtual void resolve_with(IDatabase& other_db) override
  {
    for (u64 i = 0; i < m_unresolved.size(); ++i)
    {
      auto& prop_ptr = m_unresolved[i];
      if (prop_ptr->try_resolve(other_db))
      {
        // Success..
      }
      else
      {
        // Failure, not resolved. Report to log?
      }
    }

    m_unresolved.clear();
  }

  Token get_type() const override;
  bool  add_or_patch_row_from_file(const std::string& file_path, std::string& error) override;
  // ~IDatabase

private:
  Token                                m_db_name;
  std::unordered_map<KeyType, RowType> m_data;
};

template<typename T>
struct UnresolvedProperty : IUnresolvedProperty
{
  virtual bool try_resolve(IDatabase& with) override
  {
    // Check if the type matches
    if (with.get_row_type_info() != row_info)
    {
      return false;
    }

    // Can be safely casted
    EntityDatabase<T>* typed_db = static_cast<EntityDatabase<T>*>(&with);
    const T* value = typed_db->try_get(key);
    if (!value)
    {
      // Key not present
      reference = nullptr;
      return false;
    }

    // All ok
    reference = const_cast<T*>(value);
    return true;
  }

  Token                 key;
  T&                    reference; // to a pointer
  const std::type_info& row_info;
};

class DbSerializationCtx
{
public:
  DbSerializationCtx(IDatabase& database)
  : db(database)
  {
    
  }

  template<typename T>
  void push_unresolved(Token key, T& ref)
  {
    db.push_unresolved_property(std::make_unique<UnresolvedProperty>(key, ref, typeid(std::remove_cv_t<T>)));
  }

  enum ErrorType : u8
  {
    warn,
    error,
  };

  template<typename...Args>
  void report(ErrorType /*type*/, cstr /*txt*/, Args.../*args*/)
  {
    
  }

private:
  IDatabase& db;
};

}
