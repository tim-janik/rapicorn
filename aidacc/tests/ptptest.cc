// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html

// include AidaTypeMap Parser
#include <rapicorn-core.hh>
#include <cstring>
#include <cassert>
#include "typecodetests-api.cc"

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
  TypeCode tcseq = TypeMap::lookup ("Aida::DynamicSequence");
  assert (tcseq.kind_name() == "SEQUENCE");
  assert (tcseq.kind() == SEQUENCE);
  assert (tcseq.name() == "Aida::DynamicSequence");
  TypeCode tcrec = TypeMap::lookup ("Aida::DynamicRecord");
  assert (tcrec.kind_name() == "RECORD");
  assert (tcrec.kind() == RECORD);
  assert (tcrec.name() == "Aida::DynamicRecord");
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
    assert (t.enum_count() == 10);
    EnumValue ev;
    ev = t.enum_value (0); assert (ev.ident == String ("ENUM_VALUE_0") && ev.value == 0);
    ev = t.enum_value (1); assert (ev.ident == String ("ENUM_VALUE_1") && ev.value == 1);
    ev = t.enum_value (2); assert (ev.ident == String ("ENUM_VALUE__2") && ev.value == -2);
    ev = t.enum_value (3); assert (ev.ident == String ("ENUM_VALUE_4294967295") && ev.value == 4294967295);
    ev = t.enum_value (4); assert (ev.ident == String ("ENUM_VALUE_4294967296") && ev.value == 4294967296);
    ev = t.enum_value (5); assert (ev.ident == String ("ENUM_VALUE__4294967296") && ev.value == -4294967296);
    ev = t.enum_value (6); assert (ev.ident == String ("ENUM_VALUE_9223372036854775807") && ev.value == 9223372036854775807);
    ev = t.enum_value (7); assert (ev.ident == String ("ENUM_VALUE__9223372036854775808") && uint64_t (ev.value) == -9223372036854775808U);
    ev = t.enum_value (8); assert (ev.ident == String ("ENUM_VALUE_9223372036854775808") && uint64_t (ev.value) == 9223372036854775808U);
    ev = t.enum_value (9); assert (ev.ident == String ("ENUM_VALUE_18446744073709551615") && uint64_t (ev.value) == 18446744073709551615U);
    ev = t.enum_find (-4294967296); assert (ev.ident == String ("ENUM_VALUE__4294967296"));
    ev = t.enum_find (-9223372036854775808U); assert (ev.ident == String ("ENUM_VALUE__9223372036854775808"));
    ev = t.enum_find (18446744073709551615U); assert (ev.ident == String ("ENUM_VALUE_18446744073709551615"));
    ev = t.enum_find ("ENUM_VALUE_1"); assert (ev.value == 1);
    ev = t.enum_find ("ENUM_VALUE__2"); assert (ev.value == -2);
    ev = t.enum_find ("ENUM_VALUE_9223372036854775807"); assert (ev.value == 9223372036854775807);
    ev = t.enum_find ("ENUM_VALUE_18446744073709551615"); assert (uint64_t (ev.value) == 18446744073709551615U);
    ev = t.enum_find ("__2"); assert (ev.value == -2);
    ev = t.enum_find ("VALUE--2"); assert (ev.value == -2);
    ev = t.enum_find ("-VALUE--2"); assert (ev.value == -2);
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
    uint i = 0;
    TypeCode f = t.field (i++);
    assert (f.kind() == BOOL);
    assert (f.name() == "b1");
    f = t.field (i++);
    assert (f.kind() == BOOL);
    assert (f.name() == "b2");
    f = t.field (i++);
    assert (f.kind() == INT32);
    assert (f.name() == "int3");
    f = t.field (i++);
    assert (f.kind() == INT64);
    assert (f.name() == "int6");
    f = t.field (i++);
    assert (f.kind() == FLOAT64);
    assert (f.name() == "floatfield");
    f = t.field (i++);
    assert (f.kind() == STRING);
    assert (f.name() == "stringfield");
    f = t.field (i++);
    assert (f.name() == "enumfield");
    assert (f.kind() == TYPE_REFERENCE);
    assert (t.field_count() == i);
    TypeCode e = TypeMap::lookup (f.origin());
    assert (e.kind() == ENUM);
    assert (e.name() == "AidaTests::EnumType");
  }
  // done
  printf ("  TEST   Aida IDL type codes                                             OK\n");
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
    case 13: a <<= EnumValue (-0xc0ffeec0ffeeLL); break;
#define ANY_TEST_COUNT    14
    }
}

static bool
any_test_get (const Any &a, int what)
{
  std::string s;
  EnumValue e;
  switch (what)
    {
      typedef unsigned char uchar;
      bool b; char c; uchar uc; int i; uint ui; long l; ulong ul; int64_t i6; uint64_t u6; double d; const Any *p;
    case 0:  assert (a >>= b);	assert (b == 0); break;
    case 1:  assert (a >>= b);	assert (b == true); break;
    case 2:  assert (a >>= c);	assert (c == -117); break;
    case 3:  assert (a >>= uc);	assert (uc == 250); break;
    case 4:  assert (a >>= i);	assert (i == -134217728); break;
    case 5:  assert (a >>= ui);	assert (ui == 4294967295U); break;
    case 6:  assert (a >>= l);	assert (l == -2147483648); break;
    case 7:  assert (a >>= ul);	assert (ul == 4294967295U); break;
    case 8:  assert (a >>= i6);	assert (i6 == -0xc0ffeec0ffeeLL); break;
    case 9:  assert (a >>= u6);	assert (u6 == 0xffffffffffffffffULL); break;
    case 10: assert (a >>= s);	assert (s == "Test4test"); break;
    case 11: assert (a >>= d);	assert (d = test_double_value); break;
    case 12: assert (a >>= p);  assert (*p >>= s);       assert (s == "SecondAny"); break;
    case 13: assert (a >>= e);	assert (e.value == -0xc0ffeec0ffeeLL); break;
    }
  return true;
}

static SmartHandle
generate_broken_smart_handle (uint64_t orbid)
{
  FieldBuffer8 fb (1);
  fb.add_object (orbid);
  FieldReader fbr (fb);
  SmartMember<SmartHandle> sh;
  assert (sh._orbid() == 0);
  ObjectBroker::pop_handle (fbr, sh);
  assert (sh._orbid() == orbid);
  return sh;
}

static void
test_records ()
{
  AidaTests::SimpleRecord sr;
  sr.b1 = 1;
  sr.b2 = 0;
  sr.int3 = 3;
  sr.int6 = -6;
  sr.floatfield = 3.3;
  sr.stringfield = "two";
  sr.enumfield = AidaTests::ENUM_VALUE_1;
  FieldBuffer8 fb8;
  fb8 <<= sr;
  AidaTests::SimpleRecord sq;
  assert (sq != sr);
  FieldReader fbr (fb8);
  fbr >>= sq;
  assert (sq == sr);
  printf ("  TEST   Aida Record tests                                               OK\n");

  // SimpleRecord to Any
  Any any1;
  any1 <<= sr;
  AidaTests::SimpleRecord s2;
  s2 <<= any1;
  assert (s2 == sr);
  // ComboRecord to Any
  AidaTests::ComboRecord cr;
  cr.simple_rec = sr;
  cr.any_field <<= "STRING";
  SmartHandle sh = generate_broken_smart_handle (777); // handle doesn't work, but has an _orbid to test Any
  cr.empty_object = *(AidaTests::EmptyH*) (void*) &sh;
  assert (cr.simple_rec.stringfield == "two");
  assert (cr.simple_rec.int6 == -6);
  assert (cr.any_field.as_string() == "STRING");
  assert (cr.empty_object._orbid() == 777);
  any1 <<= cr;
  AidaTests::ComboRecord c2;
  assert (c2 != cr && c2.simple_rec != cr.simple_rec && c2.any_field != cr.any_field);
  c2 <<= any1;
  assert (c2 == cr && c2.simple_rec == cr.simple_rec && c2.any_field == cr.any_field && c2.empty_object._orbid() == 777);
  printf ("  TEST   Aida Record to Any                                              OK\n");
}

static void
test_any()
{
  String s;
  const size_t cases = ANY_TEST_COUNT;
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
  a <<= bool (0);       assert (a.kind() == BOOL && a.as_int() == 0);
  a <<= bool (1);       assert (a.kind() == BOOL && a.as_int() == 1);
  a <<= 1.;             assert (a.kind() == FLOAT64 && a.as_float() == +1.0);
  a <<= -1.;            assert (a.kind() == FLOAT64 && a.as_float() == -1.0);
  a <<= 16.5e+6;        assert (a.as_float() > 16000000.0 && a.as_float() < 17000000.0);
  a <<= 1;              assert (a.kind() == INT32 && a.as_int() == 1 && a.as_float() == 1 && a.as_string() == "1");
  a <<= -1;             assert (a.kind() == INT32 && a.as_int() == -1 && a.as_float() == -1 && a.as_string() == "-1");
  a <<= int64_t (1);    assert (a.kind() == INT64 && a.as_int() == 1 && a.as_float() == 1 && a.as_string() == "1");
  a <<= int64_t (-1);   assert (a.kind() == INT64 && a.as_int() == -1 && a.as_float() == -1 && a.as_string() == "-1");
  a <<= 0;              assert (a.kind() == INT32 && a.as_int() == 0 && a.as_float() == 0 && a.as_string() == "0");
  a <<= 32767199;       assert (a.kind() == INT32 && a.as_int() == 32767199);
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

static void
test_dynamics()
{
  // -- FieldVector --
  Any::FieldVector fv;
  fv.push_back (Any::Field ("otto", 7.7));
  fv.push_back (Any::Field ("anna", 3));
  fv.push_back (Any::Field ("ida", "ida"));
  assert (fv[0].name == "otto" && fv[0].as_float() == 7.7);
  assert (fv[1].name == "anna" && fv[1].as_int() == 3);
  assert (fv[2].name == "ida" && fv[2].as_string() == "ida");
  Any::FieldVector gv = fv;
  assert (fv == gv);
  gv[1] <<= 5;
  assert (fv != gv);
  gv[1] <<= 3;
  assert (fv == gv);
  // -- AnyVector --
  Any::AnyVector av;
  av.push_back (Any (7.7));
  av.push_back (Any (3));
  av.push_back (Any ("ida"));
  assert (av[0].as_float() == 7.7);
  assert (av[1].as_int() == 3);
  assert (av[2].as_string() == "ida");
  Any::AnyVector bv;
  assert (av != bv);
  for (auto const &f : fv)
    bv.push_back (f);
  assert (av == bv);
  // -- FieldVector & AnyVector --
  if (0)        // compare av (DynamicSequence) with fv (DynamicRecord)
    Rapicorn::printerr ("test-compare: %s == %s\n", Any (av).to_string().c_str(), Any (fv).to_string().c_str());
  Any::AnyVector cv (fv.begin(), fv.end());     // initialize AnyVector with { 7.7, 3, "ida" } from FieldVector (Field is-a Any)
  assert (av == cv);                            // as AnyVector (FieldVector) copy, both vectors contain { 7.7, 3, "ida" }
  // -- Any::DynamicSequence & Any::DynamicRecord --
  Any arec (fv), aseq (av);
  assert (arec != aseq);
  const Any::FieldVector *arv;
  arec >>= arv;
  assert (*arv == fv);
  const Any::AnyVector *asv;
  aseq >>= asv;
  assert (*asv == av);
  printf ("  TEST   Aida FieldVector & AnyVector                                    OK\n");
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
          test_dynamics();
          test_records();
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
