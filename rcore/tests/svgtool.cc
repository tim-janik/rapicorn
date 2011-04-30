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
#include <algorithm>

#include "../rsvg/svg.hh"

namespace Rapicorn { namespace Cairo {

#define CHECK_CAIRO_STATUS(status)      do {    \
  cairo_status_t ___s = (status);               \
  if (___s != CAIRO_STATUS_SUCCESS)             \
    RAPICORN_LOG (DIAG, "%s: %s", #status, cairo_status_to_string (___s)); \
  } while (0)

bool
surface_check_row_alpha (cairo_surface_t *surface,
                         const int        row,
                         const int        alpha,
                         const int        cmp = 0) // -1: alpha <= px ; +1: alpha >= px ; else alpha == px
{
  return_val_if_fail (surface != NULL, false);
  CHECK_CAIRO_STATUS (cairo_surface_status (surface));
  return_val_if_fail (cairo_image_surface_get_format (surface) == CAIRO_FORMAT_ARGB32, false);
  const int height = cairo_image_surface_get_height (surface);
  return_val_if_fail (row >= 0 && row < height, false);
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
  return_val_if_fail (surface != NULL, false);
  CHECK_CAIRO_STATUS (cairo_surface_status (surface));
  return_val_if_fail (cairo_image_surface_get_format (surface) == CAIRO_FORMAT_ARGB32, false);
  const int height = cairo_image_surface_get_height (surface);
  const int width = cairo_image_surface_get_width (surface);
  return_val_if_fail (col >= 0 && col < width, false);
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

} } // Rapicorn::Cairo

namespace {
using namespace Rapicorn;

static void
test_convert_svg2png()
{
  Svg::Library::add_library ("sample1.svg");
  Svg::Element e = Svg::Library::lookup_element ("#test-box");
  assert (!e.none());
  Svg::Allocation a = e.allocation();
  assert (a.width && a.height);
  a.width *= 9;
  a.height *= 7;
  const int frame = 25, width = a.width + 2 * frame, height = a.height + 2 * frame;
  uint8 *pixels = new uint8[int (width * height * 4)];
  assert (pixels != NULL);
  memset (pixels, 0, width * height * 4);
  cairo_surface_t *surface = cairo_image_surface_create_for_data (pixels, CAIRO_FORMAT_ARGB32, width, height, 4 * width);
  assert (surface != NULL);
  CHECK_CAIRO_STATUS (cairo_surface_status (surface));
  bool rendered = e.render (surface, Svg::Allocation (frame, frame, width - 2 * frame, height - 2 * frame));
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
  cairo_surface_destroy (surface);
  delete[] pixels;
}

} // Anon

int
main (int   argc,
      char *argv[])
{
  rapicorn_init_test (&argc, argv);
  TTITLE ("%s", argv[0]);

  TRUN ("SVG/svg2png", test_convert_svg2png);

  return TEXIT();
}
