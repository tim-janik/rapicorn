// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
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
