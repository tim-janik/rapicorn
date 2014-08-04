// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#include "bindable.hh"
#include <unordered_map>

namespace Rapicorn {

// == BindablePath ==
bool
BindablePath::match (const std::string &name) const
{
  if (path.empty())
    {
      BindablePath &mpath = const_cast<BindablePath&> (*this);
      mpath.plist.push_back (name);
      return false;
    }
  if (name == path)
    return true;
  if (name.size() < path.size() && path.data()[name.size()] == '.' &&
      path.compare (0, name.size(), name) == 0)
    return true;
  return false;
}

// == BindableIface ==
void
BindableIface::bindable_notify (const std::string &name) const
{
  BinadableAccessor::notify_property_change (this, name);
}

// == BinadableAccessor ==
static std::unordered_multimap<const BindableIface*, BinadableAccessor*> property_accessor_mmap;
void
BinadableAccessor::notify_property_change (const BindableIface *pa, const String &name)
{
  auto range = property_accessor_mmap.equal_range (pa);
  for (auto it = range.first; it != range.second; ++it)
    it->second->notify_property (name);
}

BinadableAccessor::StringList
BinadableAccessor::list_propertis ()
{
  BindablePath bpath;
  Any dummy;
  pa_.bindable_get (bpath, dummy);
  return bpath.plist;
}

Any
BinadableAccessor::get_property (const String &name)
{
  BindablePath bpath;
  const_cast<String&> (bpath.path) = name;
  Any any;
  pa_.bindable_get (bpath, any);
  return any;
}

void
BinadableAccessor::set_property (const String &name, const Any &any)
{
  fatal ("FIXME: unimplemented");
}

void
BinadableAccessor::notify_property (const String &name)
{
  fatal ("FIXME: unimplemented");
}

} // Rapicorn
