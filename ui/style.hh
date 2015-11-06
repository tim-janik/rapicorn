// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#ifndef __RAPICORN_THEME_HH__
#define __RAPICORN_THEME_HH__

#include <ui/primitives.hh>

namespace Rapicorn {

// == Decls ==
class StyleImpl;
class ThemeInfo;
typedef std::shared_ptr<StyleImpl> StyleImplP;
typedef std::shared_ptr<ThemeInfo> ThemeInfoP;

// == StyleImpl ==
class StyleImpl : public std::enable_shared_from_this<StyleImpl> {
  ThemeInfoP    theme_info_;
  explicit      StyleImpl       (ThemeInfoP theme_info);
  friend        class FriendAllocator<StyleImpl>; // allows make_shared() access to ctor/dtor
public:
  enum ColorType { FOREGROUND = 1, BACKGROUND, DARK, DARK_SHADOW, DARK_GLINT, LIGHT, LIGHT_SHADOW, LIGHT_GLINT, FOCUS_FG, FOCUS_BG };
  Color         theme_color     (double hue360, double saturation100, double brightness100, const String &detail = "");
  Color         state_color     (StateType state, ColorType color_type, const String &detail = "");
  Color         foreground      (StateType state) { return state_color (state, FOREGROUND); }
  Color         background      (StateType state) { return state_color (state, BACKGROUND); }
  Color         dark_color      (StateType state) { return state_color (state, DARK); }
  Color         dark_shadow     (StateType state) { return state_color (state, DARK_SHADOW); }
  Color         dark_glint      (StateType state) { return state_color (state, DARK_GLINT); }
  Color         light_color     (StateType state) { return state_color (state, LIGHT); }
  Color         light_shadow    (StateType state) { return state_color (state, LIGHT_SHADOW); }
  Color         light_glint     (StateType state) { return state_color (state, LIGHT_GLINT); }
  Color         focus_color     (StateType state) { return state_color (state, FOCUS_FG); }
  static StyleImplP  create     (ThemeInfoP theme_info);
};

// == ThemeInfo ==
class ThemeInfo : public std::enable_shared_from_this<ThemeInfo> {
  static ThemeInfoP find_theme     (const String &theme_name);
protected:
  virtual          ~ThemeInfo      ();
public:
  static ThemeInfoP load_theme     (const String &theme_identifier, bool from_env = false);
  static ThemeInfoP theme_info     (const String &theme_name);
  static ThemeInfoP fallback_theme ();
  virtual String    name           () = 0;
  // colors
  virtual Color     fragment_color (const String &fragment, StateType state) = 0;
  virtual Color     theme_color    (double hue360, double saturation100, double brightness100) = 0;
};

} // Rapicorn

#endif  /* __RAPICORN_THEME_HH__ */
