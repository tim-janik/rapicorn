// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#include <rcore/testutils.hh>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <poll.h>
#include <errno.h>
#include <algorithm>
#include <fcntl.h>
using namespace Rapicorn;

static void
test_failing ()
{
  // check STRINGIFY, needed for assertions
  const char *lstr = RAPICORN_CPP_STRINGIFY (__LINE__);
  if (!lstr || !(lstr[0] >= '0' && lstr[0] <= '9') || !(lstr[1] >= '0' && lstr[1] <= '9'))
    {
      errno = ENOSYS;
      perror ("Rapicorn: Failed to verify working STRINGIFY()");
      exit (127);
    }

  // check TASSERT() itself, so we can trust our other tests
  if (Test::trap_fork_silent())
    {
      TASSERT (0 == "TASSERT is working...");
      _exit (0);
    }
  if (!Test::trap_aborted() && !Test::trap_sigtrap())
    {
      errno = ENOSYS;
      perror ("Rapicorn: Failed to verify working TASSERT()");
      exit (127);
    }

  // also check test traps as such
  if (Test::trap_fork_silent())
    {
      printout ("1\n");
      printerr ("2\n");
      _exit (0);
    }
  TASSERT (Test::trap_aborted() == false);
  TASSERT (Test::trap_stdout() == "1\n");
  TASSERT (Test::trap_stderr() == "2\n");

  // fatal, basing checks on TASSERT
  if (Test::trap_fork_silent())
    {
      fatal ("BOOM!");
      _exit (0);
    }
  TASSERT (Test::trap_aborted() == true);
  TASSERT (Test::trap_stderr().find ("BOOM") != String::npos);

  // criticals
  if (Test::trap_fork_silent())
    {
      critical ("Bang!");
      _exit (0);
    }
  TASSERT (Test::trap_aborted() == true);
  TASSERT (Test::trap_stderr().find ("Bang") != String::npos);
  if (Test::trap_fork_silent())
    {
      critical_unless (42 == 3);
      _exit (0);
    }
  TASSERT (Test::trap_aborted() == true);
  TASSERT (Test::trap_stderr().find ("fail") != String::npos);
  TASSERT (Test::trap_stderr().find ("42") != String::npos);

  // assertions
  if (Test::trap_fork_silent())
    {
      assert (!"something");
      _exit (0);
    }
  TASSERT (Test::trap_aborted() == true);
  TASSERT (Test::trap_stderr().find ("fail") != String::npos);
  TASSERT (Test::trap_stderr().find ("something") != String::npos);

  if (Test::trap_fork_silent())
    {
      assert_return ("beep" == NULL);
      _exit (0);
    }
  TASSERT (Test::trap_aborted() == true);
  TASSERT (Test::trap_stderr().find ("fail") != String::npos);
  TASSERT (Test::trap_stderr().find ("beep") != String::npos);

  if (Test::trap_fork_silent())
    {
      assert_unreached();
      _exit (0);
    }
  TASSERT (Test::trap_aborted() == true);
  TASSERT (Test::trap_stderr().find ("fail") != String::npos);
  TASSERT (Test::trap_stderr().find ("reach") != String::npos);

  if (Test::trap_fork_silent())
    {
      fatal ("example allocation error");
      _exit (0);
    }
  TASSERT (Test::trap_aborted() == true);
  TASSERT (Test::trap_stderr().find ("alloc") != String::npos);
  TASSERT (Test::trap_stderr().find ("example") != String::npos);
}
REGISTER_TEST ("0-Testing/Traps & Failing Conditions", test_failing);

static void
test_cpu_info (void)
{
  const String cpi = cpu_info ();
  const size_t cpu_info_separator = cpi.find (' ');
  TASSERT (cpu_info_separator != cpi.npos && cpi.size() > 0 && cpu_info_separator > 0 && cpu_info_separator + 1 < cpi.size());
  if (Test::verbose())
    printout ("\nCPUID: %s\n", cpi.c_str());
}
REGISTER_TEST ("General/CpuInfo", test_cpu_info);

static void
test_poll_consts()
{
  TCMP (RAPICORN_SYSVAL_POLLIN,     ==, POLLIN);
  TCMP (RAPICORN_SYSVAL_POLLPRI,    ==, POLLPRI);
  TCMP (RAPICORN_SYSVAL_POLLOUT,    ==, POLLOUT);
  TCMP (RAPICORN_SYSVAL_POLLRDNORM, ==, POLLRDNORM);
  TCMP (RAPICORN_SYSVAL_POLLRDBAND, ==, POLLRDBAND);
  TCMP (RAPICORN_SYSVAL_POLLWRNORM, ==, POLLWRNORM);
  TCMP (RAPICORN_SYSVAL_POLLWRBAND, ==, POLLWRBAND);
  TCMP (RAPICORN_SYSVAL_POLLERR,    ==, POLLERR);
  TCMP (RAPICORN_SYSVAL_POLLHUP,    ==, POLLHUP);
  TCMP (RAPICORN_SYSVAL_POLLNVAL,   ==, POLLNVAL);
  RAPICORN_STATIC_ASSERT (RAPICORN_ABS (0) == 0);
  RAPICORN_STATIC_ASSERT (RAPICORN_ABS (8) == 8);
  RAPICORN_STATIC_ASSERT (RAPICORN_ABS (-8) == 8);
  RAPICORN_STATIC_ASSERT (RAPICORN_ABS (-2147483647) == +2147483647);
}
REGISTER_TEST ("General/Poll constants", test_poll_consts);

static void
test_regex (void)
{
  TCMP (Regex::match_simple ("Lion", "Lion", Regex::EXTENDED | Regex::ANCHORED, Regex::MATCH_NORMAL), ==, true);
  TCMP (Regex::match_simple ("Ok", "<TEXT>Close</TEXT>", Regex::COMPILE_NORMAL, Regex::MATCH_NORMAL), ==, false);
  TCMP (Regex::match_simple ("\\bOk", "<TEXT>Ok</TEXT>", Regex::COMPILE_NORMAL, Regex::MATCH_NORMAL), ==, true);
}
REGISTER_TEST ("General/Regex Tests", test_regex);

static void
test_debug_config ()
{
  TCMP (debug_config_get ("randomFOObarCOOBDPlnjdhsa51cU8RYB62ags"), ==, "");
  TCMP (debug_config_get ("randomFOObarCOOBDPlnjdhsa51cU8RYB62ags", "dflt"), ==, "dflt");
  setenv ("RAPICORN_INTERN_TEST_DATA", "randomFOObarCOOBDPlnjdhsa51cU8RYB62ags=17", 1);
  TCMP (debug_config_get ("randomFOObarCOOBDPlnjdhsa51cU8RYB62ags"), ==, "");
  debug_envvar ("RAPICORN_INTERN_TEST_DATA");
  TCMP (debug_config_get ("randomFOObarCOOBDPlnjdhsa51cU8RYB62ags"), ==, "17");
  debug_config_add ("randomFOObarCOOBDPlnjdhsa51cU8RYB62ags=hello");
  TCMP (debug_config_get ("randomFOObarCOOBDPlnjdhsa51cU8RYB62ags"), ==, "hello");
  debug_config_del ("randomFOObarCOOBDPlnjdhsa51cU8RYB62ags");
  TCMP (debug_config_get ("randomFOObarCOOBDPlnjdhsa51cU8RYB62ags"), ==, "17");
}
REGISTER_TEST ("General/Debug Configuration", test_debug_config);

static void
test_paths()
{
  String p, s;
  // Path::join
  s = Path::join ("0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "a", "b", "c", "d", "e", "f");
#if RAPICORN_DIR_SEPARATOR == '/'
  p = "0/1/2/3/4/5/6/7/8/9/a/b/c/d/e/f";
#else
  p = "0\\1\\2\\3\\4\\5\\6\\7\\8\\9\\a\\b\\c\\d\\e\\f";
#endif
  TCMP (s, ==, p);
  // Path::searchpath_join
  s = Path::searchpath_join ("0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "a", "b", "c", "d", "e", "f");
#if RAPICORN_SEARCHPATH_SEPARATOR == ';'
  p = "0;1;2;3;4;5;6;7;8;9;a;b;c;d;e;f";
#else
  p = "0:1:2:3:4:5:6:7:8:9:a:b:c:d:e:f";
#endif
  TCMP (s, ==, p);
  // Path
  bool b = Path::isabs (p);
  TCMP (b, ==, false);
#if RAPICORN_DIR_SEPARATOR == '/'
  s = Path::join (RAPICORN_DIR_SEPARATOR_S, s);
#else
  s = Path::join ("C:\\", s);
#endif
  b = Path::isabs (s);
  TCMP (b, ==, true);
  s = Path::skip_root (s);
  TCMP (s, ==, p);
  TASSERT (Path::dir_separator == "/" || Path::dir_separator == "\\");
  TASSERT (Path::searchpath_separator == ":" || Path::searchpath_separator == ";");
  TCMP (Path::basename ("simple"), ==, "simple");
  TCMP (Path::basename ("skipthis" RAPICORN_DIR_SEPARATOR_S "file"), ==, "file");
  TCMP (Path::basename (RAPICORN_DIR_SEPARATOR_S "skipthis" RAPICORN_DIR_SEPARATOR_S "file"), ==, "file");
  TCMP (Path::dirname ("file"), ==, ".");
  TCMP (Path::dirname ("dir" RAPICORN_DIR_SEPARATOR_S), ==, "dir");
  TCMP (Path::dirname ("dir" RAPICORN_DIR_SEPARATOR_S "file"), ==, "dir");
  TCMP (Path::cwd(), !=, "");
  TCMP (Path::check (Path::join (Path::cwd(), "..", "tests"), "rd"), ==, true); // ./../tests/ should be a readable directory
  TCMP (Path::isdirname (""), ==, false);
  TCMP (Path::isdirname ("foo"), ==, false);
  TCMP (Path::isdirname ("foo/"), ==, true);
  TCMP (Path::isdirname ("/foo"), ==, false);
  TCMP (Path::isdirname ("foo/."), ==, true);
  TCMP (Path::isdirname ("foo/.."), ==, true);
  TCMP (Path::isdirname ("foo/..."), ==, false);
  TCMP (Path::isdirname ("foo/..../"), ==, true);
  TCMP (Path::isdirname ("/."), ==, true);
  TCMP (Path::isdirname ("/.."), ==, true);
  TCMP (Path::isdirname ("/"), ==, true);
  TCMP (Path::isdirname ("."), ==, true);
  TCMP (Path::isdirname (".."), ==, true);
}
REGISTER_TEST ("General/Path handling", test_paths);

static void
test_file_io()
{
  String fname = string_format ("xtmp-infotest.%u", getpid());
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
REGISTER_TEST ("General/File IO", test_file_io);

static void
test_zintern()
{
  static const unsigned char TEST_DATA[] = "x\332K\312,\312K-\321\255\312\314+I-\312S(I-.QHI,I\4\0v\317\11V";
  uint8 *data = zintern_decompress (24, TEST_DATA, sizeof (TEST_DATA) / sizeof (TEST_DATA[0]));
  TCMP (String ((char*) data), ==, "birnet-zintern test data");
  zintern_free (data);
}
REGISTER_TEST ("General/ZIntern", test_zintern);

static void
test_files (void)
{
  const String argv0 = program_argv0();
  TCMP (Path::equals ("/bin", "/../bin"), ==, TRUE);
  TCMP (Path::equals ("/bin", "/sbin"), ==, FALSE);
  TCMP (Path::check (argv0, "e"), ==, TRUE);
  TCMP (Path::check (argv0, "r"), ==, TRUE);
  // TCMP (Path::check (argv0, "w"), ==, TRUE); // fails on kfreebsd
  TCMP (Path::check (argv0, "x"), ==, TRUE);
  TCMP (Path::check (argv0, "d"), ==, FALSE);
  TCMP (Path::check (argv0, "l"), ==, FALSE);
  TCMP (Path::check (argv0, "c"), ==, FALSE);
  TCMP (Path::check (argv0, "b"), ==, FALSE);
  TCMP (Path::check (argv0, "p"), ==, FALSE);
  TCMP (Path::check (argv0, "s"), ==, FALSE);
}
REGISTER_TEST ("General/FileChecks", test_files);

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
REGISTER_TEST ("General/Id Allocator", test_id_allocator);

static void
test_dtoi32()
{
  TCMP (_dtoi32_generic (0.0), ==, 0);
  TCMP (_dtoi32_generic (+0.3), ==, +0);
  TCMP (_dtoi32_generic (-0.3), ==, -0);
  TCMP (_dtoi32_generic (+0.7), ==, +1);
  TCMP (_dtoi32_generic (-0.7), ==, -1);
  TCMP (_dtoi32_generic (+2147483646.3), ==, +2147483646);
  TCMP (_dtoi32_generic (+2147483646.7), ==, +2147483647);
  TCMP (_dtoi32_generic (-2147483646.3), ==, -2147483646);
  TCMP (_dtoi32_generic (-2147483646.7), ==, -2147483647);
  TCMP (_dtoi32_generic (-2147483647.3), ==, -2147483647);
  TCMP (_dtoi32_generic (-2147483647.7), ==, -2147483648LL);
  TCMP (dtoi32 (0.0), ==, 0);
  TCMP (dtoi32 (+0.3), ==, +0);
  TCMP (dtoi32 (-0.3), ==, -0);
  TCMP (dtoi32 (+0.7), ==, +1);
  TCMP (dtoi32 (-0.7), ==, -1);
  TCMP (dtoi32 (+2147483646.3), ==, +2147483646);
  TCMP (dtoi32 (+2147483646.7), ==, +2147483647);
  TCMP (dtoi32 (-2147483646.3), ==, -2147483646);
  TCMP (dtoi32 (-2147483646.7), ==, -2147483647);
  TCMP (dtoi32 (-2147483647.3), ==, -2147483647);
  TCMP (dtoi32 (-2147483647.7), ==, -2147483648LL);
}
REGISTER_TEST ("Math/dtoi32", test_dtoi32);

static void
test_dtoi64()
{
  TCMP (_dtoi64_generic (0.0), ==, 0);
  TCMP (_dtoi64_generic (+0.3), ==, +0);
  TCMP (_dtoi64_generic (-0.3), ==, -0);
  TCMP (_dtoi64_generic (+0.7), ==, +1);
  TCMP (_dtoi64_generic (-0.7), ==, -1);
  TCMP (_dtoi64_generic (+2147483646.3), ==, +2147483646);
  TCMP (_dtoi64_generic (+2147483646.7), ==, +2147483647);
  TCMP (_dtoi64_generic (-2147483646.3), ==, -2147483646);
  TCMP (_dtoi64_generic (-2147483646.7), ==, -2147483647);
  TCMP (_dtoi64_generic (-2147483647.3), ==, -2147483647);
  TCMP (_dtoi64_generic (-2147483647.7), ==, -2147483648LL);
  TCMP (_dtoi64_generic (+4294967297.3), ==, +4294967297LL);
  TCMP (_dtoi64_generic (+4294967297.7), ==, +4294967298LL);
  TCMP (_dtoi64_generic (-4294967297.3), ==, -4294967297LL);
  TCMP (_dtoi64_generic (-4294967297.7), ==, -4294967298LL);
  TCMP (_dtoi64_generic (+1125899906842624.3), ==, +1125899906842624LL);
  TCMP (_dtoi64_generic (+1125899906842624.7), ==, +1125899906842625LL);
  TCMP (_dtoi64_generic (-1125899906842624.3), ==, -1125899906842624LL);
  TCMP (_dtoi64_generic (-1125899906842624.7), ==, -1125899906842625LL);
  TCMP (dtoi64 (0.0), ==, 0);
  TCMP (dtoi64 (+0.3), ==, +0);
  TCMP (dtoi64 (-0.3), ==, -0);
  TCMP (dtoi64 (+0.7), ==, +1);
  TCMP (dtoi64 (-0.7), ==, -1);
  TCMP (dtoi64 (+2147483646.3), ==, +2147483646);
  TCMP (dtoi64 (+2147483646.7), ==, +2147483647);
  TCMP (dtoi64 (-2147483646.3), ==, -2147483646);
  TCMP (dtoi64 (-2147483646.7), ==, -2147483647);
  TCMP (dtoi64 (-2147483647.3), ==, -2147483647);
  TCMP (dtoi64 (-2147483647.7), ==, -2147483648LL);
  TCMP (dtoi64 (+4294967297.3), ==, +4294967297LL);
  TCMP (dtoi64 (+4294967297.7), ==, +4294967298LL);
  TCMP (dtoi64 (-4294967297.3), ==, -4294967297LL);
  TCMP (dtoi64 (-4294967297.7), ==, -4294967298LL);
  TCMP (dtoi64 (+1125899906842624.3), ==, +1125899906842624LL);
  TCMP (dtoi64 (+1125899906842624.7), ==, +1125899906842625LL);
  TCMP (dtoi64 (-1125899906842624.3), ==, -1125899906842624LL);
  TCMP (dtoi64 (-1125899906842624.7), ==, -1125899906842625LL);
}
REGISTER_TEST ("Math/dtoi64", test_dtoi64);

static void
test_iround()
{
  TCMP (round (0.0), ==, 0.0);
  TCMP (round (+0.3), ==, +0.0);
  TCMP (round (-0.3), ==, -0.0);
  TCMP (round (+0.7), ==, +1.0);
  TCMP (round (-0.7), ==, -1.0);
  TCMP (round (+4294967297.3), ==, +4294967297.0);
  TCMP (round (+4294967297.7), ==, +4294967298.0);
  TCMP (round (-4294967297.3), ==, -4294967297.0);
  TCMP (round (-4294967297.7), ==, -4294967298.0);
  TCMP (iround (0.0), ==, 0);
  TCMP (iround (+0.3), ==, +0);
  TCMP (iround (-0.3), ==, -0);
  TCMP (iround (+0.7), ==, +1);
  TCMP (iround (-0.7), ==, -1);
  TCMP (iround (+4294967297.3), ==, +4294967297LL);
  TCMP (iround (+4294967297.7), ==, +4294967298LL);
  TCMP (iround (-4294967297.3), ==, -4294967297LL);
  TCMP (iround (-4294967297.7), ==, -4294967298LL);
  TCMP (iround (+1125899906842624.3), ==, +1125899906842624LL);
  TCMP (iround (+1125899906842624.7), ==, +1125899906842625LL);
  TCMP (iround (-1125899906842624.3), ==, -1125899906842624LL);
  TCMP (iround (-1125899906842624.7), ==, -1125899906842625LL);
}
REGISTER_TEST ("Math/iround", test_iround);

static void
test_iceil()
{
  TCMP (ceil (0.0), ==, 0.0);
  TCMP (ceil (+0.3), ==, +1.0);
  TCMP (ceil (-0.3), ==, -0.0);
  TCMP (ceil (+0.7), ==, +1.0);
  TCMP (ceil (-0.7), ==, -0.0);
  TCMP (ceil (+4294967297.3), ==, +4294967298.0);
  TCMP (ceil (+4294967297.7), ==, +4294967298.0);
  TCMP (ceil (-4294967297.3), ==, -4294967297.0);
  TCMP (ceil (-4294967297.7), ==, -4294967297.0);
  TCMP (iceil (0.0), ==, 0);
  TCMP (iceil (+0.3), ==, +1);
  TCMP (iceil (-0.3), ==, -0);
  TCMP (iceil (+0.7), ==, +1);
  TCMP (iceil (-0.7), ==, -0);
  TCMP (iceil (+4294967297.3), ==, +4294967298LL);
  TCMP (iceil (+4294967297.7), ==, +4294967298LL);
  TCMP (iceil (-4294967297.3), ==, -4294967297LL);
  TCMP (iceil (-4294967297.7), ==, -4294967297LL);
  TCMP (iceil (+1125899906842624.3), ==, +1125899906842625LL);
  TCMP (iceil (+1125899906842624.7), ==, +1125899906842625LL);
  TCMP (iceil (-1125899906842624.3), ==, -1125899906842624LL);
  TCMP (iceil (-1125899906842624.7), ==, -1125899906842624LL);
}
REGISTER_TEST ("Math/iceil", test_iceil);

static void
test_ifloor()
{
  TCMP (floor (0.0), ==, 0.0);
  TCMP (floor (+0.3), ==, +0.0);
  TCMP (floor (-0.3), ==, -1.0);
  TCMP (floor (+0.7), ==, +0.0);
  TCMP (floor (-0.7), ==, -1.0);
  TCMP (floor (+4294967297.3), ==, +4294967297.0);
  TCMP (floor (+4294967297.7), ==, +4294967297.0);
  TCMP (floor (-4294967297.3), ==, -4294967298.0);
  TCMP (floor (-4294967297.7), ==, -4294967298.0);
  TCMP (ifloor (0.0), ==, 0);
  TCMP (ifloor (+0.3), ==, +0);
  TCMP (ifloor (-0.3), ==, -1);
  TCMP (ifloor (+0.7), ==, +0);
  TCMP (ifloor (-0.7), ==, -1);
  TCMP (ifloor (+4294967297.3), ==, +4294967297LL);
  TCMP (ifloor (+4294967297.7), ==, +4294967297LL);
  TCMP (ifloor (-4294967297.3), ==, -4294967298LL);
  TCMP (ifloor (-4294967297.7), ==, -4294967298LL);
  TCMP (ifloor (+1125899906842624.3), ==, +1125899906842624LL);
  TCMP (ifloor (+1125899906842624.7), ==, +1125899906842624LL);
  TCMP (ifloor (-1125899906842624.3), ==, -1125899906842625LL);
  TCMP (ifloor (-1125899906842624.7), ==, -1125899906842625LL);
}
REGISTER_TEST ("Math/ifloor", test_ifloor);

static void
comparison_tests()
{
  assert (compare_lesser (1., 2.) == -1);
  assert (compare_lesser (2., 1.) == +1);
  assert (compare_lesser (2., 2.) == 0);
  assert (compare_greater (2., 1.) == -1);
  assert (compare_greater (1., 2.) == +1);
  assert (compare_greater (2., 2.) == 0);
}
REGISTER_TEST ("Math/compare", comparison_tests);

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
  TCMP (fit, ==, sv.end());
  fit = binary_lookup_sibling (sv.begin(), sv.end(), compare_floats, -INFINITY);
  TCMP (fit, !=, sv.end());
  /* 0-size lookups */
  vector<float> ev;
  fit = binary_lookup (ev.begin(), ev.end(), compare_floats, 0);
  TCMP (fit, ==, ev.end());
  fit = binary_lookup_sibling (ev.begin(), ev.end(), compare_floats, 0);
  TCMP (fit, ==, ev.end());
  pit = binary_lookup_insertion_pos (ev.begin(), ev.end(), compare_floats, 0);
  TCMP (pit.first, ==, ev.end());
  TCMP (pit.second, ==, false);
  TDONE();
  TSTART ("General/Binary Binary lookup");
  for (uint i = 0; i < fv.size(); i++)
    {
      fit = binary_lookup (sv.begin(), sv.end(), compare_floats, fv[i]);
      TCMP (fit, !=, sv.end());
      TCMP (fv[i], ==, *fit);
      if (i % 10000 == 9999)
        TCMP (fv[i], ==, *fit);
    }
  TDONE();
  TSTART ("General/Binary Sibling lookup");
  for (uint i = 1; i < sv.size(); i++)
    {
      double target = (sv[i - 1] + sv[i]) / 2.0;
      fit = binary_lookup_sibling (sv.begin(), sv.end(), compare_floats, target);
      TCMP (fit, !=, sv.end());
      TASSERT (sv[i] == *fit || sv[i - 1] == *fit);
      if (i % 10000 == 9999)
        TOK();
    }
  TDONE();
  TSTART ("General/Binary Insertion lookup1");
  seen_inexact = false;
  for (uint i = 0; i < fv.size(); i++)
    {
      pit = binary_lookup_insertion_pos (sv.begin(), sv.end(), compare_floats, fv[i]);
      fit = pit.first;
      seen_inexact |= pit.second == false;
      TCMP (fit, !=, sv.end());
      TCMP (fv[i], ==, *fit);
      if (i % 10000 == 9999)
        TOK();
    }
  TCMP (seen_inexact, ==, false);
  TDONE();
  TSTART ("General/Binary Insertion lookup2");
  seen_inexact = false;
  for (uint i = 1; i < sv.size(); i++)
    {
      double target = (sv[i - 1] + sv[i]) / 2.0;
      pit = binary_lookup_insertion_pos (sv.begin(), sv.end(), compare_floats, target);
      fit = pit.first;
      seen_inexact |= pit.second == false;
      TCMP (fit, !=, sv.end());
      TASSERT (sv[i] == *fit || sv[i - 1] == *fit);
      if (i % 10000 == 9999)
        TOK();
    }
  TCMP (seen_inexact, ==, true);
}
REGISTER_TEST ("General/Binary Lookups", binary_lookup_tests);

/// [Blob-EXAMPLE]
// Declare text resources for later use in a program, see also rapidres(1).
RAPICORN_RES_STATIC_DATA  (text_resource) = "Alpha Beta Gamma"; // Compiler adds trailing 0
RAPICORN_RES_STATIC_ENTRY (text_resource, "tests/text_resource.txt");

// If a resource data length is given, it must match the initializer size (it may omit the trailing zero).
RAPICORN_RES_STATIC_DATA  (digit_resource) = "0123456789"; // Provide exactly 10 characters.
RAPICORN_RES_STATIC_ENTRY (digit_resource, "tests/digit_resource.txt", 10);

static void // Access a previously declared resource from anywhere within a program.
access_text_resources ()
{
  // Load a Blob from "tests/text_resource.txt"
  Blob blob = Res ("@res tests/text_resource.txt");
  assert (blob.size() > 0); // Verify lookup success.

  // Access the Blob as string (automatically strips trailing 0s).
  std::string text = blob.string();
  assert (text == "Alpha Beta Gamma"); // Verify its contents.

  // Load the other defined "tests/digit_resource.txt" blob.
  blob = Res ("@res tests/digit_resource.txt");

  // Access Blob size and data,
  assert (10 == blob.size() && 0 == strcmp (blob.data(), "0123456789"));
}
/// [Blob-EXAMPLE]
REGISTER_TEST ("Resource/Test Example", access_text_resources);

static void
test_builtin_resources()
{
  Blob blob;
  assert (!blob && blob.size() == 0);
  blob = Res ("@res Rapicorn/stock.ini");
  assert (blob && blob.size() > 0);
  blob = Res ("@res Rapicorn/icons/broken-image.svg");
  assert (blob && blob.size() > 0);
}
REGISTER_TEST ("Resource/Builtin Tests", test_builtin_resources);

static String
open_temporary (int *fdp)
{
  String dir;
  dir = P_tmpdir;
  if (!Path::check (dir, "dxw"))
    dir = "/tmp";
  String path;
  for (uint i = 0; i < 77; i++)
    {
      path = Path::join (P_tmpdir, string_format ("task%u-%x.tmp", ThisThread::thread_pid(), Test::random_irange (256, 4095)));
      if (!Path::check (path, "e"))
        {
          int temporary_fd = open (path.c_str(), O_RDWR | O_EXCL | O_CREAT | O_CLOEXEC | O_NOFOLLOW | O_NOCTTY,
                                   S_IRUSR | S_IWUSR); // 0600
          if (temporary_fd >= 0)
            {
              *fdp = temporary_fd;
              return path;
            }
        }
    }
  RAPICORN_FATAL ("Failed to create temporary file in directory: %s", dir);
}

static void
more_blob_tests ()
{
  // load this source file and check for a random string
  Blob fblob = Blob::load (Path::vpath_find (__FILE__));
  assert (!!fblob);
  assert (fblob.string().find ("F2GlZ1s5FrRzsA") != String::npos);
  // create a big example file aceeding internal mmap thresholds
  int temporary_fd = -1;
  String temporary_filename = open_temporary (&temporary_fd);
  assert (temporary_fd >= 0);
  String string_data =
    string_multiply (string_multiply ("blub", 1024), 128) +
    "LONGshot" +
    string_multiply (string_multiply ("COOL", 1024), 128) +
    String ("\0\0\0\0", 4); // 0-termination
  ssize_t result = write (temporary_fd, string_data.c_str(), string_data.size());
  assert (result > 0);
  result = close (temporary_fd);
  assert (result == 0);
  // load big example file to excercise mmap() code path
  fblob = Blob::load (temporary_filename);
  assert (fblob && fblob.size());
  assert (fblob.data()[fblob.size() - 1] == 0); // ensure 0-termination for strstr
  assert (strstr (fblob.data(), "blubblubLONGshotCOOLCOOL") != NULL); // accessing data() avoids string copy
  unlink (temporary_filename.c_str()); // cleanup example file
  // load a file with unknown size
  fblob = Blob::load ("/proc/cpuinfo");
  assert (fblob || errno == ENOENT);
  if (fblob)
    assert (fblob.string().find ("cpu") != String::npos);
  // test string blobs
  const String foo = "foo";
  fblob = Blob::from (foo);
  Blob bblob = Blob::from ("bar");
  assert (fblob && bblob);
  assert (fblob.string() == foo);
  assert (bblob.string() == "bar");
  assert (fblob.string() != bblob.string());
  assert (fblob.name() != bblob.name());
}
REGISTER_TEST ("Blob/File IO Tests", more_blob_tests);

static void // Test Mutextes before GLib's thread initialization
test_before_thread_init()
{
  /* check C++ mutex init + destruct before GLib starts up */
  Mutex *mutex = new Mutex;
  Cond *cond = new Cond;
  delete mutex;
  delete cond;
}

static uint constructur_attribute_test = 0;

static void RAPICORN_CONSTRUCTOR
constructur_attribute_test_initializer (void)
{
  constructur_attribute_test = 0x1237ABBA;
}

int
main (int   argc,
      char *argv[])
{
  if (constructur_attribute_test != 305638330)
    fatal ("static constructors have not been called before main");

  test_before_thread_init();

  String app_ident = __PRETTY_FILE__;
  if (argc >= 2 && String ("--print-process-handle") == argv[1])
    {
      init_core (app_ident, &argc, argv);
      printout ("%s", process_handle().c_str());
      return 0;
    }
  if (argc >= 3 && String ("--task-status") == argv[1])
    {
      init_core (app_ident, &argc, argv);
      TaskStatus ts = TaskStatus (string_to_int (argv[2]));
      String result;
      bool valid = ts.update();
      sleep (1);
      valid &= ts.update();
      if (valid)
        result = ts.string();
      else
        result = "no stats";
      printout ("TaskStatus: %s\n", result.c_str());
      return 0;
    }

  init_core_test (app_ident, &argc, argv);

  return Test::run();
}

/* vim:set ts=8 sts=2 sw=2: */
