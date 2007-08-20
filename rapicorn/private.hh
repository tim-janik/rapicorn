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
#ifndef __RAPICORN_PRIVATE_HH__
#define __RAPICORN_PRIVATE_HH__

/* pull in common system headers */
#include <assert.h> /* assert() and friends */

namespace Rapicorn {

#define _(str)  rapicorn_gettext (str)
#define N_(str) (str)

/* --- safe standard macros --- */
#define DIR_SEPARATOR                   RAPICORN_DIR_SEPARATOR
#define DIR_SEPARATOR_S                 RAPICORN_DIR_SEPARATOR_S
#define SEARCHPATH_SEPARATOR            RAPICORN_SEARCHPATH_SEPARATOR
#define SEARCHPATH_SEPARATOR_S          RAPICORN_SEARCHPATH_SEPARATOR_S

#define MakeProperty                    RAPICORN_MakeProperty
#define MakeNamedCommand                RAPICORN_MakeNamedCommand
#define MakeSimpleCommand               RAPICORN_MakeSimpleCommand

/* --- catch broken compilers --- */
#define RAPICORN_GNUC_CHECK(major,minor,plevel) \
                                        (__GNUC__ > (major) || \
                                         (__GNUC__ == (major) && __GNUC_MINOR__ > (minor)) || \
                                         (__GNUC__ == (major) && __GNUC_MINOR__ == (minor) && \
                                          __GNUC_PATCHLEVEL__ >= (plevel)))
#if     !RAPICORN_GNUC_CHECK (3, 4, 6)
#error This GNU C++ compiler version is known to be broken - please consult rapicorn/README
#endif

} // Rapicorn

#endif  /* __RAPICORN_PRIVATE_HH__ */
