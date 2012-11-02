// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
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
