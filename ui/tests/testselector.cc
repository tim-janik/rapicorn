/* Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html */
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
  if (!Parser::parse_css_nth (&formula, &a, &b) || *formula != 0)
    return false;
  return Parser::match_css_nth (pos, a, b);
}

static bool
test_css_nthset (const char *formula, const vector<int64> &matchset, const vector<int64> &unmatchset)
{
  int64 a, b;
  const char *p = formula;
  if (!Parser::parse_css_nth (&p, &a, &b) || *p != 0)
    {
      printerr ("FAILURE:%s: failed to parse css-nth formula: %s\n", __func__, formula);
      return false;
    }
  for (size_t i = 0; i < matchset.size(); i++)
    {
      const bool matched = parse_match_css_nth (formula, matchset[i]);
      if (!matched)
        {
          printerr ("FAILURE:%s: failed to match %lld against: %s\n", __func__, matchset[i], formula);
          return false;
        }
    }
  for (size_t i = 0; i < unmatchset.size(); i++)
    {
      const bool matched = parse_match_css_nth (formula, unmatchset[i]);
      if (matched)
        {
          printerr ("FAILURE:%s: erroneously matched %lld against: %s\n", __func__, unmatchset[i], formula);
          return false;
        }
    }
  return true;
}

static void
test_selector_primitives()
{
  using namespace Parser;
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

#define SN(...)       ({ SelectorNode n (SelectorNode:: __VA_ARGS__); n; })
static const Parser::SelectorNode __csr0 (Parser::SelectorNode::NONE);
typedef const Parser::SelectorNode &Csr;

static Parser::SelectorChain
schain (Csr a = __csr0, Csr b = __csr0, Csr c = __csr0, Csr d = __csr0, Csr e = __csr0, Csr f = __csr0, Csr g = __csr0,
        Csr h = __csr0, Csr i = __csr0, Csr j = __csr0, Csr k = __csr0, Csr l = __csr0, Csr m = __csr0, Csr n = __csr0)
{
  Parser::SelectorChain sc;
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
  using namespace Parser;
  const char *s, *o;
  // element selectors
  Parser::SelectorChain sc;
  TASSERT ((s = "*") && (o = s) && parse_selector_chain (&s, sc) && s == o + 1 && sc == schain (SN (UNIVERSAL, "*")));
  TASSERT ((s = "ABC") && (o = s) && parse_selector_chain (&s, sc) && s == o + 3 && sc == schain (SN (TYPE, "ABC")));
  TASSERT ((s = "A B") && (o = s) && parse_selector_chain (&s, sc) && s == o + 3 &&
           sc == schain (SN (TYPE, "A"), SN (DESCENDANT, ""), SN (TYPE, "B")));
  TASSERT ((s = "A>B") && (o = s) && parse_selector_chain (&s, sc) && s == o + 3 &&
           sc == schain (SN (TYPE, "A"), SN (CHILD, ""), SN (TYPE, "B")));
  TASSERT ((s = "A ~ B") && (o = s) && parse_selector_chain (&s, sc) && s == o + 5 &&
           sc == schain (SN (TYPE, "A"), SN (FOLLOWING, ""), SN (TYPE, "B")));
  TASSERT ((s = "A+ B") && (o = s) && parse_selector_chain (&s, sc) && s == o + 4 &&
           sc == schain (SN (TYPE, "A"), SN (NEIGHBOUR, ""), SN (TYPE, "B")));
  TASSERT ((s = "A +B") && (o = s) && parse_selector_chain (&s, sc) && s == o + 4 &&
           sc == schain (SN (TYPE, "A"), SN (NEIGHBOUR, ""), SN (TYPE, "B")));
  TASSERT ((s = "A B > * + D ~ E") && (o = s) && parse_selector_chain (&s, sc) && s == o + 15 &&
           sc == schain (SN (TYPE, "A"), SN (DESCENDANT, ""), SN (TYPE, "B"), SN (CHILD, ""), SN (UNIVERSAL, "*"),
                         SN (NEIGHBOUR, ""), SN (TYPE, "D"), SN (FOLLOWING, ""), SN (TYPE, "E")));
  TASSERT ((s = "A B > $C + D ~ E") && (o = s) && parse_selector_chain (&s, sc) && s == o + 16 &&
           sc == schain (SN (TYPE, "A"), SN (DESCENDANT, ""), SN (TYPE, "B"), SN (CHILD, ""), SN (SUBJECT), SN (TYPE, "C"),
                         SN (NEIGHBOUR, ""), SN (TYPE, "D"), SN (FOLLOWING, ""), SN (TYPE, "E")));
  // attribute and id selectors
  TASSERT ((s =            "A#id") && (o = s) && parse_selector_chain (&s, sc) && s == o +  4 && sc == schain (SN (TYPE, "A"), SN (ID, "id")));
  TASSERT ((s =             "A.C") && (o = s) && parse_selector_chain (&s, sc) && s == o +  3 && sc == schain (SN (TYPE, "A"), SN (CLASS, "C")));
  TASSERT ((s =            "A[b]") && (o = s) && parse_selector_chain (&s, sc) && s == o +  4 && sc == schain (SN (TYPE, "A"), SN (ATTRIBUTE_EXISTS, "b")));
  TASSERT ((s =          "A[b=c]") && (o = s) && parse_selector_chain (&s, sc) && s == o +  6 && sc == schain (SN (TYPE, "A"), SN (ATTRIBUTE_EQUALS, "b", "c")));
  TASSERT ((s =           "[b=c]") && (o = s) && parse_selector_chain (&s, sc) && s == o +  5 && sc == schain (SN (ATTRIBUTE_EQUALS, "b", "c")));
  TASSERT ((s =   "$[b=c] > #fun") && (o = s) && parse_selector_chain (&s, sc) && s == o + 13 &&
           sc == schain (SN (SUBJECT), SN (ATTRIBUTE_EQUALS, "b", "c"), SN (CHILD), SN (ID, "fun")));
  TASSERT ((s =   "A[b = \"c\" ]") && (o = s) && parse_selector_chain (&s, sc) && s == o + 11 && sc == schain (SN (TYPE, "A"), SN (ATTRIBUTE_EQUALS, "b", "c")));
  TASSERT ((s =     "A[b = 'c' ]") && (o = s) && parse_selector_chain (&s, sc) && s == o + 11 && sc == schain (SN (TYPE, "A"), SN (ATTRIBUTE_EQUALS, "b", "c")));
  TASSERT ((s =       "A[b |=c ]") && (o = s) && parse_selector_chain (&s, sc) && s == o +  9 && sc == schain (SN (TYPE, "A"), SN (ATTRIBUTE_DASHSTART, "b", "c")));
  TASSERT ((s =      "A[b^= 'c']") && (o = s) && parse_selector_chain (&s, sc) && s == o + 10 && sc == schain (SN (TYPE, "A"), SN (ATTRIBUTE_PREFIX, "b", "c")));
  TASSERT ((s =       "A[b $= c]") && (o = s) && parse_selector_chain (&s, sc) && s == o +  9 && sc == schain (SN (TYPE, "A"), SN (ATTRIBUTE_SUFFIX, "b", "c")));
  TASSERT ((s =      "A[b *='c']") && (o = s) && parse_selector_chain (&s, sc) && s == o + 10 && sc == schain (SN (TYPE, "A"), SN (ATTRIBUTE_SUBSTRING, "b", "c")));
  TASSERT ((s =       "A[b ~= c]") && (o = s) && parse_selector_chain (&s, sc) && s == o +  9 && sc == schain (SN (TYPE, "A"), SN (ATTRIBUTE_INCLUDES, "b", "c")));
  TASSERT ((s = "A[b~=c][d1*=e2]") && (o = s) && parse_selector_chain (&s, sc) && s == o + 15 &&
           sc == schain (SN (TYPE, "A"), SN (ATTRIBUTE_INCLUDES, "b", "c"), SN (ATTRIBUTE_SUBSTRING, "d1", "e2")));
  // pseudo selectors
  TASSERT ((s =  "*::before") && (o = s) && parse_selector_chain (&s, sc) && s == o +  9 && sc == schain (SN (UNIVERSAL, "*"), SN (PSEUDO_ELEMENT, "before")));
  TASSERT ((s =      ":root") && (o = s) && parse_selector_chain (&s, sc) && s == o +  5 && sc == schain (SN (PSEUDO_CLASS, "root")));
  TASSERT ((s =  ":root ~ C") && (o = s) && parse_selector_chain (&s, sc) && s == o +  9 && sc == schain (SN (PSEUDO_CLASS, "root"), SN (FOLLOWING), SN (TYPE, "C")));
  TASSERT ((s =     "$:root") && (o = s) && parse_selector_chain (&s, sc) && s == o +  6 && sc == schain (SN (SUBJECT), SN (PSEUDO_CLASS, "root")));
  TASSERT ((s = ":root ~ $C") && (o = s) && parse_selector_chain (&s, sc) && s == o + 10 && sc == schain (SN (PSEUDO_CLASS, "root"), SN (FOLLOWING), SN (SUBJECT), SN (TYPE, "C")));
  TASSERT ((s =     "A:root") && (o = s) && parse_selector_chain (&s, sc) && s == o +  6 && sc == schain (SN (TYPE, "A"), SN (PSEUDO_CLASS, "root")));
  TASSERT ((s =     "A:root") && (o = s) && parse_selector_chain (&s, sc) && s == o +  6 && sc == schain (SN (TYPE, "A"), SN (PSEUDO_CLASS, "root")));
  TASSERT ((s =  "A:root( )") && (o = s) && parse_selector_chain (&s, sc) && s == o +  1 && sc == schain (SN (TYPE, "A"))); // invalid empty expression
  TASSERT ((s =      ":root ~ $::after") && (o = s) && parse_selector_chain (&s, sc) && s == o + 16 && sc == schain (SN (PSEUDO_CLASS, "root"), SN (FOLLOWING), SN (SUBJECT), SN (PSEUDO_ELEMENT, "after")));
  TASSERT ((s = "$:root ~ :first-child") && (o = s) && parse_selector_chain (&s, sc) && s == o + 21 && sc == schain (SN (SUBJECT), SN (PSEUDO_CLASS, "root"), SN (FOLLOWING), SN (PSEUDO_CLASS, "first-child")));
  TASSERT ((s =     "*:nth-child(2n+1)") && (o = s) && parse_selector_chain (&s, sc) && s == o + 17 && sc == schain (SN (UNIVERSAL, "*"), SN (PSEUDO_CLASS, "nth-child", "2n+1")));
  TASSERT ((s = "*:nth-last-of-type( -3n-1 )::after") && (o = s) && parse_selector_chain (&s, sc) && s == o + 34 && sc == schain (SN (UNIVERSAL, "*"), SN (PSEUDO_CLASS, "nth-last-of-type", "-3n-1"), SN (PSEUDO_ELEMENT, "after")));
  TASSERT ((s = "A[b$='c\\\nc']:lang('foo')::first-letter > D") && (o = s) && parse_selector_chain (&s, sc) && s == o + 42 && sc == schain (SN (TYPE, "A"), SN (ATTRIBUTE_SUFFIX, "b", "c\nc"), SN (PSEUDO_CLASS, "lang", "'foo'"), SN (PSEUDO_ELEMENT, "first-letter"), SN (CHILD), SN (TYPE, "D")));
  TASSERT ((s = "A[b$='c\\0a c']:lang('foo[]')::first-letter > $D + E") && (o = s) && parse_selector_chain (&s, sc) && o == sc.string());
  //{ s = ":root ~ $::after"; o = s; bool r = parse_selector_chain (&s, sc); printerr ("\"%s\": d=%ld r=%d : %s\n", o, s - o, r, sc.string().c_str()); }
}
REGISTER_UITHREAD_TEST ("Selector/Combinator Parsing", test_selector_parser);
