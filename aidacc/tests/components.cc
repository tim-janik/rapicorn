// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#include <stdio.h>
#include "components-a1-server.hh"
#include "components-a1-client.hh"
#include "a1-server.cc"

using namespace Rapicorn;

// == data samples ==
enum Plain {
  P0 = 0,
  P88 = 88
};

struct Simple {
  int   i1;
  Plain p2;
  Simple (int i, Plain p) : i1 (i), p2 (p) {}
  template<class Visitor> void
  __accept__ (Visitor _visitor_)
  {
    _visitor_ (i1, "int1");
    _visitor_ (p2, "enum2");
  }
};

static void
fill_simple_data_pack (A1::SimpleDataPack &simple)
{
  simple.vbool = true;
  simple.vi32 = 32;
  simple.vi64t = 64;
  simple.vf64 = 123456789e-11;
  simple.vstr = "donkeykong";
  simple.count = A1::THREE;
  simple.location.x = -3;
  simple.location.y = -3;
  simple.strings.push_back ("ABCDEFGHI");
  simple.strings.push_back ("jklmnopqr");
  simple.strings.push_back ("STUVWXYZ");
}

static void
fill_big_data_pack (A1::BigDataPack &big)
{
  const double TstNAN = 0 ? NAN : 909.909; // "NAN" is useful to test but breaks assert (in == out)
  fill_simple_data_pack (big.simple_pack);
  std::vector<bool> bools = { false, false, false, true, true, true, false, false, false };
  bools.swap (big.bools);
  std::vector<int32_t> ints32 = { -3, -2, -1, 0, 1, 2, 3 };
  ints32.swap (big.ints32);
  std::vector<int64_t> ints64 = { -1, 0x11111111deadbeefLL, -2 };
  ints64.swap (big.ints64);
  std::vector<double> floats = { -TstNAN, -INFINITY, -5.5, -0.75, -0, +0, +0.25, +1.25, +INFINITY, +TstNAN };
  floats.swap (big.floats);
  std::vector<A1::CountEnum> counts = { A1::THREE, A1::TWO, A1::ONE };
  counts.swap (big.counts);
  A1::Location l;
  l.x = -3; l.y = -5; big.locations.push_back (l);
  l.x = -1; l.y = -2; big.locations.push_back (l);
  l.x =  0; l.y =  0; big.locations.push_back (l);
  l.x = +1; l.y = +1; big.locations.push_back (l);
  l.x = +9; l.y = +2; big.locations.push_back (l);
  // any1;
  // anys;
  // objects;
  // other;
  // derived;
}

// == struct utilities ==
template<class Struct>
static String
struct_to_ini (Struct &s, const String &name)
{
  IniWriter iniwriter;
  ToIniVisitor writer_visitor (iniwriter, name + ".");
  s.__accept__ (writer_visitor);
  return iniwriter.output();
}

template<class Struct>
static bool
struct_from_ini (Struct &s, const String &name, const String &inidata)
{
  IniFile inifile ("-", inidata);
  if (!inifile.has_sections())
    return false;
  FromIniVisitor fromini_visitor (inifile, name + ".");
  s.__accept__ (fromini_visitor);
  return true;
}

template<class Struct>
static String
struct_to_xml (Struct &s, const String &name)
{
  XmlNodeP xnode = XmlNode::create_parent (name, 0, 0, "");
  ToXmlVisitor toxml_visitor (xnode);
  s.__accept__ (toxml_visitor);
  return xnode->xml_string (0, true);
}

template<class Struct>
static bool
struct_from_xml (Struct &s, const String &xmldata)
{
  MarkupParser::Error xerror;
  if (xerror.code != MarkupParser::NONE)
    return false;
  XmlNodeP xroot = XmlNode::parse_xml ("-", xmldata.c_str(), xmldata.size(), &xerror, "");
  if (xerror.code != MarkupParser::NONE)
    return false;
  FromXmlVisitor fromxml_visitor (xroot);
  s.__accept__ (fromxml_visitor);
  return true;
}

// == main ==
int
main (int   argc,
      char *argv[])
{
  bool ok;

  // rich data type test setup
  A1::BigDataPack big;
  fill_big_data_pack (big);
  switch (0)
    {
    case 1: printout ("%s\n", struct_to_xml (big, "BigDataPack")); break;
    case 2: printout ("%s", struct_to_ini (big, "BigDataPack")); break;
    case 0: ;
    }

  // test rich data type conversion to and from INI
  {
    String iniout = struct_to_ini (big, "BigDataPack");
    A1::BigDataPack b2;
    assert (big != b2);
    ok = struct_from_ini (b2, "BigDataPack", iniout);
    assert (ok);
    assert (big == b2);
  }

  // test rich data type conversion to and from XML
  {
    String xmlout = struct_to_xml (big, "BigDataPack");
    A1::BigDataPack b2;
    assert (big != b2);
    ok = struct_from_xml (b2, xmlout);
    assert (ok);
    assert (big == b2);
  }

  {
    Any any = big.__aida_to_any__();
    A1::BigDataPack buck;
    buck.__aida_from_any__ (any);
    assert (big == buck);
    if (0)
      printerr ("Big->Buck via Any:\n%s\n", struct_to_xml (buck, "BigDataPack"));
  }

  // test non-introspectible enums
  Simple s1 (111, P88);
  {
    Simple s2 (0, P0);
    assert (s1.i1 != s2.i1 && s1.p2 != s2.p2);
    ok = struct_from_xml (s2, struct_to_xml (s1, "Simple"));
    assert (ok);
    assert (s1.i1 == s2.i1 && s1.p2 == s2.p2);
  }
  {
    Simple s2 (0, P0);
    assert (s1.i1 != s2.i1 && s1.p2 != s2.p2);
    ok = struct_from_ini (s2, "Simple", struct_to_ini (s1, "Simple"));
    assert (ok);
    assert (s1.i1 == s2.i1 && s1.p2 == s2.p2);
  }

  // test compiling property visitor
  if (0) // code wouldn't work, since there's no remote A1::Richie instance
    {
      A1::RichieIface &richie_iface = *(A1::RichieIface*) NULL;
      std::vector<Parameter> params;
      Parameter::ListVisitor av (params);
      richie_iface.__accept_accessor__ (av);
      A1::RichieH richie_handle;
      richie_handle.__accept_accessor__ (av);
      Rapicorn::Aida::Any any = richie_iface.__aida_get__ ("");
      bool b2 = richie_iface.__aida_set__ ("", any);
      std::vector<std::string> aux = richie_iface.__aida_dir__();
      any = richie_handle.__aida_get__ ("");
      b2 = richie_handle.__aida_set__ ("", any);
      aux = richie_handle.__aida_dir__();
    }

  test_a1_server();

  printout ("  %-8s %-60s  %s\n", "TEST", argv[0], "OK");
  return 0;
}

#include "components-a1-client.cc"
#include "components-a1-server.cc"
