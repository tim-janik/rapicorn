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
#include "application.hh"
#include "factory.hh"
#include "viewport.hh"
#include "image.hh"
#include "root.hh"

namespace Rapicorn {
using namespace std;

Application::ApplicationMutex Application::mutex;

void
Application::init_with_x11 (int           *argcp,
                            char        ***argvp,
                            const char    *app_name)
{
  rapicorn_init_with_gtk_thread (argcp, argvp, app_name);
  assert (rapicorn_thread_entered() == false);
  rapicorn_thread_enter();
}

Window
Application::create_window (const String            &window_identifier,
                            const std::list<String> &arguments,
                            const std::list<String> &env_variables)
{
  return Factory::create_window (window_identifier, arguments, env_variables);
}

String
Application::auto_path (const String  &file_name,
                        const String  &binary_path,
                        bool           search_vpath)
{
  assert (rapicorn_thread_entered());
  /* test absolute file_name */
  if (Path::isabs (file_name))
    return file_name;
  /* assign bpath the absolute binary path */
  String bpath = binary_path;
  if (!Path::isabs (bpath))
    bpath = Path::join (Path::cwd(), bpath);
  /* extract binary dirname */
  if (!Path::check (bpath, "d") && !Path::isdirname (bpath))
    {
      bpath = Path::dirname (bpath);
      /* strip .libs or _libs directories */
      String tbase = Path::basename (bpath);
      if (tbase == ".libs" or tbase == "_libs")
        bpath = Path::dirname (bpath);
    }
  /* construct search path list */
  const char *gvp = getenv ("RAPICORN_VPATH");
  StringVector spl;
  if (search_vpath)
    spl = Path::searchpath_split (gvp ? gvp : "");
  spl.insert (spl.begin(), bpath);
  /* pick first existing file_name */
  for (uint i = 0; i < spl.size(); i++)
    {
      String fullname = Path::join (spl[i], file_name);
      if (Path::check (fullname, "e"))
        return fullname;
    }
  // fallback to cwd/binary_path relative file name (non-existing)
  return Path::join (bpath, file_name);
}

void
Application::auto_load (const String  &i18n_domain,
                        const String  &file_name,
                        const String  &binary_path)
{
  String fullname = auto_path (file_name, binary_path, true);
  int err = Factory::parse_file (i18n_domain, file_name);
  if (err)
    error ("failed to load \"%s\": %s", file_name.c_str(), string_from_errno (err).c_str());
}

void
Application::pixstream (const String  &pix_name,
                        const uint8   *static_const_pixstream)
{
  Pixmap::add_stock (pix_name, static_const_pixstream);
}

static uint     exit_code = 0;

uint
Application::execute_loops ()
{
  assert (rapicorn_thread_entered());           // guards exit_code
  while (!EventLoop::loops_exitable())
    EventLoop::iterate_loops (true, true);      // prepare/check/dispatch and may_block
  uint ecode = exit_code;
  exit_code = 0;
  return ecode;
}

void
Application::exit (uint code)
{
  assert (rapicorn_thread_entered());           // guards exit_code
  if (!exit_code)
    exit_code = code;
  return EventLoop::kill_loops ();
}

} // Rapicorn
