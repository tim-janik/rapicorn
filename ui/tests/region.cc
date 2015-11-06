/* This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0 */
#include <rcore/testutils.hh>
#include <ui/uithread.hh>
#include <stdio.h>
#include <stdlib.h>     // lrand48

namespace {
using namespace Rapicorn;

static void
test_rect (void)
{
  Rect r1;
  TASSERT (r1.empty());
  r1.assign (Point (7.1, 7.2), Point (8.4, 8.5));       // stores width=1.3, height=1.3 in double precision
  TASSERT (!r1.empty());
  Rect r2 (7.1, 7.2, 1.3, 1.3);
  TCMP (r2, ==, Rect (7.1, 7.2, 1.3, 1.3));
  TASSERT (false == (r2 != Rect (7.1, 7.2, 1.3, 1.3)));
  Rect r3 (7.1, 7.2, 1.0, 1.0);
  TCMP (r3.equals (r2, 0.1), ==, false);
  TCMP (r3.equals (r2, 0.5), ==, true);
  TASSERT (r2.equals (r1, 0.00000000000001));           // using epsilon due to precision artefacts
  r3.translate (-2.5, +1.6);
  Rect r4 (4.6, 8.8, 1.0, 1.0);
  TASSERT (r3.equals (r4, 0.00000000000001));           // using epsilon due to precision artefacts
  r1 = Rect (-1, -1, 0, 0);
  r2 = Rect (-1, -1, -1, -1);                           // width and height are MAXed up to 0
  r3 = Rect (0, 0, 0, 0);
  r4 = Rect (1, 1, 1, 1);
  TASSERT (r1 == r2);                                   // both empty at (-1,-1)
  TASSERT (r1 != r3);                                   // both empty, but at diferent coords
  TASSERT (r1 != r4);                                   // empty vs. non-empty
  TASSERT (r2 != r3);                                   // both empty, but at diferent coords
  TASSERT (r2 != r4);                                   // empty vs. non-empty
  TASSERT (r3 != r4);                                   // empty vs. non-empty
}
REGISTER_UITHREAD_TEST ("Region/rectangles", test_rect);

static void
test_region_basics (void)
{
  Region r, z;
  TASSERT (r.empty());          TASSERT (r.empty() == !r.count_rects());
  TASSERT (z.empty());          TASSERT (z.empty() == !z.count_rects());
  TASSERT (r.equal (r));
  TASSERT (r.equal (z));
  r.add (Rect (Point (-1, -1), Point (1, 1)));
  TASSERT (!r.empty());         TASSERT (r.empty() == !r.count_rects());
  TASSERT (z.empty());          TASSERT (z.empty() == !z.count_rects());
  TASSERT (!r.equal (z));
  r.swap (z);
  TASSERT (r.empty());          TASSERT (r.empty() == !r.count_rects());
  TASSERT (!z.empty());         TASSERT (z.empty() == !z.count_rects());
  TASSERT (!r.equal (z));
  z.clear();
  TASSERT (r.empty());          TASSERT (r.empty() == !r.count_rects());
  TASSERT (z.empty());          TASSERT (z.empty() == !z.count_rects());
  TASSERT (r.equal (z));
}
REGISTER_UITHREAD_TEST ("Region/region basics", test_region_basics);

static void
test_region_rect1 (void)
{
  Region r;
  r.clear();
  TASSERT (r.empty());          TASSERT (r.empty() == !r.count_rects());
  TCMP (r.contains (Point (0, 0)), ==, false);
  r.add (Rect (Point (3, 3), Point (5, 5)));
  TASSERT (!r.empty());         TASSERT (r.empty() == !r.count_rects());
  TCMP (r.contains (2, 2), ==, false);
  TASSERT (r.contains (3, 3));
  TASSERT (r.contains (4, 4));
  TCMP (r.contains (5, 5), ==, false);
  TCMP (r.contains (6, 6), ==, false);
  TCMP (r.contains (Rect (Point (3, 3), Point (5, 5))), ==, r.INSIDE);
  TCMP (r.contains (Rect (Point (4, 4), Point (6, 6))), ==, r.PARTIAL);
  TCMP (r.contains (Rect (Point (5, 5), Point (6, 6))), ==, r.OUTSIDE);
  vector<Rect> rects;
  r.list_rects (rects);
  TCMP (rects.size(), ==, 1);
  TCMP (rects[0].lower_left(), ==, Point (3, 3));
  TCMP (rects[0].upper_right(), ==, Point (5, 5));
  Region z (r);
  TASSERT (r.equal (z));
  r.exor (r);
  TASSERT (r.empty());          TASSERT (r.empty() == !r.count_rects());
  TASSERT (!r.equal (z));
  z.clear();
  TASSERT (r.equal (z));
  TCMP (r.extents(), ==, Rect (Point (0, 0), Point (0, 0)));
  const Region e;
  TCMP (e.contains (Point (0, 0)), ==, false);
  TCMP (e.contains (Rect (Point (0, 0), Point (0, 0))), ==, true);
  TCMP (e.contains (Rect (Point (0, 0), Point (1, 1))), ==, false);
  TCMP (e.contains (Region()), ==, true);
}
REGISTER_UITHREAD_TEST ("Region/region rec1t", test_region_rect1);

static void
test_region2 (void)
{
  Region a (Rect (Point (-1, -1), Point (1, 1)));
  Region b (Rect (Point (-1, -1), Point (0, 0)));
  TCMP (a.equal (b), ==, false);
  TASSERT ((a != b) == true);
  TASSERT ((a == b) == false);
  TCMP (a.cmp (b), !=, 0);
  if (a.cmp (b) < 0)
    TCMP (a, <, b);
  else
    TCMP (b, <, a);
  a.clear();
  b.clear();
  TASSERT (a.empty());          TASSERT (a.empty() == !a.count_rects());
  TASSERT (b.empty());          TASSERT (b.empty() == !b.count_rects());
  TCMP (a.equal (b), ==, true);
  TASSERT ((a != b) == false);
  TASSERT ((a == b) == true);
  TCMP (a.cmp (b), ==, 0);
  TCMP ((a < b), ==, false);
  TCMP ((b < a), ==, false);
}
REGISTER_UITHREAD_TEST ("Region/region2", test_region2);

static void
test_region2_ops (void)
{
  Region a (Rect (Point (-1, -1), Point (1, 1))), sa = a;
  Region b (Rect (Point (-1, -1), Point (0, 0))), sb = b;
  Region c (Rect (Point (-1, -1), Point (0, 0))), sc = c;
  b.intersect (a);
  TCMP (b, ==, c);
  TCMP (a, !=, c);
  TCMP (b, ==, sb);
  TCMP (a, ==, sa);
  a.intersect (b);
  TCMP (a, !=, sa);
  TCMP (a, ==, c);
  a = sa;
  TCMP (a, ==, sa);
  TCMP (a.contains (c.extents()), ==, true);
  a.subtract (b);
  TCMP (a, !=, sa);
  TCMP (a.contains (c.extents()), ==, false);
  a.add (b);
  TCMP (a.contains (c.extents()), ==, true);
  TCMP (a, ==, sa);
  a.add (b);
  TCMP (a, ==, sa);
  a.intersect (a);
  TCMP (a, ==, sa);
  a.exor (a);
  TASSERT (a.empty());          TASSERT (a.empty() == !a.count_rects());
  a.exor (b);
  TCMP (a, ==, b);
  b.exor (a);
  TASSERT (b.empty());          TASSERT (b.empty() == !b.count_rects());
  TCMP (b, !=, sb);
  b.exor (a);
  TCMP (b, ==, sb);
}
REGISTER_UITHREAD_TEST ("Region/region2 ops", test_region2_ops);

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
  Region r;
  TASSERT (r.empty());                  TASSERT (r.empty() == !r.count_rects());
  const double epsilon = r.epsilon();   // allow errors within +-epsilon
  TCMP (epsilon, <=, 0.5);              // assert at least pxiel resolution */
  r.clear();
  TASSERT (r.empty());                  TASSERT (r.empty() == !r.count_rects());
  TCMP (r.contains (Point (0, 0)), ==, false);
  r.add (Rect (Point (0.1, 0.2), Point (3.4, 4.5)));
  TASSERT (!r.empty());                 TASSERT (r.empty() == !r.count_rects());
  TASSERT (r.contains (2, 2));
  TCMP (r.contains (0, 0), ==, false);
  TASSERT (r.contains (1, 1));
  TASSERT (r.contains (3, 3));
  TASSERT (r.contains (3.3, 3));
  TCMP (r.contains (3.5, 3), ==, false);
  TASSERT (r.contains (3.3, 4.4));
  TCMP (r.contains (3.3, 4.6), ==, false);
  TCMP (r.contains (6, 6), ==, false);
  TCMP (r.contains (Rect (1, 1, 2, 2)), ==, r.INSIDE);
  TCMP (r.contains (Rect (0, 0, 3, 3)), ==, r.PARTIAL);
  TCMP (r.contains (Rect (3.5, 0, 70, 70)), ==, r.OUTSIDE);
  Region r2 = r;
  TASSERT (r2.extents().equals (Rect (0.1, 0.2, 3.3, 4.3), epsilon));
  r2.add (Rect (0, 0, 0.1, 4.5));
  r2.add (Rect (0, 0, 3.4, 0.2));
  Region r3 (Rect (0, 0, 3.4, 4.5));
  TCMP (r2, ==, r3);
  std::vector<Rect> rects;
  r3.list_rects (rects);
  TCMP (rects.size(), ==, 1);
  TASSERT (rects[0].equals (Rect (0, 0, 3.4, 4.5), epsilon));
  if (0)
    printf ("\nREGION:\n%s\nEMPTY:\n%s\n", create_rand_region().string().c_str(), Region().string().c_str());
}
REGISTER_UITHREAD_TEST ("Region/fractional regions", test_region_fract);

static void
test_region_cmp ()
{
  int cases = 0;
  for (uint i = 0; i < 65 || cases != 7; i++)
    {
      Region r1 = create_rand_region();
      TCMP (r1.contains (r1), ==, r1.INSIDE);
      TCMP (r1.contains (Region (r1)), ==, r1.INSIDE);
      Region r2 = create_rand_region();
      TCMP (r2.contains (r2), ==, r2.INSIDE);
      TCMP (r2.contains (Region (r2)), ==, r2.INSIDE);
      int c1 = r1.cmp (r2);
      int c2 = r2.cmp (r1);
      TCMP (c1, ==, -c2);
      if (c1)
        {
          TCMP (r1, !=, r2);
          TCMP (r2, !=, r1);
        }
      else
        {
          TCMP (r1, ==, r2);
          TCMP (r2, ==, r1);
        }
      if (c1 < 0)
        {
          TCMP (r1, <, r2);
          TCMP (r1.equal (r2), ==, false);
          TCMP ((r2 < r1), ==, false);
          TCMP (r2.equal (r1), ==, false);
          cases |= 1;
        }
      else if (c1 == 0)
        {
          TCMP ((r1 < r2), ==, false);
          TCMP (r1.equal (r2), ==, true);
          TCMP ((r2 < r1), ==, false);
          TCMP (r2.equal (r1), ==, true);
          cases |= 2;
        }
      else if (c1 > 0)
        {
          TCMP (r2, <, r1);
          TCMP (r2.equal (r1), ==, false);
          TCMP ((r1 < r2), ==, false);
          TCMP (r1.equal (r2), ==, false);
          cases |= 4;
        }
      Region c;
      TASSERT (r1.contains (c)); // empty containment
      TASSERT (r2.contains (c)); // empty containment
      c.add (r1);
      c.add (r2);
      TASSERT (c.contains (r1));
      TASSERT (c.contains (r2));
      c.subtract (r1);
      c.subtract (r2);
      if (!r1.empty())
        TCMP (c.contains (r1), ==, false);
      if (!r2.empty())
        TCMP (c.contains (r2), ==, false);
      TASSERT (c.empty());              TASSERT (c.empty() == !c.count_rects());
    }
}
REGISTER_UITHREAD_TEST ("Region/random-cmp", test_region_cmp);

} // Anon
