/* Birnet Programming Utilities
 * Copyright (C) 2003,2005 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#include "birnetutils.hh"
#include <stdarg.h>
#include <algorithm>
#include <stdexcept>
#include <signal.h>

namespace Birnet {

/* --- exceptions --- */
const std::nothrow_t dothrow = {};

Exception::Exception (const String &s1, const String &s2, const String &s3, const String &s4) :
  reason (NULL)
{
  String s (s1);
  if (s2.size())
    s += s2;
  if (s3.size())
    s += s3;
  if (s4.size())
    s += s4;
  set (s);
}

Exception::Exception (const Exception &e) :
  reason (e.reason ? strdup (e.reason) : NULL)
{}

void
Exception::set (const String &s)
{
  if (reason)
    free (reason);
  reason = strdup (s.c_str());
}

Exception::~Exception() throw()
{
  if (reason)
    free (reason);
}

/* --- generic named data --- */
DataList::NodeBase::~NodeBase ()
{
}

void
DataList::set_data (NodeBase    *node)
{
  /* delete old node */
  NodeBase *it = rip_data (node->key);
  if (it)
    delete it;
  /* prepend node */
  node->next = nodes;
  nodes = node;
}

DataList::NodeBase*
DataList::get_data (DataKey<void> *key)
{
  NodeBase *it;
  for (it = nodes; it; it = it->next)
    if (it->key == key)
      return it;
  return NULL;
}

DataList::NodeBase*
DataList::rip_data (DataKey<void> *key)
{
  NodeBase *last = NULL, *it;
  for (it = nodes; it; it = it->next)
    if (it->key == key)
      {
        /* unlink existing node */
        if (last)
          last->next = it->next;
        else
          nodes = it->next;
        it->next = NULL;
        return it;
      }
  return NULL;
}

DataList::~DataList()
{
  while (nodes)
    {
      NodeBase *it = nodes;
      nodes = it->next;
      it->next = NULL;
      delete it;
    }
}

void
warning_expr_failed (const char *file, uint line, const char *function, const char *expression)
{
  warning ("%s:%d:%s: assertion failed: (%s)", file, line, function, expression);
}

void
error_expr_failed (const char *file, uint line, const char *function, const char *expression)
{
  error ("%s:%d:%s: assertion failed: (%s)", file, line, function, expression);
}

void
warning_expr_reached (const char *file, uint line, const char *function)
{
  warning ("%s:%d:%s: code location should not be reached", file, line, function);
}

String
string_printf (const char *format,
               ...)
{
  String str;
  va_list args;
  va_start (args, format);
  try {
    str = string_vprintf (format, args);
  } catch (...) {
    va_end (args);
    throw;
  }
  va_end (args);
  return str;
}

String
string_vprintf (const char *format,
                va_list     vargs)
{
  char *str = NULL;
  if (vasprintf (&str, format, vargs) < 0 || !str)
    throw std::length_error (__func__);
  String s (str);
  free (str);
  return s;
}

bool
string_to_bool (const String &string)
{
  const char *p = string.c_str();
  while (*p == ' ' || *p == '\n' || *p == '\t' || *p == '\r')
    p++;
  if (!p[0] || p[0] == 'n' || p[0] == 'N' || p[0] == 'f' || p[0] == 'F' || p[0] == '0')
    return false;
  else
    return true;
}

String
string_from_bool (bool value)
{
  return String (value ? "1" : "0");
}

uint64
string_to_uint (const String &string)
{
  const char *p = string.c_str();
  while (*p == ' ' || *p == '\n' || *p == '\t' || *p == '\r')
    p++;
  bool hex = p[0] == '0' && (p[1] == 'X' || p[1] == 'x');
  return strtoull (hex ? p + 2 : p, NULL, hex ? 16 : 10);
}

String
string_from_uint (uint64 value)
{
  return string_printf ("%llu", value);
}

bool
string_has_int (const String &string)
{
  const char *p = string.c_str();
  while (*p == ' ' || *p == '\n' || *p == '\t' || *p == '\r')
    p++;
  return p[0] >= '0' && p[0] <= '9';
}

int64
string_to_int (const String &string)
{
  const char *p = string.c_str();
  while (*p == ' ' || *p == '\n' || *p == '\t' || *p == '\r')
    p++;
  bool hex = p[0] == '0' && (p[1] == 'X' || p[1] == 'x');
  return strtoll (hex ? p + 2 : p, NULL, hex ? 16 : 10);
}

String
string_from_int (int64 value)
{
  return string_printf ("%lld", value);
}

double
string_to_float (const String &string)
{
  return strtod (string.c_str(), NULL);
}

String
string_from_float (float value)
{
  return string_printf ("%.7g", value);
}

String
string_from_float (double value)
{
  return string_printf ("%.17g", value);
}

void
raise_sigtrap ()
{
  raise (SIGTRAP);
}

void
error (const char *format,
       ...)
{
  va_list args;
  va_start (args, format);
  String ers = string_vprintf (format, args);
  va_end (args);
  error (ers);
}

void
error (const String &s)
{
  fflush (stdout);
  fputs ("\nERROR: ", stderr);
  fputs (s.c_str(), stderr);
  fputs ("\naborting...\n", stderr);
  fflush (stderr);
  abort();
}

void
warning (const char *format,
         ...)
{
  fflush (stdout);
  va_list args;
  va_start (args, format);
  String ers = string_vprintf (format, args);
  va_end (args);
  warning (ers);
  fflush (stderr);
}

void
warning (const String &s)
{
  fputs ("\nWARNING: ", stderr);
  fputs (s.c_str(), stderr);
  fputs ("\n", stderr);
}

void
diag (const char *format,
      ...)
{
  fflush (stdout);
  va_list args;
  va_start (args, format);
  String ers = string_vprintf (format, args);
  va_end (args);
  diag (ers);
  fflush (stderr);
}

void
diag (const String &s)
{
  fputs ("DIAG: ", stderr);
  fputs (s.c_str(), stderr);
  fputs ("\n", stderr);
}

} // Birnet
