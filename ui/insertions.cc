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

struct DUMMY { // dummy class for auto indentation

class_scope:Requisition:
  inline Requisition (double w, double h) : width (w), height (h) {}

class_scope:Application:
  static void        pixstream     (const String &pix_name, const uint8 *static_const_pixstream);
  static void        init_with_x11 (int        *argcp,
                                    char     ***argvp,
                                    const char *app_name);
  WinPtr             create_winptr (const std::string  &window_identifier,
                                    const std::vector<String> &arguments = std::vector<String>(),
                                    const std::vector<String> &env_variables = std::vector<String>());
  /* global mutex */
  struct ApplicationMutex {
    static void lock    () { rapicorn_thread_enter (); }
    static bool trylock () { return rapicorn_thread_try_enter (); }
    static void unlock  () { rapicorn_thread_leave (); }
  };
  static ApplicationMutex mutex;  // singleton
  /* singleton defs */
protected:
  explicit           Application ();
private:
  int                m_tc;
  RAPICORN_PRIVATE_CLASS_COPY (Application);

IGNORE: // close last _scope
}; // close dummy class scope
