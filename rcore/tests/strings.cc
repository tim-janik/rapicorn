// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#include <rcore/testutils.hh>
#include <rcore/randomhash.hh>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <glib.h>
#include <random>
using namespace Rapicorn;

namespace {
using namespace Rapicorn;

static void
random_utf8_and_unichar_test (ptrdiff_t count)
{
  for (uint i = 0; i < count; i++)
    {
      unichar nc, uc = rand() % (0x100 << (i % 24));
      if (!uc)
        continue;
      char buffer[8], gstr[10] = { 0, };
      int l;

      l = utf8_from_unichar (uc, buffer);
      TCMP (l, >, 0);
      TCMP (l, <=, 7);
      TCMP (buffer[l], ==, 0);
      TCMP (l, ==, g_unichar_to_utf8 (uc, gstr));
      TCMP (strcmp (gstr, buffer), ==, 0);
      nc = utf8_to_unichar (buffer);
      TCMP (nc, ==, g_utf8_get_char (buffer));
      TCMP (uc, ==, nc);
      TCMP (l, ==, utf8_from_unichar (uc, NULL));
      char *p1 = utf8_next (buffer);
      TCMP (p1, ==, buffer + l);
      char *p2 = utf8_prev (p1);
      TCMP (p2, ==, buffer);

      char cbuffer[1024];
      snprintf (cbuffer, 1024, "x%sy7", buffer);
      char *cur = cbuffer, *pn, *gn, *pp;
      /* x */
      pn = utf8_find_next (cur);
      TCMP (pn, ==, cur + 1);
      gn = g_utf8_find_next_char (cur, NULL);
      TCMP (pn, ==, gn);
      pp = utf8_find_prev (cbuffer, pn);
      TCMP (pp, ==, cur);
      /* random unichar */
      cur = pn;
      pn = utf8_find_next (cur);
      TCMP (pn, ==, cur + l);
      gn = g_utf8_find_next_char (cur, NULL);
      TCMP (pn, ==, gn);
      pp = utf8_find_prev (cbuffer, pn);
      TCMP (pp, ==, cur);
      /* y */
      cur = pn;
      pn = utf8_find_next (cur);
      TCMP (pn, ==, cur + 1);
      gn = g_utf8_find_next_char (cur, NULL);
      TCMP (pn, ==, gn);
      pp = utf8_find_prev (cbuffer, pn);
      TCMP (pp, ==, cur);
      /* 7 (last) */
      cur = pn;
      pn = utf8_find_next (cur);
      TCMP (pn, ==, cur + 1);
      gn = g_utf8_find_next_char (cur, NULL);
      TCMP (pn, ==, gn);
      pp = utf8_find_prev (cbuffer, pn);
      TCMP (pp, ==, cur);
      /* last with bounds */
      pn = utf8_find_next (cur, cur + strlen (cur));
      TCMPS (pn, ==, NULL);
      gn = g_utf8_find_next_char (cur, cur + strlen (cur));
      TCMP (pn, ==, gn);
      /* first with bounds */
      pp = utf8_find_prev (cbuffer, cbuffer);
      TCMPS (pp, ==, NULL);

      /* validate valid UTF-8 */
      bool bb = utf8_validate (cbuffer);
      bool gb = g_utf8_validate (cbuffer, -1, NULL);
      TCMP (bb, ==, gb);
      /* validate invalid UTF-8 */
      cbuffer[rand() % (l + 3)] = rand();
      const char *gp;
      int indx;
      bb = utf8_validate (cbuffer, &indx);
      gb = g_utf8_validate (cbuffer, -1, &gp);
      TCMP (bb, ==, gb);
      if (!bb)
        TCMP (cbuffer + indx, ==, gp);
    }
}
REGISTER_TEST ("Strings/random UTF8", random_utf8_and_unichar_test, 30000);
REGISTER_SLOWTEST ("Strings/random UTF8 (slow)", random_utf8_and_unichar_test, 1000000);

#define UC_CMP(uc,a,eq,b)       do { if (a != b) printerr ("unichar(0x%08x): ", uc); TCMP (a, eq, b); } while (0)

static void
random_unichar_test (ptrdiff_t count)
{
  for (uint i = 0; i < count; i++)
    {
      unichar uc;
      do
        uc = rand() % (0x100 << (i % 24));
      while ((uc & 0xffff) == 0xffff || // filter some questionable discrepancies
             (uc & 0xfd00) == 0xfd00);
      unichar bc, gc;
      gboolean gb;
      bool bb;
      int bv, gv;

      bb = Unicode::isvalid (uc);
      gb = g_unichar_validate (uc);
      UC_CMP (uc, bb, ==, gb);
      bb = Unicode::isalnum (uc);
      gb = g_unichar_isalnum (uc);
      UC_CMP (uc, bb, ==, gb);
      bb = Unicode::isalpha (uc);
      gb = g_unichar_isalpha (uc);
      UC_CMP (uc, bb, ==, gb);
      bb = Unicode::iscntrl (uc);
      gb = g_unichar_iscntrl (uc);
      UC_CMP (uc, bb, ==, gb);
      bb = Unicode::isdigit (uc);
      gb = g_unichar_isdigit (uc);
      UC_CMP (uc, bb, ==, gb);
      bv = Unicode::digit_value (uc);
      gv = g_unichar_digit_value (uc);
      UC_CMP (uc, bv, ==, gv);
      bv = Unicode::digit_value ('0' + uc % 10);
      gv = g_unichar_digit_value ('0' + uc % 10);
      UC_CMP (uc, bv, ==, gv);
      bb = Unicode::isgraph (uc);
      gb = g_unichar_isgraph (uc);
      UC_CMP (uc, bv, ==, gv);
      bb = Unicode::islower (uc);
      gb = g_unichar_islower (uc);
      UC_CMP (uc, bb, ==, gb);
      bc = Unicode::tolower (uc);
      gc = g_unichar_tolower (uc);
      UC_CMP (uc, bc, ==, gc);
      bb = Unicode::isprint (uc);
      gb = g_unichar_isprint (uc);
      UC_CMP (uc, bb, ==, gb);
      bb = Unicode::ispunct (uc);
      gb = g_unichar_ispunct (uc);
      UC_CMP (uc, bb, ==, gb);
      bb = Unicode::isspace (uc);
      gb = g_unichar_isspace (uc);
      UC_CMP (uc, bb, ==, gb);
      bb = Unicode::isupper (uc);
      gb = g_unichar_isupper (uc);
      UC_CMP (uc, bb, ==, gb);
      bc = Unicode::toupper (uc);
      gc = g_unichar_toupper (uc);
      UC_CMP (uc, bc, ==, gc);
      bb = Unicode::isxdigit (uc);
      gb = g_unichar_isxdigit (uc);
      UC_CMP (uc, bb, ==, gb);
      bv = Unicode::xdigit_value (uc);
      gv = g_unichar_xdigit_value (uc);
      UC_CMP (uc, bv, ==, gv);
      bv = Unicode::xdigit_value ('0' + uc % 10);
      gv = g_unichar_xdigit_value ('0' + uc % 10);
      UC_CMP (uc, bv, ==, gv);
      bv = Unicode::xdigit_value ('a' + uc % 6);
      gv = g_unichar_xdigit_value ('a' + uc % 6);
      UC_CMP (uc, bv, ==, gv);
      bv = Unicode::xdigit_value ('A' + uc % 6);
      gv = g_unichar_xdigit_value ('A' + uc % 6);
      UC_CMP (uc, bv, ==, gv);
      bb = Unicode::istitle (uc);
      gb = g_unichar_istitle (uc);
      UC_CMP (uc, bb, ==, gb);
      bc = Unicode::totitle (uc);
      gc = g_unichar_totitle (uc);
      UC_CMP (uc, bc, ==, gc);
      bb = Unicode::isdefined (uc);
      gb = g_unichar_isdefined (uc);
      UC_CMP (uc, bb, ==, gb);
      bb = Unicode::iswide (uc);
      gb = g_unichar_iswide (uc);
      UC_CMP (uc, bb, ==, gb);
#if GLIB_CHECK_VERSION (2, 12, 0)
      bb = Unicode::iswide_cjk (uc);
      gb = g_unichar_iswide_cjk (uc);
      UC_CMP (uc, bb, ==, gb);
#endif
      TCMP (Unicode::get_type (uc), ==, (int) g_unichar_type (uc));
      TCMP (Unicode::get_break (uc), ==, (int) g_unichar_break_type (uc));
    }
}
REGISTER_TEST ("Strings/random unichar", random_unichar_test, 30000);
REGISTER_SLOWTEST ("Strings/random unichar (slow)", random_unichar_test, 1000000);

static void
unichar_noncharacter_tests ()
{
  unichar uc;
  bool tt;
  // U+FDD0..U+FDEF
  uc = 0xfdcf;  tt = Unicode::isnoncharacter (uc);  UC_CMP (uc, tt, ==, false);
  for (uc = 0xfdd0; uc <= 0xfdef; uc++)
    {
      tt = Unicode::isnoncharacter (uc); UC_CMP (uc, tt, ==, true);
    }
  uc = 0xfdf0;  tt = Unicode::isnoncharacter (uc);  UC_CMP (uc, tt, ==, false);
  // U+nFFFE and U+nFFFF
  const unichar ncs[] = { 0x00fffe, 0x01fffe, 0x02fffe, 0x03fffe, 0x04fffe, 0x05fffe, 0x06fffe, 0x07fffe, 0x08fffe,
                          0x09fffe, 0x0afffe, 0x0bfffe, 0x0cfffe, 0x0dfffe, 0x0efffe, 0x0ffffe, 0x10fffe };
  for (uint i = 0; i < ARRAY_SIZE (ncs); i++)
    {
      uc = ncs[i] - 1;  // U+nFFFD
      tt = Unicode::isnoncharacter (uc); UC_CMP (uc, tt, ==, false);
      uc = ncs[i];      // U+nFFFE
      tt = Unicode::isnoncharacter (uc); UC_CMP (uc, tt, ==, true);
      uc = ncs[i] + 1;  // U+nFFFF
      tt = Unicode::isnoncharacter (uc); UC_CMP (uc, tt, ==, true);
      uc = ncs[i] + 2;  // U+nFFFF + 1
      tt = Unicode::isnoncharacter (uc); UC_CMP (uc, tt, ==, false);
    }
  uc = 0x11fffe; tt = Unicode::isnoncharacter (uc);  UC_CMP (uc, tt, ==, false);
  uc = 0x20ffff; tt = Unicode::isnoncharacter (uc);  UC_CMP (uc, tt, ==, false);
}
REGISTER_TEST ("Strings/noncharacter unichar", unichar_noncharacter_tests);

static void
uuid_tests (void)
{
  /* uuid string test */
  TCMP (string_is_uuid ("00000000-0000-0000-0000-000000000000"), ==, true);
  TCMP (string_is_uuid ("6ba7b812-9dad-11d1-80b4-00c04fd430c8"), ==, true);
  TCMP (string_is_uuid ("6BA7B812-9DAD-11D1-80B4-00C04FD430C8"), ==, true);
  TCMP (string_is_uuid ("a425fd92-4f06-11db-aea9-000102e7e309"), ==, true);
  TCMP (string_is_uuid ("A425FD92-4F06-11DB-AEA9-000102E7E309"), ==, true);
  TCMP (string_is_uuid ("dc380602-a739-4be1-a5cb-53c437ffe39f"), ==, true);
  TCMP (string_is_uuid ("DC380602-A739-4BE1-A5CB-53C437FFE39F"), ==, true);
  // TCMP (string_is_uuid (NULL), ==, false);
  TCMP (string_is_uuid (""), ==, false);
  TCMP (string_is_uuid ("gba7b812-9dad-11d1-80b4-00c04fd430c8"), ==, false);
  TCMP (string_is_uuid ("Gba7b812-9dad-11d1-80b4-00c04fd430c8"), ==, false);
  TCMP (string_is_uuid ("6ba7b812.9dad-11d1-80b4-00c04fd430c8"), ==, false);
  TCMP (string_is_uuid ("6ba7b812-9dad.11d1-80b4-00c04fd430c8"), ==, false);
  TCMP (string_is_uuid ("6ba7b812-9dad-11d1.80b4-00c04fd430c8"), ==, false);
  TCMP (string_is_uuid ("6ba7b812-9dad-11d1-80b4.00c04fd430c8"), ==, false);
  TCMP (string_is_uuid ("6ba7b812-9dad-11d1-80b4-00c04fd430c8-"), ==, false);
  TCMP (string_is_uuid ("6ba7b812-9dad-11d1-80b4-00c04fd430c80"), ==, false);
  TCMP (string_is_uuid ("6ba7b812-9dad-11d1-80b4-00c04fd430c"), ==, false);
  /* uuid string cmp */
  TCMP (string_cmp_uuid ("00000000-0000-0000-0000-000000000000", "A425FD92-4F06-11DB-AEA9-000102E7E309"), <, 0);
  TCMP (string_cmp_uuid ("A425FD92-4F06-11DB-AEA9-000102E7E309", "00000000-0000-0000-0000-000000000000"), >, 0);
  TCMP (string_cmp_uuid ("00000000-0000-0000-0000-000000000000", "6ba7b812-9dad-11d1-80b4-00c04fd430c8"), <, 0);
  TCMP (string_cmp_uuid ("6BA7B812-9DAD-11D1-80B4-00C04FD430C8", "00000000-0000-0000-0000-000000000000"), >, 0);
  TCMP (string_cmp_uuid ("00000000-0000-0000-0000-000000000000", "00000000-0000-0000-0000-000000000000"), ==, 0);
  TCMP (string_cmp_uuid ("6BA7B812-9DAD-11D1-80B4-00C04FD430C8", "A425FD92-4F06-11DB-AEA9-000102E7E309"), <, 0);
  TCMP (string_cmp_uuid ("6ba7b812-9dad-11d1-80b4-00c04fd430c8", "A425FD92-4F06-11DB-AEA9-000102E7E309"), <, 0);
  TCMP (string_cmp_uuid ("6BA7B812-9DAD-11D1-80B4-00C04FD430C8", "a425fd92-4f06-11db-aea9-000102e7e309"), <, 0);
  TCMP (string_cmp_uuid ("6ba7b812-9dad-11d1-80b4-00c04fd430c8", "a425fd92-4f06-11db-aea9-000102e7e309"), <, 0);
  TCMP (string_cmp_uuid ("A425FD92-4F06-11DB-AEA9-000102E7E309", "a425fd92-4f06-11db-aea9-000102e7e309"), ==, 0);
  TCMP (string_cmp_uuid ("6ba7b812-9DAD-11d1-80B4-00c04fd430c8", "6BA7B812-9dad-11D1-80b4-00C04FD430C8"), ==, 0);
  TCMP (string_cmp_uuid ("A425FD92-4F06-11DB-AEA9-000102E7E309", "6BA7B812-9DAD-11D1-80B4-00C04FD430C8"), >, 0);
  TCMP (string_cmp_uuid ("A425FD92-4F06-11DB-AEA9-000102E7E309", "6ba7b812-9dad-11d1-80b4-00c04fd430c8"), >, 0);
  TCMP (string_cmp_uuid ("a425fd92-4f06-11db-aea9-000102e7e309", "6BA7B812-9DAD-11D1-80B4-00C04FD430C8"), >, 0);
  TCMP (string_cmp_uuid ("a425fd92-4f06-11db-aea9-000102e7e309", "6ba7b812-9dad-11d1-80b4-00c04fd430c8"), >, 0);
}
REGISTER_TEST ("Strings/UUID", uuid_tests);

static void
string_conversions (void)
{
  TCMP (string_from_pretty_function_name ("Anon::Type::Type(int)"), ==, "Anon::Type::Type");
  TCMP (string_from_pretty_function_name ("void (** (* Anon::Class::func())(void (*zoot)(int)))(int)"), ==, "Anon::Class::func");
  TCMP (string_to_cescape ("\""), ==, "\\\"");
  TCMP (string_to_cescape ("\""), ==, "\\\"");
  TCMP (string_to_cescape ("\\"), ==, "\\\\");
  TCMP (string_to_cescape ("\1"), ==, "\\001");
  TCMP (string_from_cquote ("''"), ==, "");
  TCMP (string_from_cquote ("\"\""), ==, "");
  TCMP (string_from_cquote ("\"\\1\""), ==, "\1");
  TCMP (string_from_cquote ("\"\\11\""), ==, "\t");
  TCMP (string_from_cquote ("\"\\111\""), ==, "I");
  TCMP (string_from_cquote ("\"\\1111\""), ==, "I1");
  TCMP (string_from_cquote ("\"\\7\\8\\9\""), ==, "\a89");
  TCMP (string_from_cquote ("'foo\"bar'"), ==, "foo\"bar");
  TCMP (string_from_cquote ("\"newline\\nreturn\\rtab\\tbell\\bformfeed\\fvtab\\vbackslash\\\\tick'end\""),
        ==,
        "newline\nreturn\rtab\tbell\bformfeed\fvtab\vbackslash\\tick'end");
  const char *quotetests[] = {
    "", "\"", "\\", "abcdefg\b\v\ffoo\"bar'baz'zonk\"end",
    "~\377\277\154\22\01", "null\000null",
  };
  for (uint i = 0; i < ARRAY_SIZE (quotetests); i++)
    if (string_from_cquote (string_to_cquote (quotetests[i])) != quotetests[i])
      fatal ("cquote inconsistency for \"%s\": %s -> %s", quotetests[i],
             string_to_cquote (quotetests[i]).c_str(),
             string_from_cquote (string_to_cquote (quotetests[i])).c_str());
  TCMP (string_substitute_char ("foo", '3', '4'), ==, "foo");
  TCMP (string_substitute_char ("foobar", 'o', '_'), ==, "f__bar");
  TCMP (string_to_double ("1.0"), ==, 1.0);
  TCMP (string_to_double ("0.5"), ==, 0.5);
  TCMP (string_to_double ("-1.0"), ==, -1.0);
  TCMP (string_to_double ("-0.5"), ==, -0.5);
  double tfloat;
  tfloat = string_to_double ("+NAN");
  assert (isnan (tfloat) && std::signbit (tfloat) == 0);
  tfloat = string_to_double ("-NAN");
  assert (isnan (tfloat) && std::signbit (tfloat) == 1);
  TCMP (string_capitalize ("fOO bar"), ==, "Foo Bar");
  TCMP (string_capitalize ("foo BAR BAZ", 2), ==, "Foo Bar BAZ");
}
REGISTER_TEST ("Strings/conversions", string_conversions);

static void
split_string_tests (void)
{
  StringVector sv;
  sv = string_split ("a;b;c", ";");
  TCMP (string_join ("//", sv), ==, "a//b//c");
  TCMP (string_join ("_", string_split ("a;b;c;", ";")), ==, "a_b_c_");
  TCMP (string_join ("_", string_split ("a;b;c;d;e", ";", 2)), ==, "a_b_c;d;e");
  TCMP (string_join ("_", string_split (";;a;b;c", ";")), ==, "__a_b_c");
  TCMP (string_join ("+", string_split (" a  b \n c \t\n\r\f\v d")), ==, "a+b+c+d");
  TCMP (string_join ("^", Path::searchpath_split ("one;two;three;;")), ==, "one^two^three");
  TCMP (string_join ("^", Path::searchpath_split (";one;two;three")), ==, "one^two^three");
#ifdef RAPICORN_OS_UNIX
  TCMP (string_join ("^", Path::searchpath_split (":one:two:three:")), ==, "one^two^three");
#endif
  TCMP (string_join ("", string_split ("all  white\tspaces\nwill\vbe\fstripped")), ==, "allwhitespaceswillbestripped");
  TCMP (string_multiply ("x", 0), ==, "");
  TCMP (string_multiply ("x", 1), ==, "x");
  TCMP (string_multiply ("x", 2), ==, "xx");
  TCMP (string_multiply ("x", 3), ==, "xxx");
  TCMP (string_multiply ("x", 4), ==, "xxxx");
  TCMP (string_multiply ("x", 5), ==, "xxxxx");
  TCMP (string_multiply ("x", 6), ==, "xxxxxx");
  TCMP (string_multiply ("x", 7), ==, "xxxxxxx");
  TCMP (string_multiply ("x", 8), ==, "xxxxxxxx");
  TCMP (string_multiply ("x", 9), ==, "xxxxxxxxx");
  TCMP (string_multiply ("x", 99999).size(), ==, 99999);
  TCMP (string_multiply ("x", 99998).size(), ==, 99998);
  TCMP (string_multiply ("x", 99997).size(), ==, 99997);
  TCMP (string_multiply ("x", 99996).size(), ==, 99996);
  TCMP (string_multiply ("x", 99995).size(), ==, 99995);
  TCMP (string_multiply ("x", 99994).size(), ==, 99994);
  TCMP (string_multiply ("x", 99993).size(), ==, 99993);
  TCMP (string_multiply ("x", 99992).size(), ==, 99992);
  TCMP (string_multiply ("x", 99991).size(), ==, 99991);
  TCMP (string_multiply ("x", 99990).size(), ==, 99990);
  TCMP (string_multiply ("x", 9999999).size(), ==, 9999999); // needs 10MB, should finish within 1 second
  sv = string_split_any ("a, b, c", ", ");
  TCMP (string_join (";", sv), ==, "a;;b;;c");
  string_vector_erase_empty (sv);
  TCMP (string_join (";", sv), ==, "a;b;c");
  sv = string_split_any ("a, b, c", ", ", 1);
  TCMP (string_join (";", sv), ==, "a; b, c");
  sv = string_split_any ("abcdef", "");
  TCMP (string_join (";", sv), ==, "a;b;c;d;e;f");
  string_vector_erase_empty (sv);
  TCMP (string_join (";", sv), ==, "a;b;c;d;e;f");
  sv = string_split_any ("abcdef", "", 2);
  TCMP (string_join (";", sv), ==, "a;b;cdef");
  sv = string_split_any ("  foo  , bar     , \t\t baz \n", ",");
  string_vector_lstrip (sv);
  TCMP (string_join (";", sv), ==, "foo  ;bar     ;baz \n");
  sv = string_split_any ("  foo  , bar     , \t\t baz \n", ",");
  string_vector_rstrip (sv);
  TCMP (string_join (";", sv), ==, "  foo; bar; \t\t baz");
  sv = string_split_any ("  foo  , bar     , \t\t baz \n", ",");
  string_vector_strip (sv);
  TCMP (string_join (" ", sv), ==, "foo bar baz");
}
REGISTER_TEST ("Strings/split strings", split_string_tests);

static void
test_string_charsets (void)
{
  String output;
  bool res;
  res = text_convert ("UTF-8", output, "ASCII", "Hello");
  assert (res);
  TCMP (output, ==, "Hello");
  /* latin1 <-> UTF-8 */
  String latin1_umlauts = "\xc4 \xd6 \xdc \xdf \xe4 \xf6 \xfc";
  String utf8_umlauts = "Ä Ö Ü ß ä ö ü";
  res = text_convert ("UTF-8", output, "LATIN1", latin1_umlauts);
  assert (res);
  TCMP (output, ==, utf8_umlauts);
  res = text_convert ("LATIN1", output, "UTF-8", utf8_umlauts);
  assert (res);
  TCMP (output, ==, latin1_umlauts);
  /* assert failinmg conversions */
  res = text_convert ("invalidNOTEXISTINGcharset", output, "UTF-8", "Hello", "");
  TCMP (res, ==, false);
  /* check fallback charset (ISO-8859-15) */
  res = text_convert ("UTF-8", output, "ASCII", "Euro: \xa4");
  assert (res);
  TCMP (output, ==, "Euro: \xe2\x82\xac");
  /* check alias LATIN9 -> ISO-8859-15 */
  res = text_convert ("UTF-8", output, "LATIN9", "9/15: \xa4", "");
  assert (res);
  TCMP (output, ==, "9/15: \xe2\x82\xac");
  /* check invalid-character marks */
  res = text_convert ("UTF-8", output, "ASCII", "non-ascii \xa4 char", "", "[?]");
  assert (res);
  TCMP (output, ==, "non-ascii [?] char");
}
REGISTER_TEST ("Strings/charsets", test_string_charsets);

static void
test_string_misc (void)
{
  TCMP (string_canonify ("", "", ""), ==, "");
  TCMP (string_canonify ("a", "", "+"), ==, "+");
  TCMP (string_canonify ("abc", "", "+"), ==, "+++");
  TCMP (string_canonify ("AbraCadaBra", string_set_A2Z(), "#+"), ==, "A#+#+#+C#+#+#+B#+#+");
  TCMP (string_canonify ("Foo123bar+baz", string_set_a2z() + string_set_A2Z(), "^"), ==, "Foo^^^bar^baz");
  TCMP (string_canonify ("#Foo_Frob", string_set_a2z() + string_set_A2Z(), "-"), ==, "-Foo-Frob");
  TCMP (string_canonify ("/usr/bin/ls", string_set_ascii_alnum(), "_"), ==, "_usr_bin_ls");
}
REGISTER_TEST ("Strings/Misc", test_string_misc);

static void
test_string_matching()
{
  assert (string_match_identifier ("foo", "foo") == true);
  assert (string_match_identifier ("foo", "ffoo") == false);
  assert (string_match_identifier ("foo", "oo") == false);
  assert (string_match_identifier ("foo", "bar") == false);
  assert (string_match_identifier ("FOO", "foo") == true);
  assert (string_match_identifier ("x.FOO", "X-Foo") == true);
  assert (string_match_identifier ("x.FOO", "Foo") == false);
  assert (string_match_identifier_tail ("x.FOO", "Foo") == true);
  assert (string_match_identifier_tail ("x.FOO", "-Foo") == true);
  assert (string_match_identifier_tail ("xFOO", "Foo") == false);
}
REGISTER_TEST ("Strings/Matching", test_string_matching);

static void
test_string_stripping (void)
{
  TCMP (string_strip (""), ==, "");
  TCMP (string_lstrip (""), ==, "");
  TCMP (string_rstrip (""), ==, "");
  TCMP (string_strip ("123"), ==, "123");
  TCMP (string_lstrip ("123"), ==, "123");
  TCMP (string_rstrip ("123"), ==, "123");
  TCMP (string_strip (" \t\v\f\n\r"), ==, "");
  TCMP (string_lstrip (" \t\v\f\n\r"), ==, "");
  TCMP (string_rstrip (" \t\v\f\n\r"), ==, "");
  TCMP (string_strip (" \t\v\f\n\rX \t\v\f\n\r"), ==, "X");
  TCMP (string_lstrip (" \t\v\f\n\rX \t\v\f\n\r"), ==, "X \t\v\f\n\r");
  TCMP (string_rstrip (" \t\v\f\n\rX \t\v\f\n\r"), ==, " \t\v\f\n\rX");
}
REGISTER_TEST ("Strings/stripping", test_string_stripping);

static void
test_string_options (void)
{
  TASSERT (string_option_get ("foo", "foo") == "1");
  TASSERT (string_option_get ("foo", "BAR") == "0");
  TASSERT (string_option_get (" foo ", "foo") == "1");
  TASSERT (string_option_get (" foo =7", "foo") == "7");
  TASSERT (string_option_get (" foo = 7 ", "foo") == " 7 ");
  TASSERT (string_option_get (" foo =7:bar ", "foo") == "7");
  TASSERT (string_option_get (" foo =7:bar ", "bar") == "1");
  TASSERT (string_option_get (":foo;;;;bar:::zonk:", "foo") == "1");
  TASSERT (string_option_get (":foo;;;;bar:::zonk:", "bar") == "1");
  TASSERT (string_option_get (":foo;;;;bar:::zonk:", "zonk") == "1");
  TASSERT (string_option_check (" foo ", "foo") == true);
  TASSERT (string_option_check (" foo9 ", "foo9") == true);
  TASSERT (string_option_check (" foo7 ", "foo9") == false);
  TASSERT (string_option_check (" bar ", "bar") == true);
  TASSERT (string_option_check (" bar= ", "bar") == true);
  TASSERT (string_option_check (" bar=0 ", "bar") == false);
  TASSERT (string_option_check (" bar=no ", "bar") == false);
  TASSERT (string_option_check (" bar=false ", "bar") == false);
  TASSERT (string_option_check (" bar=off ", "bar") == false);
  TASSERT (string_option_check (" bar=1 ", "bar") == true);
  TASSERT (string_option_check (" bar=2 ", "bar") == true);
  TASSERT (string_option_check (" bar=3 ", "bar") == true);
  TASSERT (string_option_check (" bar=4 ", "bar") == true);
  TASSERT (string_option_check (" bar=5 ", "bar") == true);
  TASSERT (string_option_check (" bar=6 ", "bar") == true);
  TASSERT (string_option_check (" bar=7 ", "bar") == true);
  TASSERT (string_option_check (" bar=8 ", "bar") == true);
  TASSERT (string_option_check (" bar=9 ", "bar") == true);
  TASSERT (string_option_check (" bar=09 ", "bar") == true);
  TASSERT (string_option_check (" bar=yes ", "bar") == true);
  TASSERT (string_option_check (" bar=true ", "bar") == true);
  TASSERT (string_option_check (" bar=on ", "bar") == true);
  TASSERT (string_option_check (" bar=1false ", "bar") == true);
  TASSERT (string_option_check (" bar=0true ", "bar") == false);
}
REGISTER_TEST ("Strings/String Options", test_string_options);

struct UncopyablePoint {
  double x, y;
  friend std::ostream&
  operator<< (std::ostream& stream, const UncopyablePoint &self)
  {
    stream << "{" << self.x << ";" << self.y << "}";
    return stream;
  }
  UncopyablePoint (double _x, double _y) : x (_x), y (_y) {}
  RAPICORN_CLASS_NON_COPYABLE (UncopyablePoint);
};

static void
test_string_functions()
{
  TASSERT (string_startswith ("foo", "fo") == true);
  TASSERT (string_startswith ("foo", "o") == false);
  TASSERT (string_endswith ("foo", "o") == true);
  TASSERT (string_endswith ("foo", "oo") == true);
  TASSERT (string_endswith ("foo", "foo") == true);
  TASSERT (string_endswith ("foo", "loo") == false);
  // string_hexdump
  String dump;
  const uint8 mem[] = { 0x00, 0x01, 0x02, 0x03, 0x34, 0x35, 0x36, 0x37,
                        0x63, 0x30, 0x66, 0x66, 0x65, 0x65, 0x2e, 0x20,
                        0xff, };
  const char *expect00 = "00000000  00 01 02 03 34 35 36 37  63 30 66 66 65 65 2e 20  |....4567c0ffee. |\n";
  const char *expect10 = "00000010  ff                                                |.|\n";
  dump = string_hexdump (mem, 16);      static_assert (sizeof (mem) >= 16, "!");
  TCMP (dump, ==, expect00);
  dump = string_hexdump (mem, sizeof (mem));
  TCMP (dump, ==, String() + expect00 + expect10);
}
REGISTER_TEST ("Strings/String Functions", test_string_functions);

static void
test_cxxprintf()
{
  // string_format
  enum { TEST17 = 17 };
  TCMP (string_format ("%d %s", -9223372036854775808uLL, "FOO"), ==, "-9223372036854775808 FOO");
  TCMP (string_format ("%g %d", 0.5, TEST17), ==, "0.5 17");
  TCMP (string_format ("0x%08x", 0xc0ffee), ==, "0x00c0ffee");
  static_assert (TEST17 == 17, "!");
  TCMP (string_format ("Only %c%%", '3'), ==, "Only 3%");
  UncopyablePoint point { 1, 2 };
  TCMP (string_format ("%s", point), ==, "{1;2}");
  String sfoo ("foo");
  typedef char MutableChar;
  MutableChar *foo = &sfoo[0];
  TCMP (string_format ("%s", foo), ==, "foo");
  // test robustness for arcane/rarely-used width modifiers
  TCMP (string_format ("| %qd %Zd %LF |", (long long) 1234, size_t (4321), (long double) 1234.), ==, "| 1234 4321 1234.000000 |");
  TCMP (string_format ("- %C - %lc -", long ('X'), long ('x')), ==, "- X - x -");
  // TCMP (string_format ("+ %S +", (wchar_t*) "\1\1\1\1\0\0\0\0"), ==, "+ \1\1\1\1 +");
}
REGISTER_TEST ("Strings/CxxPrintf", test_cxxprintf);

#define cxxoutput_printf(fmt,...)       fputs (string_format (fmt, __VA_ARGS__).c_str(), stdout)

static void
test_cxxprintf_output()
{
  // %c
  cxxoutput_printf ("Char 'Y'                  right-adjusted: |%22c|\n", 'Y');
  cxxoutput_printf ("Char 'Y'                   left-adjusted: |%-22c|\n", 'Y');
  cxxoutput_printf ("Char 'Y'     positional 0      truncated: |%*c|\n", 0, 'Y');
  // %u %i %o %x %X
  cxxoutput_printf ("decimal unsigned negative right-adjusted: |%22u|\n", -987654321);
  cxxoutput_printf ("decimal signed-i negative right-adjusted: |%22i|\n", -987654321);
  cxxoutput_printf ("decimal signed-d negative right-adjusted: |%22d|\n", -987654321);
  cxxoutput_printf ("octal   unsigned negative right-adjusted: |%22o|\n", -987654321);
  cxxoutput_printf ("hexadec unsigned negative right-adjusted: |%22x|\n", -987654321);
  cxxoutput_printf ("Hexadec unsigned negative right-adjusted: |%22X|\n", -987654321);
  cxxoutput_printf ("decimal unsigned negative  left-adjusted: |%-22u|\n", -987654321);
  cxxoutput_printf ("decimal signed-i negative  left-adjusted: |%-22i|\n", -987654321);
  cxxoutput_printf ("decimal signed-d negative  left-adjusted: |%-22d|\n", -987654321);
  cxxoutput_printf ("octal   unsigned negative  left-adjusted: |%-22o|\n", -987654321);
  cxxoutput_printf ("hexadec unsigned negative  left-adjusted: |%-22x|\n", -987654321);
  cxxoutput_printf ("Hexadec unsigned negative  left-adjusted: |%-22X|\n", -987654321);
  cxxoutput_printf ("decimal signed-i negative   space-padded: |% 22i|\n", -987654321);
  cxxoutput_printf ("decimal signed-d negative   space-padded: |% 22d|\n", -987654321);
  cxxoutput_printf ("decimal unsigned negative    zero-padded: |%022u|\n", -987654321);
  cxxoutput_printf ("decimal signed-i negative    zero-padded: |%022i|\n", -987654321);
  cxxoutput_printf ("decimal signed-d negative    zero-padded: |%022d|\n", -987654321);
  cxxoutput_printf ("octal   unsigned negative    zero-padded: |%022o|\n", -987654321);
  cxxoutput_printf ("hexadec unsigned negative    zero-padded: |%022x|\n", -987654321);
  cxxoutput_printf ("Hexadec unsigned negative    zero-padded: |%022X|\n", -987654321);
  cxxoutput_printf ("decimal unsigned positive right-adjusted: |%22u|\n", +987654321);
  cxxoutput_printf ("decimal signed-i positive right-adjusted: |%22i|\n", +987654321);
  cxxoutput_printf ("decimal signed-d positive right-adjusted: |%22d|\n", +987654321);
  cxxoutput_printf ("octal   unsigned positive right-adjusted: |%22o|\n", +987654321);
  cxxoutput_printf ("hexadec unsigned positive right-adjusted: |%22x|\n", +987654321);
  cxxoutput_printf ("Hexadec unsigned positive right-adjusted: |%22X|\n", +987654321);
  cxxoutput_printf ("decimal unsigned positive  left-adjusted: |%-22u|\n", +987654321);
  cxxoutput_printf ("decimal signed-i positive  left-adjusted: |%-22i|\n", +987654321);
  cxxoutput_printf ("decimal signed-d positive  left-adjusted: |%-22d|\n", +987654321);
  cxxoutput_printf ("octal   unsigned positive  left-adjusted: |%-22o|\n", +987654321);
  cxxoutput_printf ("hexadec unsigned positive  left-adjusted: |%-22x|\n", +987654321);
  cxxoutput_printf ("Hexadec unsigned positive  left-adjusted: |%-22X|\n", +987654321);
  cxxoutput_printf ("decimal signed-i positive   space-padded: |% 22i|\n", +987654321);
  cxxoutput_printf ("decimal signed-d positive   space-padded: |% 22d|\n", +987654321);
  cxxoutput_printf ("decimal unsigned positive    zero-padded: |%022u|\n", +987654321);
  cxxoutput_printf ("decimal signed-i positive    zero-padded: |%022i|\n", +987654321);
  cxxoutput_printf ("decimal signed-d positive    zero-padded: |%022d|\n", +987654321);
  cxxoutput_printf ("octal   unsigned positive    zero-padded: |%022o|\n", +987654321);
  cxxoutput_printf ("hexadec unsigned positive    zero-padded: |%022x|\n", +987654321);
  cxxoutput_printf ("Hexadec unsigned positive    zero-padded: |%022X|\n", +987654321);
#define PI    3.1415926535897932384626433832795028841971693993751058209749445923
  // %f
  cxxoutput_printf ("float   %%f       positive right-adjusted: |%22f|\n", PI);
  cxxoutput_printf ("float   %%-f      positive  left-adjusted: |%-22f|\n", PI);
  cxxoutput_printf ("float   %% f      positive   space-padded: |% 22f|\n", PI);
  cxxoutput_printf ("float   %%0f      positive    zero-padded: |%022f|\n", PI);
  cxxoutput_printf ("float   %%.0f     positive      truncated: |%22.0f|\n", PI);
  cxxoutput_printf ("float   %%#f      positive      alternate: |%#22.0f|\n", PI);
  cxxoutput_printf ("float   %%+f      positive    always-sign: |%+22f|\n", PI);
  cxxoutput_printf ("float   %%.5f     positive      precision: |%+22.5f|\n", PI);
  // %F
  cxxoutput_printf ("float   %%F       positive right-adjusted: |%22F|\n", PI);
  cxxoutput_printf ("float   %%-F      positive  left-adjusted: |%-22F|\n", PI);
  cxxoutput_printf ("float   %% F      positive   space-padded: |% 22F|\n", PI);
  cxxoutput_printf ("float   %%0F      positive    zero-padded: |%022F|\n", PI);
  cxxoutput_printf ("float   %%.0F     positive      truncated: |%22.0F|\n", PI);
  cxxoutput_printf ("float   %%#F      positive      alternate: |%#22.0F|\n", PI);
  cxxoutput_printf ("float   %%+F      positive    always-sign: |%+22F|\n", PI);
  cxxoutput_printf ("float   %%.5F     positive      precision: |%+22.5F|\n", PI);
  // %e
  cxxoutput_printf ("float   %%e       positive right-adjusted: |%22e|\n", PI);
  cxxoutput_printf ("float   %%-e      positive  left-adjusted: |%-22e|\n", PI);
  cxxoutput_printf ("float   %% e      positive   space-padded: |% 22e|\n", PI);
  cxxoutput_printf ("float   %%0e      positive    zero-padded: |%022e|\n", PI);
  cxxoutput_printf ("float   %%.0e     positive      truncated: |%22.0e|\n", PI);
  cxxoutput_printf ("float   %%#e      positive      alternate: |%#22.0e|\n", PI);
  cxxoutput_printf ("float   %%+e      positive    always-sign: |%+22e|\n", PI);
  cxxoutput_printf ("float   %%.5e     positive      precision: |%+22.5e|\n", PI);
  // %E
  cxxoutput_printf ("float   %%E       positive right-adjusted: |%22E|\n", PI);
  cxxoutput_printf ("float   %%-E      positive  left-adjusted: |%-22E|\n", PI);
  cxxoutput_printf ("float   %% E      positive   space-padded: |% 22E|\n", PI);
  cxxoutput_printf ("float   %%0E      positive    zero-padded: |%022E|\n", PI);
  cxxoutput_printf ("float   %%.0E     positive      truncated: |%22.0E|\n", PI);
  cxxoutput_printf ("float   %%#E      positive      alternate: |%#22.0E|\n", PI);
  cxxoutput_printf ("float   %%+E      positive    always-sign: |%+22E|\n", PI);
  cxxoutput_printf ("float   %%.5E     positive      precision: |%+22.5E|\n", PI);
#define GOOGOL  1e+100
  // %g
  cxxoutput_printf ("float   %%g       positive right-adjusted: |%22g|\n", GOOGOL);
  cxxoutput_printf ("float   %%-g      positive  left-adjusted: |%-22g|\n", GOOGOL);
  cxxoutput_printf ("float   %% g      positive   space-padded: |% 22g|\n", GOOGOL);
  cxxoutput_printf ("float   %%0g      positive    zero-padded: |%022g|\n", GOOGOL);
  cxxoutput_printf ("float   %%.0g     positive      truncated: |%22.0g|\n", GOOGOL);
  cxxoutput_printf ("float   %%#g      positive      alternate: |%#22.0g|\n", GOOGOL);
  cxxoutput_printf ("float   %%+g      positive    always-sign: |%+22g|\n", GOOGOL);
  cxxoutput_printf ("float   %%.5g     positive      precision: |%+22.5g|\n", GOOGOL);
  // %G
  cxxoutput_printf ("float   %%G       positive right-adjusted: |%22G|\n", GOOGOL);
  cxxoutput_printf ("float   %%-G      positive  left-adjusted: |%-22G|\n", GOOGOL);
  cxxoutput_printf ("float   %% G      positive   space-padded: |% 22G|\n", GOOGOL);
  cxxoutput_printf ("float   %%0G      positive    zero-padded: |%022G|\n", GOOGOL);
  cxxoutput_printf ("float   %%.0G     positive      truncated: |%22.0G|\n", GOOGOL);
  cxxoutput_printf ("float   %%#G      positive      alternate: |%#22.0G|\n", GOOGOL);
  cxxoutput_printf ("float   %%+G      positive    always-sign: |%+22G|\n", GOOGOL);
  cxxoutput_printf ("float   %%.5G     positive      precision: |%+22.5G|\n", GOOGOL);
  // %a
  cxxoutput_printf ("float   %%a       positive right-adjusted: |%22a|\n", GOOGOL);
  cxxoutput_printf ("float   %%-a      positive  left-adjusted: |%-22a|\n", GOOGOL);
  cxxoutput_printf ("float   %% a      positive   space-padded: |% 22a|\n", GOOGOL);
  cxxoutput_printf ("float   %%0a      positive    zero-padded: |%022a|\n", GOOGOL);
  cxxoutput_printf ("float   %%.0a     positive      truncated: |%22.0a|\n", GOOGOL);
  cxxoutput_printf ("float   %%#a      positive      alternate: |%#22.0a|\n", GOOGOL);
  cxxoutput_printf ("float   %%+a      positive    always-sign: |%+22a|\n", GOOGOL);
  cxxoutput_printf ("float   %%.5a     positive      precision: |%+22.5a|\n", GOOGOL);
  // %A
  cxxoutput_printf ("float   %%A       positive right-adjusted: |%22A|\n", GOOGOL);
  cxxoutput_printf ("float   %%-A      positive  left-adjusted: |%-22A|\n", GOOGOL);
  cxxoutput_printf ("float   %% A      positive   space-padded: |% 22A|\n", GOOGOL);
  cxxoutput_printf ("float   %%0A      positive    zero-padded: |%022A|\n", GOOGOL);
  cxxoutput_printf ("float   %%.0A     positive      truncated: |%22.0A|\n", GOOGOL);
  cxxoutput_printf ("float   %%#A      positive      alternate: |%#22.0A|\n", GOOGOL);
  cxxoutput_printf ("float   %%+A      positive    always-sign: |%+22A|\n", GOOGOL);
  cxxoutput_printf ("float   %%.5A     positive      precision: |%+22.5A|\n", GOOGOL);
  // %s
  constexpr const char *some_words = "char-string";
  cxxoutput_printf ("String  'char-string'     right-adjusted: |%22s|\n", some_words);
  cxxoutput_printf ("String  'char-string'      left-adjusted: |%-22s|\n", some_words);
  cxxoutput_printf ("String   0x0                null pointer: |%22s|\n", (char*) NULL);
  // widths
  typedef long long LLong;
  typedef unsigned long long ULLong;
  typedef long double LDouble;
  cxxoutput_printf ("char       : %hhd %hhu %c %lc\n", char (-107), char (+107), wchar_t (+107), wint_t (+107));
  cxxoutput_printf ("short      : %hd %hu\n", short (-107), short (+107));
  cxxoutput_printf ("int        : %d %u\n", int (-107), int (+107));
  cxxoutput_printf ("long       : %ld %lu\n", long (-107), long (+107));
  cxxoutput_printf ("long long  : %lld %llu\n", LLong (-107), LLong (+107));
  cxxoutput_printf ("float      : %f %g %e %a\n", float (PI), float (PI), float (PI), float (PI));
  cxxoutput_printf ("double     : %f %g %e %a\n", double (PI), double (PI), double (PI), double (PI));
  cxxoutput_printf ("long double: %Lf %Lg %Le %La\n", LDouble (PI), LDouble (PI), LDouble (PI), LDouble (PI));
  constexpr LLong llmin = -9223372036854775807LL - 1;
  constexpr ULLong ullmax = +18446744073709551615ULL;
  cxxoutput_printf ("min long long: %llx %llo %llu %lli %lld\n", llmin, llmin, llmin, llmin, llmin);
  cxxoutput_printf ("max long long: %llx %llo %llu %lli %lld\n", ullmax, ullmax, ullmax, ullmax, ullmax);
  // positional args
  cxxoutput_printf ("positionals-reorder: |%3$d %2$s %1$c|\n", '1', "2", 3);
  cxxoutput_printf ("positionals-repeat:  |%2$f %1$s %2$f|\n", "==", PI);
  cxxoutput_printf ("positionals-widths:  |%3$*1$.*2$s|\n", 5, 3, "abcdefg");
}
REGISTER_LOGTEST ("Strings/CxxPrintf Output", test_cxxprintf_output);

struct AAData
{
  static int destructor_calls;
  int value;
  AAData()      { value = 42; }
  ~AAData()
  {
    TASSERT (value == 42);
    value = 0;
    destructor_calls++;
  }
};

int AAData::destructor_calls = 0;

static void
test_aligned_array()
{
  AlignedArray<int, 65540> array (3);           // choose an alignment that is unlikely to occur by chance
  TASSERT (array[0] == 0);
  TASSERT (array[1] == 0);
  TASSERT (array[2] == 0);
  TASSERT (size_t (&array[0]) % 65540 == 0);
  {
    AlignedArray<AAData, 40> foo_array (5);
    TASSERT (size_t (&foo_array[0]) % 40 == 0);
    for (size_t i = 0; i < foo_array.size(); i++)
      TASSERT (foo_array[i].value == 42);
  }
  TASSERT (AAData::destructor_calls == 5);      // verifies that all elements have been destructed
}
REGISTER_TEST ("Memory/AlignedArray", test_aligned_array);

static void
test_entropy()
{
  const uint64_t seed1 = Entropy::get_seed();
  const uint64_t seed2 = Entropy::get_seed();
  TASSERT (seed1 != seed2);
  Entropy e;
  KeccakPRNG k1, k2 (e);
  TASSERT (k1 != k2);
}
REGISTER_TEST ("RandomHash/Entropy", test_entropy);

static void
test_random_numbers()
{
  TASSERT (random_int64() != random_int64());
  TASSERT (random_float() != random_float());
  TASSERT (random_irange (-999999999999999999, +999999999999999999) != random_irange (-999999999999999999, +999999999999999999));
  TASSERT (random_frange (-999999999999999999, +999999999999999999) != random_frange (-999999999999999999, +999999999999999999));
  for (size_t i = 0; i < 999999; i++)
    {
      const uint64_t ri = random_irange (989617512, 9876547656);
      TASSERT (ri >= 989617512 && ri < 9876547656);
      const double rf = random_frange (989617512, 9876547656);
      TASSERT (rf >= 989617512 && rf < 9876547656);
    }
  TASSERT (isnan (random_frange (NAN, 1)));
  TASSERT (isnan (random_frange (0, NAN)));
#if 0 // example penalty paid in random_int64()
  size_t i, j = 0;
  for (i = 0; i < 100; i++)
    {
      uint64_t r = k2();
      j += r > 9223372036854775808ULL;
      printout (" rand64: %d: 0x%016x\n", r > 9223372036854775808ULL, r);
    }
  printout (" rand64: fail: %d/%d -> %f%%\n", j, i, j * 100.0 / i);
  for (size_t i = 0; i < 100; i++)
    {
      // printout (" rand: %+d\n", random_irange (-5, 5));
      // printout (" rand: %f\n", random_float());
      printout (" rand: %g\n", random_frange (1000000000000000, 1000000000000000.912345678)-1000000000000000);
    }
#endif
}
REGISTER_TEST ("RandomHash/Random Numbers", test_random_numbers);

template<class Gen>
struct GeneratorBench64 {
  enum {
    N_RUNS = 1000,
    BLOCK_NUMS = 128,
  };
  Gen gen_;
  GeneratorBench64() : gen_() {}
  void
  operator() ()
  {
    uint64_t sum = 0;
    for (size_t j = 0; j < N_RUNS; j++)
      {
        for (size_t i = 0; i < BLOCK_NUMS; i++)
          sum ^= gen_();
      }
    TASSERT ((sum & 0xffffffff) != 0 && sum >> 32 != 0);
  }
  size_t
  bytes_per_run()
  {
    return sizeof (uint64_t) * BLOCK_NUMS * N_RUNS;
  }
};
struct Gen_lrand48 { uint64_t operator() () { return uint64_t (lrand48()) << 32 | lrand48(); } };
struct Gen_minstd  {
  std::minstd_rand minstd;
  uint64_t operator() () { return uint64_t (minstd()) << 32 | minstd(); }
};

static void
random_hash_benchmarks()
{
  Test::Timer timer (1); // 1 second maximum
  GeneratorBench64<std::mt19937_64> mb; // core-i7: 1415.3MB/s
  double bench_time = timer.benchmark (mb);
  TMSG ("mt19937_64: timing results: fastest=%fs prng=%.1fMB/s\n", bench_time, mb.bytes_per_run() / bench_time / 1048576.);
  GeneratorBench64<Gen_minstd> sb;      // core-i7:  763.7MB/s
  bench_time = timer.benchmark (sb);
  TMSG ("minstd:     timing results: fastest=%fs prng=%.1fMB/s\n", bench_time, sb.bytes_per_run() / bench_time / 1048576.);
  GeneratorBench64<Gen_lrand48> lb;     // core-i7:  654.8MB/s
  bench_time = timer.benchmark (lb);
  TMSG ("lrand48():  timing results: fastest=%fs prng=%.1fMB/s\n", bench_time, lb.bytes_per_run() / bench_time / 1048576.);
  GeneratorBench64<KeccakPRNG> kb;      // core-i7:  185.3MB/s
  bench_time = timer.benchmark (kb);
  TMSG ("KeccakPRNG: timing results: fastest=%fs prng=%.1fMB/s\n", bench_time, kb.bytes_per_run() / bench_time / 1048576.);
}
REGISTER_TEST ("RandomHash/~ Benchmarks", random_hash_benchmarks);

static void
test_keccak_prng()
{
  KeccakPRNG krandom1;
  String digest;
  for (size_t i = 0; i < 6; i++)
    {
      const uint64_t r = krandom1();
      const uint32_t h = r >> 32, l = r;
      digest += string_format ("%02x%02x%02x%02x%02x%02x%02x%02x",
                               l & 0xff, (l>>8) & 0xff, (l>>16) & 0xff, l>>24, h & 0xff, (h>>8) & 0xff, (h>>16) & 0xff, h>>24);
    }
  // printf ("KeccakPRNG: %s\n", digest.c_str());
  TASSERT (digest == "c336e57d8674ec52528a79e41c5e4ec9b31aa24c07cdf0fc8c6e8d88529f583b37a389883d2362639f8cc042abe980e0");

  std::stringstream kss;
  kss << krandom1;
  KeccakPRNG krandom2;
  TASSERT (krandom1 != krandom2 && !(krandom1 == krandom2));
  kss >> krandom2;
  TASSERT (krandom1 == krandom2 && !(krandom1 != krandom2));
  TASSERT (krandom1() == krandom2() && krandom1() == krandom2() && krandom1() == krandom2() && krandom1() == krandom2());
  krandom1();
  TASSERT (krandom1 != krandom2 && krandom1() != krandom2());
  krandom2();
  TASSERT (krandom1 == krandom2 && krandom1() == krandom2());
  krandom1.discard (0);
  TASSERT (krandom1 == krandom2 && krandom1() == krandom2());
  krandom1.discard (777);
  TASSERT (krandom1 != krandom2 && krandom1() != krandom2());
  for (size_t i = 0; i < 777; i++)
    krandom2();
  TASSERT (krandom1 == krandom2 && krandom1() == krandom2());
  krandom1();
  krandom2.discard (1);
  TASSERT (krandom1 == krandom2 && krandom1() == krandom2());
  krandom2.forget();
  TASSERT (krandom1 != krandom2);
  krandom1.seed (0x11007700affe0101);
  krandom2.seed (0x11007700affe0101);
  TASSERT (krandom1 == krandom2 && krandom1() == krandom2());
  const uint64_t one = 1;
  krandom1.seed (one);          // seed with 0x1 directly
  std::seed_seq seq { 0x01 };   // seed_seq generates "random" bits from its input
  krandom2.seed (seq);
  TASSERT (krandom1 != krandom2);
  krandom2.seed (&one, 1);      // seed with array containing just 0x1
  TASSERT (krandom1 == krandom2 && krandom1() == krandom2());
  krandom2.seed (krandom1);     // uses krandom1.generate
  TASSERT (krandom1 != krandom2 && krandom1() != krandom2());
}
REGISTER_TEST ("RandomHash/KeccakPRNG", test_keccak_prng);

static inline int
nibble (char c)
{
  if (c >= '0' && c <= '9')
    return c - '0';
  else if (c >= 'A' && c <= 'F')
    return c - 'A' + 10;
  else if (c >= 'a' && c <= 'f')
    return c - 'a' + 10;
  else
    return -1;
}

static std::string
image_to_bytes (const std::string &hex)
{
  std::string result;
  for (size_t i = 0; i + 1 < hex.size(); i += 2)
    {
      const int h = nibble (hex[i]), l = nibble (hex[i + 1]);
      if (h < 0 || l < 0)
        return "";      // invalid format
      char b = (unsigned (h) << 4) + l;
      result.append (&b, 1);
    }
  return result;
}

static std::string
byte_image (const uint8_t *bytes, size_t n_bytes)
{
  std::string s;
  char buf[8];
  for (size_t i = 0; i < n_bytes; i++)
    {
      sprintf (buf, "%02x", bytes[i]);
      s += buf;
    }
  return s;
}

static std::string
short_image (const std::string &input, size_t maximum = 50 * 2)
{
  if (input.size() > maximum + 3)
    return input.substr (0, maximum) + "...";
  else
    return input;
}

template<size_t N> static void
tprint (const char *name, const char *input, uint8_t (&output)[N])
{
  if (!Test::verbose())
    return;
  printout ("%s(%s): %s\n", name, short_image (input, 24), short_image (byte_image (output, N), 44));
}

static void
test_sha3_hashing()
{
  std::string inputdata;

  // SHA3-224
  const char *testdata_sha3_224_0f_in = "0F8B2D8FCFD9D68CFFC17CCFB117709B53D26462A3F346FB7C79B85E";
  const char *testdata_sha3_224_0f_md = "1e693b0bce2372550daef35b14f13ab43441ed6742dee3e86fd1d8ef";
  uint8_t sha3_224_hashvalue[28];
  inputdata = image_to_bytes (testdata_sha3_224_0f_in);
  sha3_224_hash (inputdata.data(), inputdata.size(), sha3_224_hashvalue);
  tprint ("SHA3-224", testdata_sha3_224_0f_in, sha3_224_hashvalue);
  assert (byte_image (sha3_224_hashvalue, sizeof (sha3_224_hashvalue)) == testdata_sha3_224_0f_md);

  // SHA3-256
  const char *testdata_sha3_256_de_in =
    "DE286BA4206E8B005714F80FB1CDFAEBDE91D29F84603E4A3EBC04686F99A46C9E880B96C574825582E8812A26E5A857FFC6579F63742F";
  const char *testdata_sha3_256_de_md =
    "1bc1bcc70f638958db1006af37b02ebd8954ec59b3acbad12eacedbc5b21e908";
  uint8_t sha3_256_hashvalue[32];
  inputdata = image_to_bytes (testdata_sha3_256_de_in);
  sha3_256_hash (inputdata.data(), inputdata.size(), sha3_256_hashvalue);
  tprint ("SHA3-256", testdata_sha3_256_de_in, sha3_256_hashvalue);
  assert (byte_image (sha3_256_hashvalue, sizeof (sha3_256_hashvalue)) == testdata_sha3_256_de_md);

  // SHA3-384
  const char *testdata_sha3_384_1f_in = "1F877C";
  const char *testdata_sha3_384_1f_md =
    "14f6f486fb98ed46a4a198040da8079e79e448daacebe905fb4cf0df86ef2a7151f62fe095bf8516eb0677fe607734e2";
  uint8_t sha3_384_hashvalue[48];
  inputdata = image_to_bytes (testdata_sha3_384_1f_in);
  sha3_384_hash (inputdata.data(), inputdata.size(), sha3_384_hashvalue);
  tprint ("SHA3-384", testdata_sha3_384_1f_in, sha3_384_hashvalue);
  assert (byte_image (sha3_384_hashvalue, sizeof (sha3_384_hashvalue)) == testdata_sha3_384_1f_md);

  // SHA3-512
  const char *testdata_sha3_512_cc_in = "CC";
  const char *testdata_sha3_512_cc_md =
    "3939fcc8b57b63612542da31a834e5dcc36e2ee0f652ac72e02624fa2e5adeecc7dd6bb3580224b4d6138706fc6e80597b528051230b00621cc2b22999eaa205";
  uint8_t sha3_512_hashvalue[64];
  inputdata = image_to_bytes (testdata_sha3_512_cc_in);
  sha3_512_hash (inputdata.data(), inputdata.size(), sha3_512_hashvalue);
  tprint ("SHA3-512", testdata_sha3_512_cc_in, sha3_512_hashvalue);
  assert (byte_image (sha3_512_hashvalue, sizeof (sha3_512_hashvalue)) == testdata_sha3_512_cc_md);
}
REGISTER_TEST ("RandomHash/SHA3 Hashing", test_sha3_hashing);

static void
test_shake_hashing()
{
  std::string inputdata;

  // SHAKE128
  const char *testdata_shake128__in = "";
  const char *testdata_shake128__md =
    "7f9c2ba4e88f827d616045507605853ed73b8093f6efbc88eb1a6eacfa66ef263cb1eea988004b93103cfb0aeefd2a686e01fa4a58e8a3639ca8a1e3f9ae"
    "57e235b8cc873c23dc62b8d260169afa2f75ab916a58d974918835d25e6a435085b2badfd6dfaac359a5efbb7bcc4b59d538df9a04302e10c8bc1cbf1a0b"
    "3a5120ea17cda7cfad765f5623474d368ccca8af0007cd9f5e4c849f167a580b14aabdefaee7eef47cb0fca9767be1fda69419dfb927e9df07348b196691"
    "abaeb580b32def58538b8d23f87732ea63b02b4fa0f4873360e2841928cd60dd4cee8cc0d4c922a96188d032675c8ac850933c7aff1533b94c834adbb69c"
    "6115bad4692d8619f90b0cdf8a7b9c264029ac185b70b83f2801f2f4b3f70c593ea3aeeb613a7f1b1de33fd75081f592305f2e4526edc09631b10958f464"
    "d889f31ba010250fda7f1368ec2967fc84ef2ae9aff268e0b1700affc6820b523a3d917135f2dff2ee06bfe72b3124721d4a26c04e53a75e30e73a7a9c4a"
    "95d91c55d495e9f51dd0b5e9d83c6d5e8ce803aa62b8d654db53d09b8dcff273cdfeb573fad8bcd45578bec2e770d01efde86e721a3f7c6cce275dabe6e2"
    "143f1af18da7efddc4c7b70b5e345db93cc936bea323491ccb38a388f546a9ff00dd4e1300b9b2153d2041d205b443e41b45a653f2a5c4492c1add544512"
    "dda2529833462b71a41a45be97290b6f"; // 4096 bits
  uint8_t testdata_shake128_hashvalue[512];
  inputdata = image_to_bytes (testdata_shake128__in);
  shake128_hash (inputdata.data(), inputdata.size(), testdata_shake128_hashvalue, sizeof (testdata_shake128_hashvalue));
  tprint ("SHAKE128", testdata_shake128__in, testdata_shake128_hashvalue);
  assert (byte_image (testdata_shake128_hashvalue, sizeof (testdata_shake128_hashvalue)) == testdata_shake128__md);
  // SHAKE128 3a...
  const char *testdata_shake128_3a3_in =
    "3A3A819C48EFDE2AD914FBF00E18AB6BC4F14513AB27D0C178A188B61431E7F5623CB66B23346775D386B50E982C493ADBBFC54B9A3CD383382336A1A0B2"
    "150A15358F336D03AE18F666C7573D55C4FD181C29E6CCFDE63EA35F0ADF5885CFC0A3D84A2B2E4DD24496DB789E663170CEF74798AA1BBCD4574EA0BBA4"
    "0489D764B2F83AADC66B148B4A0CD95246C127D5871C4F11418690A5DDF01246A0C80A43C70088B6183639DCFDA4125BD113A8F49EE23ED306FAAC576C3F"
    "B0C1E256671D817FC2534A52F5B439F72E424DE376F4C565CCA82307DD9EF76DA5B7C4EB7E085172E328807C02D011FFBF33785378D79DC266F6A5BE6BB0"
    "E4A92ECEEBAEB1";
  const char *testdata_shake128_3a3_md =
    "14236e75b9784df4f57935f945356cbe383fe513ed30286f91060759bcb0ef4baac858ecae7c6e7edd498f01a082b63fa57d22540231e2e25c83efb3b3f2"
    "953a5f674502ab635226446b84937643dcd5789ee73f1d734bc8fe5f7f0883ab10961b9a31ff60dee16159bc6982efb08545984bf71fed1c4cd81c0914b4"
    "c19fcfeef54af4bbe372f18cfcd3a18657f5b9450f99a78f0fa2c3cdca7461c4ed7569536883b66cd87e9c200962902eaa16a54db6a0a5cc26d889038c07"
    "60810b5bb4f33f1e5d639b6f9bc7ca62ba6f8c9f8de770260afe47f4e0f82f102198eba27f543252ac8ddd83e1b8db0a91ac65633fd12a550ebe96f93aa6"
    "704ed5905c234fa6d9203910cbd02de166c4c3348fb81ef7b84ae1455fe318b5fd170883f49ba2f24289c479a2c7531406ba989beaef3a79f659028642e9"
    "b033f7deb9ecec3a7a9f1dbd2451fcb47c81e21e91d20b924c6bd04c1f0b2710d2e570cd24bad5b5de4e49aa80b6add5507b4d2e510370c7afa814d7e1a7"
    "e278e53d7ccf49a0a866ca3a7b5bb71ef3425e460feeb29149f217066613695f85506a0946cf68979f04ae073af8028976bf0c5bdc2212e8c364583de9fb"
    "d03b34ddee5ec4cfa8ed8ce592971d0108faf76c8940e25e6c5f865584c34a233c14f00532673fdbe388cc7e98a5b867b1c591307a9015112b567ff6b4f3"
    "18114111fc95e5bd7c9c60b74c1f8725"; // 4096 bits
  inputdata = image_to_bytes (testdata_shake128_3a3_in);
  shake128_hash (inputdata.data(), inputdata.size(), testdata_shake128_hashvalue, sizeof (testdata_shake128_hashvalue));
  tprint ("SHAKE128", testdata_shake128_3a3_in, testdata_shake128_hashvalue);
  assert (byte_image (testdata_shake128_hashvalue, sizeof (testdata_shake128_hashvalue)) == testdata_shake128_3a3_md);

  // SHAKE256
  const char *testdata_shake256_3a3_in =
    "3A3A819C48EFDE2AD914FBF00E18AB6BC4F14513AB27D0C178A188B61431E7F5623CB66B23346775D386B50E982C493ADBBFC54B9A3CD383382336A1A0B2"
    "150A15358F336D03AE18F666C7573D55C4FD181C29E6CCFDE63EA35F0ADF5885CFC0A3D84A2B2E4DD24496DB789E663170CEF74798AA1BBCD4574EA0BBA4"
    "0489D764B2F83AADC66B148B4A0CD95246C127D5871C4F11418690A5DDF01246A0C80A43C70088B6183639DCFDA4125BD113A8F49EE23ED306FAAC576C3F"
    "B0C1E256671D817FC2534A52F5B439F72E424DE376F4C565CCA82307DD9EF76DA5B7C4EB7E085172E328807C02D011FFBF33785378D79DC266F6A5BE6BB0"
    "E4A92ECEEBAEB1"; // 2040 bits
  const char *testdata_shake256_3a3_md =
    "8a5199b4a7e133e264a86202720655894d48cff344a928cf8347f48379cef347dfc5bcffab99b27b1f89aa2735e23d30088ffa03b9edb02b9635470ab9f1"
    "038985d55f9ca774572dd006470ea65145469609f9fa0831bf1ffd842dc24acade27bd9816e3b5bf2876cb112232a0eb4475f1dff9f5c713d9ffd4ccb89a"
    "e5607fe35731df06317949eef646e9591cf3be53add6b7dd2b6096e2b3fb06e662ec8b2d77422daad9463cd155204acdbd38e319613f39f99b6dfb35ca93"
    "65160066db19835888c2241ff9a731a4acbb5663727aac34a401247fbaa7499e7d5ee5b69d31025e63d04c35c798bca1262d5673a9cf0930b5ad89bd4855"
    "99dc184528da4790f088ebd170b635d9581632d2ff90db79665ced430089af13c9f21f6d443a818064f17aec9e9c5457001fa8dc6afbadbe3138f388d89d"
    "0e6f22f66671255b210754ed63d81dce75ce8f189b534e6d6b3539aa51e837c42df9df59c71e6171cd4902fe1bdc73fb1775b5c754a1ed4ea7f3105fc543"
    "ee0418dad256f3f6118ea77114a16c15355b42877a1db2a7df0e155ae1d8670abcec3450f4e2eec9838f895423ef63d261138baaf5d9f104cb5a957aea06"
    "c0b9b8c78b0d441796dc0350ddeabb78a33b6f1f9e68ede3d1805c7b7e2cfd54e0fad62f0d8ca67a775dc4546af9096f2edb221db42843d65327861282dc"
    "946a0ba01a11863ab2d1dfd16e3973d4"; // 4096 bits
  uint8_t testdata_shake256_hashvalue[512];
  inputdata = image_to_bytes (testdata_shake256_3a3_in);
  shake256_hash (inputdata.data(), inputdata.size(), testdata_shake256_hashvalue, sizeof (testdata_shake256_hashvalue));
  tprint ("SHAKE256", testdata_shake256_3a3_in, testdata_shake256_hashvalue);
  assert (byte_image (testdata_shake256_hashvalue, sizeof (testdata_shake256_hashvalue)) == testdata_shake256_3a3_md);

  // testing squeeze_hash() && reset()
  SHAKE256 shake256;
  shake256.update ((const uint8_t*) inputdata.data(), inputdata.size());
  memset (testdata_shake256_hashvalue, 0, sizeof (testdata_shake256_hashvalue));
  const size_t svlen = sizeof (testdata_shake256_hashvalue);
  for (size_t i = 0; i < 16; i++) // test incremental readouts
    shake256.squeeze_digest (testdata_shake256_hashvalue + i * (svlen / 16), svlen / 16);
  assert (byte_image (testdata_shake256_hashvalue, sizeof (testdata_shake256_hashvalue)) == testdata_shake256_3a3_md);
  shake256.squeeze_digest (testdata_shake256_hashvalue, sizeof (testdata_shake256_hashvalue));
  tprint ("SHAKE256", "sequeeze...", testdata_shake256_hashvalue);
  assert (byte_image (testdata_shake256_hashvalue, sizeof (testdata_shake256_hashvalue)) != testdata_shake256_3a3_md);
  shake256.reset();
  shake256.update ((const uint8_t*) inputdata.data(), inputdata.size());
  shake256.squeeze_digest (testdata_shake256_hashvalue, sizeof (testdata_shake256_hashvalue));
  tprint ("SHAKE256", "reset...", testdata_shake256_hashvalue);
  assert (byte_image (testdata_shake256_hashvalue, sizeof (testdata_shake256_hashvalue)) == testdata_shake256_3a3_md);
}
REGISTER_TEST ("RandomHash/SHAKE Hashing", test_shake_hashing);

} // Anon

/* vim:set ts=8 sts=2 sw=2: */
