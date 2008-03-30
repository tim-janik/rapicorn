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

namespace Rapicorn {

/* --- Type Information --- */
class Enumeration {
  const char *enum_def;
public:
  struct Entry          { const char *ident, *label, *blurb; };
  vector<Entry>         entries();
  static Enumeration    enumeration     (const String &enumeration_ident);
  /* internal */
  struct Registrator    { Registrator   (const char   *_enumdef); };
};

class Type {
public:
  /* basic typing */
#define RAPICORN_TYPE_STORAGE_CHARS     ("nOfsvA")
  enum Storage {
    NUM = 'n', FLOAT = 'f', STRING = 's',
    OBJECT = 'O', STRING_VECTOR = 'v', ARRAY = 'A',
  };
  explicit              Type            (const char *_typedef);
  Storage               storage         () const;
  /* recursive type data */
  Enumeration&          enumeration     () const;   // for ELECTION and COMBO
  vector<Type>          contained_types () const;   // Type* for ARRAY and RECORD
  /* convenience API */
  String                ident           () const;
  String                label           () const;
  String                blurb           () const;
  String                hints           () const;
  /* generic auxillary data */
  String                aux_string      (const String &auxname) const;
  long double           aux_float       (const String &auxname) const;
  int64                 aux_num         (const String &auxname) const;
  /* internal */
  static const char*    type_name       (Storage storage);
  struct Registrator  { Registrator     (const char   *_typedef); };
  struct Info {
    Type::Storage  storage;
    String         name;
    StringVector   auxdata;
  };
protected:
  Info          m_info;
};

class TypeTest {
public:
  static TypeTest* parse_type_info (const char *type_info_string,
                                    uint        type_info_length);
};

} // Rapicorn

#endif /* __RAPICORN_TYPES_HH__ */
