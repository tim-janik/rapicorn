/* Rapicorn
 * Copyright (C) 2008 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * A copy of the GNU Lesser General Public License should ship along
 * with this library; if not, see http://www.gnu.org/copyleft/.
 */
#include <rcore/testutils.hh>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <algorithm>

#include "../rsvg/svg.hh"

#include "resources.cc"

namespace Rapicorn { namespace Cairo {

#define CHECK_CAIRO_STATUS(status)      do {    \
  cairo_status_t ___s = (status);               \
  if (___s != CAIRO_STATUS_SUCCESS)             \
    DEBUG ("%s: %s", cairo_status_to_string (___s), #status);   \
  } while (0)

bool
surface_check_row_alpha (cairo_surface_t *surface,
                         const int        row,
                         const int        alpha,
                         const int        cmp = 0) // -1: alpha <= px ; +1: alpha >= px ; else alpha == px
{
  assert_return (surface != NULL, false);
  CHECK_CAIRO_STATUS (cairo_surface_status (surface));
  assert_return (cairo_image_surface_get_format (surface) == CAIRO_FORMAT_ARGB32, false);
  const int height = cairo_image_surface_get_height (surface);
  assert_return (row >= 0 && row < height, false);
  const int width = cairo_image_surface_get_width (surface);
  const int stride = cairo_image_surface_get_stride (surface);
  const uint32 *pixels = (const uint32*) cairo_image_surface_get_data (surface);
  for (int i = 0; i < width; i++)
    {
      const uint32 argb = pixels[stride / 4 * row + i];
      const uint8 palpha = argb >> 24;
      if ((cmp < 0 && alpha > palpha) ||
          (cmp > 0 && alpha < palpha) ||
          (cmp == 0 && alpha != palpha))
        return false;
    }
  return true;
}

bool
surface_check_col_alpha (cairo_surface_t *surface,
                         const int        col,
                         const int        alpha,
                         const int        cmp = 0) // -1: alpha <= px ; +1: alpha >= px ; else alpha == px
{
  assert_return (surface != NULL, false);
  CHECK_CAIRO_STATUS (cairo_surface_status (surface));
  assert_return (cairo_image_surface_get_format (surface) == CAIRO_FORMAT_ARGB32, false);
  const int height = cairo_image_surface_get_height (surface);
  const int width = cairo_image_surface_get_width (surface);
  assert_return (col >= 0 && col < width, false);
  const int stride = cairo_image_surface_get_stride (surface);
  const uint32 *pixels = (const uint32*) cairo_image_surface_get_data (surface);
  for (int i = 0; i < height; i++)
    {
      const uint32 argb = pixels[stride / 4 * i + col];
      const uint8 palpha = argb >> 24;
      if ((cmp < 0 && alpha > palpha) ||
          (cmp > 0 && alpha < palpha) ||
          (cmp == 0 && alpha != palpha))
        return false;
    }
  return true;
}

static int
uint16_cmp (const uint16 &i1, const uint16 &i2)
{
  return i1 < i2 ? -1 : i1 > i2;
}

bool
surface_to_ascii (cairo_surface_t *surface,
                  vector<char>    &vc,
                  uint            *vwidth,
                  uint64          *pixhash,
                  uint             cwidth)
{
  const char chars[] = " `-_.,':=^;\"~+rz\\!c/<>i(|)xvts*eL{T}7o[l]Fanuj?1IfZJCEYS24w5qpk3hyPVbgd069XmG#%8OAUK&BRD$HQ@WNM";
  const uint16 NC = 95, LC = NC - 1; // n_chars, last_char
  const uint16 weights[] = {
    /* ' ' */ 0x000, /* '`' */ 0x034, /* '-' */ 0x038, /* '_' */ 0x03c, /* '.' */ 0x04c, /* ',' */ 0x050,
    /*'\'' */ 0x05c, /* ':' */ 0x06c, /* '=' */ 0x07c, /* '^' */ 0x08c, /* ';' */ 0x090, /* '"' */ 0x09c,
    /* '~' */ 0x0a4, /* '+' */ 0x0c4, /* 'r' */ 0x0f4, /* 'z' */ 0x0fc, /*'\\' */ 0x10c, /* '!' */ 0x10e,
    /* 'c' */ 0x110, /* '/' */ 0x112, /* '<' */ 0x114, /* '>' */ 0x118, /* 'i' */ 0x11c, /* '(' */ 0x124,
    /* '|' */ 0x126, /* ')' */ 0x129, /* 'x' */ 0x12c, /* 'v' */ 0x130, /* 't' */ 0x13c, /* 's' */ 0x140,
    /* '*' */ 0x144, /* 'e' */ 0x14c, /* 'L' */ 0x154, /* '{' */ 0x155, /* 'T' */ 0x157, /* '}' */ 0x158,
    /* '7' */ 0x15a, /* 'o' */ 0x15c, /* '[' */ 0x164, /* 'l' */ 0x166, /* ']' */ 0x169, /* 'F' */ 0x174,
    /* 'a' */ 0x178, /* 'n' */ 0x17c, /* 'u' */ 0x17e, /* 'j' */ 0x180, /* '?' */ 0x182, /* '1' */ 0x184,
    /* 'I' */ 0x186, /* 'f' */ 0x188, /* 'Z' */ 0x18a, /* 'J' */ 0x194, /* 'C' */ 0x1a4, /* 'E' */ 0x1a6,
    /* 'Y' */ 0x1a9, /* 'S' */ 0x1b4, /* '2' */ 0x1b8, /* '4' */ 0x1c4, /* 'w' */ 0x1cc, /* '5' */ 0x1d4,
    /* 'q' */ 0x1d6, /* 'p' */ 0x1d9, /* 'k' */ 0x1dc, /* '3' */ 0x1e0, /* 'h' */ 0x1f4, /* 'y' */ 0x1f6,
    /* 'P' */ 0x1f9, /* 'V' */ 0x1fc, /* 'b' */ 0x204, /* 'g' */ 0x206, /* 'd' */ 0x209, /* '0' */ 0x21c,
    /* '6' */ 0x224, /* '9' */ 0x226, /* 'X' */ 0x228, /* 'm' */ 0x22a, /* 'G' */ 0x22c, /* '#' */ 0x230,
    /* '%' */ 0x23c, /* '8' */ 0x244, /* 'O' */ 0x24c, /* 'A' */ 0x254, /* 'U' */ 0x258, /* 'K' */ 0x25c,
    /* '&' */ 0x264, /* 'B' */ 0x26c, /* 'R' */ 0x26e, /* 'D' */ 0x271, /* '$' */ 0x274, /* 'H' */ 0x278,
    /* 'Q' */ 0x294, /* '@' */ 0x2bc, /* 'W' */ 0x2dc, /* 'N' */ 0x2ec, /* 'M' */ 0x2f4, /* MAX: 0x2fd */
  };
  assert (sizeof (chars) == NC + 1 && sizeof (weights) / sizeof (weights[0]) == NC);
  assert_return (surface != NULL, false);
  CHECK_CAIRO_STATUS (cairo_surface_status (surface));
  assert_return (cairo_image_surface_get_format (surface) == CAIRO_FORMAT_ARGB32, false);
  const uint swidth = cairo_image_surface_get_width (surface);
  const uint sheight = cairo_image_surface_get_height (surface);
  const uint sstride = cairo_image_surface_get_stride (surface);
  const uint32 *pixels = (const uint32*) cairo_image_surface_get_data (surface);
  cwidth = CLAMP (cwidth, 4, 1022);
  const uint hshrink = ceil (MAX (swidth, cwidth) / double (cwidth));
  const uint vshrink = 1.8 * hshrink;
  *vwidth = (swidth + hshrink - 1) / hshrink;
  vc.resize (*vwidth * (sheight + vshrink - 1) / vshrink);
  for (uint row = 0; row < sheight; row += vshrink)
    for (uint col = 0; col < swidth; col += hshrink)
      {
        uint r = 0, g = 0, b = 0;
        for (uint j = 0; j < vshrink && row + j < sheight; j++)
          for (uint i = 0; i < hshrink && col + i < swidth; i++)
            {
              const uint argb = pixels[(row + j) * sstride/4 + col + i]; // premultiplied
              r += (argb >> 16) & 0xff;
              g += (argb >>  8) & 0xff;
              b += (argb >>  0) & 0xff;
            }
        const uint16 v = (r + g + b) / (hshrink * vshrink); // average, max: 0x2fd
        const uint16 *weightsx = binary_lookup_sibling (&weights[0], &weights[NC], uint16_cmp, v);
        if (weightsx < &weights[LC] && v > *weightsx) // lookup closest on mismatch
          weightsx = v - weightsx[0] > weightsx[1] - v ? &weightsx[1] : &weightsx[0];
        const char c = chars[MIN (LC, uint (weightsx - weights))];
        vc.at ((row * *vwidth) / vshrink + col / hshrink) = c;
      }
  // DEK Hash (FIXME: should use crypto hash here)
  *pixhash = swidth * sheight;
  for (uint row = 0; row < sheight; row++)
    for (uint col = 0; col < swidth; col++)
      {
        const uint argb = pixels[row * sstride/4 + col]; // premultiplied
        *pixhash = ((*pixhash << 5) ^ (*pixhash >> 27)) ^ ((argb >> 24) & 0xff);
        *pixhash = ((*pixhash << 5) ^ (*pixhash >> 27)) ^ ((argb >> 16) & 0xff);
        *pixhash = ((*pixhash << 5) ^ (*pixhash >> 27)) ^ ((argb >>  8) & 0xff);
        *pixhash = ((*pixhash << 5) ^ (*pixhash >> 27)) ^ ((argb >>  0) & 0xff);
      }
  return true;
}

bool
surface_printout (cairo_surface_t *surface,
                  uint             cwidth = 78)
{
  vector<char> ascii;
  uint         awidth;
  uint64       pixhash;
  if (Cairo::surface_to_ascii (surface, ascii, &awidth, &pixhash, cwidth))
    {
      const uint swidth = cairo_image_surface_get_width (surface);
      const uint sheight = cairo_image_surface_get_height (surface);
      printout ("  ARGB:%ux%u:%08llx\n  ", swidth, sheight, pixhash);
      for (uint j = 0; j < awidth; j++)
        printout ("-");
      printout ("\n");
      for (uint r = 0; r < ascii.size() / awidth; r++)
        {
          String s (&ascii[r * awidth], awidth);
          printout (" |%s|\n", s.c_str());
        }
      printout ("  ");
      for (uint j = 0; j < awidth; j++)
        printout ("-");
      printout ("\n");
      return true;
    }
  return false;
}

} } // Rapicorn::Cairo

namespace {
using namespace Rapicorn;

static void
test_convert_svg2png()
{
  Svg::FileP file = Svg::File::load ("sample1.svg");
  Svg::ElementP e = file->lookup ("#test-box");
  assert (e);
  const Svg::BBox bbox = e->bbox();
  assert (bbox.width && bbox.height);
  Svg::BBox a = bbox;
  a.width *= 9;
  a.height *= 7;
  const int frame = 11, width = a.width + 2 * frame, height = a.height + 2 * frame;
  uint8 *pixels = new uint8[int (width * height * 4)];
  assert (pixels != NULL);
  memset (pixels, 0, width * height * 4);
  cairo_surface_t *surface = cairo_image_surface_create_for_data (pixels, CAIRO_FORMAT_ARGB32, width, height, 4 * width);
  assert (surface != NULL);
  CHECK_CAIRO_STATUS (cairo_surface_status (surface));
  cairo_surface_set_device_offset (surface, frame, frame);
  bool rendered = e->render (surface, (width - 2 * frame) / bbox.width, (height - 2 * frame) / bbox.height);
  assert (rendered);
  cairo_status_t cstatus = cairo_surface_write_to_png (surface, "tmp-testsvg.png");
  assert (cstatus == CAIRO_STATUS_SUCCESS);
  bool clear_frame = Cairo::surface_check_col_alpha (surface, frame - 1, 0) &&
                     Cairo::surface_check_row_alpha (surface, frame - 1, 0) &&
                     Cairo::surface_check_col_alpha (surface, width - frame, 0) &&
                     Cairo::surface_check_row_alpha (surface, height - frame, 0);
  assert (clear_frame); // checks for an empty pixel frame around rendered image
  bool cross_content = !Cairo::surface_check_col_alpha (surface, width / 2, 0) &&
                       !Cairo::surface_check_row_alpha (surface, height / 2, 0);
  assert (cross_content); // checks for centered cross-hair to detect rendered contents
  Cairo::surface_printout (surface, 56);
  cairo_surface_destroy (surface);
  delete[] pixels;
  unlink ("tmp-testsvg.png");
}
REGISTER_LOGTEST ("SVG/svg2png", test_convert_svg2png);

#if 0
static void
test_convert_png2ascii()
{
  const char *filename = "xtest.png";
  cairo_surface_t *surface = cairo_image_surface_create_from_png (filename);
  assert (surface);
  CHECK_CAIRO_STATUS (cairo_surface_status (surface));
  Cairo::surface_printout (surface, 76);
  cairo_surface_destroy (surface);
}
#endif

} // Anon
