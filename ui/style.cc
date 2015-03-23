// This Source Code Form is licensed MPLv2: http://mozilla.org/MPL/2.0
#include "style.hh"
#include "painter.hh"
#include <unordered_map>
#include "../rcore/rsvg/svg.hh"

namespace Rapicorn {

// == FallbackTheme ==
class FallbackTheme : public ThemeInfo {
  friend class FriendAllocator<FallbackTheme>; // allows make_shared() access to ctor/dtor
public:
  virtual Color state_color (StateType state, bool foreground, const String &detail) override;
  virtual Color theme_color (double hue360, double saturation100, double brightness100, const String &detail) override;
};

Color
FallbackTheme::state_color (StateType state, bool foreground, const String &detail)
{
  return foreground ? 0xff000000 : 0xff808080;
}

Color
FallbackTheme::theme_color (double hue360, double saturation100, double brightness100, const String &detail)
{
  Color c;
  c.set_hsv (hue360, saturation100 * 0.01, brightness100 * 0.01);
  return c.argb();
}

// == FileTheme ==
class FileTheme : public ThemeInfo {
  friend class FriendAllocator<FileTheme>; // allows make_shared() access to ctor/dtor
public:
  explicit      FileTheme   (const Blob &blob, bool local_files);
  virtual Color state_color (StateType state, bool foreground, const String &detail) override;
  virtual Color theme_color (double hue360, double saturation100, double brightness100, const String &detail) override;
};

Color
FileTheme::state_color (StateType state, bool foreground, const String &detail)
{
  return fallback_theme()->state_color (state, foreground, detail);
}

Color
FileTheme::theme_color (double hue360, double saturation100, double brightness100, const String &detail)
{
  return fallback_theme()->theme_color (hue360, saturation100, brightness100, detail);
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

FileTheme::FileTheme (const Blob &blob, bool local_files)
{
  printerr ("Theme Blob: %s\n", blob.name());
  IniFile ini (blob);
  printerr ("theme.xml_file: %s\n", ini.value_as_string ("theme.xml_file"));
  String sf = ini.value_as_string ("theme.svg_file");
  printerr ("theme.svg_file: %s\n", sf);
  Blob svgblob = Res ("@res " + sf); // FIXME: load SVG relative to blob.name + local_files flag
  printerr ("svgblob: %zd: %s\n", svgblob.size(), strerror (errno));
  auto svgf = Svg::File::load (svgblob);
  printerr ("loading: %s: %s\n", sf, strerror (errno));
  if (svgf)
    {
      const char *fragment = "#normal-bg";
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
          printerr ("  Sample: %s -> 0x%08x\n", fragment, argb);
        }
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
ThemeInfo::load_theme (const String &theme_resource)
{
  assert_return (theme_resource.empty() == false, NULL);
  String theme_name = theme_resource;
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
            user_warning (UserSource (theme_resource), "failed to load theme \"%s\": %s", evtheme, strerror (errno));
          else
            {
              theme = FriendAllocator<FileTheme>::make_shared (data, true); // allow local file includes
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
    user_warning (UserSource (theme_resource), "failed to load theme \"%s\": %s", resource, strerror (errno));
  else
    {
      theme = FriendAllocator<FileTheme>::make_shared (blob, false); // disallow local file
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
