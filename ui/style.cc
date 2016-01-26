// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#include "style.hh"
#include "painter.hh"
#include "factory.hh"
#include <unordered_map>
#include "../rcore/rsvg/svg.hh"

#define TDEBUG(...)     RAPICORN_KEY_DEBUG ("Theme", __VA_ARGS__)

namespace Rapicorn {

// == Colors ==
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
StyleImpl::StyleImpl (ThemeInfoP theme_info) :
  theme_info_ (theme_info)
{
  assert_return (theme_info != NULL);
}

static std::map<ThemeInfoP, StyleImplP> style_impl_cache; /// FIXME: threadsafe?

StyleImplP
StyleImpl::create (ThemeInfoP theme_info)
{
  assert_return (theme_info != NULL, NULL);
  StyleImplP style = style_impl_cache[theme_info];
  if (!style)
    {
      style = FriendAllocator<StyleImpl>::make_shared (theme_info);
      style_impl_cache[theme_info] = style;
    }
  return style;
}

Color
StyleImpl::theme_color (double hue360, double saturation100, double brightness100, const String &detail)
{
  return theme_info_->theme_color (hue360, saturation100, brightness100);
}

Color
StyleImpl::state_color (WidgetState state, ColorType color_type, const String &detail)
{
  switch (color_type)
    {
    case FOREGROUND:    return theme_info_->fragment_color ("#fg", state);
    case BACKGROUND:    return theme_info_->fragment_color ("#bg", state);
    case DARK:          return 0xff9f9c98;
    case DARK_SHADOW:   return adjust_color (state_color (state, DARK, detail), 1, 0.9); // 0xff8f8c88
    case DARK_GLINT:    return adjust_color (state_color (state, DARK, detail), 1, 1.1); // 0xffafaca8
    case LIGHT:         return 0xffdfdcd8;
    case LIGHT_SHADOW:  return adjust_color (state_color (state, LIGHT, detail), 1, 0.93); // 0xffcfccc8
    case LIGHT_GLINT:   return adjust_color (state_color (state, LIGHT, detail), 1, 1.07); // 0xffefece8
    case FOCUS_FG:      return 0xff000060;
    case FOCUS_BG:      return 0xff000060;
    default:            return 0x00000000; // silence warnings
    }
}

// == FallbackTheme ==
class FallbackTheme : public ThemeInfo {
  friend class FriendAllocator<FallbackTheme>; // allows make_shared() access to ctor/dtor
public:
  virtual String name           () override        { return "Rapicorn::FallbackTheme"; }
  virtual Color  fragment_color (const String &fragment, WidgetState state) override;
  virtual Color  theme_color    (double hue360, double saturation100, double brightness100) override;
};

Color
FallbackTheme::fragment_color (const String &fragment, WidgetState state)
{
  return fragment == "#fg" ? 0xff000000 : 0xffdfdcd8;
}

Color
FallbackTheme::theme_color (double hue360, double saturation100, double brightness100)
{
  Color c;
  c.set_hsv (hue360, saturation100 * 0.01, brightness100 * 0.01);
  return c.argb();
}

// == FileTheme ==
class FileTheme : public ThemeInfo {
  friend class FriendAllocator<FileTheme>; // allows make_shared() access to ctor/dtor
  const String theme_name_;
public:
  explicit       FileTheme      (const String &theme_name, const Blob &blob, bool local_files);
  virtual String name           () override         { return theme_name_; }
  virtual Color  fragment_color (const String &fragment, WidgetState state) override;
  virtual Color  theme_color    (double hue360, double saturation100, double brightness100) override;
};

Color
FileTheme::fragment_color (const String &fragment, WidgetState state)
{
  return fallback_theme()->fragment_color (fragment, state);
}

Color
FileTheme::theme_color (double hue360, double saturation100, double brightness100)
{
  return fallback_theme()->theme_color (hue360, saturation100, brightness100);
}

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

FileTheme::FileTheme (const String &theme_name, const Blob &iniblob, bool local_files) :
  theme_name_ (theme_name)
{
  TDEBUG ("%s: initialize from blob='%s' size=%d", theme_name, iniblob.name(), iniblob.size());
  StringVector theme_args;
  IniFile ini (iniblob);
  const String sf = ini.value_as_string ("theme.svg_file");
  Svg::FileP svgf;
  if (!sf.empty())
    {
      const String res = "@res themes/" + sf; // FIXME: load file relative to blob.name + local_files flag
      Blob svgblob = Res (res);
      svgf = Svg::File::load (svgblob);
      const int errno_ = errno;
      TDEBUG ("%s: load '%s': blob='%s' size=%d: %s", theme_name, res, svgblob.name(), svgblob.size(), strerror (errno_));
      if (!errno)
        theme_args.push_back ("theme_svg_file=" + res);
    }
  if (svgf)
    {
      const char *fragment = "#bg:normal";
      ImagePainter painter (svgf, fragment);
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
          TDEBUG ("%s: sample: %s -> 0x%08x", theme_name, fragment, argb);
        }
    }
  const String xf = ini.value_as_string ("theme.xml_file");
  if (!xf.empty())
    {
      const String res = "@res themes/" + xf; // FIXME: load file relative to blob.name + local_files flag
      Blob xmlblob = Res (res);
      TDEBUG ("%s: load '%s': blob='%s' size=%d", theme_name, res, xmlblob.name(), xmlblob.size());
      const String err = Factory::parse_ui_data (xmlblob.name(), xmlblob.size(), xmlblob.data(), "", &theme_args, NULL);
      if (!err.empty())
        TDEBUG ("%s: loading '%s': %s", theme_name, res, err);
    }
}

// == ThemeInfo ==
static std::unordered_map<String, ThemeInfoP> theme_info_map;

ThemeInfo::~ThemeInfo ()
{}

ThemeInfoP
ThemeInfo::find_theme (const String &theme_name)
{
  auto it = theme_info_map.find (theme_name);
  return it != theme_info_map.end() ? it->second : NULL;
}

static String
strip_name (const String &input)
{
  ssize_t slash = input.rfind ('/');
  if (slash < 0)
    slash = 0;
  else
    slash += 1; // skip slash
  ssize_t dot = input.rfind ('.');
  if (dot < 0 || dot < slash)
    dot = input.size();
  return input.substr (slash, dot - slash);
}

ThemeInfoP
ThemeInfo::load_theme (const String &theme_identifier, bool from_env)
{
  assert_return (theme_identifier.empty() == false, NULL);
  assert_return (from_env == (theme_identifier[0] == '$'), NULL);
  String theme_name = theme_identifier;
  if (theme_name[0] == '$')
    {
      const char *evtheme = getenv (theme_name.c_str() + 1);
      if (!evtheme || !evtheme[0])
        return NULL;
      if (Path::isabs (evtheme))
        {
          theme_name = strip_name (evtheme);
          ThemeInfoP theme = find_theme (theme_name);
          if (theme)
            return theme;
          Blob data = Blob::load (evtheme);
          if (!data.size())
            user_warning (UserSource (theme_identifier), "failed to load theme \"%s\": %s", evtheme, strerror (errno));
          else
            {
              theme = FriendAllocator<FileTheme>::make_shared (theme_name, data, true); // allow local file includes
              if (theme)
                theme_info_map[theme_name] = theme;
            }
          return theme;
        }
      // else
      theme_name = evtheme;
    }
  ThemeInfoP theme = find_theme (theme_name);
  if (theme)
    return theme;
  const String resource = "themes/" + theme_name + ".ini";
  Blob blob = Res ("@res " + resource);
  if (!blob.size())
    user_warning (UserSource (theme_identifier), "failed to load theme \"%s\": %s", resource, strerror (errno));
  else
    {
      theme = FriendAllocator<FileTheme>::make_shared (theme_name, blob, false); // disallow local file
      if (theme)
        theme_info_map[theme_name] = theme;
    }
  return theme;
}

ThemeInfoP
ThemeInfo::theme_info (const String &theme_name)
{
  return find_theme (theme_name);
}

// Returns fallback ThemeInfo object that is guaranteed to stay alive (singleton).
ThemeInfoP
ThemeInfo::fallback_theme ()
{
  static ThemeInfoP fallback_theme_info = FriendAllocator<FallbackTheme>::make_shared ();
  return fallback_theme_info;
}

} // Rapicorn
