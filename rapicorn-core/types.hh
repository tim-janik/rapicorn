/* Rapicorn
 * Copyright (C) 2008 Tim Janik
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
#ifndef __RAPICORN_TYPES_HH__
#define __RAPICORN_TYPES_HH__

#include <rapicorn-core/rapicornsignal.hh>
#include <rapicorn-core/enumdefs.hh>

namespace Rapicorn {

/* --- Type Information --- */
class Type {
public:
  const char*           storage_name    (StorageType storage);
  String                name            () const;
  StorageType           storage         () const;
  /* enumeration values */
  struct                Entry           { const String ident, label, blurb; };
  int64                 n_entries       ();
  Entry                 entry           (uint64 nth);
  /* interface */
  StringVector          prerequisites   ();
  /* record fields */
  int64                 n_fields        ();
  Type                  field           (uint64 nth);
  /* sequence element */
  Type                  elements        ();
  /* for sequence/record field types */
  Type                  main_type       ();
  /* convenience API */
  String                ident           () const { return name(); }
  String                label           () const; // defaults to ident()
  String                blurb           () const { return aux_string ("blurb"); }
  String                hints           () const; // ensures enclosing ':'
  /* generic auxillary data */
  String                aux_string      (const String &auxname) const;
  double                aux_float       (const String &auxname) const;
  int64                 aux_num         (const String &auxname) const;
  /* retrieve types */
  static Type           try_lookup      (const String &full_name);
  static Type           lookup          (const String &full_name);
  static Type           notype          ();
  bool                  istype          () const; // false for notype()
  explicit              Type            (const String &full_name);
  /*Copy*/              Type            (const Type&);
  /*Des*/              ~Type            ();
  static void           register_package      (uint          static_data_length,
                                               const char   *static_data);
  static void           register_package_file (const String &filename);
private:
  class Info;
  explicit              Type            (Info &tinfo);
  Info                 &m_info;
  Type&                 operator=       (const Type&); // must not be used
};

void _rapicorn_init_types (void);

} // Rapicorn

#endif /* __RAPICORN_TYPES_HH__ */
