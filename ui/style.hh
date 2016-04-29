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

// == StyleIface ==
class StyleIface : public std::enable_shared_from_this<StyleIface> {
public:
  enum StyleColor { FOREGROUND = 1, BACKGROUND, DARK, DARK_SHADOW, DARK_GLINT, LIGHT, LIGHT_SHADOW, LIGHT_GLINT, FOCUS_FG, FOCUS_BG };
  Color foreground      (WidgetState state) { return style_color (state, FOREGROUND); }
  Color background      (WidgetState state) { return style_color (state, BACKGROUND); }
  Color dark_color      (WidgetState state) { return style_color (state, DARK); }
  Color dark_shadow     (WidgetState state) { return style_color (state, DARK_SHADOW); }
  Color dark_glint      (WidgetState state) { return style_color (state, DARK_GLINT); }
  Color light_color     (WidgetState state) { return style_color (state, LIGHT); }
  Color light_shadow    (WidgetState state) { return style_color (state, LIGHT_SHADOW); }
  Color light_glint     (WidgetState state) { return style_color (state, LIGHT_GLINT); }
  Color focus_color     (WidgetState state) { return style_color (state, FOCUS_FG); }
  virtual Color      theme_color        (double hue360, double saturation100, double brightness100, const String &detail = "") = 0;
  virtual Color      style_color        (WidgetState state, StyleColor color_type, const String &detail = "") = 0;
  static StyleIfaceP load               (const String &theme_file);
  static StyleIfaceP fallback           ();
};

} // Rapicorn

#endif  /* __RAPICORN_THEME_HH__ */
