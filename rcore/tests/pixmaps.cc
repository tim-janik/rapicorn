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

namespace {
using namespace Rapicorn;

static void
test_pixmap_compare (void)
{
  Pixmap *p1 = new Pixmap (4, 4);
  Pixmap *p2 = new Pixmap (4, 4);
  ref_sink (p1);
  ref_sink (p2);
  double averr, maxerr, nerr, npix;
  bool cmp = p1->compare (*p2, 0, 0, -1, -1, 0, 0, &averr, &maxerr, &nerr, &npix);
  assert (cmp == false && npix == 16);
  p1->row (2)[2] = 0xff667788;
  cmp = p1->compare (*p2, 0, 0, -1, -1, 0, 0, &averr, &maxerr, &nerr, &npix);
  assert (cmp == true && npix == 16);
  assert (nerr == 1 && averr > 0 && maxerr > averr);
  p2->row (2)[2] = 0xff667788;
  cmp = p1->compare (*p2, 0, 0, -1, -1, 0, 0, &averr, &maxerr, &nerr, &npix);
  assert (cmp == false && npix == 16);
  unref (p1);
  unref (p2);
}
REGISTER_TEST ("Pixmaps/compare", test_pixmap_compare);

static Pixmap*
create_sample_pixmap (void)
{
  // create sample, ensure it's properly premultiplied, i.e. alpha=max(ARGB)
  Pixmap *pixmap = new Pixmap (4, 4);
  ref_sink (pixmap);
  uint32 *row = pixmap->row (0);
  row[1] = 0xff667788;
  row = pixmap->row (1);
  row[0] = Test::rand_int() | 0xff000000;
  row[1] = Test::rand_int() | 0xff000000;
  row = pixmap->row (2);
  memset (row, 0x80, pixmap->width() * sizeof (row[0]));
  row = pixmap->row (3);
  row[0] = 0xffffffff;
  row[1] = 0xff008000;
  row[2] = 0xaa773311;
  row[3] = 0xff000000;
  pixmap->comment ("Created by Rapicorn test");
  return pixmap;
}

static void
test_pixmap_save_load (void)
{
  const char *target = "pixmaps-tmp.png";
  Pixmap *pixmap = create_sample_pixmap();
  assert (pixmap->comment().find ("Rapicorn") < 99);
  /* test saving */
  unlink (target);
  assert (Path::check (target, "e") == false);
  if (!pixmap->save_png (target))
    fatal ("failed to save \"%s\": %s", target, string_from_errno (errno).c_str());
  /* test load and compare */
  Pixmap *pixmap2 = Pixmap::load_png (target);
  if (!pixmap2 || errno)
    fatal ("failed to load \"%s\": %s", target, string_from_errno (errno).c_str());
  ref_sink (pixmap2);
  assert (pixmap->comment() == pixmap2->comment());
  assert (pixmap->compare (*pixmap2, 0, 0, -1, -1, 0, 0) == false);
  /* cleanup */
  unlink (target);
  unref (pixmap2);
  unref (pixmap);
}
REGISTER_TEST ("Pixmaps/save & load", test_pixmap_save_load);

static void
test_pixmap_try_alloc (void)
{
  const uint imax = 2048; // 1073741824;
  for (uint ii = 1; ii <= imax; ii <<= 1)
    if (Pixbuf::try_alloc (ii, ii))
      {
        Pixmap *p = new Pixmap (ii, ii);
        unref (p);
      }
  assert (Pixbuf::try_alloc (65536, 65536) == false);
}
REGISTER_TEST ("Pixmaps/try-alloc", test_pixmap_try_alloc);

#include "testpixs.c" // defines alpha_rle, alpha_raw, rgb_rle, rgb_raw

static void
test_pixstreams (void)
{
  /* decode pixstreams */
  Pixmap *pixmap1 = Pixmap::pixstream (alpha_rle);
  if (!pixmap1)
    fatal ("%s(): failed to decode pixstream: %s", STRFUNC, string_from_errno (errno).c_str());
  Pixmap *pixmap2 = Pixmap::pixstream (alpha_raw);
  if (!pixmap2)
    fatal ("%s(): failed to decode pixstream: %s", STRFUNC, string_from_errno (errno).c_str());
  Pixmap *pixmap3 = Pixmap::pixstream (rgb_rle);
  if (!pixmap3)
    fatal ("%s(): failed to decode pixstream: %s", STRFUNC, string_from_errno (errno).c_str());
  Pixmap *pixmap4 = Pixmap::pixstream (rgb_raw);
  if (!pixmap4)
    fatal ("%s(): failed to decode pixstream: %s", STRFUNC, string_from_errno (errno).c_str());
  /* concat pixstreams */
  Pixmap *pixall = new Pixmap (32, 128);
  pixall->copy (*pixmap1, 0, 0, -1, -1, 0,  0);
  pixall->copy (*pixmap2, 0, 0, -1, -1, 0, 32);
  pixall->copy (*pixmap3, 0, 0, -1, -1, 0, 64);
  pixall->copy (*pixmap4, 0, 0, -1, -1, 0, 96);

  /* load reference */
  String reference = "testpixs.png";
  Pixmap *pixref = Pixmap::load_png (Path::vpath_find (reference).c_str());
  if (!pixref || errno)
    fatal ("failed to load \"%s\": %s", reference.c_str(), string_from_errno (errno).c_str());
  ref_sink (pixmap2);
  /* check equality */
  bool cmp = pixall->compare (*pixref, 0, 0, -1, -1, 0, 0);
  assert (cmp == false);
}
REGISTER_TEST ("Pixmaps/pixstreams", test_pixstreams);

} // Anon
