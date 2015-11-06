// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#include "aidaprops.hh"
#include "thread.hh"
#include <set>
#include <unordered_map>
#include <cstring>
#include <malloc.h>

namespace Rapicorn { namespace Aida {

static inline String
propcanonify (String s)
{
  for (uint i = 0; i < s.size(); i++)
    if (!((s[i] >= 'A' && s[i] <= 'Z') ||
          (s[i] >= 'a' && s[i] <= 'z') ||
          (s[i] >= '0' && s[i] <= '9') ||
          s[i] == '_'))
      s[i] = '_';
  return s;
}

// == PropertyHostInterface ==
const PropertyList&
ImplicitBase::__aida_properties__ ()
{
  static const PropertyList empty_property_list;
  return empty_property_list;
  // default implementation to allow explicit calls
}

static Mutex plist_map_mutex;

Property*
ImplicitBase::__aida_lookup__ (const String &property_name)
{
  // provide PropertyMaps globally
  typedef std::unordered_map<String, Property*> PropertyMap;
  static std::map<const PropertyList*,PropertyMap*> *plist_map = NULL;
  do_once {
    static uint64 space[sizeof (*plist_map) / sizeof (uint64)];
    plist_map = new (space) std::map<const PropertyList*,PropertyMap*>();
  }
  // find or construct property map
  const PropertyList &plist = __aida_properties__();
  ScopedLock<Mutex> plist_map_locker (plist_map_mutex);
  PropertyMap *pmap = (*plist_map)[&plist];
  if (!pmap)
    {
      pmap = new PropertyMap;
      size_t n_properties = 0;
      Property **properties = plist.list_properties (&n_properties);
      for (uint i = 0; i < n_properties; i++)
        {
          Property *prop = properties[i];
          if (prop)
            (*pmap)[prop->ident] = prop;
        }
      (*plist_map)[&plist] = pmap;
    }
  plist_map_locker.unlock();
  PropertyMap::iterator it = pmap->find (property_name);
  if (it == pmap->end())        // try canonicalized
    it = pmap->find (string_substitute_char (property_name, '-', '_'));
  if (it != pmap->end())
    return it->second;
  else
    return NULL;
}

bool
ImplicitBase::__aida_setter__ (const String &property_name, const String &value)
{
  Property *prop = __aida_lookup__ (property_name);
  if (!prop)
    return false;
  prop->set_value (*this, value);
  return true;
}

String
ImplicitBase::__aida_getter__ (const String &property_name)
{
  Property *prop = __aida_lookup__ (property_name);
  if (!prop)
    return ""; // be more verbose here?
  return prop->get_value (*this);
}

// == Parameter ==
Parameter::~Parameter()
{}

void
Parameter::set (const Any &any)
{
  return setter_ (any);
}

Any
Parameter::get () const
{
  return getter_();
}

String
Parameter::name () const
{
  return name_;
}

String
Parameter::find_aux (const std::vector<String> &auxvector, const String &field_name, const String &key, const String &fallback)
{
  const String name = field_name + "." + key + "=";
  for (const auto &kv : auxvector)
    if (name.compare (0, name.size(), kv, 0, name.size()) == 0)
      return kv.substr (name.size());
  return fallback;
}

// == PropertyList ==
Property::Property (const char *cident, const char *clabel, const char *cblurb, const char *chints) :
  ident (cident),
  label (clabel ? strdup (clabel) : NULL),
  blurb (cblurb ? strdup (cblurb) : NULL),
  hints (chints ? strdup (chints) : NULL)
{
  assert (ident != NULL);
  ident = strdup (propcanonify (ident).c_str());
}

Property::~Property()
{
  if (ident)
    free (const_cast<char*> (ident));
  if (label)
    free (label);
  if (blurb)
    free (blurb);
  if (hints)
    free (hints);
}

bool
Property::readable () const
{
  if (hints)
    return string_option_check (hints, "rw") || string_option_check (hints, "ro");
  return false;
}

bool
Property::writable () const
{
  if (hints)
    return string_option_check (hints, "rw") || string_option_check (hints, "wo");
  return false;
}

// == PropertyList ==
void
PropertyList::append_properties (size_t n_props, Property **props, const PropertyList &c0, const PropertyList &c1,
                                 const PropertyList &c2, const PropertyList &c3, const PropertyList &c4, const PropertyList &c5,
                                 const PropertyList &c6, const PropertyList &c7, const PropertyList &c8, const PropertyList &c9)
{
  std::set<Property*> pset;
  std::vector<Property*> parray;
  for (size_t i = 0; i < n_properties_; i++)
    if (pset.find (properties_[i]) == pset.end())
      {
        pset.insert (properties_[i]);
        parray.push_back (properties_[i]);
      }
  for (size_t i = 0; i < n_props; i++)
    if (pset.find (props[i]) == pset.end())
      {
        pset.insert (props[i]);
        parray.push_back (props[i]);
      }
  const PropertyList *const chains[] = { &c0, &c1, &c2, &c3, &c4, &c5, &c6, &c7, &c8, &c9 };
  for (size_t j = 0; j < sizeof (chains) / sizeof (chains[0]); j++)
    for (size_t i = 0; i < chains[j]->n_properties_; i++)
      if (pset.find (chains[j]->properties_[i]) == pset.end())
        {
          pset.insert (chains[j]->properties_[i]);
          parray.push_back (chains[j]->properties_[i]);
        }
  delete[] properties_;
  n_properties_ = parray.size();
  properties_ = new Property* [n_properties_];
  for (size_t i = 0; i < n_properties_; i++)
    properties_[i] = parray[i];
}

Property**
PropertyList::list_properties (size_t *n_properties) const
{
  if (n_properties)
    {
      *n_properties = n_properties_;
      return properties_;
    }
  else
    return NULL;
}

PropertyList::~PropertyList ()
{
}

} } // Rapicorn::Aida
