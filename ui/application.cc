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
#include "root.hh"
#include "factory.hh"
#include "viewport.hh"
#include "image.hh"
#include "compath.hh"
#include <algorithm>
#include <stdlib.h>

namespace Rapicorn {

ApplicationImpl &app = *new ApplicationImpl();

Application::ApplicationMutex Application::mutex;

void
Application::pixstream (const String  &pix_name,
                        const uint8   *static_const_pixstream)
{
  Pixmap::add_stock (pix_name, static_const_pixstream);
}

void
ApplicationImpl::init_with_x11 (int           *argcp,
                                char        ***argvp,
                                const char    *app_name)
{
  rapicorn_init_with_gtk_thread (argcp, argvp, app_name);
  assert (rapicorn_thread_entered() == false);
  rapicorn_thread_enter();
}

void
ApplicationImpl::init_with_x11 (const std::string &application_name,
                                const StringList  &cmdline_args)
{
  int dummy_argc = cmdline_args.size();
  char **dummy_argv = (char**) alloca ((sizeof (char*) + 1) * dummy_argc);
  for (int i = 0; i < dummy_argc; i++)
    dummy_argv[i] = const_cast<char*> (cmdline_args[i].c_str());
  dummy_argv[dummy_argc] = NULL;
  init_with_x11 (&dummy_argc, &dummy_argv, application_name.c_str());
}

Window*
ApplicationImpl::create_window (const std::string    &window_identifier,
                                const StringList     &arguments,
                                const StringList     &env_variables)
{
  return &Factory::create_window (window_identifier,
                                  arguments,
                                  env_variables).root().window();
}

String
ApplicationImpl::auto_path (const String  &file_name,
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
ApplicationImpl::auto_load (const String  &defs_domain,
                            const String  &file_name,
                            const String  &binary_path,
                            const String  &i18n_domain)
{
  String fullname = auto_path (file_name, binary_path, true);
  int err = Factory::parse_file (i18n_domain, fullname, defs_domain);
  if (err)
    error ("failed to load \"%s\": %s", fullname.c_str(), string_from_errno (err).c_str());
}

void
ApplicationImpl::load_string (const std::string &xml_string,
                              const std::string &i18n_domain)
{
  int err = Factory::parse_string (xml_string, i18n_domain);
  if (err)
    error ("failed to parse string: %s\n%s", string_from_errno (err).c_str(), xml_string.c_str());
}

int
Application::execute_loops ()
{
  assert (rapicorn_thread_entered());           // guards exit_code
  while (!EventLoop::loops_exitable())
    EventLoop::iterate_loops (true, true);      // prepare/check/dispatch and may_block
  return 0;
}

bool
Application::has_primary ()
{
  return !EventLoop::loops_exitable();
}

void
Application::close ()
{
}

WindowList
ApplicationImpl::list_windows ()
{
  WindowList wl;
  for (uint i = 0; i < m_windows.size(); i++)
    wl.push_back (m_windows[i]);
  return wl;
}

It3m*
ApplicationImpl::unique_component (const String &path)
{
  ItemSeq items = collect_components (path);
  if (items.size() == 1)
    return &*items[0];
  return NULL;
}

ItemSeq
ApplicationImpl::collect_components (const String &path)
{
  ComponentMatcher *cmatcher = ComponentMatcher::parse_path (path);
  ItemSeq result;
  if (cmatcher) // valid path
    {
      for (uint i = 0; i < m_windows.size(); i++)
        {
          vector<Item*> more = collect_items (*m_windows[i], *cmatcher);
          result.insert (result.end(), more.begin(), more.end());
        }
      delete cmatcher;
    }
  return result;
}

void
ApplicationImpl::add_window (Window &window)
{
  ref_sink (window);
  m_windows.push_back (&window);
}

void
ApplicationImpl::check_primaries()
{
  if (m_windows.size() == 0) // FIXME: check temporary window types
    sig_missing_primary.emit();
}

bool
ApplicationImpl::remove_window (Window &window)
{
  vector<Window*>::iterator it = std::find (m_windows.begin(), m_windows.end(), &window);
  if (it == m_windows.end())
    return false;
  m_windows.erase (it);
  unref (window);
  check_primaries(); // FIXME: should exec_update
  return true;
}

void
ApplicationImpl::test_counter_set (int val)
{
  m_tc = val;
}

void
ApplicationImpl::test_counter_add (int val)
{
  m_tc += val;
}

int
ApplicationImpl::test_counter_get ()
{
  return m_tc;
}

int
ApplicationImpl::test_counter_inc_fetch ()
{
  return ++m_tc;
}

} // Rapicorn
