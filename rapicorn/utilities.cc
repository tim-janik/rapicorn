/* Rapicorn
 * Copyright (C) 2005 Tim Janik
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
#include "utilities.hh"

namespace Rapicorn {

static const char *rapicorn_gettext_domain = NULL;
void
rapicorn_gettext_init (const char *domainname,
                       const char *dirname)
{
  rapicorn_gettext_domain = domainname;
  bindtextdomain (rapicorn_gettext_domain, dirname);
  bind_textdomain_codeset (rapicorn_gettext_domain, "UTF-8");
}

const char*
rapicorn_gettext (const char *text)
{
  assert (rapicorn_gettext_domain != NULL);
  return dgettext (rapicorn_gettext_domain, text);
}

/* include generated enum definitions */
#include "gen-enums.cc"

} // Rapicorn
