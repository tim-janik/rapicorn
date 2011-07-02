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
#include <cstring>

namespace Rapicorn {


static void
item_print (ItemImpl           &item,
            const StringList   &args)
{
  bool last_empty = false;
  const StringVector &strings = args;
  for (uint i = 0; i < strings.size(); i++)
    {
      String str = command_string_unquote (strings[i]);
      last_empty = str == "" && string_strip (strings[i]) == "";
      printout ("%s%s", i ? " " : "", str.c_str());
    }
  if (!last_empty)
    printout ("\n");
}

static struct {
  void      (*cmd) (ItemImpl&, const StringList&);
  const char *name;
} item_cmds[] = {
  { item_print,         "Item::print" },
};

static void
wind0w_close (Wind0wIface      &wind0w,
              const StringList &args)
{
  wind0w.close();
}

static struct {
  void      (*cmd) (Wind0wIface&, const StringList&);
  const char *name;
} wind0w_cmds[] = {
  { wind0w_close,       "Wind0w::close" },
};

static void
application_close (ItemImpl           &item,
                   const StringList   &args)
{
  printout ("app.close()\n");
}

static struct {
  void      (*cmd) (ItemImpl&, const StringList&);
  const char *name;
} application_cmds[] = {
  { application_close,  "Application::close" },
};

bool
command_lib_exec (ItemImpl           &item,
                  const String       &cmd_name,
                  const StringList   &args)
{
  for (uint ui = 0; ui < ARRAY_SIZE (item_cmds); ui++)
    if (item_cmds[ui].name == cmd_name)
      {
        item_cmds[ui].cmd (item, args);
        return true;
      }
  WindowImpl *window = item.get_window();
  if (window)
    {
      Wind0wIface &wind0w = window->wind0w();
      for (uint ui = 0; ui < ARRAY_SIZE (wind0w_cmds); ui++)
        if (wind0w_cmds[ui].name == cmd_name)
          {
            wind0w_cmds[ui].cmd (wind0w, args);
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

static bool
parse_arg (const String &input,
           uint         *pos,
           String       *arg)
{
  uint a0 = *pos, level = 0;
  bool sq = false, dq = false, be = false, done = false;
  while (*pos < input.size())
    {
      switch (input[*pos])
        {
        case '\\':
          if (!be && (sq || dq))
            {
              be = true;
              (*pos)++;
              continue;
            }
          break;
        case '\'':
          if (!dq && (!sq || !be))
            sq = !sq;
          break;
        case '"':
          if (!sq && (!dq || !be))
            dq = !dq;
          break;
        case '(':
          if (!sq && !dq)
            level++;
          break;
        case ',':
          if (!sq && !dq && !level)
            done = true;
          break;
        case ')':
          if (!sq && !dq)
            {
              if (level)
                level--;
              else
                done = true;
            }
          break;
        default: ;
        }
      if (done)
        break;
      be = false;
      (*pos)++;
    }
  if (sq)
    {
      DEBUG ("unclosed single-quotes in command arg: %s", input.c_str());
      return false;
    }
  if (dq)
    {
      DEBUG ("unclosed double-quotes in command arg: %s", input.c_str());
      return false;
    }
  if (level)
    {
      DEBUG ("unmatched parenthesis in command arg: %s", input.c_str());
      return false;
    }
  if (be)
    {
      DEBUG ("invalid command arg: %s", input.c_str());
      return false;
    }
  if (!done)
    {
      DEBUG ("unclosed command arg list: %s", input.c_str());
      return false;
    }
  *arg = input.substr (a0, *pos - a0);
  return true;
}

static const char *whitespaces = " \t\v\f\n\r";

bool
command_scan (const String &input,
              String       *cmd_name,
              StringList   *args)
{
  const char *ident0 = "abcdefghijklmnopqrstuvwxyz_ABCDEFGHIJKLMNOPQRSTUVWXYZ";
  const char *identn = "abcdefghijklmnopqrstuvwxyz_ABCDEFGHIJKLMNOPQRSTUVWXYZ:0123456789";
  *cmd_name = "";
  args->resize (0);
  uint i = 0;
  /* skip leading spaces */
  while (i < input.size() && strchr (whitespaces, input[i]))
    i++;
  /* parse command name */
  const uint c0 = i;
  if (i < input.size() && strchr (ident0, input[i]))
    i++;
  while (i < input.size() && strchr (identn, input[i]))
    i++;
  const uint cl = i;
  /* skip intermediate spaces */
  while (i < input.size() && strchr (whitespaces, input[i]))
    i++;
  /* check command name */
  if (cl <= c0)
    {
      DEBUG ("invalid command name: %s", input.c_str());
      return false;
    }
  *cmd_name = input.substr (c0, cl - c0);
  /* parse args */
  if (i < input.size() && input[i] == '(')
    {
      i++; // skip '('
      String arg;
      if (!parse_arg (input, &i, &arg))
        return false; // invalid arg syntax
      args->push_back (arg);
      while (i < input.size() && input[i] == ',')
        {
          i++;
          if (!parse_arg (input, &i, &arg))
            return false; // invalid arg syntax
          args->push_back (arg);
        }
      if (i >= input.size() || input[i] != ')')
        {
          DEBUG ("missing closing parenthesis in command: %s", input.c_str());
          return false;
        }
      i++; // skip ')'
    }
  /* skip trailing spaces */
  while (i < input.size() && strchr (whitespaces, input[i]))
    i++;
  if (i < input.size() && input[i] != 0)
    {
      DEBUG ("encountered junk after command: %s", input.c_str());
      return false;
    }
  return true;
}

String
command_string_unquote (const String &input)
{
  uint i = 0;
  while (i < input.size() && strchr (whitespaces, input[i]))
    i++;
  if (i < input.size() && (input[i] == '"' || input[i] == '\''))
    {
      const char qchar = input[i];
      i++;
      String out;
      bool be = false;
      while (i < input.size() && (input[i] != qchar || be))
        {
          if (!be && input[i] == '\\')
            be = true;
          else
            {
              out += input[i];
              be = false;
            }
          i++;
        }
      if (i < input.size() && input[i] == qchar)
        {
          i++;
          while (i < input.size() && strchr (whitespaces, input[i]))
            i++;
          if (i >= input.size())
            return out;
          else
            DEBUG ("extraneous characters after string: %s", input.c_str());
        }
      else
        DEBUG ("unclosed string: %s", input.c_str());
    }
  else if (i == input.size())
    ; // empty string arg: ""
  else
    DEBUG ("invalid string argument: %s", input.c_str());
  return "";
}

} // Rapicorn
