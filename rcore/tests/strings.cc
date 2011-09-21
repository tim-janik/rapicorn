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
      TCMP (pn, ==, NULL);
      gn = g_utf8_find_next_char (cur, cur + strlen (cur));
      TCMP (pn, ==, gn);
      /* first with bounds */
      pp = utf8_find_prev (cbuffer, cbuffer);
      TCMP (pp, ==, NULL);

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
REGISTER_TEST ("/Strings/random UTF8", random_utf8_and_unichar_test, 30000);
REGISTER_SLOWTEST ("/Strings/random UTF8 (slow)", random_utf8_and_unichar_test, 1000000);

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

      bb = Unichar::isvalid (uc);
      gb = g_unichar_validate (uc);
      TCMP (bb, ==, gb);
      bb = Unichar::isalnum (uc);
      gb = g_unichar_isalnum (uc);
      TCMP (bb, ==, gb);
      bb = Unichar::isalpha (uc);
      gb = g_unichar_isalpha (uc);
      TCMP (bb, ==, gb);
      bb = Unichar::iscntrl (uc);
      gb = g_unichar_iscntrl (uc);
      TCMP (bb, ==, gb);
      bb = Unichar::isdigit (uc);
      gb = g_unichar_isdigit (uc);
      TCMP (bb, ==, gb);
      bv = Unichar::digit_value (uc);
      gv = g_unichar_digit_value (uc);
      TCMP (bv, ==, gv);
      bv = Unichar::digit_value ('0' + uc % 10);
      gv = g_unichar_digit_value ('0' + uc % 10);
      TCMP (bv, ==, gv);
      bb = Unichar::isgraph (uc);
      gb = g_unichar_isgraph (uc);
      TCMP (bv, ==, gv);
      bb = Unichar::islower (uc);
      gb = g_unichar_islower (uc);
      TCMP (bb, ==, gb);
      bc = Unichar::tolower (uc);
      gc = g_unichar_tolower (uc);
      TCMP (bc, ==, gc);
      bb = Unichar::isprint (uc);
      gb = g_unichar_isprint (uc);
      TCMP (bb, ==, gb);
      bb = Unichar::ispunct (uc);
      gb = g_unichar_ispunct (uc);
      TCMP (bb, ==, gb);
      bb = Unichar::isspace (uc);
      gb = g_unichar_isspace (uc);
      TCMP (bb, ==, gb);
      bb = Unichar::isupper (uc);
      gb = g_unichar_isupper (uc);
      TCMP (bb, ==, gb);
      bc = Unichar::toupper (uc);
      gc = g_unichar_toupper (uc);
      TCMP (bc, ==, gc);
      bb = Unichar::isxdigit (uc);
      gb = g_unichar_isxdigit (uc);
      TCMP (bb, ==, gb);
      bv = Unichar::xdigit_value (uc);
      gv = g_unichar_xdigit_value (uc);
      TCMP (bv, ==, gv);
      bv = Unichar::xdigit_value ('0' + uc % 10);
      gv = g_unichar_xdigit_value ('0' + uc % 10);
      TCMP (bv, ==, gv);
      bv = Unichar::xdigit_value ('a' + uc % 6);
      gv = g_unichar_xdigit_value ('a' + uc % 6);
      TCMP (bv, ==, gv);
      bv = Unichar::xdigit_value ('A' + uc % 6);
      gv = g_unichar_xdigit_value ('A' + uc % 6);
      TCMP (bv, ==, gv);
      bb = Unichar::istitle (uc);
      gb = g_unichar_istitle (uc);
      TCMP (bb, ==, gb);
      bc = Unichar::totitle (uc);
      gc = g_unichar_totitle (uc);
      TCMP (bc, ==, gc);
      bb = Unichar::isdefined (uc);
      gb = g_unichar_isdefined (uc);
      TCMP (bb, ==, gb);
      bb = Unichar::iswide (uc);
      gb = g_unichar_iswide (uc);
      TCMP (bb, ==, gb);
#if GLIB_CHECK_VERSION (2, 12, 0)
      bb = Unichar::iswide_cjk (uc);
      gb = g_unichar_iswide_cjk (uc);
      TCMP (bb, ==, gb);
#endif
      TCMP (Unichar::get_type (uc), ==, (int) g_unichar_type (uc));
      TCMP (Unichar::get_break (uc), ==, (int) g_unichar_break_type (uc));
    }
}
REGISTER_TEST ("/Strings/random unichar", random_unichar_test, 30000);
REGISTER_SLOWTEST ("/Strings/random unichar (slow)", random_unichar_test, 1000000);

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
REGISTER_TEST ("/Strings/UUID", uuid_tests);

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
REGISTER_TEST ("/Strings/conversions", string_conversions);

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
REGISTER_TEST ("/Strings/split strings", split_string_tests);

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
REGISTER_TEST ("/Strings/charsets", test_string_charsets);

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
REGISTER_TEST ("/Strings/stripping", test_string_stripping);

} // Anon

/* vim:set ts=8 sts=2 sw=2: */
