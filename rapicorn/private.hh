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

/* --- safe standard macros --- */
#undef STRFUNC
#define STRFUNC                         ((const char*) BIRNET_PRETTY_FUNCTION)
#undef EXPECT
#define EXPECT(expr,val)                BIRNET_EXPECT(expr,val)
#undef LIKELY
#define LIKELY(expr)                    BIRNET_LIKELY(expr)
#undef ISLIKELY
#define ISLIKELY(expr)                  BIRNET_ISLIKELY(expr)
#undef UNLIKELY
#define UNLIKELY(expr)                  BIRNET_UNLIKELY(expr)
#undef static_assert_named
#define static_assert_named(expr,asnam) BIRNET_STATIC_ASSERT_NAMED(expr,asnam)
#undef static_assert
#define static_assert(expr)             BIRNET_STATIC_ASSERT(expr)
#undef return_if_fail
#define return_if_fail(e)               BIRNET_RETURN_IF_FAIL(e)
#undef return_val_if_fail
#define return_val_if_fail(e,v)         BIRNET_RETURN_VAL_IF_FAIL(e,v)
#undef return_if_reached
#define return_if_reached()             BIRNET_RETURN_IF_REACHED()
#undef return_val_if_reached
#define return_val_if_reached(v)        BIRNET_RETURN_VAL_IF_REACHED(v)
#undef assert_not_reached
#define assert_not_reached()            BIRNET_ASSERT_NOT_REACHED()
#undef assert
#define assert(e)                       BIRNET_ASSERT(e)

#define DIR_SEPARATOR                   BIRNET_DIR_SEPARATOR
#define DIR_SEPARATOR_S                 BIRNET_DIR_SEPARATOR_S
#define SEARCHPATH_SEPARATOR            BIRNET_SEARCHPATH_SEPARATOR
#define SEARCHPATH_SEPARATOR_S          BIRNET_SEARCHPATH_SEPARATOR_S

#define MakeProperty                    RAPICORN_MakeProperty
#define MakeNamedCommand                RAPICORN_MakeNamedCommand
#define MakeSimpleCommand               RAPICORN_MakeSimpleCommand

#define PRIVATE_CLASS_COPY(C)           BIRNET_PRIVATE_CLASS_COPY(C)

} // Rapicorn

#endif  /* __RAPICORN_PRIVATE_HH__ */
