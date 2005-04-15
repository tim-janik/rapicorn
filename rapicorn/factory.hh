/* Rapicorn
 * Copyright (C) 2002-2005 Tim Janik
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
#include <list>

namespace Rapicorn {

/* --- Factory --- */
struct Factory {
  typedef map<String,String>            VariableMap;
  typedef std::list<String>             ArgumentList;   /* elements: key=utf8string */
  virtual Item& create_gadget           (const String           &gadget_identifier,
                                         const ArgumentList     &arguments = ArgumentList()) = 0;
  virtual void  parse_resource          (const String           &file_name,
                                         const String           &i18n_domain,
                                         const String           &domain,
                                         const std::nothrow_t   &nt = dothrow) = 0;
  void          parse_resource          (const String           &file_name,
                                         const String           &i18n_domain,
                                         const std::nothrow_t   &nt = dothrow)  { return parse_resource (file_name, i18n_domain, i18n_domain, nt); }
  struct        ItemTypeFactory;
  static void   announce_item_factory   (const ItemTypeFactory  *itfactory);
private:
  virtual void  register_item_factory   (const ItemTypeFactory  *itfactory) = 0;
  BIRNET_PRIVATE_CLASS_COPY (Factory);
protected:
  Factory();
  virtual ~Factory();
};

/* --- Factory Singleton --- */
extern class Factory &Factory;

/* --- item type registration --- */
struct Factory::ItemTypeFactory : Deletable {
  const String qualified_type;
  ItemTypeFactory (const char *namespaced_ident) :
    qualified_type (namespaced_ident)
  {}
  virtual Item* create_item (const String &name) const = 0;
};
template<class Type>
struct ItemFactory : Factory::ItemTypeFactory {
  ItemFactory (const char *namespaced_ident) :
    ItemTypeFactory (namespaced_ident)
  {
    Factory::announce_item_factory (this);
  }
  virtual Item*
  create_item (const String &name) const
  {
    Item *item = new Type();
    item->name (name);
    return item;
  }
};

} // Rapicorn

#endif /* __RAPICORN_FACTORY_HH__ */
