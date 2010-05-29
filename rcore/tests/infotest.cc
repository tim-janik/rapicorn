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
#include <rcore/testutils.hh>
#include <stdio.h>
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
test_regex (void)
{
  TASSERT (Regex::match_simple ("Lion", "Lion", Regex::EXTENDED | Regex::ANCHORED, Regex::MATCH_NORMAL) == true);
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

struct SomeObject : public ReferenceCountImpl {
};

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

  rapicorn_init_test (&argc, &argv);

  Test::add ("Misc", test_misc);
  Test::add ("CpuInfo", test_cpu_info);
  Test::add ("Regex Tests", test_regex);
  Test::add ("Path handling", test_paths);
  Test::add ("File IO", test_file_io);
  Test::add ("ZIntern", test_zintern);
  Test::add ("FileChecks", test_files, argv[0]);
  Test::add ("VirtualTypeid", test_virtual_typeid);
  Test::add ("Id Allocator", test_id_allocator);
  Test::add ("Locatable IDs", test_locatable_ids);
  Test::add ("Message Types", test_messaging);

  return Test::run();
}

/* vim:set ts=8 sts=2 sw=2: */
