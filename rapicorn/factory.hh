/* Rapicorn
 * Copyright (C) 2002-2006 Tim Janik
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
#ifndef __RAPICORN_FACTORY_HH__
#define __RAPICORN_FACTORY_HH__

#include <rapicorn/item.hh>
#include <rapicorn/handle.hh>
#include <list>

namespace Rapicorn {

namespace Factory {

/* --- Factory API --- */
typedef map<String,String>      VariableMap;
typedef std::list<String>       ArgumentList;   /* elements: key=utf8string */
void           parse_file      (const String           &file_name,
                                const String           &i18n_domain,
                                const String           &domain,
                                const std::nothrow_t   &nt = dothrow);
void           parse_file      (const String           &file_name,
                                const String           &i18n_domain,
                                const std::nothrow_t   &nt = dothrow);
Handle<Item>   create_item     (const String           &gadget_identifier,
                                const ArgumentList     &arguments = ArgumentList());

/* --- item type registration --- */
struct ItemTypeFactory : Deletable {
  const String  qualified_type;
  BIRNET_PRIVATE_CLASS_COPY (ItemTypeFactory);
protected:
  static void   register_item_factory (const ItemTypeFactory  *itfactory);
public:
  explicit      ItemTypeFactory       (const char             *namespaced_ident);
  virtual Item* create_item           (const String           &name) const = 0;
  static void   initialize_factories  ();
};

} // Factory

// namespace Rapicorn

/* --- item factory template --- */
template<class Type>
class ItemFactory : Factory::ItemTypeFactory {
  BIRNET_PRIVATE_CLASS_COPY (ItemFactory);
  virtual Item*
  create_item (const String &name) const
  {
    Item *item = new Type();
    item->name (name);
    return item;
  }
public:
  ItemFactory (const char *namespaced_ident) :
    ItemTypeFactory (namespaced_ident)
  {
    register_item_factory (this);
  }
};

} // Rapicorn

#endif /* __RAPICORN_FACTORY_HH__ */
