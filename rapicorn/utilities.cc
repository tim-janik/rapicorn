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
#include "loop.hh"

namespace Rapicorn {

static const char *rapicorn_i18n_domain = NULL;

void
rapicorn_init (int        *argcp,
               char     ***argvp,
               const char *app_name)
{
  /* initialize i18n functions */
  rapicorn_i18n_domain = RAPICORN_I18N_DOMAIN;
  // bindtextdomain (rapicorn_i18n_domain, dirname);
  bind_textdomain_codeset (rapicorn_i18n_domain, "UTF-8");
  /* initialize sub components */
  birnet_init (argcp, argvp, app_name);
  MainLoopPool::rapicorn_init();
}

const char*
rapicorn_gettext (const char *text)
{
  assert (rapicorn_i18n_domain != NULL);
  return dgettext (rapicorn_i18n_domain, text);
}

static RecMutex rapicorn_global_mutex;

RecMutex*
rapicorn_mutex ()
{
  return &rapicorn_global_mutex;
}

Convertible::Convertible()
{}

bool
Convertible::match_interface (InterfaceMatch &imatch) const
{
  Convertible *self = const_cast<Convertible*> (this);
  return imatch.done() || imatch.match (self);
}

} // Rapicorn
