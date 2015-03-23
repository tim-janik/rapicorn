// This Source Code Form is licensed MPLv2: http://mozilla.org/MPL/2.0
#include "style.hh"

namespace Rapicorn {

ThemeInfoP
ThemeInfo::create (const String &filename)
{
  return FriendAllocator<ThemeInfo>::make_shared (filename);
}

ThemeInfo::ThemeInfo (const String &filename) :
  theme_file_ (filename)
{}

ThemeInfo::~ThemeInfo ()
{}

String
ThemeInfo::theme_file () const
{
  return theme_file_;
}

} // Rapicorn
