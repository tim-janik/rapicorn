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
#include <rapicorn-core/testutils.hh>
using namespace Rapicorn;

namespace {
using namespace Rapicorn;

static void
random_utf8_and_unichar_test (void)
{
  const uint count = init_settings().test_quick ? 30000 : 1000000;
  for (uint i = 0; i < count; i++)
    {
      unichar nc, uc = rand() % (0x100 << (i % 24));
      if (!uc)
        continue;
      char buffer[8], gstr[10] = { 0, };
      int l;

      l = utf8_from_unichar (uc, buffer);
      TCHECK (l > 0);
      TCHECK (l <= 7);
      TCHECK (buffer[l] == 0);
      TCHECK (l == g_unichar_to_utf8 (uc, gstr));
      TCHECK (strcmp (gstr, buffer) == 0);
      nc = utf8_to_unichar (buffer);
      TCHECK (nc == g_utf8_get_char (buffer));
      TCHECK (uc == nc);
      TCHECK (l == utf8_from_unichar (uc, NULL));
      char *p1 = utf8_next (buffer);
      TCHECK (p1 == buffer + l);
      char *p2 = utf8_prev (p1);
      TCHECK (p2 == buffer);

      char cbuffer[1024];
      snprintf (cbuffer, 1024, "x%sy7", buffer);
      char *cur = cbuffer, *pn, *gn, *pp;
      /* x */
      pn = utf8_find_next (cur);
      TCHECK (pn == cur + 1);
      gn = g_utf8_find_next_char (cur, NULL);
      TCHECK (pn == gn);
      pp = utf8_find_prev (cbuffer, pn);
      TCHECK (pp == cur);
      /* random unichar */
      cur = pn;
      pn = utf8_find_next (cur);
      TCHECK (pn == cur + l);
      gn = g_utf8_find_next_char (cur, NULL);
      TCHECK (pn == gn);
      pp = utf8_find_prev (cbuffer, pn);
      TCHECK (pp == cur);
      /* y */
      cur = pn;
      pn = utf8_find_next (cur);
      TCHECK (pn == cur + 1);
      gn = g_utf8_find_next_char (cur, NULL);
      TCHECK (pn == gn);
      pp = utf8_find_prev (cbuffer, pn);
      TCHECK (pp == cur);
      /* 7 (last) */
      cur = pn;
      pn = utf8_find_next (cur);
      TCHECK (pn == cur + 1);
      gn = g_utf8_find_next_char (cur, NULL);
      TCHECK (pn == gn);
      pp = utf8_find_prev (cbuffer, pn);
      TCHECK (pp == cur);
      /* last with bounds */
      pn = utf8_find_next (cur, cur + strlen (cur));
      TCHECK (pn == NULL);
      gn = g_utf8_find_next_char (cur, cur + strlen (cur));
      TCHECK (pn == gn);
      /* first with bounds */
      pp = utf8_find_prev (cbuffer, cbuffer);
      TCHECK (pp == NULL);

      /* validate valid UTF-8 */
      bool bb = utf8_validate (cbuffer);
      bool gb = g_utf8_validate (cbuffer, -1, NULL);
      TCHECK (bb == gb);
      /* validate invalid UTF-8 */
      cbuffer[rand() % (l + 3)] = rand();
      const char *gp;
      int indx;
      bb = utf8_validate (cbuffer, &indx);
      gb = g_utf8_validate (cbuffer, -1, &gp);
      TCHECK (bb == gb);
      if (!bb)
        TCHECK (cbuffer + indx == gp);
    }
}

static void
random_unichar_test (void)
{
  const uint count = init_settings().test_quick ? 30000 : 1000000;
  for (uint i = 0; i < count; i++)
    {
      unichar uc = rand() % (0x100 << (i % 24));
      unichar bc, gc;
      gboolean gb;
      bool bb;
      int bv, gv;

      bb = Unichar::isvalid (uc);
      gb = g_unichar_validate (uc);
      TCHECK (bb == gb);
      bb = Unichar::isalnum (uc);
      gb = g_unichar_isalnum (uc);
      TCHECK (bb == gb);
      bb = Unichar::isalpha (uc);
      gb = g_unichar_isalpha (uc);
      TCHECK (bb == gb);
      bb = Unichar::iscntrl (uc);
      gb = g_unichar_iscntrl (uc);
      TCHECK (bb == gb);
      bb = Unichar::isdigit (uc);
      gb = g_unichar_isdigit (uc);
      TCHECK (bb == gb);
      bv = Unichar::digit_value (uc);
      gv = g_unichar_digit_value (uc);
      TCHECK (bv == gv);
      bv = Unichar::digit_value ('0' + uc % 10);
      gv = g_unichar_digit_value ('0' + uc % 10);
      TCHECK (bv == gv);
      bb = Unichar::isgraph (uc);
      gb = g_unichar_isgraph (uc);
      TCHECK (bv == gv);
      bb = Unichar::islower (uc);
      gb = g_unichar_islower (uc);
      TCHECK (bb == gb);
      bc = Unichar::tolower (uc);
      gc = g_unichar_tolower (uc);
      TCHECK (bc == gc);
      bb = Unichar::isprint (uc);
      gb = g_unichar_isprint (uc);
      TCHECK (bb == gb);
      bb = Unichar::ispunct (uc);
      gb = g_unichar_ispunct (uc);
      TCHECK (bb == gb);
      bb = Unichar::isspace (uc);
      gb = g_unichar_isspace (uc);
      TCHECK (bb == gb);
      bb = Unichar::isupper (uc);
      gb = g_unichar_isupper (uc);
      TCHECK (bb == gb);
      bc = Unichar::toupper (uc);
      gc = g_unichar_toupper (uc);
      TCHECK (bc == gc);
      bb = Unichar::isxdigit (uc);
      gb = g_unichar_isxdigit (uc);
      TCHECK (bb == gb);
      bv = Unichar::xdigit_value (uc);
      gv = g_unichar_xdigit_value (uc);
      TCHECK (bv == gv);
      bv = Unichar::xdigit_value ('0' + uc % 10);
      gv = g_unichar_xdigit_value ('0' + uc % 10);
      TCHECK (bv == gv);
      bv = Unichar::xdigit_value ('a' + uc % 6);
      gv = g_unichar_xdigit_value ('a' + uc % 6);
      TCHECK (bv == gv);
      bv = Unichar::xdigit_value ('A' + uc % 6);
      gv = g_unichar_xdigit_value ('A' + uc % 6);
      TCHECK (bv == gv);
      bb = Unichar::istitle (uc);
      gb = g_unichar_istitle (uc);
      TCHECK (bb == gb);
      bc = Unichar::totitle (uc);
      gc = g_unichar_totitle (uc);
      TCHECK (bc == gc);
      bb = Unichar::isdefined (uc);
      gb = g_unichar_isdefined (uc);
      TCHECK (bb == gb);
      bb = Unichar::iswide (uc);
      gb = g_unichar_iswide (uc);
      TCHECK (bb == gb);
#if GLIB_CHECK_VERSION (2, 12, 0)
      bb = Unichar::iswide_cjk (uc);
      gb = g_unichar_iswide_cjk (uc);
      TCHECK (bb == gb);
#endif
      TCHECK (Unichar::get_type (uc) == (int) g_unichar_type (uc));
      TCHECK (Unichar::get_break (uc) == (int) g_unichar_break_type (uc));
    }
}

static void
uuid_tests (void)
{
  /* uuid string test */
  TASSERT (string_is_uuid ("00000000-0000-0000-0000-000000000000") == true);
  TASSERT (string_is_uuid ("6ba7b812-9dad-11d1-80b4-00c04fd430c8") == true);
  TASSERT (string_is_uuid ("6BA7B812-9DAD-11D1-80B4-00C04FD430C8") == true);
  TASSERT (string_is_uuid ("a425fd92-4f06-11db-aea9-000102e7e309") == true);
  TASSERT (string_is_uuid ("A425FD92-4F06-11DB-AEA9-000102E7E309") == true);
  TASSERT (string_is_uuid ("dc380602-a739-4be1-a5cb-53c437ffe39f") == true);
  TASSERT (string_is_uuid ("DC380602-A739-4BE1-A5CB-53C437FFE39F") == true);
  // TASSERT (string_is_uuid (NULL) == false);
  TASSERT (string_is_uuid ("") == false);
  TASSERT (string_is_uuid ("gba7b812-9dad-11d1-80b4-00c04fd430c8") == false);
  TASSERT (string_is_uuid ("Gba7b812-9dad-11d1-80b4-00c04fd430c8") == false);
  TASSERT (string_is_uuid ("6ba7b812.9dad-11d1-80b4-00c04fd430c8") == false);
  TASSERT (string_is_uuid ("6ba7b812-9dad.11d1-80b4-00c04fd430c8") == false);
  TASSERT (string_is_uuid ("6ba7b812-9dad-11d1.80b4-00c04fd430c8") == false);
  TASSERT (string_is_uuid ("6ba7b812-9dad-11d1-80b4.00c04fd430c8") == false);
  TASSERT (string_is_uuid ("6ba7b812-9dad-11d1-80b4-00c04fd430c8-") == false);
  TASSERT (string_is_uuid ("6ba7b812-9dad-11d1-80b4-00c04fd430c80") == false);
  TASSERT (string_is_uuid ("6ba7b812-9dad-11d1-80b4-00c04fd430c") == false);
  /* uuid string cmp */
  TASSERT (string_cmp_uuid ("00000000-0000-0000-0000-000000000000", "A425FD92-4F06-11DB-AEA9-000102E7E309") < 0);
  TASSERT (string_cmp_uuid ("A425FD92-4F06-11DB-AEA9-000102E7E309", "00000000-0000-0000-0000-000000000000") > 0);
  TASSERT (string_cmp_uuid ("00000000-0000-0000-0000-000000000000", "6ba7b812-9dad-11d1-80b4-00c04fd430c8") < 0);
  TASSERT (string_cmp_uuid ("6BA7B812-9DAD-11D1-80B4-00C04FD430C8", "00000000-0000-0000-0000-000000000000") > 0);
  TASSERT (string_cmp_uuid ("00000000-0000-0000-0000-000000000000", "00000000-0000-0000-0000-000000000000") == 0);
  TASSERT (string_cmp_uuid ("6BA7B812-9DAD-11D1-80B4-00C04FD430C8", "A425FD92-4F06-11DB-AEA9-000102E7E309") < 0);
  TASSERT (string_cmp_uuid ("6ba7b812-9dad-11d1-80b4-00c04fd430c8", "A425FD92-4F06-11DB-AEA9-000102E7E309") < 0);
  TASSERT (string_cmp_uuid ("6BA7B812-9DAD-11D1-80B4-00C04FD430C8", "a425fd92-4f06-11db-aea9-000102e7e309") < 0);
  TASSERT (string_cmp_uuid ("6ba7b812-9dad-11d1-80b4-00c04fd430c8", "a425fd92-4f06-11db-aea9-000102e7e309") < 0);
  TASSERT (string_cmp_uuid ("A425FD92-4F06-11DB-AEA9-000102E7E309", "a425fd92-4f06-11db-aea9-000102e7e309") == 0);
  TASSERT (string_cmp_uuid ("6ba7b812-9DAD-11d1-80B4-00c04fd430c8", "6BA7B812-9dad-11D1-80b4-00C04FD430C8") == 0);
  TASSERT (string_cmp_uuid ("A425FD92-4F06-11DB-AEA9-000102E7E309", "6BA7B812-9DAD-11D1-80B4-00C04FD430C8") > 0);
  TASSERT (string_cmp_uuid ("A425FD92-4F06-11DB-AEA9-000102E7E309", "6ba7b812-9dad-11d1-80b4-00c04fd430c8") > 0);
  TASSERT (string_cmp_uuid ("a425fd92-4f06-11db-aea9-000102e7e309", "6BA7B812-9DAD-11D1-80B4-00C04FD430C8") > 0);
  TASSERT (string_cmp_uuid ("a425fd92-4f06-11db-aea9-000102e7e309", "6ba7b812-9dad-11d1-80b4-00c04fd430c8") > 0);
}

static void
string_conversions (void)
{
  assert (string_from_pretty_function_name ("Anon::Type::Type(int)") == "Anon::Type::Type");
  assert (string_from_pretty_function_name ("void (** (* Anon::Class::func())(void (*zoot)(int)))(int)") == "Anon::Class::func");
  assert (string_to_cescape ("\"") == "\\\"");
  assert (string_to_cescape ("\"") == "\\\"");
  assert (string_to_cescape ("\\") == "\\\\");
  assert (string_to_cescape ("\1") == "\\001");
  assert (string_from_cquote ("''") == "");
  assert (string_from_cquote ("\"\"") == "");
  assert (string_from_cquote ("\"\\1\"") == "\1");
  assert (string_from_cquote ("\"\\11\"") == "\t");
  assert (string_from_cquote ("\"\\111\"") == "I");
  assert (string_from_cquote ("\"\\1111\"") == "I1");
  assert (string_from_cquote ("\"\\7\\8\\9\"") == "\a89");
  assert (string_from_cquote ("'foo\"bar'") == "foo\"bar");
  assert (string_from_cquote ("\"newline\\nreturn\\rtab\\tbell\\bformfeed\\fvtab\\vbackslash\\\\tick'end\"") ==
          "newline\nreturn\rtab\tbell\bformfeed\fvtab\vbackslash\\tick'end");
}

static void
split_string_tests (void)
{
  StringVector sv;
  sv = string_split ("a;b;c", ";");
  assert (string_join ("//", sv) == "a//b//c");
  assert (string_join ("_", string_split ("a;b;c;", ";")) == "a_b_c_");
  assert (string_join ("_", string_split (";;a;b;c", ";")) == "__a_b_c");
  assert (string_join ("+", string_split (" a  b \n c \t\n\r\f\v d")) == "a+b+c+d");
  assert (string_join ("^", Path::searchpath_split ("one;two;three;;")) == "one^two^three");
  assert (string_join ("^", Path::searchpath_split (";one;two;three")) == "one^two^three");
#ifdef RAPICORN_OS_UNIX
  assert (string_join ("^", Path::searchpath_split (":one:two:three:")) == "one^two^three");
#endif
  assert (string_join ("", string_split ("all  white\tspaces\nwill\vbe\fstripped")) == "allwhitespaceswillbestripped");
}

static void
test_string_charsets (void)
{
  String output;
  bool res;
  res = text_convert ("UTF-8", output, "ASCII", "Hello");
  assert (res && output == "Hello");
  /* latin1 <-> UTF-8 */
  String latin1_umlauts = "\xc4 \xd6 \xdc \xdf \xe4 \xf6 \xfc";
  String utf8_umlauts = "Ä Ö Ü ß ä ö ü";
  res = text_convert ("UTF-8", output, "LATIN1", latin1_umlauts);
  assert (res && output == utf8_umlauts);
  res = text_convert ("LATIN1", output, "UTF-8", utf8_umlauts);
  assert (res && output == latin1_umlauts);
  /* assert failinmg conversions */
  res = text_convert ("invalidNOTEXISTINGcharset", output, "UTF-8", "Hello", "");
  assert (res == false);
  /* check fallback charset (ISO-8859-15) */
  res = text_convert ("UTF-8", output, "ASCII", "Euro: \xa4");
  assert (res && output == "Euro: \xe2\x82\xac");
  /* check alias LATIN9 -> ISO-8859-15 */
  res = text_convert ("UTF-8", output, "LATIN9", "9/15: \xa4", "");
  assert (res && output == "9/15: \xe2\x82\xac");
  /* check invalid-character marks */
  res = text_convert ("UTF-8", output, "ASCII", "non-ascii \xa4 char", "", "[?]");
  assert (res && output == "non-ascii [?] char");
}

static void
test_string_stripping (void)
{
  assert (string_strip ("") == "");
  assert (string_lstrip ("") == "");
  assert (string_rstrip ("") == "");
  assert (string_strip ("123") == "123");
  assert (string_lstrip ("123") == "123");
  assert (string_rstrip ("123") == "123");
  assert (string_strip (" \t\v\f\n\r") == "");
  assert (string_lstrip (" \t\v\f\n\r") == "");
  assert (string_rstrip (" \t\v\f\n\r") == "");
  assert (string_strip (" \t\v\f\n\rX \t\v\f\n\r") == "X");
  assert (string_lstrip (" \t\v\f\n\rX \t\v\f\n\r") == "X \t\v\f\n\r");
  assert (string_rstrip (" \t\v\f\n\rX \t\v\f\n\r") == " \t\v\f\n\rX");
}

} // Anon

int
main (int   argc,
      char *argv[])
{
  rapicorn_init_test (&argc, &argv);

  Test::add ("/Strings/UUID", uuid_tests);
  Test::add ("/Strings/random unichar", random_unichar_test);
  Test::add ("/Strings/random UTF8", random_utf8_and_unichar_test);
  Test::add ("/Strings/conversions", string_conversions);
  Test::add ("/Strings/split strings", split_string_tests);
  Test::add ("/Strings/charsets", test_string_charsets);
  Test::add ("/Strings/stripping", test_string_stripping);

  return Test::run();
}

/* vim:set ts=8 sts=2 sw=2: */
