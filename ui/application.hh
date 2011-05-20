/* Rapicorn
 * Copyright (C) 2007 Tim Janik
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
#ifndef __RAPICORN_APPLICATION_HH__
#define __RAPICORN_APPLICATION_HH__

#include <ui/commands.hh>

namespace Rapicorn {

class ApplicationImpl : public Application {
  vector<Wind0w*>     m_wind0ws;
  void                check_primaries        ();
public:
  virtual void        init_with_x11          (const std::string &application_name,
                                              const StringList  &cmdline_args);
  virtual String      auto_path              (const String  &file_name,
                                              const String  &binary_path,
                                              bool           search_vpath = true);
  virtual void        auto_load              (const std::string &defs_domain,
                                              const std::string &file_name,
                                              const std::string &binary_path,
                                              const std::string &i18n_domain = "");
  virtual void        load_string            (const std::string &xml_string,
                                              const std::string &i18n_domain = "");
  virtual Wind0w*     create_wind0w          (const std::string &wind0w_identifier,
                                              const StringList &arguments = StringList(),
                                              const StringList &env_variables = StringList());
  void                add_wind0w             (Wind0w &wind0w);
  bool                remove_wind0w          (Wind0w &wind0w);
  virtual Wind0wList  list_wind0ws           ();
  virtual It3m*       unique_component       (const String &path);
  virtual ItemSeq     collect_components     (const String &path);
  virtual void        test_counter_set       (int val);
  virtual void        test_counter_add       (int val);
  virtual int         test_counter_get       ();
  virtual int         test_counter_inc_fetch ();
  static void         init_with_x11          (int        *argcp,
                                              char     ***argvp,
                                              const char *app_name);
};

extern ApplicationImpl &app;

} // Rapicorn

#endif  /* __RAPICORN_APPLICATION_HH__ */
