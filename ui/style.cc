// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#include "style.hh"
#include "painter.hh"
#include "factory.hh"
#include <unordered_map>
#include "../rcore/svg.hh"

#define TDEBUG(...)     RAPICORN_KEY_DEBUG ("Theme", __VA_ARGS__)
#define SDEBUG(...)     RAPICORN_KEY_DEBUG ("Style", __VA_ARGS__)

namespace Rapicorn {

// == Config ==
static StringVector
user_ini_files()
{
  /* INI file search order:
   * 1. ${XDG_CONFIG_HOME:-~/.config}/application/rapicorn.ini
   * 2. ${XDG_CONFIG_HOME:-~/.config}/rapicorn/config.ini
   * 3. ${XDG_CONFIG_DIRS} * /application/rapicorn.ini
   * 4. ${XDG_CONFIG_DIRS} * /rapicorn/config.ini
   * 5. /etc/application/rapicorn.ini
   * 6. /etc/rapicorn/config.ini
   */
  String config_searchpaths = Path::searchpath_join (Path::config_home(), Path::config_dirs());
  if (!Path::searchpath_contains (config_searchpaths, "/etc/"))
    config_searchpaths = Path::searchpath_join (config_searchpaths, "/etc");
  String config_files = Path::searchpath_multiply (Path::config_names(), "rapicorn.ini");
  config_files = Path::searchpath_join (config_files, "rapicorn/config.ini");
  config_searchpaths = Path::searchpath_multiply (config_searchpaths, config_files);
  StringVector inifiles = Path::searchpath_list (config_searchpaths, "r");
  return inifiles;
}

String
Config::get (const String &setting, const String &fallback)
{
  static const StringVector ini_file_names = user_ini_files();
  static vector<IniFile*> ini_files (ini_file_names.size());
  String value;
  for (size_t i = 0; i < ini_files.size(); i++)
    {
      if (!ini_files[i])
        ini_files[i] = new IniFile (Blob::load (ini_file_names[i]));
      if (ini_files[i]->has_value (setting, &value))
        return value;
    }
  return fallback;
}

// == Colors ==
static inline uint32
cairo_image_surface_peek_argb (cairo_surface_t *surface, uint x, uint y)
{
  assert_return (surface != NULL, 0x00000000);
  assert_return (cairo_image_surface_get_format (surface) == CAIRO_FORMAT_ARGB32, 0x00000000);
  const uint height = cairo_image_surface_get_height (surface);
  const uint width = cairo_image_surface_get_width (surface);
  const uint stride = cairo_image_surface_get_stride (surface);
  if (x < width && y < height)
    {
      const uint32 *pixels = (const uint32*) cairo_image_surface_get_data (surface);
      const uint32 argb = pixels[stride / 4 * y + x];
      return argb;
    }
  return 0x00000000;
}

static Color
adjust_color (Color color, double saturation_factor, double value_factor)
{
  double hue, saturation, value;
  color.get_hsv (&hue, &saturation, &value);
  saturation *= saturation_factor;
  value *= value_factor;
  color.set_hsv (hue, MIN (1, saturation), MIN (1, value));
  return color;
}
// static Color lighten   (Color color) { return adjust_color (color, 1.0, 1.1); }
// static Color darken    (Color color) { return adjust_color (color, 1.0, 0.9); }
// static Color alternate (Color color) { return adjust_color (color, 1.0, 0.98); } // tainting for even-odd alterations

// == StyleImpl ==
class StyleImpl : public StyleIface {
  Svg::FileP svg_file_;
public:
  explicit      StyleImpl       (Svg::FileP svgf);
  virtual Color theme_color     (double hue360, double saturation100, double brightness100, const String &detail) override;
  virtual Color state_color     (WidgetState state, StyleColor color_type, const String &detail) override;
  Color         fragment_color  (const String &fragment, WidgetState state);
};

StyleImpl::StyleImpl (Svg::FileP svgf) :
  svg_file_ (svgf)
{}

Color
StyleImpl::theme_color (double hue360, double saturation100, double brightness100, const String &detail)
{
  Color c;
  c.set_hsv (hue360, saturation100 * 0.01, brightness100 * 0.01);
  return c.argb();
}

Color
StyleImpl::state_color (WidgetState state, StyleColor color_type, const String &detail)
{
  switch (color_type)
    {
    case StyleColor::FOREGROUND:    return fragment_color ("fg", state);
    case StyleColor::BACKGROUND:    return fragment_color ("bg", state);
    case StyleColor::DARK:          return 0xff9f9c98;
    case StyleColor::DARK_SHADOW:   return adjust_color (state_color (state, StyleColor::DARK, detail), 1, 0.9); // 0xff8f8c88
    case StyleColor::DARK_GLINT:    return adjust_color (state_color (state, StyleColor::DARK, detail), 1, 1.1); // 0xffafaca8
    case StyleColor::LIGHT:         return 0xffdfdcd8;
    case StyleColor::LIGHT_SHADOW:  return adjust_color (state_color (state, StyleColor::LIGHT, detail), 1, 0.93); // 0xffcfccc8
    case StyleColor::LIGHT_GLINT:   return adjust_color (state_color (state, StyleColor::LIGHT, detail), 1, 1.07); // 0xffefece8
    case StyleColor::FOCUS_FG:      return 0xff000060;
    case StyleColor::FOCUS_BG:      return 0xff000060;
    default:                        return 0x00000000; // silence warnings
    }
}

Color
StyleImpl::fragment_color (const String &fragment, WidgetState state)
{
  Color color;
  color = string_startswith (fragment, "fg") ? 0xff000000 : 0xffdfdcd8;
  if (svg_file_)
    {
      ImagePainter painter (svg_file_, fragment);
      Requisition size = painter.image_size ();
      if (size.width >= 1 && size.height >= 1)
        {
          cairo_surface_t *surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, iceil (size.width), iceil (size.height));
          cairo_t *cr = cairo_create (surface);
          Rect rect (0, 0, size.width, size.height);
          painter.draw_image (cr, rect, rect);
          const uint argb = cairo_image_surface_peek_argb (surface, size.width / 2, size.height / 2);
          cairo_surface_destroy (surface);
          cairo_destroy (cr);
          color = argb;
        }
    }
  return color;
}


// == StyleIface ==
static std::map<const String, StyleIfaceP> style_file_cache;

StyleIfaceP
StyleIface::fallback()
{
  const String theme_file = "@res Rapicorn::FallbackTheme";
  StyleIfaceP style = style_file_cache[theme_file];
  if (!style)
    {
      Svg::FileP svgf;
      style = FriendAllocator<StyleImpl>::make_shared (svgf);
      style_file_cache[theme_file] = style;
    }
  return style;
}

StyleIfaceP
StyleIface::load (const String &theme_file)
{
  StyleIfaceP style = style_file_cache[theme_file];
  if (!style)
    {
      Svg::FileP svgf;
      bool do_svg_file_io = false;
      if (!string_startswith (theme_file, "@res"))
        do_svg_file_io = RAPICORN_FLIPPER ("svg-file-io", "Rapicorn::Style: allow loading of SVG files from local file system.");
      Blob blob;
      if (do_svg_file_io)
        blob = Blob::load (theme_file);
      else
        blob = Res (theme_file); // call this even if !startswith("@res") to preserve debug message about missing resource
      if (string_endswith (blob.name(), ".svg"))
        {
          svgf = Svg::File::load (blob);
          SDEBUG ("loading: %s: %s", theme_file, strerror (errno));
        }
      if (!svgf)
        {
          user_warning (UserSource (theme_file), "failed to load theme file \"%s\": %s", theme_file, strerror (ENOENT));
          style = fallback();
        }
      else
        style = FriendAllocator<StyleImpl>::make_shared (svgf);
      style_file_cache[theme_file] = style;
    }
  return style;
}

} // Rapicorn
