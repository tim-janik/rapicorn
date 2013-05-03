// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#include <rcore/testutils.hh>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <glib.h>
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

static void
random_unichar_test (ptrdiff_t count)
{
  for (uint i = 0; i < count; i++)
    {
      unichar uc = rand() % (0x100 << (i % 24));
      unichar bc, gc;
      gboolean gb;
      bool bb;
      int bv, gv;

      bb = Unicode::isvalid (uc);
      gb = g_unichar_validate (uc);
      TCMP (bb, ==, gb);
      bb = Unicode::isalnum (uc);
      gb = g_unichar_isalnum (uc);
      TCMP (bb, ==, gb);
      bb = Unicode::isalpha (uc);
      gb = g_unichar_isalpha (uc);
      TCMP (bb, ==, gb);
      bb = Unicode::iscntrl (uc);
      gb = g_unichar_iscntrl (uc);
      TCMP (bb, ==, gb);
      bb = Unicode::isdigit (uc);
      gb = g_unichar_isdigit (uc);
      TCMP (bb, ==, gb);
      bv = Unicode::digit_value (uc);
      gv = g_unichar_digit_value (uc);
      TCMP (bv, ==, gv);
      bv = Unicode::digit_value ('0' + uc % 10);
      gv = g_unichar_digit_value ('0' + uc % 10);
      TCMP (bv, ==, gv);
      bb = Unicode::isgraph (uc);
      gb = g_unichar_isgraph (uc);
      TCMP (bv, ==, gv);
      bb = Unicode::islower (uc);
      gb = g_unichar_islower (uc);
      TCMP (bb, ==, gb);
      bc = Unicode::tolower (uc);
      gc = g_unichar_tolower (uc);
      TCMP (bc, ==, gc);
      bb = Unicode::isprint (uc);
      gb = g_unichar_isprint (uc);
      TCMP (bb, ==, gb);
      bb = Unicode::ispunct (uc);
      gb = g_unichar_ispunct (uc);
      TCMP (bb, ==, gb);
      bb = Unicode::isspace (uc);
      gb = g_unichar_isspace (uc);
      TCMP (bb, ==, gb);
      bb = Unicode::isupper (uc);
      gb = g_unichar_isupper (uc);
      TCMP (bb, ==, gb);
      bc = Unicode::toupper (uc);
      gc = g_unichar_toupper (uc);
      TCMP (bc, ==, gc);
      bb = Unicode::isxdigit (uc);
      gb = g_unichar_isxdigit (uc);
      TCMP (bb, ==, gb);
      bv = Unicode::xdigit_value (uc);
      gv = g_unichar_xdigit_value (uc);
      TCMP (bv, ==, gv);
      bv = Unicode::xdigit_value ('0' + uc % 10);
      gv = g_unichar_xdigit_value ('0' + uc % 10);
      TCMP (bv, ==, gv);
      bv = Unicode::xdigit_value ('a' + uc % 6);
      gv = g_unichar_xdigit_value ('a' + uc % 6);
      TCMP (bv, ==, gv);
      bv = Unicode::xdigit_value ('A' + uc % 6);
      gv = g_unichar_xdigit_value ('A' + uc % 6);
      TCMP (bv, ==, gv);
      bb = Unicode::istitle (uc);
      gb = g_unichar_istitle (uc);
      TCMP (bb, ==, gb);
      bc = Unicode::totitle (uc);
      gc = g_unichar_totitle (uc);
      TCMP (bc, ==, gc);
      bb = Unicode::isdefined (uc);
      gb = g_unichar_isdefined (uc);
      TCMP (bb, ==, gb);
      bb = Unicode::iswide (uc);
      gb = g_unichar_iswide (uc);
      TCMP (bb, ==, gb);
#if GLIB_CHECK_VERSION (2, 12, 0)
      bb = Unicode::iswide_cjk (uc);
      gb = g_unichar_iswide_cjk (uc);
      TCMP (bb, ==, gb);
#endif
      TCMP (Unicode::get_type (uc), ==, (int) g_unichar_type (uc));
      TCMP (Unicode::get_break (uc), ==, (int) g_unichar_break_type (uc));
    }
}
REGISTER_TEST ("Strings/random unichar", random_unichar_test, 30000);
REGISTER_SLOWTEST ("Strings/random unichar (slow)", random_unichar_test, 1000000);

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
}
REGISTER_TEST ("Strings/conversions", string_conversions);

static void
split_string_tests (void)
{
  StringVector sv;
  sv = string_split ("a;b;c", ";");
  TCMP (string_join ("//", sv), ==, "a//b//c");
  TCMP (string_join ("_", string_split ("a;b;c;", ";")), ==, "a_b_c_");
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
test_string_functions (void)
{
  TASSERT (string_startswith ("foo", "fo") == true);
  TASSERT (string_startswith ("foo", "o") == false);
  TASSERT (string_endswith ("foo", "o") == true);
  TASSERT (string_endswith ("foo", "oo") == true);
  TASSERT (string_endswith ("foo", "foo") == true);
  TASSERT (string_endswith ("foo", "loo") == false);
  // string_format
  enum { TEST17 = 17 };
  TCMP (string_format ("%d %s", -9223372036854775808uLL, "FOO"), ==, "-9223372036854775808 FOO");
  TCMP (string_format ("%g %d", 0.5, int (TEST17)), ==, "0.5 17"); // gcc corrupts anonymous enums without the cast
  TCMP (string_format ("0x%08x", 0xc0ffee), ==, "0x00c0ffee");
  static_assert (TEST17 == 17, "!");
  TCMP (string_format ("Only %c%%", '3'), ==, "Only 3%");
  UncopyablePoint point { 1, 2 };
  vector<String> foo;
  TCMP (string_format ("%s", point), ==, "{1;2}");
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

} // Anon

/* vim:set ts=8 sts=2 sw=2: */
