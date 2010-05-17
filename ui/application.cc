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

class AppImpl : public Application {};

Application &App = *new AppImpl();

Application::Application() : // : sig_missing_primary (*this)
  m_tc (0)
{
  assert (&App == NULL);
}

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

void
Application::init_with_x11 (const std::string &application_name,
                            const StringList  &cmdline_args)
{
  int dummy_argc = cmdline_args.strings.size();
  char **dummy_argv = (char**) alloca ((sizeof (char*) + 1) * dummy_argc);
  for (int i = 0; i < dummy_argc; i++)
    dummy_argv[i] = const_cast<char*> (cmdline_args.strings[i].c_str());
  dummy_argv[dummy_argc] = NULL;
  Application::init_with_x11 (&dummy_argc, &dummy_argv, application_name.c_str());
}

WinPtr
Application::create_winptr (const std::string         &window_identifier,
                            const std::vector<String> &arguments,
                            const std::vector<String> &env_variables)
{
  return Factory::create_winptr (window_identifier, arguments, env_variables);
}

Window*
Application::create_window (const std::string    &window_identifier,
                            const StringList     &arguments,
                            const StringList     &env_variables)
{
  return &Factory::create_winptr (window_identifier,
                                  arguments.strings,
                                  env_variables.strings).root().window();
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
  int err = Factory::parse_file (i18n_domain, fullname);
  if (err)
    error ("failed to load \"%s\": %s", fullname.c_str(), string_from_errno (err).c_str());
}

void
Application::pixstream (const String  &pix_name,
                        const uint8   *static_const_pixstream)
{
  Pixmap::add_stock (pix_name, static_const_pixstream);
}

static uint     exit_code = 0;

int
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
Application::exit (int code)
{
  assert (rapicorn_thread_entered());           // guards exit_code
  if (!exit_code)
    exit_code = code;
  return EventLoop::kill_loops ();
}

void
Application::test_counter_set (int val)
{
  m_tc = val;
}

void
Application::test_counter_add (int val)
{
  m_tc += val;
}

int
Application::test_counter_get ()
{
  return m_tc;
}

int
Application::test_counter_inc_fetch ()
{
  return ++m_tc;
}

} // Rapicorn
