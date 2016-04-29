// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#ifndef __RAPICORN_THEME_HH__
#define __RAPICORN_THEME_HH__

#include <ui/primitives.hh>

namespace Rapicorn {

// == Decls ==
class StyleIface;
typedef std::shared_ptr<StyleIface> StyleIfaceP;

// == Config ==
class Config {
public:
  static String get (const String &setting, const String &fallback = "");
};

// == StyleColor ==
enum class StyleColor {
  NONE,
  FOREGROUND,
  BACKGROUND,
  DARK,
  DARK_SHADOW,
  DARK_GLINT,
  LIGHT,
  LIGHT_SHADOW,
  LIGHT_GLINT,
  FOCUS_FG,
  FOCUS_BG
};

// == StyleIface ==
class StyleIface : public std::enable_shared_from_this<StyleIface> {
public:
  Color foreground      (WidgetState state) { return state_color (state, StyleColor::FOREGROUND); }
  Color background      (WidgetState state) { return state_color (state, StyleColor::BACKGROUND); }
  Color dark_color      (WidgetState state) { return state_color (state, StyleColor::DARK); }
  Color dark_shadow     (WidgetState state) { return state_color (state, StyleColor::DARK_SHADOW); }
  Color dark_glint      (WidgetState state) { return state_color (state, StyleColor::DARK_GLINT); }
  Color light_color     (WidgetState state) { return state_color (state, StyleColor::LIGHT); }
  Color light_shadow    (WidgetState state) { return state_color (state, StyleColor::LIGHT_SHADOW); }
  Color light_glint     (WidgetState state) { return state_color (state, StyleColor::LIGHT_GLINT); }
  Color focus_color     (WidgetState state) { return state_color (state, StyleColor::FOCUS_FG); }
  virtual Color      theme_color        (double hue360, double saturation100, double brightness100, const String &detail = "") = 0;
  virtual Color      state_color        (WidgetState state, StyleColor color_type, const String &detail = "") = 0;
  static StyleIfaceP load               (const String &theme_file);
  static StyleIfaceP fallback           ();
  static WidgetState state_from_name    (const String &lower_case_state_name);
  static uint64      state_from_list    (const String &state_list);
  static String      pick_fragment      (const String &fragment, WidgetState state, const StringVector &fragment_list);
};

} // Rapicorn

#endif  /* __RAPICORN_THEME_HH__ */
