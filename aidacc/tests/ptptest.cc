// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html

// include AidaTypeMap Parser
#include <rapicorn-core.hh>
#include <cstring>
#include <cassert>

#define error(...) do { fputs ("ERROR: ", stderr); fprintf (stderr, __VA_ARGS__); fputs ("\n", stderr); abort(); } while (0)

namespace { // Anon

using namespace Rapicorn::Aida;
using Rapicorn::Aida::int64_t;
using Rapicorn::Aida::uint64_t;
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
  TypeCode tcin32 = TypeMap::lookup ("int32");
  assert (tcin32.kind() == INT32);
  assert (tcin32.name() == "int32");
  assert (tcin32.kind_name() == "INT32");
  TypeCode tcin64 = TypeMap::lookup ("int64");
  assert (tcin64.kind() == INT64);
  assert (tcin64.name() == "int64");
  assert (tcin64.kind_name() == "INT64");
  TypeCode tcfloat = TypeMap::lookup ("float64");
  assert (tcfloat.kind_name() == "FLOAT64");
  assert (tcfloat.kind() == FLOAT64);
  assert (tcfloat.name() == "float64");
  TypeCode tcstring = TypeMap::lookup ("String");
  assert (tcstring.kind() == STRING);
  assert (tcstring.name() == "String");
  assert (tcstring.kind_name () == "STRING");
  TypeCode tcany = TypeMap::lookup ("Any");
  assert (tcany.kind() == ANY);
  assert (tcany.name() == "Any");
  assert (tcany.kind_name () == "ANY");
  TypeCode tcnot = TypeMap::lookup (".@-nosuchtype?");
  assert (tcnot.untyped() == true);
  // SmartHandle
  assert (SmartHandle::_null_handle() == NULL);
  assert (!SmartHandle::_null_handle());
  assert (SmartHandle::_null_handle()._orbid() == 0);
  struct TestOrbObject : OrbObject { TestOrbObject (ptrdiff_t x) : OrbObject (x) {} };
  struct OneHandle : SmartHandle { OneHandle (OrbObject &orbo) : SmartHandle (orbo) {} };
  TestOrbObject torbo (1);
  assert (OneHandle (torbo) != NULL);
  assert (OneHandle (torbo));
  assert (OneHandle (torbo)._orbid() == 1);
  assert (OneHandle (torbo)._null_handle() == NULL);
  assert (!OneHandle (torbo)._null_handle());
  assert (OneHandle (torbo)._null_handle()._orbid() == 0);
  printf ("  TEST   Aida standard types                                             OK\n");
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
    TypeCode t1 = tp.lookup_local ("AidaTests::IntWithFooAsLabel");
    assert (t1.kind() == INT32);
    assert (t1.name() == "AidaTests::IntWithFooAsLabel");
    assert (t1.aux_value ("label") == "Foo");
    assert (t1.hints() == ":");
    // simple float lookup
    TypeCode t2 = tp.lookup_local ("AidaTests::FloatWithBlurbBlurb");
    assert (t2.kind() == FLOAT64);
    assert (t2.name() == "AidaTests::FloatWithBlurbBlurb");
    assert (t2.aux_value ("blurb") == "Float Blurb");
    // check that type memory used above is still valid
    assert (t1.name() + "-postfix" == String ("AidaTests::IntWithFooAsLabel-postfix"));
    assert (t2.name() == String ("AidaTests::FloatWithBlurbBlurb"));
  }
  { // INT32
    TypeCode t = tp.lookup_local ("AidaTests::Int32AuxRange");
    assert (t.kind() == INT32);
    assert (t.name() == "AidaTests::Int32AuxRange");
    assert (t.aux_value ("label") == "Int32 Range");
    assert (t.aux_value ("blurb") == "This int demonstrates range data use");
    assert (t.aux_value ("min") == "-100");
    assert (t.aux_value ("max") == "100");
    assert (t.aux_value ("step") == "-5");
    assert (t.hints().find (":extra-option:") != String().npos);
  }
  { // INT64
    TypeCode t = tp.lookup_local ("AidaTests::Int64AuxRange");
    assert (t.kind() == INT64);
    assert (t.name() == "AidaTests::Int64AuxRange");
    assert (t.aux_value ("label") == "Int64 Range");
    assert (t.aux_value ("blurb") == "This int demonstrates range data use");
    assert (t.aux_value ("min") == "-1000");
    assert (t.aux_value ("max") == "1000");
    assert (t.aux_value ("step") == "-50");
    assert (t.hints().find (":extra-option:") != String().npos);
  }
  { // FLOAT64
    TypeCode t = tp.lookup_local ("AidaTests::FloatWithBlurbBlurb");
    assert (t.kind() == FLOAT64);
    assert (t.name() == "AidaTests::FloatWithBlurbBlurb");
    assert (t.aux_value ("label") == "Float Label");
    assert (t.aux_value ("blurb") == "Float Blurb");
    assert (t.aux_value ("default") == "97.97");
    assert (t.hints() == ":rw:");
  }
  { // STRING
    TypeCode t = tp.lookup_local ("AidaTests::ExtendedString");
    assert (t.kind() == STRING);
    assert (t.name() == "AidaTests::ExtendedString");
    assert (t.aux_value ("label") == "Extended String");
    assert (t.aux_value ("blurb") == "Demonstrate full string specification");
    assert (t.aux_value ("default") == "Default-String-Value");
    assert (t.hints().find (":ro:") != String().npos);
  }
  { // ENUM
    TypeCode t = tp.lookup_local ("AidaTests::Enum1");
    assert (t.kind() == ENUM);
    assert (t.kind_name () == "ENUM");
    assert (t.name() == "AidaTests::Enum1");
    assert (t.aux_value ("blurb") == "");
    assert (t.aux_value ("default") == "");
    assert (t.hints() == ":");
    assert (t.enum_count() == 2);
    assert (t.enum_value (0)[0] == "ENUM_VALUE0");
    assert (t.enum_value (1)[0] == "ENUM_VALUE1");
  }
  { // SEQUENCE
    TypeCode t = tp.lookup_local ("AidaTests::SimpleSequence");
    assert (t.kind() == SEQUENCE);
    assert (t.kind_name () == "SEQUENCE");
    assert (t.name() == "AidaTests::SimpleSequence");
    assert (t.aux_value ("blurb") == "");
    assert (t.aux_value ("default") == "");
    assert (t.hints() == ":");
    assert (t.field_count() == 1);
    TypeCode f = t.field (0);
    assert (f.kind() == INT32);
    assert (f.name() == "sample_integer");
  }
  { // SEQUENCE
    TypeCode t = tp.lookup_local ("AidaTests::Int64Sequence");
    assert (t.kind() == SEQUENCE);
    assert (t.kind_name () == "SEQUENCE");
    assert (t.name() == "AidaTests::Int64Sequence");
    assert (t.aux_value ("blurb") == "");
    assert (t.aux_value ("default") == "");
    assert (t.hints() == ":");
    assert (t.field_count() == 1);
    TypeCode f = t.field (0);
    assert (f.kind() == INT64);
    assert (f.name() == "v64");
  }
  { // RECORD
    TypeCode t = tp.lookup_local ("AidaTests::SimpleRecord");
    assert (t.kind() == RECORD);
    assert (t.kind_name () == "RECORD");
    assert (t.name() == "AidaTests::SimpleRecord");
    assert (t.aux_value ("blurb") == "");
    assert (t.aux_value ("default") == "");
    assert (t.hints() == ":");
    assert (t.field_count() == 4);
    TypeCode f = t.field (0);
    assert (f.kind() == INT32);
    assert (f.name() == "intfield");
    f = t.field (1);
    assert (f.kind() == FLOAT64);
    assert (f.name() == "floatfield");
    f = t.field (2);
    assert (f.kind() == STRING);
    assert (f.name() == "stringfield");
    f = t.field (3);
    assert (f.kind() == ANY);
    assert (f.name() == "anyfield");
  }
  // done
  printf ("  TEST   Aida type code IDL tests                                        OK\n");
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
  printf ("  TEST   Aida Any storage                                                OK\n");
  a <<= 1.;             assert (a.kind() == FLOAT64 && a.as_float() == +1.0);
  a <<= -1.;            assert (a.kind() == FLOAT64 && a.as_float() == -1.0);
  a <<= 16.5e+6;        assert (a.as_float() > 16000000.0 && a.as_float() < 17000000.0);
  a <<= 1;              assert (a.kind() == INT64 && a.as_int() == 1 && a.as_float() == 1 && a.as_string() == "1");
  a <<= -1;             assert (a.kind() == INT64 && a.as_int() == -1 && a.as_float() == -1 && a.as_string() == "-1");
  a <<= 0;              assert (a.kind() == INT64 && a.as_int() == 0 && a.as_float() == 0 && a.as_string() == "0");
  a <<= 32767199;       assert (a.kind() == INT64 && a.as_int() == 32767199);
  a <<= "";             assert (a.kind() == STRING && a.as_string() == "" && a.as_int() == 0);
  a <<= "f";            assert (a.kind() == STRING && a.as_string() == "f" && a.as_int() == 1);
  a <<= "123456789";    assert (a.kind() == STRING && a.as_string() == "123456789" && a.as_int() == 1);
  printf ("  TEST   Aida Any conversions                                            OK\n");
  Any b, c, d;
  a <<= -3;              assert (a != b); assert (!(a == b));  c <<= a; d <<= b; assert (c != d); assert (!(c == d));
  a <<= Any();           assert (a != b); assert (!(a == b));  c <<= a; d <<= b; assert (c != d); assert (!(c == d));
  a = Any();             assert (a == b); assert (!(a != b));  c <<= a; d <<= b; assert (c == d); assert (!(c != d));
           b <<= Any();  assert (a != b); assert (!(a == b));  c <<= a; d <<= b; assert (c != d); assert (!(c == d));
  a <<= Any();           assert (a == b); assert (!(a != b));  c <<= a; d <<= b; assert (c == d); assert (!(c != d));
  a <<= 13;  b <<= 13;   assert (a == b); assert (!(a != b));  c <<= a; d <<= b; assert (c == d); assert (!(c != d));
  a <<= 14;  b <<= 15;   assert (a != b); assert (!(a == b));  c <<= a; d <<= b; assert (c != d); assert (!(c == d));
  a <<= "1"; b <<= "1";  assert (a == b); assert (!(a != b));  c <<= a; d <<= b; assert (c == d); assert (!(c != d));
  a <<= "1"; b <<= 1;    assert (a != b); assert (!(a == b));  c <<= a; d <<= b; assert (c != d); assert (!(c == d));
  a <<= 1.4; b <<= 1.5;  assert (a != b); assert (!(a == b));  c <<= a; d <<= b; assert (c != d); assert (!(c == d));
  a <<= 1.6; b <<= 1.6;  assert (a == b); assert (!(a != b));  c <<= a; d <<= b; assert (c == d); assert (!(c != d));
  printf ("  TEST   Aida Any equality                                               OK\n");
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
