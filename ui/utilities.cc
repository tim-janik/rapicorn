/* Rapicorn
 * Copyright (C) 2005 Tim Janik
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
#include "utilities.hh"
#include "blitfuncs.hh"
#include <cstring>
#include <errno.h>
#include <libintl.h>
#include <malloc.h>

namespace Rapicorn {

static const char *rapicorn_i18n_domain = NULL;

static struct _InternalConstructorTest_lrc0 { int v; _InternalConstructorTest_lrc0() : v (0x123caca0) {} } _internalconstructortest;

void
rapicorn_init (const String       &app_ident,
               int                *argc,
               char              **argv,
               const StringVector &args)
{
  /* initialize i18n functions */
  rapicorn_i18n_domain = RAPICORN_I18N_DOMAIN;
  // bindtextdomain (rapicorn_i18n_domain, dirname);
  bind_textdomain_codeset (rapicorn_i18n_domain, "UTF-8");
  /* initialize sub components */
  rapicorn_init_core (app_ident, argc, argv, args);
  /* verify constructur runs to catch link errors */
  if (_internalconstructortest.v != 0x123caca0)
    fatal ("librapicorn: link error: C++ constructors have not been executed");
  /* optimize */
  CPUInfo cpu = cpu_info();
  if (cpu.x86_mmx)
    Blit::render_optimize_mmx();
}

static Mutex         thread_mutex;
static volatile uint thread_counter = 0;

void
rapicorn_thread_enter ()
{
  assert (rapicorn_thread_entered() == false);
  thread_mutex.lock();
  Atomic::add (&thread_counter, +1);
}

bool
rapicorn_thread_try_enter ()
{
  if (thread_mutex.trylock())
    {
      Atomic::add (&thread_counter, +1);
      return true;
    }
  else
    return false;
}

bool
rapicorn_thread_entered ()
{
  return Atomic::get (&thread_counter) > 0;
}

void
rapicorn_thread_leave ()
{
  assert (rapicorn_thread_entered());
  Atomic::add (&thread_counter, -1);
  thread_mutex.unlock();
}

const char*
rapicorn_gettext (const char *text)
{
  assert (rapicorn_i18n_domain != NULL);
  return dgettext (rapicorn_i18n_domain, text);
}

/* --- exceptions --- */
const std::nothrow_t dothrow = {};

Exception::Exception (const String &s1, const String &s2, const String &s3, const String &s4,
                      const String &s5, const String &s6, const String &s7, const String &s8) :
  reason (NULL)
{
  String s (s1);
  if (s2.size())
    s += s2;
  if (s3.size())
    s += s3;
  if (s4.size())
    s += s4;
  if (s5.size())
    s += s5;
  if (s6.size())
    s += s6;
  if (s7.size())
    s += s7;
  if (s8.size())
    s += s8;
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

} // Rapicorn
