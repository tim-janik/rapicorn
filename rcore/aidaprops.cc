// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#include "aidaprops.hh"
#include <set>
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
  for (size_t i = 0; i < m_n_properties; i++)
    if (pset.find (m_properties[i]) == pset.end())
      {
        pset.insert (m_properties[i]);
        parray.push_back (m_properties[i]);
      }
  for (size_t i = 0; i < n_props; i++)
    if (pset.find (props[i]) == pset.end())
      {
        pset.insert (props[i]);
        parray.push_back (props[i]);
      }
  const PropertyList *const chains[] = { &c0, &c1, &c2, &c3, &c4, &c5, &c6, &c7, &c8, &c9 };
  for (size_t j = 0; j < sizeof (chains) / sizeof (chains[0]); j++)
    for (size_t i = 0; i < chains[j]->m_n_properties; i++)
      if (pset.find (chains[j]->m_properties[i]) == pset.end())
        {
          pset.insert (chains[j]->m_properties[i]);
          parray.push_back (chains[j]->m_properties[i]);
        }
  delete[] m_properties;
  m_n_properties = parray.size();
  m_properties = new Property* [m_n_properties];
  for (size_t i = 0; i < m_n_properties; i++)
    m_properties[i] = parray[i];
}

Property**
PropertyList::list_properties (size_t *n_properties) const
{
  if (n_properties)
    {
      *n_properties = m_n_properties;
      return m_properties;
    }
  else
    return NULL;
}

PropertyList::~PropertyList ()
{
}

} } // Rapicorn::Aida
