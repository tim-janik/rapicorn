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
#ifndef __RAPICORN_UTILITIES_HH__
#define __RAPICORN_UTILITIES_HH__

#include <rapicorn/birnetutils.hh>
#ifdef  RAPICORN_PRIVATE
#include <rapicorn/private.hh>
#endif

namespace Rapicorn {

/* --- import Birnet namespace --- */
using namespace Birnet;
using Birnet::uint;     // resolve conflict with global uint

/* --- i18n macros --- */
void            rapicorn_gettext_init (const char *domainname, const char *dirname);
const char*     rapicorn_gettext      (const char *text);
#ifdef  RAPICORN_PRIVATE
#define _(str)  rapicorn_gettext (str)
#define N_(str) (str)
#endif

/* --- standard utlities --- */
using Birnet::abs;
using Birnet::clamp;
using Birnet::min;
using Birnet::max;
//template<typename T> inline const T& min   (const T &a, const T &b) { return ::std::min<T> (a, b); }
//template<typename T> inline const T& max   (const T &a, const T &b) { return ::std::min<T> (a, b); }
inline double min (double a, long   b) { return min<double> (a, b); }
inline double min (long   a, double b) { return min<double> (a, b); }
inline double max (double a, long   b) { return max<double> (a, b); }
inline double max (long   a, double b) { return max<double> (a, b); }

} // Rapicorn

#endif  /* __RAPICORN_UTILITIES_HH__ */
