/* Rapicorn
 * Copyright (C) 2010 Tim Janik
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

// PLIC insertion file
// Here we provide insertion snippets to be included in struct
// definitions when generating interface.hh from rapicorn.idl

includes:
#include <ui/utilities.hh>
namespace Rapicorn {
class Root;
class Item;
}

IGNORE:
struct DUMMY { // dummy class for auto indentation

class_scope:Requisition:
  inline Requisition (double w, double h) : width (w), height (h) {}

class_scope:StringList:
  /*Con*/  StringList () {}
  /*Con*/  StringList (const std::vector<String> &strv) : strings (strv) {}

class_scope:WindowBase:
  virtual Root&      root          () = 0;
class_scope:ApplicationBase:
  static void        pixstream     (const String &pix_name, const uint8 *static_const_pixstream);
  static void        init_with_x11 (int        *argcp,
                                    char     ***argvp,
                                    const char *app_name);
  int                execute_loops ();
  static bool        plor_add      (Item &item, const String &plor_name);
  static Item*       plor_remove   (const String &plor_name);
  /* global mutex */
  struct ApplicationMutex {
    static void lock    () { rapicorn_thread_enter (); }
    static bool trylock () { return rapicorn_thread_try_enter (); }
    static void unlock  () { rapicorn_thread_leave (); }
  };
  static ApplicationMutex mutex;  // singleton
  /* singleton defs */
protected:
  int                m_tc; // FIXME: uninitialized

IGNORE: // close last _scope
}; // close dummy class scope

global_scope:
namespace Rapicorn {
extern ApplicationBase &app;
}
