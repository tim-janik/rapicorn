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
#ifndef __RAPICORN_CMDLIB_HH__
#define __RAPICORN_CMDLIB_HH__

#include <rapicorn/root.hh>

namespace Rapicorn {

bool    command_lib_exec        (Item               &item,
                                 const String       &cmd_name,
                                 const StringVector &args);
bool    command_scan            (const String       &input,
                                 String             *cmd_name,
                                 StringVector       *args);
String  command_string_unquote  (const String       &input);

} // Rapicorn

#endif  /* __RAPICORN_CMDLIB_HH__ */
