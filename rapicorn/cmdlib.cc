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
#include "cmdlib.hh"

namespace Rapicorn {


static void
item_println (Item         &item,
            const String &args)
{
  printout ("%s\n", args.c_str());
}

static struct {
  void      (*cmd) (Item&, const String&);
  const char *name;
} item_cmds[] = {
  { item_println,         "Item::println" },
};

static void
window_close (Window       &window,
              const String &args)
{
  window.close();
}

static struct {
  void      (*cmd) (Window&, const String&);
  const char *name;
} window_cmds[] = {
  { window_close,       "Window::close" },
};

static void
application_close (Item         &item,
                   const String &args)
{
  printout ("app.close()\n");
}

static struct {
  void      (*cmd) (Item&, const String&);
  const char *name;
} application_cmds[] = {
  { application_close,  "Application::close" },
};

bool
command_lib_exec (Item         &item,
                  const String &cmd_name,
                  const String &args)
{
  for (uint ui = 0; ui < ARRAY_SIZE (item_cmds); ui++)
    if (item_cmds[ui].name == cmd_name)
      {
        item_cmds[ui].cmd (item, args);
        return true;
      }
  Root *root = item.get_root();
  if (root)
    {
      Window window = root->window();
      for (uint ui = 0; ui < ARRAY_SIZE (window_cmds); ui++)
        if (window_cmds[ui].name == cmd_name)
          {
            window_cmds[ui].cmd (window, args);
            return true;
          }
    }
  for (uint ui = 0; ui < ARRAY_SIZE (application_cmds); ui++)
    if (application_cmds[ui].name == cmd_name)
      {
        application_cmds[ui].cmd (item, args);
        return true;
      }
  return false;
}

} // Rapicorn
