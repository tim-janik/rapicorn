/* Rapicorn
 * Copyright (C) 2005 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#include "properties.hh"
#include <set>

namespace Rapicorn {

static inline String
canonify (String s)
{
  for (uint i = 0; i < s.size(); i++)
    if (!((s[i] >= 'A' && s[i] <= 'Z') ||
          (s[i] >= 'a' && s[i] <= 'z') ||
          (s[i] >= '0' && s[i] <= '9') ||
          s[i] == '_'))
      s[i] = '_';
  return s;
}

Property::Property (const char *cident, const char *clabel, const char *cblurb, const char *chints) :
  ident (cident),
  label (clabel ? strdup (clabel) : NULL),
  blurb (cblurb ? strdup (cblurb) : NULL),
  hints (chints ? strdup (chints) : NULL)
{
  assert (ident != NULL);
  ident = strdup (canonify (ident).c_str());
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

void
PropertyList::append_properties (uint                n_props,
                                 Property          **props,
                                 const PropertyList &chain0,
                                 const PropertyList &chain1,
                                 const PropertyList &chain2,
                                 const PropertyList &chain3,
                                 const PropertyList &chain4,
                                 const PropertyList &chain5,
                                 const PropertyList &chain6,
                                 const PropertyList &chain7,
                                 const PropertyList &chain8)
{
  typedef std::set<Property*> PropertySet;
  PropertySet pset;
  std::vector<Property*> parray;
  for (uint i = 0; i < n_properties; i++)
    if (pset.find (properties[i]) == pset.end())
      {
        pset.insert (properties[i]);
        parray.push_back (properties[i]);
      }
  for (uint i = 0; i < n_props; i++)
    if (pset.find (props[i]) == pset.end())
      {
        pset.insert (props[i]);
        parray.push_back (ref_sink (props[i]));
      }
  const PropertyList *chains[] = {
    &chain0, &chain1, &chain2, &chain3, &chain4, &chain5, &chain6, &chain7, &chain8
  };
  for (uint j = 0; j < sizeof (chains) / sizeof (chains[0]); j++)
    for (uint i = 0; i < chains[j]->n_properties; i++)
      if (pset.find (chains[j]->properties[i]) == pset.end())
        {
          pset.insert (chains[j]->properties[i]);
          parray.push_back (ref (chains[j]->properties[i]));
        }
  delete[] properties;
  n_properties = parray.size();
  properties = new Property*[n_properties];
  for (uint i = 0; i < n_properties; i++)
    properties[i] = parray[i];
}

PropertyList::~PropertyList()
{
  while (n_properties)
    unref (properties[--n_properties]);
  if (properties)
    delete[] properties;
}

} // Rapicorn
