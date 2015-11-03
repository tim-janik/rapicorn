// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#include "cmdlib.hh"
#include <cstring>

namespace Rapicorn {

static void
widget_print (WidgetImpl           &widget,
            const StringSeq    &args)
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
  void      (*cmd) (WidgetImpl&, const StringSeq&);
  const char *name;
} widget_cmds[] = {
  { widget_print,         "Widget::print" },
};

static void
window_close (WindowIface     &window,
              const StringSeq &args)
{
  window.close();
}

static struct {
  void      (*cmd) (WindowIface&, const StringSeq&);
  const char *name;
} window_cmds[] = {
  { window_close,       "Window::close" },
};

static void
application_close (WidgetImpl          &widget,
                   const StringSeq   &args)
{
  printout ("app.close()\n");
}

static struct {
  void      (*cmd) (WidgetImpl&, const StringSeq&);
  const char *name;
} application_cmds[] = {
  { application_close,  "Application::close" },
};

bool
command_lib_exec (WidgetImpl          &widget,
                  const String      &cmd_name,
                  const StringSeq   &args)
{
  for (uint ui = 0; ui < ARRAY_SIZE (widget_cmds); ui++)
    if (widget_cmds[ui].name == cmd_name)
      {
        widget_cmds[ui].cmd (widget, args);
        return true;
      }
  WindowImpl *window = widget.get_window();
  if (window)
    {
      for (uint ui = 0; ui < ARRAY_SIZE (window_cmds); ui++)
        if (window_cmds[ui].name == cmd_name)
          {
            window_cmds[ui].cmd (*window, args);
            return true;
          }
    }
  for (uint ui = 0; ui < ARRAY_SIZE (application_cmds); ui++)
    if (application_cmds[ui].name == cmd_name)
      {
        application_cmds[ui].cmd (widget, args);
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
      RAPICORN_DIAG ("unclosed single-quotes in command arg: %s", input.c_str());
      return false;
    }
  if (dq)
    {
      RAPICORN_DIAG ("unclosed double-quotes in command arg: %s", input.c_str());
      return false;
    }
  if (level)
    {
      RAPICORN_DIAG ("unmatched parenthesis in command arg: %s", input.c_str());
      return false;
    }
  if (be)
    {
      RAPICORN_DIAG ("invalid command arg: %s", input.c_str());
      return false;
    }
  if (!done)
    {
      RAPICORN_DIAG ("unclosed command arg list: %s", input.c_str());
      return false;
    }
  *arg = input.substr (a0, *pos - a0);
  return true;
}

static const char *whitespaces = " \t\v\f\n\r";

bool
command_scan (const String &input,
              String       *cmd_name,
              StringSeq    *args)
{
  const char *ident0 = "abcdefghijklmnopqrstuvwxyz_ABCDEFGHIJKLMNOPQRSTUVWXYZ";
  const char *identn = "abcdefghijklmnopqrstuvwxyz_ABCDEFGHIJKLMNOPQRSTUVWXYZ:0123456789-";
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
      RAPICORN_DIAG ("invalid command name: %s", input.c_str());
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
          RAPICORN_DIAG ("missing closing parenthesis in command: %s", input.c_str());
          return false;
        }
      i++; // skip ')'
    }
  /* skip trailing spaces */
  while (i < input.size() && strchr (whitespaces, input[i]))
    i++;
  if (i < input.size() && input[i] != 0)
    {
      RAPICORN_DIAG ("encountered junk after command: %s", input.c_str());
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
            RAPICORN_DIAG ("extraneous characters after string: %s", input.c_str());
        }
      else
        RAPICORN_DIAG ("unclosed string: %s", input.c_str());
    }
  else if (i == input.size())
    ; // empty string arg: ""
  else
    RAPICORN_DIAG ("invalid string argument: %s", input.c_str());
  return "";
}

} // Rapicorn
