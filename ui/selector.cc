// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#include "selector.hh"
#include <string.h>

namespace Rapicorn {
namespace Parser {

bool
parse_spaces (const char **stringp, int min_spaces)
{
  const char *p = *stringp;
  while (*p == ' ' || (*p >= 9 && *p <= 13)) // '\t\n\v\f\r '
    p++;
  if (p - *stringp >= min_spaces)
    {
      *stringp = p;
      return true;
    }
  return false;
}

bool
skip_spaces (const char **stringp)
{
  return parse_spaces (stringp, 0);
}

bool
parse_case_word (const char **stringp, const char *word)
{
  const uint l = strlen (word);
  if (strncasecmp (*stringp, word, l) == 0)
    {
      (*stringp) += l;
      return true;
    }
  return false;
}

bool
parse_unsigned_integer (const char **stringp, uint64 *up)
{ // '0' | [1-9] [0-9]* : <= 18446744073709551615
  const char *p = *stringp;
  // zero
  if (*p == '0' && !(p[1] >= '0' && p[1] <= '9'))
    {
      *up = 0;
      *stringp = p + 1;
      return true;
    }
  // first digit
  if (!(*p >= '1' && *p <= '9'))
    return false;
  uint64 u = *p - '0';
  p++;
  // rest digits
  while (*p >= '0' && *p <= '9')
    {
      const uint64 last = u;
      u = u * 10 + (*p - '0');
      p++;
      if (u < last) // overflow
        return false;
    }
  *up = u;
  *stringp = p;
  return true;
}

bool
parse_signed_integer (const char **stringp, int64 *ip)
{ // ['-'|'+']? [ '0' | [1-9] [0-9]* ]
  const char *p = *stringp;
  // sign
  const int64 sign = *p == '-' ? -1 : +1;
  p += *p == '+' || *p == '-' ? +1 : 0;
  // digits
  uint64 u;
  if (parse_unsigned_integer (&p, &u) &&
      (u <= 9223372036854775807 ||
       (sign == -1 && u == 9223372036854775808ULL)))
    {
      *ip = sign * int64 (u);
      *stringp = p;
      return true;
    }
  return false;
}

static bool
parse_plus_b (const char **stringp, int64 *b)
{ // [ S* ['-'|'+'] S* INTEGER ]?
  const char *p = *stringp;
  int sign = +1;
  bool success = true;
  success &= skip_spaces (&p);
  if (*p == '+')
    sign = +1, p++;
  else if (*p == '-')
    sign = -1, p++;
  success &= skip_spaces (&p);
  uint64 u = 0;
  success &= parse_unsigned_integer (&p, &u);
  success &= u <= 9223372036854775807 || (sign == -1 && u == 9223372036854775808ULL);
  if (success)
    {
      *b = sign * int64 (u);
      *stringp = p;
      return true;
    }
  return false;
}

static bool
parse_a_n_plus_b (const char **stringp, int64 *ap, int64 *bp)
{ // ['-'|'+']? INTEGER? {N} [ S* ['-'|'+'] S* INTEGER ]?
  int64 a;
  const char *p = *stringp;
  bool success = true;
  if (parse_signed_integer (&p, &a))            // ['-'|'+']? INTEGER {N}
    ;
  else                                          // ['-'|'+']? {N}
    {
      a = *p == '-' ? -1 : +1;
      p += *p == '+' || *p == '-' ? +1 : 0;
    }
  success &= (*p == 'n' || *p == 'N') && p++;
  if (success)
    {
      if (!parse_plus_b (&p, bp))
        *bp = 0;
      *ap = a;
      *stringp = p;
      return true;
    }
  return false;
}

bool
parse_css_nth (const char **stringp, int64 *ap, int64 *bp)
{
  return_val_if_fail (stringp != NULL && ap != NULL && bp != NULL, false);
  /* nth : S* [ ['-'|'+']? INTEGER? {N} [ S* ['-'|'+'] S* INTEGER ]? |
   *            ['-'|'+']? INTEGER | {O}{D}{D} | {E}{V}{E}{N}          ] S*
   */
  const char *p = *stringp;
  skip_spaces (&p);
  if (parse_a_n_plus_b (&p, ap, bp))
    {
      skip_spaces (&p);
      *stringp = p;
      return true;
    }
  if (parse_case_word (&p, "ODD"))
    {
      skip_spaces (&p);
      *ap = 2;
      *bp = 1;
      *stringp = p;
      return true;
    }
  if (parse_case_word (&p, "EVEN"))
    {
      skip_spaces (&p);
      *ap = 2;
      *bp = 0;
      *stringp = p;
      return true;
    }
  if (parse_signed_integer (&p, bp))
    {
      skip_spaces (&p);
      *ap = 0;
      *stringp = p;
      return true;
    }
  return false;
}

bool
match_css_nth (int64 pos, int64 a, int64 b)
{
  if (pos <= 0)
    return false;
  if (a == 0)
    return pos == b;
  // for matches, find unsigned integer n, so that a * n + b == pos
  const int64 n = (pos - b) / a;
  return n >= 0 && n * a == pos - b; // check int truncation
}

} // Parser
} // Rapicorn
