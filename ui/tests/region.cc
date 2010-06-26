/* Tests
 * Copyright (C) 2006 Tim Janik
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
//#define TEST_VERBOSE
#include <rcore/rapicorntests.h>
#include <rapicorn.hh>
#include <stdio.h>
#include <stdlib.h>     // lrand48

namespace {
using namespace Rapicorn;

static void
test_rect (void)
{
  TSTART ("rectangles");
  Rect r1;
  TASSERT (r1.empty());
  r1.assign (Point (7.1, 7.2), Point (8.4, 8.5));       // stores width=1.3, height=1.3 in double precision
  TASSERT (!r1.empty());
  Rect r2 (7.1, 7.2, 1.3, 1.3);
  TASSERT (r2 == Rect (7.1, 7.2, 1.3, 1.3));
  TASSERT (false == (r2 != Rect (7.1, 7.2, 1.3, 1.3)));
  Rect r3 (7.1, 7.2, 1.0, 1.0);
  TASSERT (r3.equals (r2, 0.1) == false);
  TASSERT (r3.equals (r2, 0.5) == true);
  TASSERT (r2.equals (r1, 0.00000000000001));           // using epsilon due to precision artefacts
  TDONE();
}

static void
test_region_basics (void)
{
  TSTART ("region basics");
  {
    Region r, z;
    TASSERT (r.empty());
    TASSERT (z.empty());
    TASSERT (r.equal (r));
    TASSERT (r.equal (z));
    r.add (Rect (Point (-1, -1), Point (1, 1)));
    TASSERT (!r.empty());
    TASSERT (z.empty());
    TASSERT (!r.equal (z));
    r.swap (z);
    TASSERT (r.empty());
    TASSERT (!z.empty());
    TASSERT (!r.equal (z));
    z.clear();
    TASSERT (r.empty());
    TASSERT (z.empty());
    TASSERT (r.equal (z));
  }
  TDONE();
}

static void
test_region_rect1 (void)
{
  TSTART ("region rec1t");
  Region r;
  r.clear();
  TASSERT (r.empty());
  TASSERT (r.contains (Point (0, 0)) == false);
  r.add (Rect (Point (3, 3), Point (5, 5)));
  TASSERT (!r.empty());
  TASSERT (r.contains (2, 2) == false);
  TASSERT (r.contains (3, 3));
  TASSERT (r.contains (4, 4));
  TASSERT (r.contains (5, 5) == false);
  TASSERT (r.contains (6, 6) == false);
  TASSERT (r.contains (Rect (Point (3, 3), Point (5, 5))) == r.INSIDE);
  TASSERT (r.contains (Rect (Point (4, 4), Point (6, 6))) == r.PARTIAL);
  TASSERT (r.contains (Rect (Point (5, 5), Point (6, 6))) == r.OUTSIDE);
  vector<Rect> rects;
  r.list_rects (rects);
  TASSERT (rects.size() == 1);
  TASSERT (rects[0].lower_left() == Point (3, 3));
  TASSERT (rects[0].upper_right() == Point (5, 5));
  Region z (r);
  TASSERT (r.equal (z));
  r.exor (r);
  TASSERT (r.empty());
  TASSERT (!r.equal (z));
  z.clear();
  TASSERT (r.equal (z));
  TASSERT (r.extents() == Rect (Point (0, 0), Point (0, 0)));
  const Region e;
  TASSERT (e.contains (Point (0, 0)) == false);
  TASSERT (e.contains (Rect (Point (0, 0), Point (0, 0))) == true);
  TASSERT (e.contains (Rect (Point (0, 0), Point (1, 1))) == false);
  TASSERT (e.contains (Region()) == true);
  TDONE();
}

static void
test_region2 (void)
{
  TSTART ("region2");
  Region a (Rect (Point (-1, -1), Point (1, 1)));
  Region b (Rect (Point (-1, -1), Point (0, 0)));
  TASSERT (a.equal (b) == false);
  TASSERT ((a != b) == true);
  TASSERT ((a == b) == false);
  TASSERT (a.cmp (b) != 0);
  if (a.cmp (b) < 0)
    TASSERT (a < b);
  else
    TASSERT (b < a);
  a.clear();
  b.clear();
  TASSERT (a.empty());
  TASSERT (b.empty());
  TASSERT (a.equal (b) == true);
  TASSERT ((a != b) == false);
  TASSERT ((a == b) == true);
  TASSERT (a.cmp (b) == 0);
  TASSERT ((a < b) == false);
  TASSERT ((b < a) == false);
  TDONE();
}

static void
test_region2_ops (void)
{
  TSTART ("region2 ops");
  Region a (Rect (Point (-1, -1), Point (1, 1))), sa = a;
  Region b (Rect (Point (-1, -1), Point (0, 0))), sb = b;
  Region c (Rect (Point (-1, -1), Point (0, 0))), sc = c;
  b.intersect (a);
  TASSERT (b == c);
  TASSERT (a != c);
  TASSERT (b == sb);
  TASSERT (a == sa);
  a.intersect (b);
  TASSERT (a != sa);
  TASSERT (a == c);
  a = sa;
  TASSERT (a == sa);
  TASSERT (a.contains (c.extents()) == true);
  a.subtract (b);
  TASSERT (a != sa);
  TASSERT (a.contains (c.extents()) == false);
  a.add (b);
  TASSERT (a.contains (c.extents()) == true);
  TASSERT (a == sa);
  a.add (b);
  TASSERT (a == sa);
  a.intersect (a);
  TASSERT (a == sa);
  a.exor (a);
  TASSERT (a.empty());
  a.exor (b);
  TASSERT (a == b);
  b.exor (a);
  TASSERT (b.empty() && b != sb);
  b.exor (a);
  TASSERT (b == sb);
  TDONE();
}

static Region
create_rand_region (void)
{
  Region r;
  int u = 1 + (abs (lrand48()) % 157);
  if (u >= 50 && u <= 99)
    return Region (Point (70, 123455), Point (12345, 1987)); // need equal regions with some probability
  for (int i = 0; i < u; i++)
    r.add (Rect (Point (lrand48() * 0.005 + (((int64) lrand48()) << 16), lrand48() * 0.005 + (((int64) lrand48()) << 16)),
                 Point (lrand48() * 0.005 + (((int64) lrand48()) << 16), lrand48() * 0.005 + (((int64) lrand48()) << 16))));
  return r;
}

static void
test_region_fract (void)
{
  TSTART ("fractional regions");
  Region r;
  TASSERT (r.empty());
  const double epsilon = r.epsilon();   /* allow errors within +-epsilon */
  TASSERT (epsilon <= 0.5);             /* assert at least pxiel resolution */
  r.clear();
  TASSERT (r.empty());
  TASSERT (r.contains (Point (0, 0)) == false);
  r.add (Rect (Point (0.1, 0.2), Point (3.4, 4.5)));
  TASSERT (!r.empty());
  TASSERT (r.contains (2, 2));
  TASSERT (r.contains (0, 0) == false);
  TASSERT (r.contains (1, 1));
  TASSERT (r.contains (3, 3));
  TASSERT (r.contains (3.3, 3));
  TASSERT (r.contains (3.5, 3) == false);
  TASSERT (r.contains (3.3, 4.4));
  TASSERT (r.contains (3.3, 4.6) == false);
  TASSERT (r.contains (6, 6) == false);
  TASSERT (r.contains (Rect (1, 1, 2, 2)) == r.INSIDE);
  TASSERT (r.contains (Rect (0, 0, 3, 3)) == r.PARTIAL);
  TASSERT (r.contains (Rect (3.5, 0, 70, 70)) == r.OUTSIDE);
  Region r2 = r;
  TASSERT (r2.extents().equals (Rect (0.1, 0.2, 3.3, 4.3), epsilon));
  r2.add (Rect (0, 0, 0.1, 4.5));
  r2.add (Rect (0, 0, 3.4, 0.2));
  Region r3 (Rect (0, 0, 3.4, 4.5));
  TASSERT (r2 == r3);
  std::vector<Rect> rects;
  r3.list_rects (rects);
  TASSERT (rects.size() == 1);
  TASSERT (rects[0].equals (Rect (0, 0, 3.4, 4.5), epsilon));
  TDONE();
  if (0)
    printf ("\nREGION:\n%s\nEMPTY:\n%s\n", create_rand_region().string().c_str(), Region().string().c_str());
}

static void
test_region_cmp ()
{
  TSTART ("random-cmp");
  int cases = 0;
  for (uint i = 0; i < 65 || cases != 7; i++)
    {
      Region r1 = create_rand_region();
      TCHECK (r1.contains (r1) == r1.INSIDE);
      TCHECK (r1.contains (Region (r1)) == r1.INSIDE);
      Region r2 = create_rand_region();
      TCHECK (r2.contains (r2) == r2.INSIDE);
      TCHECK (r2.contains (Region (r2)) == r2.INSIDE);
      int c1 = r1.cmp (r2);
      int c2 = r2.cmp (r1);
      TCHECK (c1 == -c2);
      if (c1)
        {
          TCHECK (r1 != r2);
          TCHECK (r2 != r1);
        }
      else
        {
          TCHECK (r1 == r2);
          TCHECK (r2 == r1);
        }
      if (c1 < 0)
        {
          TCHECK (r1 < r2);
          TCHECK (r1.equal (r2) == false);
          TCHECK ((r2 < r1) == false);
          TCHECK (r2.equal (r1) == false);
          cases |= 1;
        }
      else if (c1 == 0)
        {
          TCHECK ((r1 < r2) == false);
          TCHECK (r1.equal (r2) == true);
          TCHECK ((r2 < r1) == false);
          TCHECK (r2.equal (r1) == true);
          cases |= 2;
        }
      else if (c1 > 0)
        {
          TCHECK (r2 < r1);
          TCHECK (r2.equal (r1) == false);
          TCHECK ((r1 < r2) == false);
          TCHECK (r1.equal (r2) == false);
          cases |= 4;
        }
      Region c;
      TCHECK (r1.contains (c)); // empty containment
      TCHECK (r2.contains (c)); // empty containment
      c.add (r1);
      c.add (r2);
      TCHECK (c.contains (r1));
      TCHECK (c.contains (r2));
      c.subtract (r1);
      c.subtract (r2);
      if (!r1.empty())
        TCHECK (c.contains (r1) == false);
      if (!r2.empty())
        TCHECK (c.contains (r2) == false);
      TASSERT (c.empty());
    }
  TDONE();
}

extern "C" int
main (int   argc,
      char *argv[])
{
  rapicorn_init_test (&argc, &argv);

  /* initialize rapicorn */
  rapicorn_init_with_gtk_thread (&argc, &argv, NULL); // FIXME: should work without Gtk+

  /* run tests */
  test_rect();
  test_region_basics();
  test_region_rect1();
  test_region2();
  test_region2_ops();
  test_region_fract();
  test_region_cmp();

  return 0;
}

} // Anon
