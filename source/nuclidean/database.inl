// Project Nuclidean Source File
#pragma once

#include <database.h>

#include <filesystem>
#include <format>
#include <tuple>
#include <array>

namespace nc::detail
{

//==================================================================================================
template<typename Tuple>
constexpr auto get_property_offsets()
{
  std::array<u64, std::tuple_size_v<Tuple>> offsets;
  Tuple dummy;

  u64 offset = 0;
}

//==================================================================================================
template<typename T, typename F>
void visit_row_properties(T& row, F func)
{
  using row_tuple = to_tuple_t<T>;
  constexpr u64 tuple_size = std::tuple_size_v<row_tuple>;
  nc_assert(get_db_rows().size() == tuple_size); // Code bug

  using sequence = std::make_integer_sequence<u64, tuple_size>;
}

//==================================================================================================
template<typename KeyType, typename RowType>
bool deserialize_row_from_json(const std::filesystem::path& path, RowType& row_out)
{
  
}

//==================================================================================================
template<typename KeyType, typename RowType>
bool deserialize_row_from_binary(const std::filesystem::path& path, RowType& row_out)
{
  
}

}

namespace nc
{

//==================================================================================================
template<typename KeyType, typename RowType>
bool EntityDatabase::add_or_patch_row_from_file(const std::string& file_path, std::string& error)
{
  // Check if the file is binary or a JSON
  std::filesystem::path path = file_path;

  bool is_json = path.extension() == ".json";
  bool is_bin  = path.extension() == ".nce";

  // Report failure if incorrect extension
  if (!is_json || !is_bin)
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
  bool error = ext_idx != std::string::npos || ext_idx + ext_len != stem_len;
  if (error)
  {
    error = std::format
    (
      "Wrong naming convention for the file \"{}\". Expected extensions: \"{}.{}\" or \"{}.{}\"",
      path.filename(), ext, "json", ext, "nce"
    );
    return false;
  }

  // The filename without the final extension
  std::string pure_filename = stem.substr(0, ext_idx);
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

}
