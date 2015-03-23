// This Source Code Form is licensed MPLv2: http://mozilla.org/MPL/2.0
#include "style.hh"
#include "painter.hh"
#include "../rcore/rsvg/svg.hh"

namespace Rapicorn {

ThemeInfoP
ThemeInfo::create (const String &filename)
{
  return FriendAllocator<ThemeInfo>::make_shared (filename);
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

ThemeInfo::ThemeInfo (const String &theme_file) :
  theme_file_ (theme_file)
{
  Blob data = Res (theme_file_);
  printerr ("IniFile: %s\n", theme_file_);
  IniFile ini (data);
  String sf = ini.value_as_string ("theme.svg_file");
  printerr ("theme.svg_file: %s\n", sf);
  Blob svgblob = Res ("@res " + sf);
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

ThemeInfo::~ThemeInfo ()
{}

String
ThemeInfo::theme_file () const
{
  return theme_file_;
}

} // Rapicorn
