// This Source Code Form is licensed MPLv2: http://mozilla.org/MPL/2.0
#ifndef __RAPICORN_THEME_HH__
#define __RAPICORN_THEME_HH__

#include <ui/primitives.hh>

namespace Rapicorn {

class ThemeInfo;
typedef std::shared_ptr<ThemeInfo> ThemeInfoP;

class ThemeInfo : public std::enable_shared_from_this<ThemeInfo> {
  static ThemeInfoP find_theme     (const String &theme_name);
protected:
  virtual          ~ThemeInfo      ();
public:
  static ThemeInfoP load_theme     (const String &theme_resource);
  static ThemeInfoP theme_info     (const String &theme_name);
  static ThemeInfoP fallback_theme ();
};

} // Rapicorn

#endif  /* __RAPICORN_THEME_HH__ */
