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
Application::create_window (const String            &gadget_identifier,
                            const std::list<String> &arguments)
{
  return Factory::create_window (gadget_identifier, arguments);
}

void
Application::auto_load (const String  &i18n_domain,
                        const String  &file_name,
                        const String  &binary_path)
{
  assert (rapicorn_thread_entered());
  printerr ("LOAD-GUI: domain=\"%s\" file=\"%s\" path=\"%s\"\n", i18n_domain.c_str(), file_name.c_str(), binary_path.c_str());
  /* - search for /file_name if isabs (file_name), then let file_name = basename (file_name)
   * - if isdir(binary_path) search binary_path/file_name
   * - if !isabs (binary_path), then let binary_path=CWD/binary_path
   * - if isprogram (binary_path) search dirname (binary_path)/file_name
   * - if isprogram (binary_path) in ".lib" or "_libs" search dirname (dirname (binary_path))/file_name
   */
  String fname = file_name;
  String bpath = binary_path;
  /* support absolute file names */
  if (Path::isabs (fname))
    {
      int err = Factory::parse_file (i18n_domain, fname);
      if (!err)
        return;
      /* need relative file name for composition */
      fname = Path::basename (fname);
    }
  /* handle directory location */
  if (Path::check (bpath, "d")) // isdir
    {
      String tname = Path::join (bpath, fname);
      int err = Factory::parse_file (i18n_domain, tname);
      if (!err)
        return;
    }
  /* ensure absolute binary path */
  if (bpath != "" && !Path::isabs (bpath))
    bpath = Path::join (Path::cwd(), bpath);
  /* handle executable location */
  if (Path::check (bpath, "fx")) // executable file
    {
      String dir = Path::dirname (bpath);
      String tname = Path::join (dir, fname);
      int err = Factory::parse_file (i18n_domain, tname);
      if (!err)
        return;
      /* libtool work around: skip .libs or _libs */
      while (dir.size() && dir[dir.size() - 1] == Path::dir_separator[0])
        dir.erase (dir.size() - 1);             // strip trailing separators
      while (dir.size() >= 2 && dir[dir.size() - 1] == '.' && dir[dir.size() - 2] == Path::dir_separator[0])
        {
          dir.erase (dir.size() - 2, 2);        // strip /./ self references
          while (dir.size() && dir[dir.size() - 1] == Path::dir_separator[0])
            dir.erase (dir.size() - 1);         // strip trailing separators
        }
      String bdir = Path::basename (dir);
      if (bdir == ".libs" || bdir == "_libs")
        {
          tname = Path::join (Path::dirname (dir), fname);
          int err = Factory::parse_file (i18n_domain, tname);
          if (!err)
            return;
        }
    }
  /* failed to find file */
  String assumed_path = Path::join (binary_path, fname);
  error ("failed to load GUI: %s", assumed_path.c_str());
}

void
Application::pixstream (const String  &pix_name,
                        const uint8   *static_const_pixstream)
{
  Image::register_builtin_pixstream (pix_name, static_const_pixstream);
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
