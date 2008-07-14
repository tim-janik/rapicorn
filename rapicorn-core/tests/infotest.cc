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
//#define TEST_VERBOSE
#include <rapicorn-core/testutils.hh>
using namespace Rapicorn;

#if RAPICORN_CHECK_VERSION (2147483647, 2147483647, 2147483647) || !RAPICORN_CHECK_VERSION (0, 0, 0)
#error RAPICORN_CHECK_VERSION() implementation is broken
#endif

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
test_messaging ()
{
  TASSERT (Msg::NONE    == Msg::lookup_type ("none"));
  TASSERT (Msg::ALWAYS  == Msg::lookup_type ("always"));
  TASSERT (Msg::ERROR   == Msg::lookup_type ("error"));
  TASSERT (Msg::WARNING == Msg::lookup_type ("warning"));
  TASSERT (Msg::SCRIPT  == Msg::lookup_type ("script"));
  TASSERT (Msg::INFO    == Msg::lookup_type ("info"));
  TASSERT (Msg::DIAG    == Msg::lookup_type ("diag"));
  TASSERT (Msg::DEBUG   == Msg::lookup_type ("debug"));
  TASSERT (Msg::check (Msg::NONE) == false);
  TASSERT (Msg::check (Msg::ALWAYS) == true);
  Msg::enable (Msg::NONE);
  Msg::disable (Msg::ALWAYS);
  TASSERT (Msg::check (Msg::NONE) == false);
  TASSERT (Msg::check (Msg::ALWAYS) == true);
  TASSERT (Msg::check (Msg::INFO) == true);
  Msg::disable (Msg::INFO);
  TASSERT (Msg::check (Msg::INFO) == false);
  Msg::enable (Msg::INFO);
  TASSERT (Msg::check (Msg::INFO) == true);
  Msg::display (Msg::WARNING,
                Msg::Title ("Warning Title"),
                Msg::Text1 ("Primary warning message."),
                Msg::Text2 ("Secondary warning message."),
                Msg::Text2 ("Continuation of secondary warning message."),
                Msg::Text3 ("Message details: 1, 2, 3."),
                Msg::Text3 ("And more message details: a, b, c."),
                Msg::Check ("Show this message again."));
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

int
main (int   argc,
      char *argv[])
{
  rapicorn_init_test (&argc, &argv);

  Test::add ("CpuInfo", test_cpu_info);
  Test::add ("Path handling", test_paths);
  Test::add ("File IO", test_file_io);
  Test::add ("ZIntern", test_zintern);
  Test::add ("FileChecks", test_files, argv[0]);
  Test::add ("VirtualTypeid", test_virtual_typeid);
  Test::add ("Message Types", test_messaging);

  return Test::run();
}

/* vim:set ts=8 sts=2 sw=2: */
