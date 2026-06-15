// Project Nuclidean Source File
#pragma once

#include <engine/database/database.h>
#include <engine/database/database_property.h>
#include <util/struct_to_tie.h>

// Database types
#include <game/enemies.h>

// JSON
#include <json/json.hpp>

#include <utility>
#include <type_traits>
#include <concepts>
#include <filesystem>
#include <format>
#include <tuple>
#include <array>
#include <fstream>

namespace nc
{

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
    auto* ptr = new UnresolvedDbProperty<T>{key, ref};
    db.push_unresolved_property(std::unique_ptr<IUnresolvedDbProperty>{ptr});
  }

  enum class ErrorType : u8
  {
    log,
    warn,
    error,
  };

  template<typename...Args>
  void report(ErrorType /*type*/, cstr /*txt*/, Args.../*args*/)
  {
    
  }

  template<typename...Args>
  void log(cstr txt, Args...args)
  {
    this->report(ErrorType::log, txt, std::forward<Args>(args)...);
  }

  template<typename...Args>
  void warn(cstr txt, Args...args)
  {
    this->report(ErrorType::warn, txt, std::forward<Args>(args)...);
  }

  template<typename...Args>
  void error(cstr txt, Args...args)
  {
    this->report(ErrorType::error, txt, std::forward<Args>(args)...);
  }

private:
  IDatabase& db;
};

}

namespace nc::json_parsers
{

//==================================================================================================
auto type_check_int    = [](const auto& json_it) { return json_it->is_number_integer(); };
auto type_check_float  = [](const auto& json_it) { return json_it->is_number_float();   };
auto type_check_bool   = [](const auto& json_it) { return json_it->is_boolean();        };
auto type_check_string = [](const auto& json_it) { return json_it->is_string();         };

//==================================================================================================
template<typename TypeCheck>
static bool possible_quick_exit(cstr name, const auto& json, DbSerializationCtx& ctx, TypeCheck&& check)
{
  auto it = json.find(name);
  if (it == json.end())
  {
    ctx.log("Value for property \"{}\" missing in JSON, using default value.", name);
    return false;
  }

  if (!check(it))
  {
    ctx.warn("Value for property \"{}\" has incorrect type. Expected: {}, got: {}", name);
    return false;
  }

  return true;
}

//==================================================================================================
template<typename Type>
void deserialize(Type& /*t*/, cstr /*name*/, const auto& /*json*/, DbSerializationCtx& ctx)
{
  // Non implemented
}

//==================================================================================================
template<std::integral Type>
void deserialize(Type& t, cstr name, const auto& json, DbSerializationCtx& ctx)
{
  if (possible_quick_exit(name, json, ctx, type_check_int))
  {
    t = json[name];
  }
}

//==================================================================================================
template<typename Type>
  requires std::is_pointer_v<Type>
void deserialize(Type& t, cstr name, const auto& json, DbSerializationCtx& ctx)
{
  auto it = json.find(name);
  if (it == json.end())
  {
    ctx.warn("Foreign property \"{}\" not found in the JSON, "
             "can't link it to foreign table.", name);
    t = nullptr;
    return;
  }

  if (!it->is_string())
  {
    ctx.error("Foreign property \"{}\" has to be a string that indexes "
              "into other table.", name);
    t = nullptr;
    return;
  }

  Token key = Token{it->get_ref<const std::string&>()};
  ctx.push_unresolved<Type>(key, t);
}

//==================================================================================================
template<std::floating_point Type>
void deserialize(Type& t, cstr name, const auto& json, DbSerializationCtx& ctx)
{
  if (possible_quick_exit(name, json, ctx, type_check_float))
  {
    t = json[name];
  }
}

//==================================================================================================
void deserialize(Token& t, cstr name, const auto& json, DbSerializationCtx& ctx)
{
  if (possible_quick_exit(name, json, ctx, type_check_string))
  {
    auto it = json.find(name);
    const std::string& ref = it->get_ref<const std::string&>();

    if (ref.length() > Token::MAX_LENGTH || !Token::can_be_tokenized(ref))
    {
      ctx.warn("Token property \"{}\" cannot be deserialized as the value is "
               "either too long or contains unsupported characters.", name);
      return;
    }

    t = Token{ref};
  }
}

}

namespace nc::detail
{

//==================================================================================================
template<typename T>
struct TupleIterator
{
  template<typename T, typename F>
  static void iterate(T& tuple, F&& func)
  {

  }
};

template<u64...Indices>
struct TupleIterator < std::index_sequence<Indices...> >
{
  template<u64 Idx, typename T, typename F>
  static void apply_one(T& tuple, F&& func)
  {
    func(std::get<Idx>(tuple));
  }

  template<typename T, typename F>
  static void iterate(T& tuple, F&& func)
  {
    (apply_one<Indices>(tuple, std::forward<F>(func)), ...);
  }
};

//==================================================================================================
template<typename T, typename F>
void tuple_for_each(T& tuple, F&& func)
{
  constexpr u64 tuple_size = std::tuple_size_v<T>;
  using Sequence = std::make_index_sequence<tuple_size>;
  TupleIterator<Sequence>::template iterate(tuple, std::forward<F>(func));
}

//==================================================================================================
template<typename T>
struct IsDbCol : std::false_type{};

template<typename T, CompileTimeString Name>
struct IsDbCol<DbCol<T, Name>> : std::true_type
{
  using Type = T;
  static constexpr cstr name = Name.str;
};

//==================================================================================================
template<typename T>
static void load_row_from_json(T& row, const auto& json, DbSerializationCtx& ctx)
{
  // We get the list of properties here
  auto tie = struct_to_tie(row);

  // Now iterate all properties and isolate them from JSON
  tuple_for_each(tie, [&]<typename T>(T& col)
  {
    if constexpr (IsDbCol<T>::value)
    {
      constexpr cstr name = IsDbCol<T>::name;
      json_parsers::deserialize(col.value, name, json, ctx);
    }
  });
}

//==================================================================================================
template<typename RowType>
bool deserialize_row_from_json
(
  const std::filesystem::path& path, RowType& row_out, DbSerializationCtx& ctx
)
{
  std::ifstream file(path);
  if (!file.is_open())
  {
    return false;
  }

  auto json_data = nlohmann::json::parse(file);

  load_row_from_json(row_out, json_data, ctx);

  return true;
}

//==================================================================================================
template<typename RowType>
bool deserialize_row_from_binary(const std::filesystem::path& /*path*/, RowType& /*row_out*/)
{
  return true;
}

}

namespace nc
{

//==================================================================================================
IDatabase::IDatabase()
{
  get_db_list_mut().push_back(this);
}

//==================================================================================================
IDatabase::~IDatabase()
{
  std::erase(get_db_list_mut(), this);
}

//==================================================================================================
/*static*/ IDatabase::DbList& IDatabase::get_db_list_mut()
{
  static DbList list;
  return list;
}

//==================================================================================================
/*static*/ const IDatabase::DbList& IDatabase::get_db_list()
{
  return get_db_list_mut();
}

//==================================================================================================
template<typename RowType>
  requires IsDbRow<RowType>
std::type_index Database<RowType>::get_row_type_index() const
{
  return std::type_index{typeid(RowType)};
}

//==================================================================================================
template<typename RowType>
  requires IsDbRow<RowType>
void Database<RowType>::resolve_with(IDatabase& other_db)
{
  std::type_index type = other_db.get_row_type_index();

  if (auto it = m_unresolved.find(type); it != m_unresolved.end())
  {
    for (PropertyPtr& ptr : it->second)
    {
      if (!ptr->resolve(other_db))
      {
        // Report an error?
      }
    }

    // Clear this unresolved list
    it->second.clear();
  }
}

//==================================================================================================
template<typename RowType>
  requires IsDbRow<RowType>
Token Database<RowType>::get_type() const
{
  return m_db_name;
}

//==================================================================================================
template<typename RowType>
  requires IsDbRow<RowType>
bool Database<RowType>::add_or_patch_row_from_file(const std::string& file_path, std::string& error)
{
  // Check if the file is binary or a JSON
  std::filesystem::path path = file_path;

  bool is_json = path.extension() == ".json";
  bool is_bin  = path.extension() == ".nce";

  // Report failure if incorrect extension
  if (!is_json && !is_bin)
  {
    return false;
  }

  // Isolate the key from the filename
  std::string stem = path.stem().string();
  std::string ext  = this->get_type().to_string();

  u64 stem_len = stem.length();
  u64 ext_len  = ext.length();
  u64 ext_idx  = stem.find(ext);

  // Check if the second extension is correct
  if (ext_idx == std::string::npos || ext_idx + ext_len != stem_len)
  {
    return false;
  }

  // The filename without the final extension
  std::string pure_filename = stem.substr(0, ext_idx-1);
  if (!KeyType::can_be_tokenized(pure_filename))
  {
    error = std::format("Filename \"{}\" contains invalid characters.", pure_filename);
    return false;
  }

  KeyType key = KeyType{pure_filename};
  RowPtr& ptr = m_data[key];

  // Create for the first time
  if (!ptr)
  {
    ptr = std::make_unique<RowType>();
  }

  DbSerializationCtx ctx(*this);

  if (is_json)
  {
    return detail::deserialize_row_from_json(path, *ptr, ctx);
  }
  else
  {
    return detail::deserialize_row_from_binary(path, *ptr);
  }
}

//==================================================================================================
template<typename RowType>
  requires IsDbRow<RowType>
const RowType* Database<RowType>::try_get(const KeyType& id) const
{
  if (auto it = m_data.find(id); it != m_data.end())
  {
    return it->second.get();
  }

  return nullptr;
}

//==================================================================================================
template<typename RowType>
  requires IsDbRow<RowType>
const RowType& Database<RowType>::get(const KeyType& id) const
{
  const RowType* row = this->try_get(id);
  nc_assert(row);
  return *row;
}

//==================================================================================================
// Explicit instantiations
template class Database<EnemyStatsSmall>;

}
