// This Source Code Form is licensed MPLv2: http://mozilla.org/MPL/2.0
#ifndef __RAPICORN_THEME_HH__
#define __RAPICORN_THEME_HH__

#include <ui/primitives.hh>

namespace Rapicorn {

class ThemeInfo;
typedef std::shared_ptr<ThemeInfo> ThemeInfoP;

class ThemeInfo : public std::enable_shared_from_this<ThemeInfo> {
  const String  theme_file_;
  friend        class FriendAllocator<ThemeInfo>; // allows make_shared() access to ctor/dtor
  explicit      ThemeInfo       (const String &filename);
  virtual      ~ThemeInfo       ();
public:
  static ThemeInfoP create      (const String &filename);
  String        theme_file      () const;
};

} // Rapicorn

#endif  /* __RAPICORN_THEME_HH__ */
