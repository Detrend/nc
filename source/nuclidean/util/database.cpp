// Project Nuclidean Source File
#pragma once

#include <util/database.h>

// Database types
#include <game/enemies.h>

// JSON
#include <json/json.hpp>

#include <type_traits>
#include <concepts>
#include <filesystem>
#include <format>
#include <tuple>
#include <array>
#include <fstream>

namespace nc::json_parsers
{

//==================================================================================================
template<typename Type>
void deserialize(Type& /*t*/, cstr /*name*/, const auto& /*json*/)
{
  // Non implemented
}

//==================================================================================================
template<std::integral Type>
void deserialize(Type& t, cstr name, const auto& json)
{
  auto it = json.find(name);
  if (it != json.end() && it->is_number())
  {
    t = *it;
  }
}

//==================================================================================================
template<std::floating_point Type>
void deserialize(Type& t, cstr name, const auto& json)
{
  auto it = json.find(name);
  if (it != json.end() && it->is_number())
  {
    t = *it;
  }
}

//==================================================================================================
void deserialize(Token& t, cstr name, const auto& json)
{
  auto it = json.find(name);
  if (it != json.end() && it->is_string())
  {
    t = Token{it->get_ref<const std::string&>()};
  }
}

}

namespace nc::detail
{

template<typename T, typename...Args>
concept IsConstructibleFrom = requires(Args&&...args)
{
  T{args...};
};

// The type returned when conversion to std::tie is impossible
struct ErrorType {};

// Can be constructed from any type
struct AnyType
{
  template<typename T> constexpr operator T() {};
};

// Converts all struct members into a std::tie.
// Supports structs with up-to 16 members.
template<typename Type>
auto struct_to_tie(Type& value)
{
  using AT = AnyType;
  using T  = std::decay_t<Type>;

  if constexpr(IsConstructibleFrom<T, AT, AT, AT, AT, AT, AT, AT, AT, AT, AT, AT, AT, AT, AT, AT, AT>) 
  {
    auto&[a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p] = value; return std::tie(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p);
  }
  else if constexpr(IsConstructibleFrom<T, AT, AT, AT, AT, AT, AT, AT, AT, AT, AT, AT, AT, AT, AT, AT>)
  {
    auto&[a, b, c, d, e, f, g, h, i, j, k, l, m, n, o] = value; return std::tie(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o);
  }
  else if constexpr(IsConstructibleFrom<T, AT, AT, AT, AT, AT, AT, AT, AT, AT, AT, AT, AT, AT, AT>)
  {
    auto&[a, b, c, d, e, f, g, h, i, j, k, l, m, n] = value; return std::tie(a, b, c, d, e, f, g, h, i, j, k, l, m, n);
  }
  else if constexpr(IsConstructibleFrom<T, AT, AT, AT, AT, AT, AT, AT, AT, AT, AT, AT, AT, AT>)
  {
    auto&[a, b, c, d, e, f, g, h, i, j, k, l, m] = value; return std::tie(a, b, c, d, e, f, g, h, i, j, k, l, m);
  }
  else if constexpr(IsConstructibleFrom<T, AT, AT, AT, AT, AT, AT, AT, AT, AT, AT, AT, AT>)
  {
    auto&[a, b, c, d, e, f, g, h, i, j, k, l] = value; return std::tie(a, b, c, d, e, f, g, h, i, j, k, l);
  }
  else if constexpr(IsConstructibleFrom<T, AT, AT, AT, AT, AT, AT, AT, AT, AT, AT, AT>)
  {
    auto&[a, b, c, d, e, f, g, h, i, j, k] = value; return std::tie(a, b, c, d, e, f, g, h, i, j, k);
  }
  else if constexpr(IsConstructibleFrom<T, AT, AT, AT, AT, AT, AT, AT, AT, AT, AT>)
  {
    auto&[a, b, c, d, e, f, g, h, i, j] = value; return std::tie(a, b, c, d, e, f, g, h, i, j);
  }
  else if constexpr(IsConstructibleFrom<T, AT, AT, AT, AT, AT, AT, AT, AT, AT>)
  {
    auto&[a, b, c, d, e, f, g, h, i] = value; return std::tie(a, b, c, d, e, f, g, h, i);
  }
  else if constexpr(IsConstructibleFrom<T, AT, AT, AT, AT, AT, AT, AT, AT>)
  {
    auto&[a, b, c, d, e, f, g, h] = value; return std::tie(a, b, c, d, e, f, g, h);
  }
  else if constexpr(IsConstructibleFrom<T, AT, AT, AT, AT, AT, AT, AT>)
  {
    auto&[a, b, c, d, e, f, g] = value; return std::tie(a, b, c, d, e, f, g);
  }
  else if constexpr(IsConstructibleFrom<T, AT, AT, AT, AT, AT, AT>)
  {
    auto&[a, b, c, d, e, f] = value; return std::tie(a, b, c, d, e, f);
  }
  else if constexpr(IsConstructibleFrom<T, AT, AT, AT, AT, AT>)
  {
    auto&[a, b, c, d, e] = value; return std::tie(a, b, c, d, e);
  }
  else if constexpr(IsConstructibleFrom<T, AT, AT, AT, AT>)
  {
    auto&[a, b, c, d] = value; return std::tie(a, b, c, d);
  }
  else if constexpr(IsConstructibleFrom<T, AT, AT, AT>)
  {
    auto&[a, b, c] = value; return std::tie(a, b, c);
  }
  else if constexpr(IsConstructibleFrom<T, AT, AT>)
  {
    auto&[a, b] = value; return std::tie(a, b);
  }
  else if constexpr(IsConstructibleFrom<T, AT>)
  {
    auto&[a] = value; return std::tie(a);
  }
  else
    return ErrorType{};
}

//==================================================================================================
template<typename T>
struct TupleIterator {};

template<u64...Indices>
struct TupleIterator<std::index_sequence<Indices...>>
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
static void load_row_from_json(T& row, const auto& json)
{
  // We get the list of properties here
  auto tie = struct_to_tie(row);

  // Now iterate all properties and isolate them from JSON
  tuple_for_each(tie, [&]<typename T>(T& col)
  {
    if constexpr (IsDbCol<T>::value)
    {
      constexpr cstr name = IsDbCol<T>::name;
      json_parsers::deserialize(col.value, name, json);
    }
  });
}

//==================================================================================================
template<typename RowType>
bool deserialize_row_from_json(const std::filesystem::path& path, RowType& row_out)
{
  std::ifstream file(path);
  if (!file.is_open())
  {
    return false;
  }

  auto json_data = nlohmann::json::parse(file);

  load_row_from_json(row_out, json_data);

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
Token EntityDatabase<RowType>::get_type() const
{
  return m_db_name;
}

//==================================================================================================
template<typename RowType>
  requires IsDbRow<RowType>
bool EntityDatabase<RowType>::add_or_patch_row_from_file(const std::string& file_path, std::string& error)
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
    error = std::format
    (
      "Wrong naming convention for the file \"{}\". Expected extensions: \"{}.{}\" or \"{}.{}\"",
      path.filename().string(), ext, "json", ext, "nce"
    );
    return false;
  }

  // The filename without the final extension
  std::string pure_filename = stem.substr(0, ext_idx-1);
  if (!KeyType::can_be_tokenized(pure_filename))
  {
    error = std::format("Filename \"{}\" contains invalid characters.", pure_filename);
    return false;
  }

  KeyType  key = KeyType{pure_filename};
  RowType& row = m_data[key];

  if (is_json)
  {
    return detail::deserialize_row_from_json(path, row);
  }
  else
  {
    return detail::deserialize_row_from_binary(path, row);
  }
}

//==================================================================================================
template<typename RowType>
  requires IsDbRow<RowType>
const RowType* EntityDatabase<RowType>::try_get(const KeyType& id) const
{
  if (auto it = m_data.find(id); it != m_data.end())
  {
    return &it->second;
  }

  return nullptr;
}

//==================================================================================================
template<typename RowType>
  requires IsDbRow<RowType>
const RowType& EntityDatabase<RowType>::get(const KeyType& id) const
{
  const RowType* row = this->try_get(id);
  nc_assert(row);
  return *row;
}

//==================================================================================================
// Explicit instantiations
template class EntityDatabase<EnemyStatsSmall>;

}
