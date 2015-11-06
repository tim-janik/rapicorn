// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#include "commands.hh"

namespace Rapicorn {

static inline String
canonify (String s)
{
  for (uint i = 0; i < s.size(); i++)
    if (!((s[i] >= 'A' && s[i] <= 'Z') ||
          (s[i] >= 'a' && s[i] <= 'z') ||
          (s[i] >= '0' && s[i] <= '9') ||
          s[i] == '-'))
      s[i] = '-';
  return s;
}

Command::Command (const char *cident,
                  const char *cblurb,
                  bool        has_arg) :
  ident (canonify (cident)),
  blurb (cblurb),
  needs_arg (has_arg)
{
  assert (cident != NULL);
}

Command::~Command()
{}

} // Rapicorn
