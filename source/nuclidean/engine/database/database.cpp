// Project Nuclidean Source File
#pragma once

#include <engine/database/database.h>
#include <engine/database/database_property.h>
#include <util/struct_to_tie.h>

// Database types
#include <game/enemies.h>
#include <game/projectiles.h>

// JSON
#include <json/json.hpp>
#include <util/evil_enum.h>

#include <math/vector.h>

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
auto type_check_int    = [](const auto& json_it) { return json_it.is_number_integer(); };
auto type_check_float  = [](const auto& json_it) { return json_it.is_number_float();   };
auto type_check_bool   = [](const auto& json_it) { return json_it.is_boolean();        };
auto type_check_string = [](const auto& json_it) { return json_it.is_string();         };
auto type_check_array  = [](const auto& json_it) { return json_it.is_array();          };

//==================================================================================================
template<typename TypeCheck>
static bool check_type(const nlohmann::json& json, DbSerializationCtx& ctx, TypeCheck&& check)
{
  if (!check(json))
  {
    ctx.warn("Value for property has incorrect type.");
    return false;
  }

  return true;
}

//==================================================================================================
// Primary template - unsupported types.
template<typename Type>
struct Deserializer
{
  static bool run(Type& /*t*/, const nlohmann::json& /*json*/, DbSerializationCtx& /*ctx*/)
  {
    // Non implemented
    nc_assert(false);
    return false;
  }
};

//==================================================================================================
template<std::integral Type>
struct Deserializer<Type>
{
  static bool run(Type& t, const nlohmann::json& json, DbSerializationCtx& ctx)
  {
    if (!check_type(json, ctx, type_check_int))
    {
      return false;
    }

    t = json;
    return true;
  }
};

//==================================================================================================
template<typename Type>
  requires std::is_pointer_v<Type>
struct Deserializer<Type>
{
  static bool run(Type& t, const nlohmann::json& json, DbSerializationCtx& ctx)
  {
    if (!json.is_string())
    {
      ctx.error("Foreign property has to be a string that indexes into other table.");
      t = nullptr;
      return false;
    }

    Token key = Token{json.get_ref<const std::string&>()};
    ctx.push_unresolved<Type>(key, t);
    return true;
  }
};

//==================================================================================================
template<std::floating_point Type>
struct Deserializer<Type>
{
  static bool run(Type& t, const nlohmann::json& json, DbSerializationCtx& ctx)
  {
    if (!check_type(json, ctx, type_check_float))
    {
      return false;
    }

    t = json;
    return true;
  }
};

//==================================================================================================
template<>
struct Deserializer<bool>
{
  static bool run(bool& value, const nlohmann::json& json, DbSerializationCtx& ctx)
  {
    if (!check_type(json, ctx, type_check_bool))
    {
      return false;
    }

    value = bool{json};
    return true;
  }
};

//==================================================================================================
template<u64 NUM_COMPONENTS, typename ComponentType>
struct Deserializer<glm::vec<NUM_COMPONENTS, ComponentType>>
{
  using VecType = glm::vec<NUM_COMPONENTS, ComponentType>;

  static bool run(VecType& value, const nlohmann::json& json, DbSerializationCtx& ctx)
  {
    if (!check_type(json, ctx, type_check_array) || json.size() != NUM_COMPONENTS)
    {
      return false;
    }

    bool ok = true;
    for (typename VecType::length_type i = 0; i < NUM_COMPONENTS; ++i)
    {
      ok &= Deserializer<ComponentType>::run(value[i], json[i], ctx);
    }

    return ok;
  }
};

//==================================================================================================
template<>
struct Deserializer<Token>
{
  static bool run(Token& t, const nlohmann::json& json, DbSerializationCtx& ctx)
  {
    if (!check_type(json, ctx, type_check_string))
    {
      return false;
    }

    const std::string& ref = json.get_ref<const std::string&>();

    if (ref.length() > Token::MAX_LENGTH || !Token::can_be_tokenized(ref))
    {
      ctx.warn("Token property cannot be deserialized as the value is "
               "either too long or contains unsupported characters.");
      return false;
    }

    t = Token{ref};
    return true;
  }
};

//==================================================================================================
template<typename EnumType>
  requires std::is_enum_v<EnumType> && std::is_same_v<std::underlying_type_t<EnumType>, u8>
struct Deserializer<EnumType>
{
  static bool run(EnumType& enum_value, const nlohmann::json& json, DbSerializationCtx& ctx)
  {
    if (!check_type(json, ctx, type_check_string))
    {
      return false;
    }

    const std::string& ref = json.get_ref<const std::string&>();

    if (!EnumNameTable<EnumType>::name_exists(ref))
    {
      ctx.warn("Can't deserialize enum property because the value \"{}\" is not a known "
               "enum item.", ref);
      return false;
    }

    enum_value = EnumNameTable<EnumType>::get_value_for_name(ref);
    return true;
  }
};

//==================================================================================================
template<typename InnerType>
struct Deserializer<std::vector<InnerType>>
{
  static bool run(std::vector<InnerType>& container, const nlohmann::json& json, DbSerializationCtx& ctx)
  {
    if (!check_type(json, ctx, type_check_array))
    {
      return false;
    }

    bool ok = true;
    for (auto it : json)
    {
      InnerType& ref = container.emplace_back();
      ok &= Deserializer<InnerType>::run(ref, it, ctx);
    }

    return ok;
  }
};

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
static bool load_row_from_json(T& row, const nlohmann::json& json, DbSerializationCtx& ctx)
{
  // We get the list of properties here
  auto tie = struct_to_tie(row);

  // Now iterate all properties and isolate them from JSON
  bool ok = true;
  tuple_for_each(tie, [&]<typename T>(T& col)
  {
    if constexpr (IsDbCol<T>::value)
    {
      if (auto it = json.find(IsDbCol<T>::name); it != json.end())
      {
        using ColType = typename IsDbCol<T>::Type;
        ok &= json_parsers::Deserializer<ColType>::run(col.value, *it, ctx);
      }
      else
      {
        // Keep the default value if not present..
      }
    }
  });

  return ok;
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
  return load_row_from_json(row_out, json_data, ctx);
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

  bool retval = false;
  if (is_json)
  {
    retval = detail::deserialize_row_from_json(path, *ptr, ctx);
  }
  else
  {
    retval = detail::deserialize_row_from_binary(path, *ptr);
  }

  return retval;
}

//==================================================================================================
template<typename RowType>
  requires IsDbRow<RowType>
bool Database<RowType>::contains(const KeyType& id) const
{
  return this->try_get(id) != nullptr;
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
template class Database<EnemyStats>;
template class Database<ProjectileStatsDb>;

}
