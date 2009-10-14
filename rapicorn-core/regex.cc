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
#include "regex.hh"

#include <glib.h>

namespace Rapicorn {
namespace Regex {

bool
match_simple (const String   &pattern,
              const String   &utf8string,
              CompileFlags    compile_flags,
              MatchFlags      match_flags)
{
  bool matched = g_regex_match_simple (pattern.c_str(), utf8string.c_str(),
                                       GRegexCompileFlags (compile_flags),
                                       GRegexMatchFlags (match_flags));
  return matched;
}

} // Regex
} // Rapicorn
