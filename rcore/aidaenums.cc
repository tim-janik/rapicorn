// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#include "aidaenums.hh"
#include "thread.hh"
#include <cstring>

namespace Rapicorn { namespace Aida {

static Mutex                       enum_info_mutex;
static std::map<String, EnumInfo> *enum_info_map = NULL;

EnumInfo
EnumInfo::from_nsid (const char *enamespace, const char *ename)
{
  ScopedLock<Mutex> locker (enum_info_mutex);
  if (enum_info_map)
    {
      const String fqen = enamespace + String ("::") + ename;
      auto it = enum_info_map->find (fqen);
      if (it != enum_info_map->end())
        return it->second;
    }
  return *(EnumInfo*)0; // shouldn't happen
}

void
EnumInfo::enlist (const EnumInfo &enum_info)
{
  ScopedLock<Mutex> locker (enum_info_mutex);
  if (!enum_info_map)
    {
      static uint64 space[sizeof (*enum_info_map) / sizeof (uint64)];
      enum_info_map = new (space) typeof (*enum_info_map);
    }
  const String fqen = enum_info.enum_namespace() + String ("::") + enum_info.enum_name();
  (*enum_info_map)[fqen] = enum_info; // dups are simply overwritten
  // dups are common if client and server bindings are linked into the same binary
}

EnumInfo&
EnumInfo::operator= (const EnumInfo &src)
{
  n_values_ = src.n_values_;
  values_ = src.values_;
  name_ = src.name_;
  namespace_ = src.namespace_;
  flag_combinable_ = src.flag_combinable_;
  return *this;
}

EnumInfo::EnumInfo () :
  namespace_ (NULL), name_ (NULL), values_ (NULL), n_values_ (0), flag_combinable_ (0)
{}

} } // Rapicorn::Aida
