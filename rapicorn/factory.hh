/* Rapicorn
 * Copyright (C) 2002-2006 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * A copy of the GNU Lesser General Public License should ship along
 * with this library; if not, see http://www.gnu.org/copyleft/.
 */
#ifndef __RAPICORN_FACTORY_HH__
#define __RAPICORN_FACTORY_HH__

#include <rapicorn/item.hh>
#include <rapicorn/window.hh>
#include <list>

namespace Rapicorn {

class Args : public std::list<String> {
  typedef const String CS;
public:
  Args (CS &s0 = "", CS &s1 = "", CS &s2 = "", CS &s3 = "",
        CS &s4 = "", CS &s5 = "", CS &s6 = "", CS &s7 = "",
        CS &s8 = "", CS &s9 = "", CS &sA = "", CS &sB = "",
        CS &sC = "", CS &sD = "", CS &sE = "", CS &sF = "");
};

namespace Factory {

/* --- Factory API --- */
typedef map<String,String>       VariableMap;
typedef std::list<String>        ArgumentList;  /* elements: key=utf8string */
int /*-errno*/  parse_file       (const String           &i18n_domain,
                                  const String           &file_name,
                                  const String           &domain = "",
                                  vector<String>         *definitions = NULL);
Item&           create_item      (const String           &item_identifier,
                                  const ArgumentList     &arguments = ArgumentList(),
                                  const ArgumentList     &env_variables = ArgumentList());
Container&      create_container (const String           &container_identifier,
                                  const ArgumentList     &arguments = ArgumentList(),
                                  const ArgumentList     &env_variables = ArgumentList());
Window          create_window    (const String           &window_identifier,
                                  const ArgumentList     &arguments = ArgumentList(),
                                  const ArgumentList     &env_variables = ArgumentList());

bool    item_definition_is_root (const String   &item_identifier);

/* --- item type registration --- */
struct ItemTypeFactory : Deletable {
  const String  qualified_type;
  RAPICORN_PRIVATE_CLASS_COPY (ItemTypeFactory);
protected:
  static void   register_item_factory   (const ItemTypeFactory  *itfactory);
  static void   sanity_check_identifier (const char             *namespaced_ident);
public:
  explicit      ItemTypeFactory         (const char             *namespaced_ident);
  virtual Item* create_item             (const String           &name) const = 0;
  static void   initialize_factories    ();
};

} // Factory

// namespace Rapicorn

/* --- item factory template --- */
template<class Type>
class ItemFactory : Factory::ItemTypeFactory {
  RAPICORN_PRIVATE_CLASS_COPY (ItemFactory);
  virtual Item*
  create_item (const String &name) const
  {
    Item *item = new Type();
    item->name (name);
    return item;
  }
public:
  explicit ItemFactory (const char *namespaced_ident) :
    ItemTypeFactory (namespaced_ident)
  {
    register_item_factory (this);
    sanity_check_identifier (namespaced_ident);
  }
};

} // Rapicorn

#endif /* __RAPICORN_FACTORY_HH__ */
