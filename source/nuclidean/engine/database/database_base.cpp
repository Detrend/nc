// Project Nuclidean Source File

#include <engine/database/database_base.h>
#include <engine/database/database_property.h>

namespace nc
{

//==================================================================================================
void IDatabase::push_unresolved_property(std::unique_ptr<IUnresolvedDbProperty>&& prop)
{
  std::type_index type = prop->get_row_type_index();
  m_unresolved[type].push_back(std::move(prop));
}

}
