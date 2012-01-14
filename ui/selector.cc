// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#include "selector.hh"
#include <string.h>

#define ISDIGIT(c)      (c >= '0' && c <= '9')
#define ISHEXDIGIT(c)   (ISDIGIT (c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'))
#define ISALPHA(c)      ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))
#define HEXVALUE(c)     (c <= '9' ? c - '0' : 10 + c - (c >= 'a' ? 'a' : 'A'))
#define ISASCIISPACE(c) (c == ' ' || (c >= 9 && c <= 13)) // ' \t\n\v\f\r'

namespace Rapicorn {
namespace Parser {

bool
parse_spaces (const char **stringp, int min_spaces)
{
  const char *p = *stringp;
  while (ISASCIISPACE (*p))
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

static inline bool
parse_unicode_hexdigits (const char **stringp, String &ident)
{ // unicode : '\\' [0-9a-f]{1,6} (\r\n|[ \n\r\t\f])?
  const char *p = *stringp;
  if (!ISHEXDIGIT (*p))
    return false;
  uint unicodechar = HEXVALUE (*p); // careful, multi-evaluation
  p++;
  for (const char *const b = p + 5; p < b && ISHEXDIGIT (*p); p++)
    unicodechar = (unicodechar << 4) + HEXVALUE (*p);
  char utf8[8];
  uint l = utf8_from_unichar (unicodechar, utf8);
  ident.append (utf8, l);
  // (\r\n | [ \n\r\t\f])?
  if (p[0] == '\r' && p[1] == '\n')
    p += 2;
  else if (ISASCIISPACE (*p))
    p++;
  *stringp = p;
  return true;
}

static inline bool
parse_identifier_char (const char **stringp, String &ident, bool numchar)
{ // char : [_a-z] | nonascii | unicode | escape | numchar
  const char *p = *stringp;
  if (ISALPHA (*p) || *p == '_' ||                      // [a-z] | '_'
      *p > '\177' ||                                    // nonascii : [^\0-\177]
      (numchar && (*p == '-' || ISDIGIT (*p))))         // numchar : [0-9-]
    {
      ident.append (p, 1);
      p++;
      *stringp = p;
      return true;
    }
  if (UNLIKELY (p[0] == '\\'))
    {
      p++;
      // unicode : '\\' [0-9a-f]{1,6}(\r\n|[ \n\r\t\f])?
      if (parse_unicode_hexdigits (&p, ident))
        {
          *stringp = p;
          return true;
        }
      // escape : '\\' [^\n\r\f0-9a-f]
      if (*p != '\n' && *p != '\r' && *p != '\f') // unicode catches ISHEXDIGIT (p[1])
        {
          ident.append (p, 1);
          p++;
          *stringp = p;
          return true;
        }
    }
  return false;
}

bool
parse_identifier (const char **stringp, String &ident)
{ // [-]?{nmstart}{nmchar}*
  return_val_if_fail (stringp != NULL, false);
  const char *p = *stringp;
  String s;
  if (UNLIKELY (*p == '-'))
    {
      p++;
      s = "-";
    }
  if (!parse_identifier_char (&p, s, false))
    return false;
  while (parse_identifier_char (&p, s, true))
    ;
  ident.swap (s);
  *stringp = p;
  return true;
}

static bool
parse_special_selector (const char **stringp, SelectorChain &chain)
{ // special_selector : HASH | class | attrib | pseudo | negation
  // FIXME
  return false;
}

static bool
parse_universal_selector (const char **stringp, SelectorChain &chain)
{ // universal : [ namespace_prefix ]? '*'
  const char *p = *stringp;
  if (*p == '*')
    {
      SelectorNode node;
      node.kind = SelectorNode::UNIVERSAL;
      node.ident = String (p, 1);
      chain.push_back (node);
      p++;
      *stringp = p;
      return true;
    }
  return false;
}

static bool
parse_type_selector (const char **stringp, SelectorChain &chain)
{ // type_selector : [ namespace_prefix ]? identifier
  SelectorNode node;
  if (parse_identifier (stringp, node.ident))
    {
      node.kind = SelectorNode::TYPE;
      chain.push_back (node);
      return true;
    }
  return false;
}

static bool
parse_simple_selector_sequence (const char **stringp, SelectorChain &chain)
{ // simple_selector_sequence : [ type_selector | universal ] [ special_selector ]* | [ special_selector ]+
  const char *p = *stringp;
  bool seen_selector = parse_type_selector (&p, chain) || parse_universal_selector (&p, chain);
  while (parse_special_selector (&p, chain))
    seen_selector = true;
  if (seen_selector)
    {
      *stringp = p;
      return true;
    }
  return false;
}

static bool
parse_selector_combinator (const char **stringp, SelectorNode::Kind *kind)
{
  const char *p = *stringp;
  /* combinator : S* '+' S* | S* '>' S* | S* '~' S* | S+ */
  const bool seen_spaces = parse_spaces (&p, 1);
  switch (*p)
    {
    case '+':   p++; *kind = SelectorNode::NEIGHBOUR;   break;
    case '>':   p++; *kind = SelectorNode::CHILD;       break;
    case '~':   p++; *kind = SelectorNode::FOLLOWING;   break;
    default:
      if (seen_spaces)
        {
          *kind = SelectorNode::DESCENDANT;
          *stringp = p;
          return true;
        }
      return false;
    }
  skip_spaces (&p);
  *stringp = p;
  return true;
}

bool
parse_selector_chain (const char **stringp, SelectorChain &chain)
{
  return_val_if_fail (stringp != NULL, false);
  const char *p = *stringp;
  /* selector : simple_selector_sequence [ combinator simple_selector_sequence ]* */
  SelectorChain tmpchain;
  if (parse_simple_selector_sequence (&p, tmpchain))
    {
      SelectorChain nextchain;
      const char *q = p;
      SelectorNode cnode;
      while (parse_selector_combinator (&q, &cnode.kind))
        {
          if (parse_simple_selector_sequence (&q, nextchain))
            {
              tmpchain.push_back (cnode);
              tmpchain.insert (tmpchain.end(), nextchain.begin(), nextchain.end());
              nextchain.clear();
              p = q;
            }
        }
      chain.swap (tmpchain);
      *stringp = p;
      return true;
    }
  return false;
}

} // Parser
} // Rapicorn
