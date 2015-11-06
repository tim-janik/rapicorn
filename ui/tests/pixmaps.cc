// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#include <rcore/testutils.hh>
#include <rapicorn.hh>
#include <string.h>
#include <unistd.h>
#include <errno.h>

namespace {
using namespace Rapicorn;

static void
test_pixmap_compare (void)
{
  Pixmap p1 (4, 4);
  Pixmap p2 (4, 4);
  double averr, maxerr, nerr, npix;
  bool cmp = p1.compare (p2, 0, 0, -1, -1, 0, 0, &averr, &maxerr, &nerr, &npix);
  assert (cmp == false && npix == 16);
  p1.row (2)[2] = 0xff667788;
  cmp = p1.compare (p2, 0, 0, -1, -1, 0, 0, &averr, &maxerr, &nerr, &npix);
  assert (cmp == true && npix == 16);
  assert (nerr == 1 && averr > 0 && maxerr > averr);
  p2.row (2)[2] = 0xff667788;
  cmp = p1.compare (p2, 0, 0, -1, -1, 0, 0, &averr, &maxerr, &nerr, &npix);
  assert (cmp == false && npix == 16);
}
REGISTER_TEST ("Pixmaps/compare", test_pixmap_compare);

static Pixmap
create_sample_pixmap (void)
{
  // create sample, ensure it's properly premultiplied, i.e. alpha=max(ARGB)
  Pixmap pixmap (4, 4);
  uint32 *row = pixmap.row (0);
  row[1] = 0xff667788;
  row = pixmap.row (1);
  row[0] = Test::random_int64() | 0xff000000;
  row[1] = Test::random_int64() | 0xff000000;
  row = pixmap.row (2);
  memset (row, 0x80, pixmap.width() * sizeof (row[0]));
  row = pixmap.row (3);
  row[0] = 0xffffffff;
  row[1] = 0xff008000;
  row[2] = 0xaa773311;
  row[3] = 0xff000000;
  pixmap.set_attribute ("comment", "Created by Rapicorn test");
  return pixmap;
}

static void
test_pixmap_save_load (void)
{
  const char *target = "pixmaps-tmp.png";
  Pixmap pixmap = create_sample_pixmap();
  assert (pixmap.get_attribute ("comment").find ("Rapicorn") < 99);
  /* test saving */
  unlink (target);
  assert (Path::check (target, "e") == false);
  if (!pixmap.save_png (target))
    fatal ("failed to save \"%s\": %s", target, string_from_errno (errno).c_str());
  /* test load and compare */
  Pixmap pixmap2;
  if (!pixmap2.load_png (target))
    fatal ("failed to load \"%s\": %s", target, string_from_errno (errno).c_str());
  assert (pixmap.get_attribute ("comment") == pixmap2.get_attribute ("comment"));
  assert (pixmap.compare (pixmap2, 0, 0, -1, -1, 0, 0) == false);
  /* cleanup */
  unlink (target);
}
REGISTER_TEST ("Pixmaps/save & load", test_pixmap_save_load);

static void
test_pixmap_try_alloc (void)
{
  const uint imax = 2048; // 1073741824;
  Pixmap tpixmap;
  for (uint ii = 1; ii <= imax; ii <<= 1)
    {
      const bool success = tpixmap.try_resize (ii, ii);
      TASSERT (success == true);
    }
  assert (tpixmap.try_resize (65536, 65536) == false);
}
REGISTER_TEST ("Pixmaps/try-alloc", test_pixmap_try_alloc);

#include "testpixs.c" // defines alpha_rle, alpha_raw, rgb_rle, rgb_raw

static void
test_pixstreams (void)
{
  /* decode pixstreams */
  Pixmap spixmap1, spixmap2, spixmap3, spixmap4;
  if (!spixmap1.load_pixstream (alpha_rle))
    fatal ("%s(): failed to decode pixstream: %s", STRFUNC, string_from_errno (errno).c_str());
  if (!spixmap2.load_pixstream (alpha_raw))
    fatal ("%s(): failed to decode pixstream: %s", STRFUNC, string_from_errno (errno).c_str());
  if (!spixmap3.load_pixstream (rgb_rle))
    fatal ("%s(): failed to decode pixstream: %s", STRFUNC, string_from_errno (errno).c_str());
  if (!spixmap4.load_pixstream (rgb_raw))
    fatal ("%s(): failed to decode pixstream: %s", STRFUNC, string_from_errno (errno).c_str());
  /* concat pixstreams */
  Pixmap pixall (32, 128);
  pixall.copy (spixmap1, 0, 0, -1, -1, 0,  0);
  pixall.copy (spixmap2, 0, 0, -1, -1, 0, 32);
  pixall.copy (spixmap3, 0, 0, -1, -1, 0, 64);
  pixall.copy (spixmap4, 0, 0, -1, -1, 0, 96);

  /* load reference */
  String reference = "testpixs.png";
  Pixmap pixref;
  if (!pixref.load_png (Path::vpath_find (reference).c_str()))
    fatal ("failed to load \"%s\": %s", reference.c_str(), string_from_errno (errno).c_str());
  /* check equality */
  bool cmp = pixall.compare (pixref, 0, 0, -1, -1, 0, 0);
  assert (cmp == false);
}
REGISTER_TEST ("Pixmaps/pixstreams", test_pixstreams);

} // Anon
