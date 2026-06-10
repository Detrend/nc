// Project Nuclidean Source File
#include <engine/database/database_system.h>
#include <util/database.h>
#include <engine/core/engine_module_types.h>

#include <filesystem>
#include <string>

namespace nc
{

//==================================================================================================
/*static*/ EngineModuleId DatabaseSystem::get_module_id()
{
  return EngineModule::database_system;
}

//==================================================================================================
void DatabaseSystem::on_event(ModuleEvent& /*event*/)
{

}

//==================================================================================================
bool DatabaseSystem::init()
{
  // Iterate all ingame files and give them to databases..
  auto& db_list = IDatabase::get_db_list_mut();
  u64 db_cnt = db_list.size();

  std::filesystem::path data_path = "content/data";

  for (const auto& entry : std::filesystem::recursive_directory_iterator(data_path))
  {
    const std::filesystem::path& entry_path = entry.path();

    if (!entry_path.has_extension())
      continue;

    const std::filesystem::path extension = entry_path.extension();
    if (extension == ".json" || extension == ".nce")
    {
      for (IDatabase* db : db_list)
      {
        std::string error;
        if (db->add_or_patch_row_from_file(entry_path.string(), error))
          break;
      }
    }
  }

  // Resolve databases with each other after the full loading
  for (u64 i = 0; i < db_cnt; ++i)
  {
    for (u64 j = 0; j < db_cnt; ++j)
    {
      if (i != j)
      {
        db_list[i]->resolve_with(*db_list[j]);
      }
    }
  }

  return true;
}

}
