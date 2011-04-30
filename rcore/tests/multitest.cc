/* Rapicorn
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
#include <rcore/testutils.hh>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <poll.h>
#include <algorithm>
using namespace Rapicorn;

#if RAPICORN_CHECK_VERSION (2147483647, 2147483647, 2147483647) || !RAPICORN_CHECK_VERSION (0, 0, 0)
#error RAPICORN_CHECK_VERSION() implementation is broken
#endif

static void
test_misc (void)
{
  const char *lstr = RAPICORN_CPP_STRINGIFY (__LINE__);
  assert (lstr);
  assert (lstr[0] >= '0' && lstr[0] <= '9');
  assert (lstr[1] >= '0' && lstr[1] <= '9');
}

static void
test_cpu_info (void)
{
  const RapicornCPUInfo cpi = cpu_info ();
  TASSERT (cpi.machine != NULL);
  String cps = cpu_info_string (cpi);
  TASSERT (cps.size() != 0);
  if (Test::verbose())
    printout ("\n#####\n%s#####\n", cps.c_str());
}

static void
test_poll_consts()
{
  TASSERT (RAPICORN_SYSVAL_POLLIN     == POLLIN);
  TASSERT (RAPICORN_SYSVAL_POLLPRI    == POLLPRI);
  TASSERT (RAPICORN_SYSVAL_POLLOUT    == POLLOUT);
  TASSERT (RAPICORN_SYSVAL_POLLRDNORM == POLLRDNORM);
  TASSERT (RAPICORN_SYSVAL_POLLRDBAND == POLLRDBAND);
  TASSERT (RAPICORN_SYSVAL_POLLWRNORM == POLLWRNORM);
  TASSERT (RAPICORN_SYSVAL_POLLWRBAND == POLLWRBAND);
  TASSERT (RAPICORN_SYSVAL_POLLERR    == POLLERR);
  TASSERT (RAPICORN_SYSVAL_POLLHUP    == POLLHUP);
  TASSERT (RAPICORN_SYSVAL_POLLNVAL   == POLLNVAL);
}

static void
test_regex (void)
{
  TASSERT (Regex::match_simple ("Lion", "Lion", Regex::EXTENDED | Regex::ANCHORED, Regex::MATCH_NORMAL) == true);
  TASSERT (Regex::match_simple ("Ok", "<TEXT>Close</TEXT>", Regex::COMPILE_NORMAL, Regex::MATCH_NORMAL) == false);
  TASSERT (Regex::match_simple ("\\bOk", "<TEXT>Ok</TEXT>", Regex::COMPILE_NORMAL, Regex::MATCH_NORMAL) == true);
}

static void
test_paths()
{
  String p, s;
  s = Path::join ("0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "a", "b", "c", "d", "e", "f");
#if RAPICORN_DIR_SEPARATOR == '/'
  p = "0/1/2/3/4/5/6/7/8/9/a/b/c/d/e/f";
#else
  p = "0\\1\\2\\3\\4\\5\\6\\7\\8\\9\\a\\b\\c\\d\\e\\f";
#endif
  // printerr ("%s == %s\n", s.c_str(), p.c_str());
  TASSERT (s == p);
  bool b = Path::isabs (p);
  TASSERT (b == false);
#if RAPICORN_DIR_SEPARATOR == '/'
  s = Path::join (RAPICORN_DIR_SEPARATOR_S, s);
#else
  s = Path::join ("C:\\", s);
#endif
  b = Path::isabs (s);
  TASSERT (b == true);
  s = Path::skip_root (s);
  TASSERT (s == p);
  TASSERT (Path::dir_separator == "/" || Path::dir_separator == "\\");
  TASSERT (Path::searchpath_separator == ":" || Path::searchpath_separator == ";");
  TASSERT (Path::basename ("simple") == "simple");
  TASSERT (Path::basename ("skipthis" RAPICORN_DIR_SEPARATOR_S "file") == "file");
  TASSERT (Path::basename (RAPICORN_DIR_SEPARATOR_S "skipthis" RAPICORN_DIR_SEPARATOR_S "file") == "file");
  TASSERT (Path::dirname ("file") == ".");
  TASSERT (Path::dirname ("dir" RAPICORN_DIR_SEPARATOR_S) == "dir");
  TASSERT (Path::dirname ("dir" RAPICORN_DIR_SEPARATOR_S "file") == "dir");
  TASSERT (Path::cwd() != "");
  TASSERT (Path::check (Path::join (Path::cwd(), "..", "tests"), "rd") == true); // ./../tests/ should be a readable directory
  TASSERT (Path::isdirname ("") == false);
  TASSERT (Path::isdirname ("foo") == false);
  TASSERT (Path::isdirname ("foo/") == true);
  TASSERT (Path::isdirname ("/foo") == false);
  TASSERT (Path::isdirname ("foo/.") == true);
  TASSERT (Path::isdirname ("foo/..") == true);
  TASSERT (Path::isdirname ("foo/...") == false);
  TASSERT (Path::isdirname ("foo/..../") == true);
  TASSERT (Path::isdirname ("/.") == true);
  TASSERT (Path::isdirname ("/..") == true);
  TASSERT (Path::isdirname ("/") == true);
  TASSERT (Path::isdirname (".") == true);
  TASSERT (Path::isdirname ("..") == true);
}

static void
test_file_io()
{
  String fname = string_printf ("xtmp-infotest.%u", getpid());
  FILE *f = fopen (fname.c_str(), "w");
  assert (f != NULL);
  static const char *data = "1234567890\nasdfghjkl\n";
  int i = fwrite (data, 1, strlen (data), f);
  assert (i == (int) strlen (data));
  i = fclose (f);
  assert (i == 0);
  size_t l;
  char *mem = Path::memread (fname, &l);
  assert (mem != NULL);
  assert (l == strlen (data));
  assert (strncmp (mem, data, l) == 0);
  Path::memfree (mem);
  unlink (fname.c_str());
  l = 17;
  mem = Path::memread ("///", &l);
  assert (mem == NULL);
  assert (l == 0);
}

static void
test_zintern()
{
  static const unsigned char TEST_DATA[] = "x\332K\312,\312K-\321\255\312\314+I-\312S(I-.QHI,I\4\0v\317\11V";
  uint8 *data = zintern_decompress (24, TEST_DATA, sizeof (TEST_DATA) / sizeof (TEST_DATA[0]));
  TASSERT (String ((char*) data) == "birnet-zintern test data");
  zintern_free (data);
}

static void
test_files (char *argv0)
{
  TASSERT (Path::equals ("/bin", "/../bin") == TRUE);
  TASSERT (Path::equals ("/bin", "/sbin") == FALSE);
  TASSERT (Path::check (argv0, "e") == TRUE);
  TASSERT (Path::check (argv0, "r") == TRUE);
  TASSERT (Path::check (argv0, "w") == TRUE);
  TASSERT (Path::check (argv0, "x") == TRUE);
  TASSERT (Path::check (argv0, "d") == FALSE);
  TASSERT (Path::check (argv0, "l") == FALSE);
  TASSERT (Path::check (argv0, "c") == FALSE);
  TASSERT (Path::check (argv0, "b") == FALSE);
  TASSERT (Path::check (argv0, "p") == FALSE);
  TASSERT (Path::check (argv0, "s") == FALSE);
}

static void
test_virtual_typeid()
{
  struct TypeA : public virtual VirtualTypeid {};
  struct TypeB : public virtual VirtualTypeid {};
  TypeA a;
  TypeB b;
  TASSERT (a.typeid_name() != b.typeid_name());
  TASSERT (strstr (a.typeid_pretty_name().c_str(), "TypeA") != NULL);
  TASSERT (strstr (b.typeid_pretty_name().c_str(), "TypeB") != NULL);
}

struct SomeObject : public BaseObject {};

static void
test_id_allocator ()
{
  IdAllocator &ida = *IdAllocator::_new (77);
  assert (&ida);
  const uint ulength = 11; // minimum number of unique ids expected
  uint buffer[11];
  /* initial set of unique ids */
  for (uint i = 0; i < ulength; i++)
    {
      buffer[i] = ida.alloc_id();
      if (i)
        assert (buffer[i] != buffer[i - 1]);
    }
  for (uint i = 0; i < ulength; i++)
    ida.release_id (buffer[i]);
  /* check infrequent reoccourance */
  uint j = 0;
  for (; j < ulength; j++)
    {
      uint id = ida.alloc_id();
      assert (true == ida.seen_id (id));
      for (uint i = 0; i < ulength; i++)
        assert (buffer[i] != id);
    }
  /* large scale id allocations and releases */
  const uint big = 99999;
  uint *b = new uint[big];
  for (j = 0; j < big; j++)
    {
      b[j] = ida.alloc_id();
      if (j)
        assert (b[j] != b[j - 1]);
    }
  for (j = 0; j < big; j++)
    ida.release_id (b[j]);
  for (j = 0; j < big / 2 + big / 4; j++)
    {
      b[j] = ida.alloc_id();
      if (j)
        assert (b[j] != b[j - 1]);
    }
  for (j = 0; j < big / 2 + big / 4; j++)
    ida.release_id (b[j]);
  for (j = 0; j < big; j++)
    {
      b[j] = ida.alloc_id();
      if (j)
        assert (b[j] != b[j - 1]);
    }
  for (j = 0; j < big; j++)
    ida.release_id (b[j]);
  /* cleanups */
  delete[] b;
  delete &ida;
}

static void
test_locatable_ids ()
{
  SomeObject *o1 = new SomeObject();
  SomeObject *o2 = new SomeObject();
  SomeObject *o3 = new SomeObject();
  uint64 id1 = o1->locatable_id();
  uint64 id2 = o2->locatable_id();
  uint64 id3 = o3->locatable_id();
  assert (id1 != id2);
  assert (id2 != id3);
  assert (id1 != id3);
  Locatable *l0 = Locatable::from_locatable_id (0x00ff00ff00ff00ffULL);
  assert (l0 == NULL);
  Locatable *l1 = Locatable::from_locatable_id (id1);
  Locatable *l2 = Locatable::from_locatable_id (id2);
  Locatable *l3 = Locatable::from_locatable_id (id3);
  assert (l1 == o1);
  assert (l2 == o2);
  assert (l3 == o3);
  assert (id1 == l1->locatable_id());
  assert (id2 == l2->locatable_id());
  assert (id3 == l3->locatable_id());
  unref (ref_sink (o1));
  unref (ref_sink (o2));
  unref (ref_sink (o3));
  l1 = Locatable::from_locatable_id (id1);
  l2 = Locatable::from_locatable_id (id2);
  l3 = Locatable::from_locatable_id (id3);
  assert (l1 == NULL);
  assert (l2 == NULL);
  assert (l3 == NULL);
  o1 = new SomeObject();
  assert (o1->locatable_id() != id1);
  assert (o1->locatable_id() != id2);
  assert (o1->locatable_id() != id3);
  o2 = new SomeObject();
  assert (o2->locatable_id() != id1);
  assert (o2->locatable_id() != id2);
  assert (o2->locatable_id() != id3);
  o3 = new SomeObject();
  assert (o3->locatable_id() != id1);
  assert (o3->locatable_id() != id2);
  assert (o3->locatable_id() != id3);
  assert (o1 == Locatable::from_locatable_id (o1->locatable_id()));
  assert (o2 == Locatable::from_locatable_id (o2->locatable_id()));
  assert (o3 == Locatable::from_locatable_id (o3->locatable_id()));
  unref (ref_sink (o1));
  unref (ref_sink (o2));
  unref (ref_sink (o3));
  const uint big = 999;
  SomeObject *buffer[big];
  for (uint j = 0; j < big; j++)
    buffer[j] = ref_sink (new SomeObject());
  for (uint j = 0; j < big; j++)
    assert (buffer[j] == Locatable::from_locatable_id (buffer[j]->locatable_id()));
  for (uint j = 0; j < big; j++)
    unref (buffer[j]);
}

static void
test_dtoi32()
{
  TASSERT (_dtoi32_generic (0.0) == 0);
  TASSERT (_dtoi32_generic (+0.3) == +0);
  TASSERT (_dtoi32_generic (-0.3) == -0);
  TASSERT (_dtoi32_generic (+0.7) == +1);
  TASSERT (_dtoi32_generic (-0.7) == -1);
  TASSERT (_dtoi32_generic (+2147483646.3) == +2147483646);
  TASSERT (_dtoi32_generic (+2147483646.7) == +2147483647);
  TASSERT (_dtoi32_generic (-2147483646.3) == -2147483646);
  TASSERT (_dtoi32_generic (-2147483646.7) == -2147483647);
  TASSERT (_dtoi32_generic (-2147483647.3) == -2147483647);
  TASSERT (_dtoi32_generic (-2147483647.7) == -2147483648LL);
  TASSERT (dtoi32 (0.0) == 0);
  TASSERT (dtoi32 (+0.3) == +0);
  TASSERT (dtoi32 (-0.3) == -0);
  TASSERT (dtoi32 (+0.7) == +1);
  TASSERT (dtoi32 (-0.7) == -1);
  TASSERT (dtoi32 (+2147483646.3) == +2147483646);
  TASSERT (dtoi32 (+2147483646.7) == +2147483647);
  TASSERT (dtoi32 (-2147483646.3) == -2147483646);
  TASSERT (dtoi32 (-2147483646.7) == -2147483647);
  TASSERT (dtoi32 (-2147483647.3) == -2147483647);
  TASSERT (dtoi32 (-2147483647.7) == -2147483648LL);
}

static void
test_dtoi64()
{
  TASSERT (_dtoi64_generic (0.0) == 0);
  TASSERT (_dtoi64_generic (+0.3) == +0);
  TASSERT (_dtoi64_generic (-0.3) == -0);
  TASSERT (_dtoi64_generic (+0.7) == +1);
  TASSERT (_dtoi64_generic (-0.7) == -1);
  TASSERT (_dtoi64_generic (+2147483646.3) == +2147483646);
  TASSERT (_dtoi64_generic (+2147483646.7) == +2147483647);
  TASSERT (_dtoi64_generic (-2147483646.3) == -2147483646);
  TASSERT (_dtoi64_generic (-2147483646.7) == -2147483647);
  TASSERT (_dtoi64_generic (-2147483647.3) == -2147483647);
  TASSERT (_dtoi64_generic (-2147483647.7) == -2147483648LL);
  TASSERT (_dtoi64_generic (+4294967297.3) == +4294967297LL);
  TASSERT (_dtoi64_generic (+4294967297.7) == +4294967298LL);
  TASSERT (_dtoi64_generic (-4294967297.3) == -4294967297LL);
  TASSERT (_dtoi64_generic (-4294967297.7) == -4294967298LL);
  TASSERT (_dtoi64_generic (+1125899906842624.3) == +1125899906842624LL);
  TASSERT (_dtoi64_generic (+1125899906842624.7) == +1125899906842625LL);
  TASSERT (_dtoi64_generic (-1125899906842624.3) == -1125899906842624LL);
  TASSERT (_dtoi64_generic (-1125899906842624.7) == -1125899906842625LL);
  TASSERT (dtoi64 (0.0) == 0);
  TASSERT (dtoi64 (+0.3) == +0);
  TASSERT (dtoi64 (-0.3) == -0);
  TASSERT (dtoi64 (+0.7) == +1);
  TASSERT (dtoi64 (-0.7) == -1);
  TASSERT (dtoi64 (+2147483646.3) == +2147483646);
  TASSERT (dtoi64 (+2147483646.7) == +2147483647);
  TASSERT (dtoi64 (-2147483646.3) == -2147483646);
  TASSERT (dtoi64 (-2147483646.7) == -2147483647);
  TASSERT (dtoi64 (-2147483647.3) == -2147483647);
  TASSERT (dtoi64 (-2147483647.7) == -2147483648LL);
  TASSERT (dtoi64 (+4294967297.3) == +4294967297LL);
  TASSERT (dtoi64 (+4294967297.7) == +4294967298LL);
  TASSERT (dtoi64 (-4294967297.3) == -4294967297LL);
  TASSERT (dtoi64 (-4294967297.7) == -4294967298LL);
  TASSERT (dtoi64 (+1125899906842624.3) == +1125899906842624LL);
  TASSERT (dtoi64 (+1125899906842624.7) == +1125899906842625LL);
  TASSERT (dtoi64 (-1125899906842624.3) == -1125899906842624LL);
  TASSERT (dtoi64 (-1125899906842624.7) == -1125899906842625LL);
}

static void
test_iround()
{
  TASSERT (round (0.0) == 0.0);
  TASSERT (round (+0.3) == +0.0);
  TASSERT (round (-0.3) == -0.0);
  TASSERT (round (+0.7) == +1.0);
  TASSERT (round (-0.7) == -1.0);
  TASSERT (round (+4294967297.3) == +4294967297.0);
  TASSERT (round (+4294967297.7) == +4294967298.0);
  TASSERT (round (-4294967297.3) == -4294967297.0);
  TASSERT (round (-4294967297.7) == -4294967298.0);
  TASSERT (iround (0.0) == 0);
  TASSERT (iround (+0.3) == +0);
  TASSERT (iround (-0.3) == -0);
  TASSERT (iround (+0.7) == +1);
  TASSERT (iround (-0.7) == -1);
  TASSERT (iround (+4294967297.3) == +4294967297LL);
  TASSERT (iround (+4294967297.7) == +4294967298LL);
  TASSERT (iround (-4294967297.3) == -4294967297LL);
  TASSERT (iround (-4294967297.7) == -4294967298LL);
  TASSERT (iround (+1125899906842624.3) == +1125899906842624LL);
  TASSERT (iround (+1125899906842624.7) == +1125899906842625LL);
  TASSERT (iround (-1125899906842624.3) == -1125899906842624LL);
  TASSERT (iround (-1125899906842624.7) == -1125899906842625LL);
}

static void
test_iceil()
{
  TASSERT (ceil (0.0) == 0.0);
  TASSERT (ceil (+0.3) == +1.0);
  TASSERT (ceil (-0.3) == -0.0);
  TASSERT (ceil (+0.7) == +1.0);
  TASSERT (ceil (-0.7) == -0.0);
  TASSERT (ceil (+4294967297.3) == +4294967298.0);
  TASSERT (ceil (+4294967297.7) == +4294967298.0);
  TASSERT (ceil (-4294967297.3) == -4294967297.0);
  TASSERT (ceil (-4294967297.7) == -4294967297.0);
  TASSERT (iceil (0.0) == 0);
  TASSERT (iceil (+0.3) == +1);
  TASSERT (iceil (-0.3) == -0);
  TASSERT (iceil (+0.7) == +1);
  TASSERT (iceil (-0.7) == -0);
  TASSERT (iceil (+4294967297.3) == +4294967298LL);
  TASSERT (iceil (+4294967297.7) == +4294967298LL);
  TASSERT (iceil (-4294967297.3) == -4294967297LL);
  TASSERT (iceil (-4294967297.7) == -4294967297LL);
  TASSERT (iceil (+1125899906842624.3) == +1125899906842625LL);
  TASSERT (iceil (+1125899906842624.7) == +1125899906842625LL);
  TASSERT (iceil (-1125899906842624.3) == -1125899906842624LL);
  TASSERT (iceil (-1125899906842624.7) == -1125899906842624LL);
}

static void
test_ifloor()
{
  TASSERT (floor (0.0) == 0.0);
  TASSERT (floor (+0.3) == +0.0);
  TASSERT (floor (-0.3) == -1.0);
  TASSERT (floor (+0.7) == +0.0);
  TASSERT (floor (-0.7) == -1.0);
  TASSERT (floor (+4294967297.3) == +4294967297.0);
  TASSERT (floor (+4294967297.7) == +4294967297.0);
  TASSERT (floor (-4294967297.3) == -4294967298.0);
  TASSERT (floor (-4294967297.7) == -4294967298.0);
  TASSERT (ifloor (0.0) == 0);
  TASSERT (ifloor (+0.3) == +0);
  TASSERT (ifloor (-0.3) == -1);
  TASSERT (ifloor (+0.7) == +0);
  TASSERT (ifloor (-0.7) == -1);
  TASSERT (ifloor (+4294967297.3) == +4294967297LL);
  TASSERT (ifloor (+4294967297.7) == +4294967297LL);
  TASSERT (ifloor (-4294967297.3) == -4294967298LL);
  TASSERT (ifloor (-4294967297.7) == -4294967298LL);
  TASSERT (ifloor (+1125899906842624.3) == +1125899906842624LL);
  TASSERT (ifloor (+1125899906842624.7) == +1125899906842624LL);
  TASSERT (ifloor (-1125899906842624.3) == -1125899906842625LL);
  TASSERT (ifloor (-1125899906842624.7) == -1125899906842625LL);
}

static int
compare_floats (float f1,
                float f2)
{
  return f1 < f2 ? -1 : f1 > f2;
}

static int
smaller_float (float f1,
               float f2)
{
  return f1 < f2;
}

static void
binary_lookup_tests()
{
  bool seen_inexact;
  vector<float> fv;
  vector<float>::iterator fit;
  std::pair<vector<float>::iterator,bool> pit;
  const uint count = 150000;
  TSTART ("Corner case lookups");
  fv.resize (count + (rand() % 10000));
  if (fv.size() % 2)
    TOK();
  else
    TOK();
  for (uint i = 0; i < fv.size(); i++)
    {
      fv[i] = rand();
      if (i % 100000 == 99999)
        TOK();
    }
  TOK();
  vector<float> sv = fv;
  stable_sort (sv.begin(), sv.end(), smaller_float);
  TOK();
  /* failed lookups */
  fit = binary_lookup (sv.begin(), sv.end(), compare_floats, -INFINITY);
  TASSERT (fit == sv.end());
  fit = binary_lookup_sibling (sv.begin(), sv.end(), compare_floats, -INFINITY);
  TASSERT (fit != sv.end());
  /* 0-size lookups */
  vector<float> ev;
  fit = binary_lookup (ev.begin(), ev.end(), compare_floats, 0);
  TASSERT (fit == ev.end());
  fit = binary_lookup_sibling (ev.begin(), ev.end(), compare_floats, 0);
  TASSERT (fit == ev.end());
  pit = binary_lookup_insertion_pos (ev.begin(), ev.end(), compare_floats, 0);
  TASSERT (pit.first == ev.end() && pit.second == false);
  TDONE();
  TSTART ("Binary lookup");
  for (uint i = 0; i < fv.size(); i++)
    {
      fit = binary_lookup (sv.begin(), sv.end(), compare_floats, fv[i]);
      TCHECK (fit != sv.end());
      TCHECK (fv[i] == *fit);           /* silent assertion */
      if (i % 10000 == 9999)
        TASSERT (fv[i] == *fit);        /* verbose assertion */
    }
  TDONE();
  TSTART ("Sibling lookup");
  for (uint i = 1; i < sv.size(); i++)
    {
      double target = (sv[i - 1] + sv[i]) / 2.0;
      fit = binary_lookup_sibling (sv.begin(), sv.end(), compare_floats, target);
      TCHECK (fit != sv.end());
      TCHECK (sv[i] == *fit || sv[i - 1] == *fit);
      if (i % 10000 == 9999)
        TOK();
    }
  TDONE();
  TSTART ("Insertion lookup1");
  seen_inexact = false;
  for (uint i = 0; i < fv.size(); i++)
    {
      pit = binary_lookup_insertion_pos (sv.begin(), sv.end(), compare_floats, fv[i]);
      fit = pit.first;
      seen_inexact |= pit.second == false;
      TCHECK (fit != sv.end());
      TCHECK (fv[i] == *fit);
      if (i % 10000 == 9999)
        TOK();
    }
  TASSERT (seen_inexact == false);
  TDONE();
  TSTART ("Insertion lookup2");
  seen_inexact = false;
  for (uint i = 1; i < sv.size(); i++)
    {
      double target = (sv[i - 1] + sv[i]) / 2.0;
      pit = binary_lookup_insertion_pos (sv.begin(), sv.end(), compare_floats, target);
      fit = pit.first;
      seen_inexact |= pit.second == false;
      TCHECK (fit != sv.end());
      TCHECK (sv[i] == *fit || sv[i - 1] == *fit);
      if (i % 10000 == 9999)
        TOK();
    }
  TASSERT (seen_inexact == true);
  TDONE();
}


int
main (int   argc,
      char *argv[])
{
  if (argc >= 2 && String ("--print-process-handle") == argv[1])
    {
      rapicorn_init_core (&argc, &argv, __FILE__);
      printout ("%s", process_handle().c_str());
      return 0;
    }

  if (argc >= 2 && String ("--print-locatable-id") == argv[1])
    {
      rapicorn_init_core (&argc, &argv, __FILE__);
      SomeObject *obj = new SomeObject();
      printout ("0x%016llx\n", obj->locatable_id());
      return 0;
    }

  rapicorn_init_test (&argc, argv);

  TRUN ("Misc", test_misc);
  TRUN ("CpuInfo", test_cpu_info);
  TRUN ("Poll constants", test_poll_consts);
  TRUN ("Regex Tests", test_regex);
  TRUN ("Path handling", test_paths);
  TRUN ("File IO", test_file_io);
  TRUN ("ZIntern", test_zintern);
  TRUN3 ("FileChecks", test_files, argv[0]);
  TRUN ("VirtualTypeid", test_virtual_typeid);
  TRUN ("Id Allocator", test_id_allocator);
  TRUN ("Locatable IDs", test_locatable_ids);
  TRUN ("Math/dtoi32", test_dtoi32);
  TRUN ("Math/dtoi64", test_dtoi64);
  TRUN ("Math/iceil", test_iceil);
  TRUN ("Math/ifloor", test_ifloor);
  TRUN ("Math/iround", test_iround);

  binary_lookup_tests();

  return 0;
}

/* vim:set ts=8 sts=2 sw=2: */
