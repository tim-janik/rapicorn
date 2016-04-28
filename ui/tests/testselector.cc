/* This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0 */
#include <rcore/testutils.hh>
#include <ui/uithread.hh>
using namespace Rapicorn;

static const int64 ivec_EOL = 9223372030000000000 + 4294964579; // random marker

static vector<int64>
ivec (int64 a = ivec_EOL, int64 b = ivec_EOL, int64 c = ivec_EOL, int64 d = ivec_EOL, int64 e = ivec_EOL,
      int64 f = ivec_EOL, int64 g = ivec_EOL, int64 h = ivec_EOL, int64 i = ivec_EOL, int64 j = ivec_EOL,
      int64 k = ivec_EOL, int64 l = ivec_EOL, int64 m = ivec_EOL, int64 n = ivec_EOL, int64 o = ivec_EOL,
      int64 p = ivec_EOL, int64 q = ivec_EOL, int64 r = ivec_EOL, int64 s = ivec_EOL, int64 t = ivec_EOL,
      int64 u = ivec_EOL, int64 v = ivec_EOL, int64 w = ivec_EOL, int64 x = ivec_EOL, int64 y = ivec_EOL, int64 z = ivec_EOL)
{
  vector<int64> iv;
  if (a != ivec_EOL) iv.push_back (a);
  if (b != ivec_EOL) iv.push_back (b);
  if (c != ivec_EOL) iv.push_back (c);
  if (d != ivec_EOL) iv.push_back (d);
  if (e != ivec_EOL) iv.push_back (e);
  if (f != ivec_EOL) iv.push_back (f);
  if (g != ivec_EOL) iv.push_back (g);
  if (h != ivec_EOL) iv.push_back (h);
  if (i != ivec_EOL) iv.push_back (i);
  if (j != ivec_EOL) iv.push_back (j);
  if (k != ivec_EOL) iv.push_back (k);
  if (l != ivec_EOL) iv.push_back (l);
  if (m != ivec_EOL) iv.push_back (m);
  if (n != ivec_EOL) iv.push_back (n);
  if (o != ivec_EOL) iv.push_back (o);
  if (p != ivec_EOL) iv.push_back (p);
  if (q != ivec_EOL) iv.push_back (q);
  if (r != ivec_EOL) iv.push_back (r);
  if (s != ivec_EOL) iv.push_back (s);
  if (t != ivec_EOL) iv.push_back (t);
  if (u != ivec_EOL) iv.push_back (u);
  if (v != ivec_EOL) iv.push_back (v);
  if (w != ivec_EOL) iv.push_back (w);
  if (x != ivec_EOL) iv.push_back (x);
  if (y != ivec_EOL) iv.push_back (y);
  if (z != ivec_EOL) iv.push_back (z);
  return iv;
}

static bool
parse_match_css_nth (const char *formula, int64 pos)
{
  int64 a, b;
  if (!Selector::parse_css_nth (&formula, &a, &b) || *formula != 0)
    return false;
  return Selector::match_css_nth (pos, a, b);
}

static bool
test_css_nthset (const char *formula, const vector<int64> &matchset, const vector<int64> &unmatchset)
{
  int64 a, b;
  const char *p = formula;
  if (!Selector::parse_css_nth (&p, &a, &b) || *p != 0)
    {
      printerr ("FAILURE:%s: failed to parse css-nth formula: %s\n", __func__, formula);
      return false;
    }
  for (size_t i = 0; i < matchset.size(); i++)
    {
      const bool matched = parse_match_css_nth (formula, matchset[i]);
      if (!matched)
        {
          printerr ("FAILURE:%s: failed to match %d against: %s\n", __func__, matchset[i], formula);
          return false;
        }
    }
  for (size_t i = 0; i < unmatchset.size(); i++)
    {
      const bool matched = parse_match_css_nth (formula, unmatchset[i]);
      if (matched)
        {
          printerr ("FAILURE:%s: erroneously matched %d against: %s\n", __func__, unmatchset[i], formula);
          return false;
        }
    }
  return true;
}

static void
test_selector_primitives()
{
  using namespace Selector;
  const char *s, *o;
  TASSERT ((s = " \t\n\v\f\r") && (o = s) && parse_spaces (&s, 6) && s == o + 6);
  TASSERT ((s = " ])}>") && (o = s) && scan_nested (&s, "([{<*'/\">}])", ']') && s == o + 1);
  TASSERT ((s = " )]}>") && (o = s) && scan_nested (&s, "([{<*'/\">}])", ')') && s == o + 1);
  TASSERT ((s = " }])>") && (o = s) && scan_nested (&s, "([{<*'/\">}])", '}') && s == o + 1);
  TASSERT ((s = " >}])") && (o = s) && scan_nested (&s, "([{<*'/\">}])", '>') && s == o + 1);
  TASSERT ((s = "])}>") && (o = s) && !scan_nested (&s, "([{<*'/\">}])", '}') && s == o); // unpaired
  TASSERT ((s = ")}]>") && (o = s) && !scan_nested (&s, "([{<*'/\">}])", ']') && s == o); // unpaired
  TASSERT ((s = "}])>") && (o = s) && !scan_nested (&s, "([{<*'/\">}])", ')') && s == o); // unpaired
  TASSERT ((s = "}])>") && (o = s) && !scan_nested (&s, "([{<*'/\">}])", '>') && s == o); // unpaired
  TASSERT ((s = "1 '' {2} \"\" 3 ')' d [d\")\"] w\\\\>") && (o = s) && scan_nested (&s, "([{<*'/\">}])", '>') && s == o + 30);
  TASSERT ((s =   "w") && (o = s) && parse_case_word (&s, "w") && s == o + 1);
  TASSERT ((s =   "W") && (o = s) && parse_case_word (&s, "w") && s == o + 1);
  TASSERT ((s =   "w") && (o = s) && parse_case_word (&s, "W") && s == o + 1);
  TASSERT ((s =   "W") && (o = s) && parse_case_word (&s, "W") && s == o + 1);
  TASSERT ((s =   "a") && (o = s) && !parse_case_word (&s, "w") && s == o);
  TASSERT ((s =   "a") && (o = s) && !parse_case_word (&s, "W") && s == o);
  TASSERT ((s =   "wa") && (o = s) && parse_case_word (&s, "w") && s == o + 1);
  TASSERT ((s =   "W;") && (o = s) && parse_case_word (&s, "w") && s == o + 1);
  TASSERT ((s =   "w0") && (o = s) && parse_case_word (&s, "W") && s == o + 1);
  TASSERT ((s =   "Ww") && (o = s) && parse_case_word (&s, "W") && s == o + 1);
  uint64 u;
  TASSERT ((s = "00") && (o = s) && !parse_unsigned_integer (&s, &u) && s == o);
  TASSERT ((s = "01") && (o = s) && !parse_unsigned_integer (&s, &u) && s == o);
  TASSERT ((s = "-1") && (o = s) && !parse_unsigned_integer (&s, &u) && s == o);
  TASSERT ((s =                   "0x") && (o = s) && parse_unsigned_integer (&s, &u) && u == 0 && s == o + 1);
  TASSERT ((s =                   "1;") && (o = s) && parse_unsigned_integer (&s, &u) && u == 1 && s == o + 1);
  TASSERT ((s =                    "0") && parse_unsigned_integer (&s, &u) && u == 0);
  TASSERT ((s =                    "1") && parse_unsigned_integer (&s, &u) && u == 1);
  TASSERT ((s =   "922337203685477579") && parse_unsigned_integer (&s, &u) && u ==  922337203685477579);
  TASSERT ((s =   "922337203685477580") && parse_unsigned_integer (&s, &u) && u ==  922337203685477580);
  TASSERT ((s =   "922337203685477581") && parse_unsigned_integer (&s, &u) && u ==  922337203685477581);
  TASSERT ((s =  "922337203685477581!") && (o = s) && parse_unsigned_integer (&s, &u) && u == 922337203685477581 && s == o + 18);
  TASSERT ((s =  "9223372036854775806") && parse_unsigned_integer (&s, &u) && u == 9223372036854775806);
  TASSERT ((s =  "9223372036854775807") && parse_unsigned_integer (&s, &u) && u == 9223372036854775807);
  TASSERT ((s =  "9223372036854775808") && parse_unsigned_integer (&s, &u) && u == 9223372036854775808ULL);
  TASSERT ((s = "18446744073709551615") && parse_unsigned_integer (&s, &u) && u == 18446744073709551615ULL);
  TASSERT ((s = "18446744073709551616") && (o = s) && !parse_unsigned_integer (&s, &u) && s == o); // overflow
  TASSERT ((s = "18446744073709551617") && (o = s) && !parse_unsigned_integer (&s, &u) && s == o); // overflow
  TASSERT ((s = "18446744073709551618") && (o = s) && !parse_unsigned_integer (&s, &u) && s == o); // overflow
  TASSERT ((s = "18446744073709551619") && (o = s) && !parse_unsigned_integer (&s, &u) && s == o); // overflow
  TASSERT ((s = "20000000000000000000") && (o = s) && !parse_unsigned_integer (&s, &u) && s == o); // overflow
  int64 i;
  TASSERT ((s =  "00") && (o = s) && !parse_signed_integer (&s, &i) && s == o);
  TASSERT ((s =  "01") && (o = s) && !parse_signed_integer (&s, &i) && s == o);
  TASSERT ((s = "--1") && (o = s) && !parse_signed_integer (&s, &i) && s == o);
  TASSERT ((s = "++1") && (o = s) && !parse_signed_integer (&s, &i) && s == o);
  TASSERT ((s = "+-1") && (o = s) && !parse_signed_integer (&s, &i) && s == o);
  TASSERT ((s = "-+1") && (o = s) && !parse_signed_integer (&s, &i) && s == o);
  TASSERT ((s = "+1") && parse_signed_integer (&s, &i) && i == +1);
  TASSERT ((s = "-1") && parse_signed_integer (&s, &i) && i == -1);
  TASSERT ((s =                   "0x") && (o = s) && parse_signed_integer (&s, &i) && i ==  0 && s == o + 1);
  TASSERT ((s =                  "+0x") && (o = s) && parse_signed_integer (&s, &i) && i ==  0 && s == o + 2);
  TASSERT ((s =                  "-0x") && (o = s) && parse_signed_integer (&s, &i) && i ==  0 && s == o + 2);
  TASSERT ((s =                   "1;") && (o = s) && parse_signed_integer (&s, &i) && i == +1 && s == o + 1);
  TASSERT ((s =                  "+1;") && (o = s) && parse_signed_integer (&s, &i) && i == +1 && s == o + 2);
  TASSERT ((s =                  "-1;") && (o = s) && parse_signed_integer (&s, &i) && i == -1 && s == o + 2);
  TASSERT ((s =   "922337203685477579") && parse_signed_integer (&s, &i) && i ==   922337203685477579);
  TASSERT ((s =   "922337203685477580") && parse_signed_integer (&s, &i) && i ==   922337203685477580);
  TASSERT ((s =   "922337203685477581") && parse_signed_integer (&s, &i) && i ==   922337203685477581);
  TASSERT ((s =  "-922337203685477579") && parse_signed_integer (&s, &i) && i ==  -922337203685477579);
  TASSERT ((s =  "-922337203685477580") && parse_signed_integer (&s, &i) && i ==  -922337203685477580);
  TASSERT ((s =  "-922337203685477581") && parse_signed_integer (&s, &i) && i ==  -922337203685477581);
  TASSERT ((s =  "922337203685477581!") && (o = s) && parse_signed_integer (&s, &i) && i == 922337203685477581 && s == o + 18);
  TASSERT ((s =  "9223372036854775806") && parse_signed_integer (&s, &i) && i ==  9223372036854775806);
  TASSERT ((s = "-9223372036854775806") && parse_signed_integer (&s, &i) && i == -9223372036854775806);
  TASSERT ((s =  "9223372036854775807") && parse_signed_integer (&s, &i) && i ==  9223372036854775807);
  TASSERT ((s = "-9223372036854775807") && parse_signed_integer (&s, &i) && i == -9223372036854775807);
  TASSERT ((s = "-9223372036854775808") && parse_signed_integer (&s, &i) && i == int64 (-9223372036854775807-1));
  TASSERT ((s = "-9223372036854775809") && (o = s) && !parse_signed_integer (&s, &i) && s == o); // overflow
  TASSERT ((s =  "9223372036854775808") && (o = s) && !parse_signed_integer (&s, &i) && s == o); // overflow
  TASSERT ((s =  "9223372036854775809") && (o = s) && !parse_signed_integer (&s, &i) && s == o); // overflow
  TASSERT ((s = "10000000000000000000") && (o = s) && !parse_signed_integer (&s, &i) && s == o); // overflow
  // strings & unicode
  String string;
  TASSERT ((s =                            "x") && (o = s) && !parse_string (&s, string) && s == o && string == "");
  TASSERT ((s =                            "''") && (o = s) && parse_string (&s, string) && s == o + 2 && string == "");
  TASSERT ((s =                         "'\\''") && (o = s) && parse_string (&s, string) && s == o + 4 && string == "'");
  TASSERT ((s =                          "\"\"") && (o = s) && parse_string (&s, string) && s == o + 2 && string == "");
  TASSERT ((s =                           "'a'") && (o = s) && parse_string (&s, string) && s == o + 3 && string == "a");
  TASSERT ((s =                         "\"a\"") && (o = s) && parse_string (&s, string) && s == o + 3 && string == "a");
  TASSERT ((s = "'_abcdefghijklmnopqrstuvwxyz_'") && (o = s) && parse_string (&s, string) && s == o + 30 && string == "_abcdefghijklmnopqrstuvwxyz_");
  TASSERT ((s = "'_ABCDEFGHIJKLMNOPQRSTUVWXYZ_'") && (o = s) && parse_string (&s, string) && s == o + 30 && string == "_ABCDEFGHIJKLMNOPQRSTUVWXYZ_");
  TASSERT ((s =               "\"-0123456789-\"") && (o = s) && parse_string (&s, string) && s == o + 14 && string == "-0123456789-");
  TASSERT ((s =    "'_\\\n_\\*\\:\\ \\\\_\\2a_'") && (o = s) && parse_string (&s, string) && s == o + 19 && string == "_\n_*: \\_*_");
  TASSERT ((s =          "\"* \\2a  * \\2A  *\"") && (o = s) && parse_string (&s, string) && s == o + 17 && string == "* * * * *");
  TASSERT ((s =               "\"abc\\\r\ndef\"") && (o = s) && parse_string (&s, string) && s == o + 11 && string == "abc\r\ndef");
  // identifier & unicode
  String ident;
  TASSERT ((s = "a") && (o = s) && parse_identifier (&s, ident) && s == o + 1 && ident == "a");
  TASSERT ((s = "\\2a \\2A") && (o = s) && parse_identifier (&s, ident) && s == o + 7 && ident == "**");
  TASSERT ((s = "\\02A_\\00007E0 ") && (o = s) && parse_identifier (&s, ident) && s == o + 13 && ident == "*_~0");
  TASSERT ((s = "_abcdefghijklmnopqrstuvwxyz_") && (o = s) && parse_identifier (&s, ident) && s == o + 28 && ident == o);
  TASSERT ((s = "_ABCDEFGHIJKLMNOPQRSTUVWXYZ_") && (o = s) && parse_identifier (&s, ident) && s == o + 28 && ident == o);
  TASSERT ((s = "-_0123456789-XYZ") && (o = s) && parse_identifier (&s, ident) && s == o + 16 && ident == o);
  TASSERT ((s = "__\\*\\:\\ \\\\_\\2a_") && (o = s) && parse_identifier (&s, ident) && s == o + 15 && ident == "__*: \\_*_");
  int64 a, b;
  TASSERT ((s =         "OdD") && (o = s) && parse_css_nth (&s, &a, &b) && a ==   2 && b ==   1 && s == o + 3);
  TASSERT ((s =      " eVeN ") && (o = s) && parse_css_nth (&s, &a, &b) && a ==   2 && b ==   0 && s == o + 6);
  TASSERT ((s =         "-99") && (o = s) && parse_css_nth (&s, &a, &b) && a ==   0 && b == -99 && s == o + 3);
  TASSERT ((s =          "+0") && (o = s) && parse_css_nth (&s, &a, &b) && a ==   0 && b ==   0 && s == o + 2);
  TASSERT ((s =           "9") && (o = s) && parse_css_nth (&s, &a, &b) && a ==   0 && b ==   9 && s == o + 1);
  TASSERT ((s = " -99N + 77 ") && (o = s) && parse_css_nth (&s, &a, &b) && a == -99 && b == +77 && s == o + 11);
  TASSERT ((s = " -7n  -  3 ") && (o = s) && parse_css_nth (&s, &a, &b) && a ==  -7 && b ==  -3 && s == o + 11);
  TASSERT ((s =  "+32N - 11 ") && (o = s) && parse_css_nth (&s, &a, &b) && a == +32 && b == -11 && s == o + 10);
  TASSERT ((s =  "  +3n +7  ") && (o = s) && parse_css_nth (&s, &a, &b) && a ==   3 && b ==   7 && s == o + 10);
  TASSERT ((s =     "N +  0 ") && (o = s) && parse_css_nth (&s, &a, &b) && a ==   1 && b ==   0 && s == o + 7);
  TASSERT ((s =          " n") && (o = s) && parse_css_nth (&s, &a, &b) && a ==   1 && b ==   0 && s == o + 2);
  TASSERT ((s =        "2n+1") && (o = s) && parse_css_nth (&s, &a, &b) && a ==   2 && b ==   1 && s == o + 4);
  TASSERT ((s =        "2n+0") && (o = s) && parse_css_nth (&s, &a, &b) && a ==   2 && b ==   0 && s == o + 4);
  TASSERT ((s =        "4n+0") && (o = s) && parse_css_nth (&s, &a, &b) && a ==   4 && b ==   0 && s == o + 4);
  TASSERT ((s =        "4n+1") && (o = s) && parse_css_nth (&s, &a, &b) && a ==   4 && b ==   1 && s == o + 4);
  TASSERT ((s =        "4n+2") && (o = s) && parse_css_nth (&s, &a, &b) && a ==   4 && b ==   2 && s == o + 4);
  TASSERT ((s =        "4n+3") && (o = s) && parse_css_nth (&s, &a, &b) && a ==   4 && b ==   3 && s == o + 4);
  TASSERT ((s =        "n -5") && (o = s) && parse_css_nth (&s, &a, &b) && a ==   1 && b ==  -5 && s == o + 4);
  TASSERT ((s =       "10n-1") && (o = s) && parse_css_nth (&s, &a, &b) && a ==  10 && b ==  -1 && s == o + 5);
  TASSERT ((s =       "10n+9") && (o = s) && parse_css_nth (&s, &a, &b) && a ==  10 && b ==  +9 && s == o + 5);
  TASSERT ((s =       "+-10n") && (o = s) && !parse_css_nth (&s, &a, &b) && s == o);
  TASSERT ((s =        "0n+5") && (o = s) && parse_css_nth (&s, &a, &b) && a ==   0 && b ==  +5 && s == o + 4);
  TASSERT ((s =           "5") && (o = s) && parse_css_nth (&s, &a, &b) && a ==   0 && b ==  +5 && s == o + 1);
  TASSERT ((s =        "1n+0") && (o = s) && parse_css_nth (&s, &a, &b) && a ==   1 && b ==   0 && s == o + 4);
  TASSERT ((s =         "n+0") && (o = s) && parse_css_nth (&s, &a, &b) && a ==   1 && b ==   0 && s == o + 3);
  TASSERT ((s =           "n") && (o = s) && parse_css_nth (&s, &a, &b) && a ==   1 && b ==   0 && s == o + 1);
  TASSERT ((s =          "2n") && (o = s) && parse_css_nth (&s, &a, &b) && a ==   2 && b ==   0 && s == o + 2);
  TASSERT ((s =         "+3n") && (o = s) && parse_css_nth (&s, &a, &b) && a ==   3 && b ==   0 && s == o + 3);
  TASSERT ((s =         "-5n") && (o = s) && parse_css_nth (&s, &a, &b) && a ==  -5 && b ==   0 && s == o + 3);
  TASSERT ((s =    " 3n + 1 ") && (o = s) && parse_css_nth (&s, &a, &b) && a ==   3 && b ==   1 && s == o + 8);
  TASSERT ((s =   " +3n - 2 ") && (o = s) && parse_css_nth (&s, &a, &b) && a ==   3 && b ==  -2 && s == o + 9);
  TASSERT ((s =      " -n+ 6") && (o = s) && parse_css_nth (&s, &a, &b) && a ==  -1 && b ==   6 && s == o + 6);
  TASSERT ((s =        " +6 ") && (o = s) && parse_css_nth (&s, &a, &b) && a ==   0 && b ==   6 && s == o + 4);
  TASSERT ((s =        "-n+6") && (o = s) && parse_css_nth (&s, &a, &b) && a ==  -1 && b ==   6 && s == o + 4);
  TASSERT ((s =         "3 n") && (o = s) && parse_css_nth (&s, &a, &b) && a ==   0 && b ==   3 && s == o + 2); /* "3 " */
  TASSERT ((s =        "+ 2n") && (o = s) && !parse_css_nth (&s, &a, &b) && s == o);
  TASSERT ((s =         "+ 2") && (o = s) && !parse_css_nth (&s, &a, &b) && s == o);
  TASSERT ((s = "-9223372036854775808n") && (o = s) && parse_css_nth (&s, &a, &b) && a == int64 (-9223372036854775807-1) && b == 0 && s == o + 21);
  TASSERT ((s = "n-9223372036854775808") && (o = s) && parse_css_nth (&s, &a, &b) && a == 1 && b == int64 (-9223372036854775807-1) && s == o + 21);
  int64 n;
  TASSERT ((s =  "2n+1") && (n = 0,1) && !parse_match_css_nth (s, n));
  TASSERT ((s =  "2n+1") && (n = 1) && parse_match_css_nth (s, n));
  TASSERT ((s =  "2n+1") && (n = 2) && !parse_match_css_nth (s, n));
  TASSERT ((s =  "2n+1") && (n = 3) && parse_match_css_nth (s, n));
  TASSERT ((s =  "even") && (n = 0,1) && !parse_match_css_nth (s, n));
  TASSERT ((s =  "even") && (n = 1) && !parse_match_css_nth (s, n));
  TASSERT ((s =  "even") && (n = 2) && parse_match_css_nth (s, n));
  TASSERT ((s =  "even") && (n = 3) && !parse_match_css_nth (s, n));
  TASSERT ((s =  "even") && (n = 4) && parse_match_css_nth (s, n));
  TASSERT ((s =  "even") && (n = 5) && !parse_match_css_nth (s, n));
  TASSERT ((s =  "-n+6") && (n = 1) && parse_match_css_nth (s, n));
  TASSERT (test_css_nthset ("n", ivec (1, 2, 3, 4, 5, 100, 999, 1000, 1001, 1002, 1003), ivec (-2, -1, 0)));
  TASSERT (test_css_nthset ("n+2", ivec (2, 3, 4, 5, 100, 999, 1000, 1001, 1002, 1003), ivec (-2, -1, 0, 1)));
  TASSERT (test_css_nthset ("2n+0", ivec (2, 4, 6, 8, 10, 100, 998, 1000, 1002), ivec (1, 3, 5, 7, 9, 11, 101, 999, 1001)));
  TASSERT (test_css_nthset ("2n+1", ivec (1, 3, 5, 7, 9, 11, 101, 999, 1001), ivec (2, 4, 6, 8, 10, 100, 998, 1000, 1002)));
  TASSERT (test_css_nthset ("4n+0", ivec (4, 8, 12, 16, 20), ivec (1, 2, 3, 5, 6, 7, 9, 10, 11, 13, 14, 15, 17, 18, 19)));
  TASSERT (test_css_nthset ("4n+1", ivec (1, 5, 9, 13, 17), ivec (2, 3, 4, 6, 7, 8, 10, 11, 12, 14, 15, 16)));
  TASSERT (test_css_nthset ("4n+2", ivec (2, 6, 10, 14, 18), ivec (1, 3, 4, 5, 7, 8, 9, 11, 12, 13, 15, 16, 17)));
  TASSERT (test_css_nthset ("4n+3", ivec (3, 7, 11, 15, 19), ivec (1, 2, 4, 5, 6, 8, 9, 10, 12, 13, 14, 16, 17, 18)));
  TASSERT (test_css_nthset ("4n+4", ivec (4, 8, 12, 16, 20), ivec (1, 2, 3, 5, 6, 7, 9, 10, 11, 13, 14, 15, 17, 18, 19)));
  TASSERT (test_css_nthset ("10n-1", ivec (9, 19, 29, 39, 49, 59, 69, 79, 89, 99, 109, 119, 999, 9999),
                            ivec (1, 2, 3, 4, 5, 6, 7, 8, 10, 11, 12, 18, 20, 21, 28, 30, 31, 32, 38, 40, 41)));
  TASSERT (test_css_nthset ("10n+9", ivec (9, 19, 29, 39, 49, 59, 69, 79, 89, 99, 109, 119, 999, 9999),
                            ivec (1, 2, 3, 4, 5, 6, 7, 8, 10, 11, 12, 18, 20, 21, 28, 30, 31, 32, 38, 40, 41)));
  TASSERT (test_css_nthset ("0n+5", ivec (5), ivec (1, 2, 3, 4, 6, 7, 8, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24)));
  TASSERT (test_css_nthset ("7", ivec (7), ivec (1, 2, 3, 4, 5, 6, 8, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24)));
  TASSERT (test_css_nthset ("n", ivec (1, 2, 3, 4, 5, 6, 7, 8, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24), ivec()));
  TASSERT (test_css_nthset ("-n+6", ivec (1, 2, 3, 4, 5, 6), ivec (7, 8, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24)));
  TASSERT (test_css_nthset ("-2n+6", ivec (2, 4, 6), ivec (1, 3, 5, 7, 8, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24)));
  TASSERT (test_css_nthset ("3n+2", ivec (2, 5, 8, 11, 14), ivec (1, 3, 4, 6, 7, 9, 10, 12, 13, 15, 16)));
}
REGISTER_UITHREAD_TEST ("Selector/Basic Parsing Primitives", test_selector_primitives);

#define SN(...)       ({ SelectorNode n (__VA_ARGS__); n; })
static const Selector::SelectorNode __csr0 (Selector::NONE);
typedef const Selector::SelectorNode &Csr;

static Selector::SelectorChain
schain (Csr a = __csr0, Csr b = __csr0, Csr c = __csr0, Csr d = __csr0, Csr e = __csr0, Csr f = __csr0, Csr g = __csr0,
        Csr h = __csr0, Csr i = __csr0, Csr j = __csr0, Csr k = __csr0, Csr l = __csr0, Csr m = __csr0, Csr n = __csr0)
{
  Selector::SelectorChain sc;
  if (a != __csr0) sc.push_back (a);
  if (b != __csr0) sc.push_back (b);
  if (c != __csr0) sc.push_back (c);
  if (d != __csr0) sc.push_back (d);
  if (e != __csr0) sc.push_back (e);
  if (f != __csr0) sc.push_back (f);
  if (g != __csr0) sc.push_back (g);
  if (h != __csr0) sc.push_back (h);
  if (i != __csr0) sc.push_back (i);
  if (j != __csr0) sc.push_back (j);
  if (k != __csr0) sc.push_back (k);
  if (l != __csr0) sc.push_back (l);
  if (m != __csr0) sc.push_back (m);
  if (n != __csr0) sc.push_back (n);
  return sc;
}

static void
test_selector_parser()
{
  using namespace Selector;
  const char *s, *o;
  // element selectors
  Selector::SelectorChain sc;
  TASSERT ((s = "*") && (o = s) && sc.parse (&s) && s == o + 1 && sc == schain (SN (UNIVERSAL, "*")));
  TASSERT ((s = "ABC") && (o = s) && sc.parse (&s) && s == o + 3 && sc == schain (SN (TYPE, "ABC")));
  TASSERT ((s = "A B") && (o = s) && sc.parse (&s) && s == o + 3 &&
           sc == schain (SN (TYPE, "A"), SN (DESCENDANT, ""), SN (TYPE, "B")));
  TASSERT ((s = "A>B") && (o = s) && sc.parse (&s) && s == o + 3 &&
           sc == schain (SN (TYPE, "A"), SN (CHILD, ""), SN (TYPE, "B")));
  TASSERT ((s = "A ~ B") && (o = s) && sc.parse (&s) && s == o + 5 &&
           sc == schain (SN (TYPE, "A"), SN (NEIGHBORING, ""), SN (TYPE, "B")));
  TASSERT ((s = "A+ B") && (o = s) && sc.parse (&s) && s == o + 4 &&
           sc == schain (SN (TYPE, "A"), SN (ADJACENT, ""), SN (TYPE, "B")));
  TASSERT ((s = "A +B") && (o = s) && sc.parse (&s) && s == o + 4 &&
           sc == schain (SN (TYPE, "A"), SN (ADJACENT, ""), SN (TYPE, "B")));
  TASSERT ((s = "A B > * + D ~ E") && (o = s) && sc.parse (&s) && s == o + 15 &&
           sc == schain (SN (TYPE, "A"), SN (DESCENDANT, ""), SN (TYPE, "B"), SN (CHILD, ""), SN (UNIVERSAL, "*"),
                         SN (ADJACENT, ""), SN (TYPE, "D"), SN (NEIGHBORING, ""), SN (TYPE, "E")));
  TASSERT ((s = "A B > C! + D ~ E") && (o = s) && sc.parse (&s) && s == o + 16 &&
           sc == schain (SN (TYPE, "A"), SN (DESCENDANT, ""), SN (TYPE, "B"), SN (CHILD, ""), SN (TYPE, "C"), SN (SUBJECT),
                         SN (ADJACENT, ""), SN (TYPE, "D"), SN (NEIGHBORING, ""), SN (TYPE, "E")));
  // attribute and id selectors
  TASSERT ((s =            "A#id") && (o = s) && sc.parse (&s) && s == o +  4 && sc == schain (SN (TYPE, "A"), SN (ID, "id")));
  TASSERT ((s =             "A.C") && (o = s) && sc.parse (&s) && s == o +  3 && sc == schain (SN (TYPE, "A"), SN (CLASS, "C")));
  TASSERT ((s =            "A[b]") && (o = s) && sc.parse (&s) && s == o +  4 && sc == schain (SN (TYPE, "A"), SN (ATTRIBUTE_EXISTS, "b")));
  TASSERT ((s =          "A[b i]") && (o = s) && sc.parse (&s) && s == o +  6 && sc == schain (SN (TYPE, "A"), SN (ATTRIBUTE_EXISTS_I, "b")));
  TASSERT ((s =          "A[b=c]") && (o = s) && sc.parse (&s) && s == o +  6 && sc == schain (SN (TYPE, "A"), SN (ATTRIBUTE_EQUALS, "b", "c")));
  TASSERT ((s =         "A[b!=c]") && (o = s) && sc.parse (&s) && s == o +  7 && sc == schain (SN (TYPE, "A"), SN (ATTRIBUTE_UNEQUALS, "b", "c")));
  TASSERT ((s =           "[b=c]") && (o = s) && sc.parse (&s) && s == o +  5 && sc == schain (SN (ATTRIBUTE_EQUALS, "b", "c")));
  TASSERT ((s =   "[b=c]! > #fun") && (o = s) && sc.parse (&s) && s == o + 13 &&
           sc == schain (SN (ATTRIBUTE_EQUALS, "b", "c"), SN (SUBJECT), SN (CHILD), SN (ID, "fun")));
  TASSERT ((s =   "A[b = \"c\" ]") && (o = s) && sc.parse (&s) && s == o + 11 && sc == schain (SN (TYPE, "A"), SN (ATTRIBUTE_EQUALS, "b", "c")));
  TASSERT ((s =     "A[b = 'c' ]") && (o = s) && sc.parse (&s) && s == o + 11 && sc == schain (SN (TYPE, "A"), SN (ATTRIBUTE_EQUALS, "b", "c")));
  TASSERT ((s =    "A[b = 'c' i]") && (o = s) && sc.parse (&s) && s == o + 12 && sc == schain (SN (TYPE, "A"), SN (ATTRIBUTE_EQUALS_I, "b", "c")));
  TASSERT ((s =    "A[b == 'c' ]") && (o = s) && sc.parse (&s) && s == o + 12 && sc == schain (SN (TYPE, "A"), SN (ATTRIBUTE_EQUALS, "b", "c")));
  TASSERT ((s =   "A[b == 'c' i]") && (o = s) && sc.parse (&s) && s == o + 13 && sc == schain (SN (TYPE, "A"), SN (ATTRIBUTE_EQUALS_I, "b", "c")));
  TASSERT ((s =    "A[b != 'c' ]") && (o = s) && sc.parse (&s) && s == o + 12 && sc == schain (SN (TYPE, "A"), SN (ATTRIBUTE_UNEQUALS, "b", "c")));
  TASSERT ((s =   "A[b != 'c' i]") && (o = s) && sc.parse (&s) && s == o + 13 && sc == schain (SN (TYPE, "A"), SN (ATTRIBUTE_UNEQUALS_I, "b", "c")));
  TASSERT ((s =       "A[b |=c ]") && (o = s) && sc.parse (&s) && s == o +  9 && sc == schain (SN (TYPE, "A"), SN (ATTRIBUTE_DASHSTART, "b", "c")));
  TASSERT ((s =      "A[b |=c i]") && (o = s) && sc.parse (&s) && s == o + 10 && sc == schain (SN (TYPE, "A"), SN (ATTRIBUTE_DASHSTART_I, "b", "c")));
  TASSERT ((s =      "A[b^= 'c']") && (o = s) && sc.parse (&s) && s == o + 10 && sc == schain (SN (TYPE, "A"), SN (ATTRIBUTE_PREFIX, "b", "c")));
  TASSERT ((s =    "A[b^= 'c' i]") && (o = s) && sc.parse (&s) && s == o + 12 && sc == schain (SN (TYPE, "A"), SN (ATTRIBUTE_PREFIX_I, "b", "c")));
  TASSERT ((s =       "A[b $= c]") && (o = s) && sc.parse (&s) && s == o +  9 && sc == schain (SN (TYPE, "A"), SN (ATTRIBUTE_SUFFIX, "b", "c")));
  TASSERT ((s =     "A[b $= c i]") && (o = s) && sc.parse (&s) && s == o + 11 && sc == schain (SN (TYPE, "A"), SN (ATTRIBUTE_SUFFIX_I, "b", "c")));
  TASSERT ((s =      "A[b *='c']") && (o = s) && sc.parse (&s) && s == o + 10 && sc == schain (SN (TYPE, "A"), SN (ATTRIBUTE_SUBSTRING, "b", "c")));
  TASSERT ((s =    "A[b *='c' i]") && (o = s) && sc.parse (&s) && s == o + 12 && sc == schain (SN (TYPE, "A"), SN (ATTRIBUTE_SUBSTRING_I, "b", "c")));
  TASSERT ((s =       "A[b ~= c]") && (o = s) && sc.parse (&s) && s == o +  9 && sc == schain (SN (TYPE, "A"), SN (ATTRIBUTE_INCLUDES, "b", "c")));
  TASSERT ((s =     "A[b ~= c i]") && (o = s) && sc.parse (&s) && s == o + 11 && sc == schain (SN (TYPE, "A"), SN (ATTRIBUTE_INCLUDES_I, "b", "c")));
  TASSERT ((s = "A[b~=c][d1*=e2]") && (o = s) && sc.parse (&s) && s == o + 15 &&
           sc == schain (SN (TYPE, "A"), SN (ATTRIBUTE_INCLUDES, "b", "c"), SN (ATTRIBUTE_SUBSTRING, "d1", "e2")));
  // pseudo selectors
  TASSERT ((s =  "*::before") && (o = s) && sc.parse (&s) && s == o +  9 && sc == schain (SN (UNIVERSAL, "*"), SN (PSEUDO_ELEMENT, "before")));
  TASSERT ((s =      ":root") && (o = s) && sc.parse (&s) && s == o +  5 && sc == schain (SN (PSEUDO_CLASS, "root")));
  TASSERT ((s =  ":root ~ C") && (o = s) && sc.parse (&s) && s == o +  9 && sc == schain (SN (PSEUDO_CLASS, "root"), SN (NEIGHBORING), SN (TYPE, "C")));
  TASSERT ((s = ":root! ~ C") && (o = s) && sc.parse (&s) && s == o + 10 && sc == schain (SN (PSEUDO_CLASS, "root"), SN (SUBJECT), SN (NEIGHBORING), SN (TYPE, "C")));
  TASSERT ((s =     "A:root") && (o = s) && sc.parse (&s) && s == o +  6 && sc == schain (SN (TYPE, "A"), SN (PSEUDO_CLASS, "root")));
  TASSERT ((s =     "A:root") && (o = s) && sc.parse (&s) && s == o +  6 && sc == schain (SN (TYPE, "A"), SN (PSEUDO_CLASS, "root")));
  TASSERT ((s =      ":root! ~ ::after") && (o = s) && sc.parse (&s) && s == o + 16 && sc == schain (SN (PSEUDO_CLASS, "root"), SN (SUBJECT), SN (NEIGHBORING), SN (PSEUDO_ELEMENT, "after")));
  TASSERT ((s = ":root! ~ :first-child") && (o = s) && sc.parse (&s) && s == o + 21 && sc == schain (SN (PSEUDO_CLASS, "root"), SN (SUBJECT), SN (NEIGHBORING), SN (PSEUDO_CLASS, "first-child")));
  TASSERT ((s =     "*:nth-child(2n+1)") && (o = s) && sc.parse (&s) && s == o + 17 && sc == schain (SN (UNIVERSAL, "*"), SN (PSEUDO_CLASS, "nth-child", "2n+1")));
  TASSERT ((s =   "*:nth-child()") && (o = s) && sc.parse (&s) && s == o + 1 && sc == schain (SN (UNIVERSAL, "*"))); // invalid empty espression
  TASSERT ((s =  "*:nth-child( )") && (o = s) && sc.parse (&s) && s == o + 1 && sc == schain (SN (UNIVERSAL, "*"))); // invalid empty espression
  TASSERT ((s =  "*:nth-child(2)") && (o = s) && sc.parse (&s) && s == o + 14 && sc == schain (SN (UNIVERSAL, "*"), SN (PSEUDO_CLASS, "nth-child", "2"))); // valid 1char expression
  TASSERT ((s = "*:nth-child(  )") && (o = s) && sc.parse (&s) && s == o + 1 && sc == schain (SN (UNIVERSAL, "*"))); // invalid empty espression
  TASSERT ((s = "*:nth-child(2n)") && (o = s) && sc.parse (&s) && s == o + 15 && sc == schain (SN (UNIVERSAL, "*"), SN (PSEUDO_CLASS, "nth-child", "2n"))); // valid 2char expression
  TASSERT ((s = "*:nth-last-of-type( -3n-1 )::after") && (o = s) && sc.parse (&s) && s == o + 34 && sc == schain (SN (UNIVERSAL, "*"), SN (PSEUDO_CLASS, "nth-last-of-type", "-3n-1"), SN (PSEUDO_ELEMENT, "after")));
  TASSERT ((s = "A[b$='c\\\nc']:lang('foo')::first-letter > D") && (o = s) && sc.parse (&s) && s == o + 42 && sc == schain (SN (TYPE, "A"), SN (ATTRIBUTE_SUFFIX, "b", "c\nc"), SN (PSEUDO_CLASS, "lang", "'foo'"), SN (PSEUDO_ELEMENT, "first-letter"), SN (CHILD), SN (TYPE, "D")));
  // stringify selector expression
  TASSERT ((s = "A B > * + D ~ E") && (o = s) && sc.parse (&s) && o == sc.string());
  TASSERT ((s = "A[b$='c\\0a c']:lang('foo[]')::first-letter > D! + E") && (o = s) && sc.parse (&s) && o == sc.string());
  TASSERT ((s = "*[a^=b][a^=b i][c$=d][c$=d i][e*=f][e*=f i][g~=h][g~=h i][i|=j][i|=j i][k!=l][k!=l i][m=n][m=n i][o][p i]") && (o = s) && sc.parse (&s) && o == sc.string());
  TASSERT ((s = "A B > C + D ~ E") && (o = s) && sc.parse (&s) && o == sc.string());
  TASSERT ((s = "A! B > C + D ~ E") && (o = s) && sc.parse (&s) && o == sc.string());
  TASSERT ((s = "A B! > C + D ~ E") && (o = s) && sc.parse (&s) && o == sc.string());
  TASSERT ((s = "A B > C! + D ~ E") && (o = s) && sc.parse (&s) && o == sc.string());
  TASSERT ((s = "A B > C + D! ~ E") && (o = s) && sc.parse (&s) && o == sc.string());
}
REGISTER_UITHREAD_TEST ("Selector/Combinator Parsing", test_selector_parser);

static void
test_selector_validation ()
{
  const vector<Selector::Selob*> empty_selobs;
  String errstr;
  errstr = ""; Selector::Matcher::query_selector_objects ("Window * .VBox > Button:baz ~ Frame:bar(7) + Label::after!", empty_selobs.begin(), empty_selobs.end(), &errstr);
  TCMP (errstr, ==, ""); // OK
  errstr = ""; Selector::Matcher::query_selector_objects ("> >> ~", empty_selobs.begin(), empty_selobs.end(), &errstr);
  TCMP (errstr, !=, ""); // snytax
  errstr = ""; Selector::Matcher::query_selector_objects ("Window! VBox || <invalidchars>", empty_selobs.begin(), empty_selobs.end(), &errstr);
  TCMP (errstr, !=, ""); // junk
  errstr = ""; Selector::Matcher::query_selector_objects ("Window! VBox! Button", empty_selobs.begin(), empty_selobs.end(), &errstr);
  TCMP (errstr, !=, ""); // multiple subjects
  errstr = ""; Selector::Matcher::query_selector_objects ("Window::after > VBox", empty_selobs.begin(), empty_selobs.end(), &errstr);
  TCMP (errstr, !=, ""); // combinator after pseudo
  errstr = ""; Selector::Matcher::query_selector_objects ("Window! > VBox::after", empty_selobs.begin(), empty_selobs.end(), &errstr);
  TCMP (errstr, !=, ""); // pseudo on non-subject
}
REGISTER_UITHREAD_TEST ("Selector/Validation", test_selector_validation);

static void
test_query (int line, WidgetIfaceP iroot, const String &selector, ssize_t expect, const String &expected_type = "")
{
  WidgetImpl *root = shared_ptr_cast<WidgetImpl> (iroot).get();
  TASSERT (root != NULL);

  WidgetIfaceP query_first = root->query_selector (selector);
  WidgetIfaceP query_unique = root->query_selector_unique (selector);
  WidgetSeq query_all = root->query_selector_all (selector);

  if (Test::verbose())
    {
      if (query_all.empty())
        printerr ("%s:%d:%s(expect=%d): %s: %s\n", __FILE__, line, __func__, expect, string_to_cquote (selector).c_str(), "...none...");
      else
        for (size_t i = 0; i < query_all.size(); i++)
          printerr ("%s:%d:%s(expect=%d): %s: %s (%p)\n", __FILE__, line, i ? string_canonify (__func__, "", " ").c_str() : __func__,
                    expect, string_to_cquote (selector).c_str(), query_all[i]->name().c_str(), query_all[i]);
    }

  TASSERT_AT (line, expect == 0 || query_first != NULL);
  TASSERT_AT (line, expect == 1 || query_unique == NULL);
  TASSERT_AT (line, expect != 1 || query_unique != NULL);
  TASSERT_AT (line, query_unique == NULL || query_unique == query_first);
  TASSERT_AT (line, query_all.size() >= (query_first != NULL));
  TASSERT_AT (line, query_first == NULL || query_all[0].get() == query_first.get()); // query_all.size() >= 1
  if (expect >= 0)
    {
      size_t expected = expect;
      TASSERT_AT (line, query_all.size() == expected);
    }
  else // expect < 0
    {
      size_t expected = -expect;
      TASSERT_AT (line, query_all.size() > expected);
    }

  if (!expected_type.empty())
    for (size_t i = 0; i < query_all.size(); i++)
      TASSERT_AT (line, query_all[i]->match_selector (expected_type));
}

static void load_ui_defs ();

static void
test_selector_matching ()
{
  load_ui_defs();
  ApplicationImpl &app = ApplicationImpl::the(); // FIXME: use Application_RemoteHandle once C++ bindings are ready

  WindowList wl = app.query_windows ("*");
  size_t prev_window_count = wl.size();

  WindowIfaceP window = app.create_window ("test-dialog");
  TASSERT (window != NULL);

  /* Here, the window has been created, but some updates, e.g. creating inner label/text widgets
   * are queued as main loop handlers. we need those completing executed before we make conclusive
   * assertions on the widget tree.
   * Forcing the main loop like this is *not* generally recommended, in this confined test setup
   * it's useful though.
   */
  uithread_main_loop ()->iterate_pending();

  WindowIfaceP w = app.query_window ("#test-dialog");
  TASSERT (window == w);

  wl = app.query_windows ("*");
  TASSERT (wl.size() == prev_window_count + 1);

  test_query (__LINE__, w, "/#", 0); // invalid path
  test_query (__LINE__, w, "X/#test-dialog", 0); // invalid syntax (junk)
  // combinators
  test_query (__LINE__, w, "* VBox  Button > Frame Label", 4, "Label"); // 4 from n
  test_query (__LINE__, w, "* VBox Button! > Frame Label", 4, "Button");
  test_query (__LINE__, w, "* VBox Button! > Frame #label1", 1, "Button");
  test_query (__LINE__, w, "* VBox Button! > Frame #label1", 1, "Button");
  test_query (__LINE__, w, "* VBox Button! > Frame #label1", 1, "Button");
  test_query (__LINE__, w, "* VBox Button > Frame #label1", 1, "Label");
  test_query (__LINE__, w, "* VBox Button > Frame #label1", 1, "Label");
  test_query (__LINE__, w, "* VBox  Button > Frame #label1", 1, "Label");
  test_query (__LINE__, w, "* VBox Frame > #special-arrow", 1, "Arrow");
  test_query (__LINE__, w, "* VBox Frame > #special-arrow", 1, "Arrow");
  test_query (__LINE__, w, "* VBox Frame > #special-arrow", 1, "Arrow");
  // identifiers
  test_query (__LINE__, w, "* #special-arrow", 1);
  test_query (__LINE__, w, "* #special-arrow", 1);
  test_query (__LINE__, w, "* #special-arrow", 1);
  test_query (__LINE__, w, "* Label #special-arrow", 0);
  test_query (__LINE__, w, "* Label #special-arrow", 0);
  test_query (__LINE__, w, "* Label #special-arrow", 0);
  // attributes
  test_query (__LINE__, w, "* #label123[frotz-xxxz]", 0); // FAIL
  test_query (__LINE__, w, "* #label123[plain-text]", 1, "Label");
  test_query (__LINE__, w, "* #label123[plain-text i]", 1, "Label");
  test_query (__LINE__, w, "* #label123[plain-text=FAIL]", 0);
  test_query (__LINE__, w, "* #label123[plain-text=one-two-three]", 1, "Label");
  test_query (__LINE__, w, "* #label123[plain-text!=one-tWo-three]", 1, "Label");
  test_query (__LINE__, w, "* #label123[plain-text!=one-two-three]", 0); // FAIL
  test_query (__LINE__, w, "* #label123[plain-text^=one]", 1, "Label");
  test_query (__LINE__, w, "* #label123[plain-text^=oNe i]", 1, "Label");
  test_query (__LINE__, w, "* #label123[plain-text^=FAIL]", 0);
  test_query (__LINE__, w, "* #label123[plain-text$=three]", 1, "Label");
  test_query (__LINE__, w, "* #label123[plain-text$=thREE i]", 1, "Label");
  test_query (__LINE__, w, "* #label123[plain-text$=FAIL]", 0);
  test_query (__LINE__, w, "* #label123[plain-text|=one]", 1, "Label");
  test_query (__LINE__, w, "* #label123[plain-text|=OnE]", 0); // FAIL
  test_query (__LINE__, w, "* #label123[plain-text|=oNe i]", 1, "Label");
  test_query (__LINE__, w, "* #label123[plain-text|=FAIL]", 0);
  test_query (__LINE__, w, "* #label123[plain-text*=e-two-t]", 1, "Label");
  test_query (__LINE__, w, "* #label123[plain-text*=e-TWO-t i]", 1, "Label");
  test_query (__LINE__, w, "* #label123[plain-text*=e-TwO-t]", 0); // FAIL
  test_query (__LINE__, w, "* #label1[plain-text='Label One']", 1, "Label");
  test_query (__LINE__, w, "* #label1[plain-text='label ONE' i]", 1, "Label");
  test_query (__LINE__, w, "* #label1[plain-text='FAIL']", 0);
  test_query (__LINE__, w, "* #label1[plain-text=='Label One']", 1, "Label");
  test_query (__LINE__, w, "* #label1[plain-text=='LABEL one' i]", 1, "Label");
  test_query (__LINE__, w, "* #label1[plain-text=='FAIL']", 0);
  test_query (__LINE__, w, "* #label1[plain-text~=Label]", 1, "Label");
  test_query (__LINE__, w, "* #label1[plain-text~=laBEL i]", 1, "Label");
  test_query (__LINE__, w, "* #label1[plain-text~=One]", 1, "Label");
  test_query (__LINE__, w, "* #label1[plain-text~=OnE i]", 1, "Label");
  test_query (__LINE__, w, "* #label1[plain-text~=oNe]", 0); // FAIL
  // siblings
  test_query (__LINE__, w, "* Button#ChildA + Label + Frame  + Label + Button#ChildE", 1, "Button");
  test_query (__LINE__, w, "* Button#ChildA + Label + Frame! + Label + Button#ChildE", 1, "Frame");
  test_query (__LINE__, w, "* Button#ChildA ~ Label ~ Frame  ~ Label ~ Button#ChildE", 1, "Button");
  test_query (__LINE__, w, "* Button#ChildA ~ Label ~ Frame! ~ Label ~ Button#ChildE", 1, "Frame");
  test_query (__LINE__, w, "* Button#ChildA     ~     Frame      ~     Button#ChildE", 1, "Button");
  test_query (__LINE__, w, "* Button#ChildA     ~     Frame!     ~     Button#ChildE", 1, "Frame");
  test_query (__LINE__, w, "* Button#ChildA!              ~            Button#ChildE", 1, "Button");
  test_query (__LINE__, w, "* Button#ChildA!              ~            Button#ChildE", 1, "Button");
  test_query (__LINE__, w, "* Label!                      ~            Button#ChildE", 2, "Label");
  test_query (__LINE__, w, "* Button#ChildA     ~     Label!     ~     Button#ChildE", 2, "Label");
  test_query (__LINE__, w, "* Label             ~     Label      ~     Label", 0); // FAIL
  // :not pseudo class
  test_query (__LINE__, w, "Button#ChildA ~ Label:not(#ChildB)", 1, "Label#ChildD");
  test_query (__LINE__, w, ":not(*)", 0);
  test_query (__LINE__, w, "#ChildA Frame:not(Label)", 1, "Frame");
  test_query (__LINE__, w, "VBox  Button > Frame Label:not(Button)", 4, "Label"); // 4 from n
  test_query (__LINE__, w, "Button:not(Label# > Label)", 0); // invalid combinator inside not()
  test_query (__LINE__, w, ":not(Label > Label)", 0); // invalid combinator inside not()
  test_query (__LINE__, w, "Button#ChildA ~ Label:not(:not(#ChildB))", 1, "Label#ChildB"); // non-standard
  test_query (__LINE__, w, "*#test-dialog > *:not(#test-dialog)", 1, "Ambience");
  // classes
  test_query (__LINE__, w, "*.Window", 1, "#test-dialog");
  test_query (__LINE__, w, "*", -10, ".Object");        // expect 10 or more...
  test_query (__LINE__, w, "*", -10, ".Widget");
  test_query (__LINE__, w, "*:not(:empty)", -10, ".Container");
  test_query (__LINE__, w, ".Widget:not(.Container)", -5, ":empty");    // find at least 5 Labels, each of which have leaf-children
  test_query (__LINE__, w, ".Label", -5, ".Widget");
  test_query (__LINE__, w, ".VBox", -1, ".Container");
  test_query (__LINE__, w, ".Container", -10, ".Widget");
  test_query (__LINE__, w, "*.Window.Container.Widget", 1, ":root");
  test_query (__LINE__, w, "* > *", -20, ":not(.Window)");
  // pseudo classes :empty :only-child :root :first-child :last-child
  test_query (__LINE__, w, "* VBox  Button > Frame Label", 4, "Label");
  test_query (__LINE__, w, "* VBox  Button > Frame Label:first-child", 4, "Label");
  test_query (__LINE__, w, "* VBox  Button > Frame Label:only-child", 4, "Label");
  test_query (__LINE__, w, "* VBox  Button > Frame Label:last-child", 4, "Label");
  test_query (__LINE__, w, "* Alignment VBox > Frame Arrow:empty", 1, "Arrow");
  test_query (__LINE__, w, "*:empty *", 0);
  test_query (__LINE__, w, "*:root", 1, ".Window");
  test_query (__LINE__, w, "*:root > *:only-child", 1, "Ambience");
  test_query (__LINE__, w, "*:root > *:last-child:first-child", 1, "Ambience");
  test_query (__LINE__, w, "*:root *:only-child", -5);
  test_query (__LINE__, w, "*:root *:first-child:last-child", -5);
  test_query (__LINE__, w, "HBox#testbox > :first-child", 1, "Button#ChildA");
  test_query (__LINE__, w, "HBox#testbox > *:last-child", 1, "Button#ChildE");
  // custom pseudo selectors
  test_query (__LINE__, w, "#test-widget:test-pass", 0);
  test_query (__LINE__, w, "#test-widget:test-pass(1)", 1, "RapicornTestWidget");
  test_query (__LINE__, w, "#test-widget:test-pass(0)", 0);
  test_query (__LINE__, w, "#test-widget:test-pass(2)", 1, "RapicornTestWidget");
  test_query (__LINE__, w, "#test-widget:test-pass(yes)", 1, "RapicornTestWidget");
  test_query (__LINE__, w, "#test-widget:Test-PASS(true)", 1, "RapicornTestWidget");
  test_query (__LINE__, w, "#test-widget:Test-PASS(false)", 0);
  test_query (__LINE__, w, "HBox! > RapicornTestWidget#test-widget", 1, "HBox"); // RapicornTestWidget parent is HBox
  test_query (__LINE__, w, "RapicornTestWidget#test-widget::test-parent", 1, "HBox"); // access parent through pseudo element
  test_query (__LINE__, w, "RapicornTestWidget#test-widget::test-parent:empty", 0); // classified pseudo element
  test_query (__LINE__, w, "RapicornTestWidget#test-widget::test-parent:not(:empty)!", 1, "HBox"); // classified matching pseudo element
  test_query (__LINE__, w, "RapicornTestWidget#test-widget::test-parent:not(:empty)!", 1, "HBox"); // pseudo element with subject indicator
  test_query (__LINE__, w, "*.Window RapicornTestWidget#test-widget::test-parent:not(:empty)", 1, "HBox"); // pseudo element and combinator
  test_query (__LINE__, w, "*.Window RapicornTestWidget#test-widget::test-parent:not(:empty)!", 1, "HBox"); // like above with subject indicator

  WidgetIfaceP i1 = w->query_selector ("#special-arrow");
  TASSERT (i1);
  TASSERT (i1->query_selector_all ("*").size() == 1);

  window->destroy();
  w = app.query_window ("#test-dialog");
  TASSERT (w == NULL);
}
REGISTER_UITHREAD_TEST ("Selector/Selector Matching", test_selector_matching);

static const char test_dialog_xml[] =
  "<?xml version='1.0' encoding='UTF-8'?>\n"
  "<interfaces>\n"
  // test-dialog
  "<Window id='test-dialog'>\n"
  "  <Ambience normal-lighting='upper-left'>\n"
  "    <Alignment padding='5'>\n"
  "      <VBox spacing='3' hexpand='1'>\n"
  "        <VBox hexpand='1'>\n"
  "          <Frame>\n"
  "            <VBox spacing='5'>\n"
  "              <Label markup-text='Test Buttons:'/>\n"
  "              <Button on-click='Widget::print(\"click on first button\")'>\n"
  "                <Label name='label1' markup-text='Label One'/>\n"
  "              </Button>\n"
  "              <HBox hexpand='1' spacing='3'>\n"
  "                <Button on-click='Widget::print(\"Normal Button\")'>\n"
  "                  <Label name='label123' markup-text='one-two-three' />\n"
  "                </Button>\n"
  "                <RapicornTestWidget name='test-widget'/>\n"
  "              </HBox>\n"
  "            </VBox>\n"
  "          </Frame>\n"
  "        </VBox>\n"
  "        <Frame>\n"
  "          <Arrow name='special-arrow' arrow-dir='right' />\n"
  "        </Frame>\n"
  "        <HBox name='testbox' >\n"
  "          <Button name='ChildA'> <Label plain-text='Child A'/> </Button>\n"
  "          <Label  name='ChildB'         plain-text='Child B'/>\n"
  "          <Frame  name='ChildC'> <Label plain-text='Child C'/> </Frame>\n"
  "          <Label  name='ChildD'         plain-text='Child D'/>\n"
  "          <Button name='ChildE'> <Label plain-text='Child E'/> </Button>\n"
  "        </HBox>\n"
  "      </VBox>\n"
  "    </Alignment>\n"
  "  </Ambience>\n"
  "</Window>\n"
  ""
  "</interfaces>\n"
  "";

static void
load_ui_defs()
{
  do_once
    {
      String errs = Factory::parse_ui_data (RAPICORN_STRLOC(), sizeof (test_dialog_xml)-1, test_dialog_xml, "", NULL, NULL);
      if (!errs.empty())
        fatal ("%s:%d: failed to parse internal XML string: %s", __FILE__, __LINE__, errs.c_str());
    }
}
