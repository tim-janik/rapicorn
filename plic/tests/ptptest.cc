/* ptptest.cc - PLIC Test Program
 * Copyright (C) 2008 Tim Janik
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * A copy of the GNU Lesser General Public License should ship along
 * with this program; if not, see http://www.gnu.org/copyleft/.
 */

// include PLIC TypeMap Parser
#include "../../rcore/plicutils.hh"
#include "../../rcore/plicutils.cc"

#define error(...) do { fputs ("ERROR: ", stderr); fprintf (stderr, __VA_ARGS__); fputs ("\n", stderr); abort(); } while (0)

namespace { // Anon

using namespace Plic;
using Plic::int64_t;
using Plic::uint64_t;
typedef uint32_t uint;
typedef std::string String;
using std::vector;

#ifndef MAX
#define MIN(a,b)        ((a) <= (b) ? (a) : (b))
#define MAX(a,b)        ((a) >= (b) ? (a) : (b))
#endif

/* --- option parsing --- */
static bool
parse_str_option (char       **argv,
                  uint        &i,
                  const char  *arg,
                  const char **strp,
                  uint         argc)
{
  uint length = strlen (arg);
  if (strncmp (argv[i], arg, length) == 0)
    {
      const char *equal = argv[i] + length;
      if (*equal == '=')              /* -x=Arg */
        *strp = equal + 1;
      else if (*equal)                /* -xArg */
        *strp = equal;
      else if (i + 1 < argc)          /* -x Arg */
        {
          argv[i++] = NULL;
          *strp = argv[i];
        }
      argv[i] = NULL;
      if (*strp)
        return true;
    }
  return false;
}

static bool
parse_bool_option (char       **argv,
                   uint        &i,
                   const char  *arg)
{
  uint length = strlen (arg);
  if (strncmp (argv[i], arg, length) == 0)
    {
      argv[i] = NULL;
      return true;
    }
  return false;
}

/* --- test program --- */

static void
aux_check (std::string    type_map_file,
           vector<String> auxtests)
{
  // read type map
  TypeMap tp = TypeMap::load (type_map_file);
  if (tp.error_status())
    error ("%s: open failed: %s", type_map_file.c_str(), strerror (tp.error_status()));
  // print types and match auxtests
  for (size_t i = 0; i < tp.type_count(); i++)
    {
      const TypeCode tc = tp.type (i);
      printf ("%s %s;\n", tc.kind_name().c_str(), tc.name().c_str());
      for (size_t k = 0; k < tc.aux_count(); k++)
        {
          String aux = tc.aux_data (k);
          uint c = ' ';
          for (size_t q = 0; q < auxtests.size(); q++)
            if (strstr (aux.c_str(), auxtests[q].c_str()))
              {
                auxtests.erase (auxtests.begin() + q);
                c = '*';
                break;
              }
          printf ("  %c %s\n", c, aux.c_str());
        }
    }
  // fail for unmatched auxtests
  if (auxtests.size())
    {
      fprintf (stderr, "ERROR: unmatched aux-tests:\n");
      for (uint q = 0; q < auxtests.size(); q++)
        fprintf (stderr, "  %s\n", auxtests[q].c_str());
      abort();
    }
}

static void
list_types (std::string type_map_file)
{
  TypeMap tmap = TypeMap::load_local (type_map_file);
  if (tmap.error_status())
    error ("%s: open failed: %s", type_map_file.c_str(), strerror (tmap.error_status()));
  errno = 0;
  printf ("TYPES: %zu\n", tmap.type_count());
  for (uint i = 0; i < tmap.type_count(); i++)
    {
      const TypeCode tcode = tmap.type (i);
      printf ("%s\n", tcode.pretty ("  ").c_str());
      // verify type search
      const TypeCode other = tmap.lookup_local (tcode.name());
      if (other != tcode)
        {
          errno = ENXIO;
          perror (std::string ("searching type: " + tcode.name()).c_str()), abort();
        }
    }
  if (errno)
    perror ("checking type package"), abort();
}

static void
standard_tests ()
{
  TypeCode tcint = TypeMap::lookup ("int");
  assert (tcint.kind() == INT);
  assert (tcint.name() == "int");
  assert (tcint.kind_name() == "INT");
  TypeCode tcfloat = TypeMap::lookup ("float");
  assert (tcfloat.kind_name() == "FLOAT");
  assert (tcfloat.kind() == FLOAT);
  assert (tcfloat.name() == "float");
  TypeCode tcstring = TypeMap::lookup ("string");
  assert (tcstring.kind() == STRING);
  assert (tcstring.name() == "string");
  assert (tcstring.kind_name () == "STRING");
  TypeCode tcany = TypeMap::lookup ("any");
  assert (tcany.kind() == ANY);
  assert (tcany.name() == "any");
  assert (tcany.kind_name () == "ANY");
  TypeCode tcnot = TypeMap::lookup (".@-nosuchtype?");
  assert (tcnot.untyped() == true);
  printf ("  TEST   Plic standard types                                             OK\n");
}

static void
type_code_tests ()
{
  const String filename = "typecodetests.typ";
  TypeMap tp = TypeMap::load_local (filename);
  if (tp.error_status())
    error ("%s: open failed: %s", filename.c_str(), strerror (tp.error_status()));
  {
    // simple int lookup
    TypeCode t1 = tp.lookup_local ("PlicTests::IntWithFooAsLabel");
    assert (t1.kind() == INT);
    assert (t1.name() == "PlicTests::IntWithFooAsLabel");
    assert (t1.aux_value ("label") == "Foo");
    assert (t1.hints() == ":");
    // simple float lookup
    TypeCode t2 = tp.lookup_local ("PlicTests::FloatWithBlurbBlurb");
    assert (t2.kind() == FLOAT);
    assert (t2.name() == "PlicTests::FloatWithBlurbBlurb");
    assert (t2.aux_value ("blurb") == "Float Blurb");
    // check that type memory used above is still valid
    assert (t1.name() + "-postfix" == String ("PlicTests::IntWithFooAsLabel-postfix"));
    assert (t2.name() == String ("PlicTests::FloatWithBlurbBlurb"));
  }
  { // INT
    TypeCode t = tp.lookup_local ("PlicTests::ExtendIntWithAux");
    assert (t.kind() == INT);
    assert (t.name() == "PlicTests::ExtendIntWithAux");
    assert (t.aux_value ("label") == "Extended int");
    assert (t.aux_value ("blurb") == "This int demonstrates extensive auxillary data use");
    assert (t.aux_value ("default") == "33");
    assert (t.aux_value ("step") == "-5");
    assert (t.hints().find (":extra-option:") != String().npos);
  }
  { // FLOAT
    TypeCode t = tp.lookup_local ("PlicTests::FloatWithBlurbBlurb");
    assert (t.kind() == FLOAT);
    assert (t.name() == "PlicTests::FloatWithBlurbBlurb");
    assert (t.aux_value ("label") == "Float Label");
    assert (t.aux_value ("blurb") == "Float Blurb");
    assert (t.aux_value ("default") == "97.97");
    assert (t.hints() == ":");
  }
  { // STRING
    TypeCode t = tp.lookup_local ("PlicTests::ExtendedString");
    assert (t.kind() == STRING);
    assert (t.name() == "PlicTests::ExtendedString");
    assert (t.aux_value ("label") == "Extended String");
    assert (t.aux_value ("blurb") == "Demonstrate full string specification");
    assert (t.aux_value ("default") == "Default-String-Value");
    assert (t.hints().find (":ro:") != String().npos);
  }
  { // ENUM
    TypeCode t = tp.lookup_local ("PlicTests::Enum1");
    assert (t.kind() == ENUM);
    assert (t.kind_name () == "ENUM");
    assert (t.name() == "PlicTests::Enum1");
    assert (t.aux_value ("blurb") == "");
    assert (t.aux_value ("default") == "");
    assert (t.hints() == ":");
    assert (t.enum_count() == 2);
    assert (t.enum_value (0)[0] == "ENUM_VALUE0");
    assert (t.enum_value (1)[0] == "ENUM_VALUE1");
  }
  { // SEQUENCE
    TypeCode t = tp.lookup_local ("PlicTests::SimpleSequence");
    assert (t.kind() == SEQUENCE);
    assert (t.kind_name () == "SEQUENCE");
    assert (t.name() == "PlicTests::SimpleSequence");
    assert (t.aux_value ("blurb") == "");
    assert (t.aux_value ("default") == "");
    assert (t.hints() == ":");
    assert (t.field_count() == 1);
    TypeCode f = t.field (0);
    assert (f.kind() == INT);
    assert (f.name() == "sample_integer");
  }
  { // RECORD
    TypeCode t = tp.lookup_local ("PlicTests::SimpleRecord");
    assert (t.kind() == RECORD);
    assert (t.kind_name () == "RECORD");
    assert (t.name() == "PlicTests::SimpleRecord");
    assert (t.aux_value ("blurb") == "");
    assert (t.aux_value ("default") == "");
    assert (t.hints() == ":");
    assert (t.field_count() == 4);
    TypeCode f = t.field (0);
    assert (f.kind() == INT);
    assert (f.name() == "intfield");
    f = t.field (1);
    assert (f.kind() == FLOAT);
    assert (f.name() == "floatfield");
    f = t.field (2);
    assert (f.kind() == STRING);
    assert (f.name() == "stringfield");
    f = t.field (3);
    assert (f.kind() == ANY);
    assert (f.name() == "anyfield");
  }
  // done
  printf ("  TEST   Plic type code IDL tests                                        OK\n");
}

static const double test_double_value = 7.76576e-306;

static void
any_test_set (Any &a, int what)
{
  switch (what)
    {
      typedef unsigned char uchar;
    case 0:  a <<= bool (0);            break;
    case 1:  a <<= bool (true);         break;
    case 2:  a <<= char (-117);         break;
    case 3:  a <<= uchar (250);         break;
    case 4:  a <<= int (-134217728);    break;
    case 5:  a <<= uint (4294967295U);  break;
    case 6:  a <<= long (-2147483648);  break;
    case 7:  a <<= ulong (4294967295U); break;
    case 8:  a <<= int64_t (-0xc0ffeec0ffeeLL);         break;
    case 9:  a <<= uint64_t (0xffffffffffffffffULL);    break;
    case 10: a <<= "Test4test";         break;
    case 11: a <<= test_double_value;   break;
    case 12: { Any a2; a2 <<= "SecondAny"; a <<= a2; }  break;
    }
}

static bool
any_test_get (const Any &a, int what)
{
  std::string s;
  switch (what)
    {
      typedef unsigned char uchar;
      bool b; char c; uchar uc; int i; uint ui; long l; ulong ul; int64_t i6; uint64_t u6; double d; const Any *p;
    case 0:  if (!(a >>= b))  return false;     assert (b == 0); break;
    case 1:  if (!(a >>= b))  return false;     assert (b == true); break;
    case 2:  if (!(a >>= c))  return false;     assert (c == -117); break;
    case 3:  if (!(a >>= uc)) return false;     assert (uc == 250); break;
    case 4:  if (!(a >>= i))  return false;     assert (i == -134217728); break;
    case 5:  if (!(a >>= ui)) return false;     assert (ui == 4294967295U); break;
    case 6:  if (!(a >>= l))  return false;     assert (l == -2147483648); break;
    case 7:  if (!(a >>= ul)) return false;     assert (ul == 4294967295U); break;
    case 8:  if (!(a >>= i6)) return false;     assert (i6 == -0xc0ffeec0ffeeLL); break;
    case 9:  if (!(a >>= u6)) return false;     assert (u6 == 0xffffffffffffffffULL); break;
    case 10: if (!(a >>= s))  return false;     assert (s == "Test4test"); break;
    case 11: if (!(a >>= d))  return false;     assert (d = test_double_value); break;
    case 12: if (!(a >>= p) ||
                 !(*p >>= s)) return false;     assert (s == "SecondAny"); break;
    }
  return true;
}

static void
test_any()
{
  String s;
  const size_t cases = 13;
  Any a;
  for (size_t j = 0; j <= cases; j++)
    for (size_t k = 0; k <= cases; k++)
      {
        size_t cs[2] = { j, k };
        for (size_t cc = 0; cc < 2; cc++)
          {
            any_test_set (a, cs[cc]);
            const bool any_getter_successfull = any_test_get (a, cs[cc]);
            assert (any_getter_successfull == true);
            Any a2 (a);
            const bool any_copy_successfull = any_test_get (a2, cs[cc]);
            assert (any_copy_successfull == true);
            Any a3;
            a3 = a2;
            const bool any_assignment_successfull = any_test_get (a2, cs[cc]);
            assert (any_assignment_successfull == true);
          }
      }
  printf ("  TEST   Plic Any storage                                                OK\n");
  a <<= 1.;             assert (a.kind() == FLOAT && a.as_float() == +1.0);
  a <<= -1.;            assert (a.kind() == FLOAT && a.as_float() == -1.0);
  a <<= 16.5e+6;        assert (a.as_float() > 16000000.0 && a.as_float() < 17000000.0);
  a <<= 1;              assert (a.kind() == INT && a.as_int() == 1 && a.as_float() == 1 && a.as_string() == "1");
  a <<= -1;             assert (a.kind() == INT && a.as_int() == -1 && a.as_float() == -1 && a.as_string() == "-1");
  a <<= 0;              assert (a.kind() == INT && a.as_int() == 0 && a.as_float() == 0 && a.as_string() == "0");
  a <<= 32767199;       assert (a.kind() == INT && a.as_int() == 32767199);
  a <<= "";             assert (a.kind() == STRING && a.as_string() == "" && a.as_int() == 0);
  a <<= "f";            assert (a.kind() == STRING && a.as_string() == "f" && a.as_int() == 1);
  a <<= "123456789";    assert (a.kind() == STRING && a.as_string() == "123456789" && a.as_int() == 1);
  printf ("  TEST   Plic Any conversions                                            OK\n");
}

} // Anon

int
main (int   argc,
      char *argv[])
{
  vector<String> auxtests;
  /* parse args */
  for (uint32_t i = 1; i < uint32_t (argc); i++)
    {
      const char *str = NULL;
      if (strcmp (argv[i], "--tests") == 0)
        {
          standard_tests();
          type_code_tests();
          test_any();
          return 0;
        }
      else if (strcmp (argv[i], "--") == 0)
        {
          argv[i] = NULL;
          break;
        }
      else if (parse_bool_option (argv, i, "--help"))
        {
          printf ("Usage: %s [options] TypeMap.typ\n", argv[0]);
          printf ("Options:\n");
          printf ("  --tests       Carry out various tests\n");
          printf ("  --help        Print usage summary\n");
          printf ("  --aux-test=x  Find 'x' in auxillary type data\n");
          exit (0);
        }
      else if (parse_str_option (argv, i, "--aux-test", &str, argc))
        auxtests.push_back (str);
    }
  /* collapse parsed args */
  uint32_t e = 1;
  for (uint32_t i = 1; i < uint32_t (argc); i++)
    if (argv[i])
      {
        argv[e++] = argv[i];
        if (i >= e)
          argv[i] = NULL;
      }
  argc = e;
  /* validate mandatory arg */
  if (argc < 2)
    error ("Usage: %s TypeMap.typ\n", argv[0]);

  if (auxtests.size())
    aux_check (argv[1], auxtests);
  else
    list_types (argv[1]);

  return 0;
}
// g++ -Wall -Os ptptest.cc && ./a.out typemap.typ
