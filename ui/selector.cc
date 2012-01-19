// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#include "selector.hh"
#include "container.hh"
#include "factory.hh"
#include <string.h>

#define ISDIGIT(c)      (c >= '0' && c <= '9')
#define ISHEXDIGIT(c)   (ISDIGIT (c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'))
#define ISALPHA(c)      ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))
#define HEXVALUE(c)     (c <= '9' ? c - '0' : 10 + c - (c >= 'a' ? 'a' : 'A'))
#define ISASCIISPACE(c) (c == ' ' || (c >= 9 && c <= 13)) // ' \t\n\v\f\r'

namespace Rapicorn {
namespace Selector {

#ifdef  UNOPTIMIZE // work around valgrind reporting uninitialized reads due to sse4.2 registers exceeding string bounds
#pragma GCC optimize "no-inline", "O0"
#define ASCIILOWER(c)   (UNLIKELY (c >= 'A' && c <= 'Z') ? c - 'A' + 'a' : c)
static inline const char*
strcasestr (const char *haystack, const char *needle)
{
  for (const char *h = haystack; *h; h++)
    if (UNLIKELY (ASCIILOWER (*h) == ASCIILOWER (*needle)))
      {
        for (const char *p = h + 1, *c = needle + 1; *c; p++, c++)
          if (ASCIILOWER (*p) != ASCIILOWER (*c))
            goto mismatch;
        return h; // full needle match
      mismatch:
        continue;
      }
  return NULL; // unmatched
}
#endif // UNOPTIMIZE

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

static inline bool
scan_escaped (const char **stringp, const char term)
{
  const char *p = *stringp;
  while (*p)
    if (*p == term)
      {
        p++;
        *stringp = p;
        return true;
      }
    else if (p[0] == '\\' && p[1])
      p += 2;
    else // *p != 0
      p++;
  return false;
}

struct ScanNestedConfig {
  bool angle, bracket, parenthesis, curly, ccomment, cppcomment, doublequote, singlequote;
};

static inline bool
scan_nested (const char **stringp, const ScanNestedConfig &scan, const char term)
{
  const char *p = *stringp;
  while (1)
    switch (*p)
      {
      case 0:
        return false;
      case '\'':
        if (!scan.singlequote)
          goto default_char;
        p++;
        if (!scan_escaped (&p, '\''))
          return false;
        break;
      case '"':
        if (!scan.doublequote)
          goto default_char;
        p++;
        if (!scan_escaped (&p, '"'))
          return false;
        break;
      case '[':
        if (!scan.bracket)
          goto default_char;
        p++;
        if (!scan_nested (&p, scan, ']'))
          return false;
        assert (*p == ']');
        p++;
        break;
      case ']':
        if (*p == term)
          {
            *stringp = p;
            return true;
          }
        if (scan.bracket)
          return false; // unpaired
        else
          goto default_char;
      case '(':
        if (!scan.parenthesis)
          goto default_char;
        p++;
        if (!scan_nested (&p, scan, ')'))
          return false;
        assert (*p == ')');
        p++;
        break;
      case ')':
        if (*p == term)
          {
            *stringp = p;
            return true;
          }
        if (scan.parenthesis)
          return false; // unpaired
        else
          goto default_char;
      case '{':
        if (!scan.curly)
          goto default_char;
        p++;
        if (!scan_nested (&p, scan, '}'))
          return false;
        assert (*p == '}');
        p++;
        break;
      case '}':
        if (*p == term)
          {
            *stringp = p;
            return true;
          }
        if (scan.curly)
          return false; // unpaired
        else
          goto default_char;
      case '<':
        if (!scan.angle)
          goto default_char;
        p++;
        if (!scan_nested (&p, scan, '>'))
          return false;
        assert (*p == '>');
        p++;
        break;
      case '>':
        if (*p == term)
          {
            *stringp = p;
            return true;
          }
        if (scan.angle)
          return false; // unpaired
        else
          goto default_char;
      case '/':
        if (p[1] == '/' && scan.cppcomment)
          {
            assert (strncmp (p, "//", 2) == 0);
            p = strchr (p + 2, '\n');
            if (!p)
              return false;
            assert (strncmp (p, "\n", 1) == 0);
            p++; // skip "\n"
          }
        else if (p[1] == '*' && scan.ccomment)
          {
            assert (strncmp (p, "/*", 2) == 0);
            p++;
            do
              p = strchr (p + 1, '*');
            while (p && p[1] != '/');
            if (!p)
              return false;
            assert (strncmp (p, "*/", 2) == 0);
            p += 2; // skip "*/"
          }
        else
          goto default_char;
        break;
      default: default_char:
        if (term == *p)
          {
            *stringp = p;
            return true;
          }
        p++;
        break;
      }
}

bool
scan_nested (const char **stringp, const char *pairflags, const char term)
{
  ScanNestedConfig scan = { 0, };
  const char *p = pairflags;
  while (*p)
    switch (*p) // parse configuration flags
      {
      case '<': scan.angle = true;        p++; continue;        // '<': <...>
      case '[': scan.bracket = true;      p++; continue;        // '[': [...]
      case '(': scan.parenthesis = true;  p++; continue;        // '(': (...)
      case '{': scan.curly = true;        p++; continue;        // '{': {...}
      case '*': scan.ccomment = true;     p++; continue;        // '*': /*...*/
      case '/': scan.cppcomment = true;   p++; continue;        // '/': //...\n
      case '"': scan.doublequote = true;  p++; continue;        // '"': "..."
      case '\'': scan.singlequote = true; p++; continue;        //  \': '...'
      default:                            p++; continue;
      }
  return scan_nested (stringp, scan, term);
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

template<bool NEWLINES> static inline bool
parse_escapes (const char **stringp, String &ident)
{ // escapes : unicode | escaped_char | newlines
  const char *p = *stringp;
  if (UNLIKELY (p[0] == '\\'))
    {
      p++;
      // unicode : '\\' [0-9a-f]{1,6}(\r\n|[ \n\r\t\f])?
      if (parse_unicode_hexdigits (&p, ident))
        {
          *stringp = p;
          return true;
        }
      // newlines : \\ [ \n | \r\n | \r | \f ]
      if (NEWLINES && p[0] == '\r' && p[1] == '\n')
        {
          ident.append (p, 2);
          p += 2;
          *stringp = p;
          return true;
        }
      // escaped_char : '\\' [^\n\r\f0-9a-f]
      if (NEWLINES || !(p[0] == '\n' || p[0] == '\f' || p[0] == '\r'))
        {
          ident.append (p, 1);
          p++;
          *stringp = p;
          return true;
        }
    }
  return false;
}

template<bool NUMCHAR> static inline bool
parse_identifier_char (const char **stringp, String &ident)
{ // char : [_a-z] | nonascii | unicode | escape | numchar
  const char *p = *stringp;
  if (ISALPHA (*p) || *p == '_' ||                      // [a-z] | '_'
      *p > '\177' ||                                    // nonascii : [^\0-\177]
      (NUMCHAR && (*p == '-' || ISDIGIT (*p))))         // numchar : [0-9-]
    {
      ident.append (p, 1);
      p++;
      *stringp = p;
      return true;
    }
  if (parse_escapes<0> (&p, ident))
    {
      *stringp = p;
      return true;
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
  if (!parse_identifier_char<0> (&p, s))
    return false;
  while (parse_identifier_char<1> (&p, s))
    ;
  ident.swap (s);
  *stringp = p;
  return true;
}

static String
maybe_quote_identifier (const String &src)
{
  bool need_quotes = false;
  String d;
  for (String::const_iterator it = src.begin(); it != src.end(); it++)
    {
      const uint8 c = *it;
      if (ISALPHA (c) || ISDIGIT (c) || c == '_' || c > '\177')
        d += c;
      else
        {
          need_quotes = true;
          if (c == '\'')
            d += "\\'";
          else if (c < 32)
            d += string_printf ("\\0%x ", c);
          else
            d += c;
        }
    }
  return need_quotes ? "'" + d + "'" : d;
}

template<int QUOTE> static inline bool
parse_string_char (const char **stringp, String &ident)
{ // string_char : [^\n\r\f] | nonascii | \\ [ \n | \r\n | \r | \f ] | escape
  const char *p = *stringp;
  if (parse_escapes<1> (&p, ident))                     // escape | \\ [ \n | \r\n | \r | \f ]
    {
      *stringp = p;
      return true;
    }
  if (*p != QUOTE &&                                    // nonascii
      *p != '\n' && *p != '\r' && *p != '\f')           // [^\n\r\f]
    {
      ident.append (p, 1);
      p++;
      *stringp = p;
      return true;
    }
  return false;
}

bool
parse_string (const char **stringp, String &ident)
{ // string : string_dq | string_sq
  return_val_if_fail (stringp != NULL, false);
  const char *p = *stringp;
  String s;
  if (*p == '"')
    { // string_dq : \" ( [^\n\r\f\\"] | \\ [ \n | \r\n | \r | \f ] | nonascii | escape )* \"
      p++;
      while (parse_string_char<'"'> (&p, s))
        ;
      if (*p != '"')
        return false;
    }
  else if (*p == '\'')
    { // string_sq : \' ( [^\n\r\f\\'] | \\ [ \n | \r\n | \r | \f ] | nonascii | escape )* \'
      p++;
      while (parse_string_char<'\''> (&p, s))
        ;
      if (*p != '\'')
        return false;
    }
  else
    return false;
  p++;
  ident.swap (s);
  *stringp = p;
  return true;
}

static bool
parse_pseudo_selector (const char **stringp, SelectorChain &chain, int parsed_colons)
{ // pseudo : ':' [ 'not' '(' S* negation_arg S* ')' | ':'? identifier [ '(' S* expression ')' ] ]
  const char *p = *stringp;
  while (*p == ':' && parsed_colons < 2)
    parsed_colons++, p++;
  if (UNLIKELY (parsed_colons < 1))
    return false;
  SelectorNode node;
  if (!parse_identifier (&p, node.ident))
    return false;
  const bool is_pseudo_element = parsed_colons == 2;
  const bool is_pseudo_class = parsed_colons == 1;
  const char *const elements[] = {
    "first-line", "first-letter", "before", "after", // syntactically, these 4 are supported as legacy classes
    "selection",
  };
  const char *const classes[] = {
    // CSS3:
    "not", "root", "nth-child", "nth-last-child", "nth-of-type", "nth-last-of-type",
    "link", "visited", "hover", "active", "focus", "target", "lang", "enabled", "disabled", "checked", "indeterminate",
    "first-child", "last-child", "first-of-type", "last-of-type", "only-child", "only-of-type", "empty",
    // CSS4:
    "scope", "any-link", "local-link", "past", "current", "future", "matches", "dir",
    "default", "valid", "invalid", "in-range", "out-of-range", "required", "optional", "read-only", "read-write",
    "nth-match", "nth-last-match", "nth-column", "nth-last-column", "column",
  };
  if (is_pseudo_element)
    {
      for (uint i = 0; i < ARRAY_SIZE (elements); i++)
        if (strcasecmp (node.ident.c_str(), elements[i]) == 0)
          {
            node.kind = PSEUDO_ELEMENT;
            chain.push_back (node);
            *stringp = p;
            return true;
          }
      return false;
    }
  bool pseudo_match = false;
  if (is_pseudo_class)
    for (uint i = 0; i < ARRAY_SIZE (classes); i++)
      if (strcasecmp (node.ident.c_str(), classes[i]) == 0)
        {
          pseudo_match = true;
          break;
        }
  if (!pseudo_match)
    return false;
  // '(' S* expression S* ')'
  if (*p == '(')
    {
      p++;
      skip_spaces (&p);
      const char *e = p;
      if (!scan_nested (&p, "([{*'\"", ')'))
        return false;
      assert (*p == ')');
      const char *t = p - 1;
      while (t >= e && ISASCIISPACE (*t))
        t--;
      if (t < e)
        return false;   // pseudo_class expression should be non-empty
      node.arg = String (e, t - e + 1);
      p++;
    }
  node.kind = PSEUDO_CLASS;
  chain.push_back (node);
  *stringp = p;
  return true;
}

static bool
parse_attribute (const char **stringp, SelectorChain &chain)
{ // attrib : '[' S* [ namespace_prefix ]? IDENT S* [ [ matchop ] S* [ IDENT | STRING ] S* 'i'? ]? ']'
  const char *p = *stringp;
  if (*p != '[')
    return false;
  p++;
  skip_spaces (&p);
  String s;
  if (!parse_identifier (&p, s))
    return false;
  skip_spaces (&p);
  // matchop : [ "^=" | "$=" | "*=" | '=' | "~=" | "|=" ]
  Kind kind;
  switch ((p[0] << 8) + (p[0] && p[1] == '=' ? p[1] : 0))
    {
    case ('^' << 8) + '=':  p += 2; kind = ATTRIBUTE_PREFIX;      break;
    case ('$' << 8) + '=':  p += 2; kind = ATTRIBUTE_SUFFIX;      break;
    case ('*' << 8) + '=':  p += 2; kind = ATTRIBUTE_SUBSTRING;   break;
    case ('~' << 8) + '=':  p += 2; kind = ATTRIBUTE_INCLUDES;    break;
    case ('|' << 8) + '=':  p += 2; kind = ATTRIBUTE_DASHSTART;   break;
    case ('!' << 8) + '=':  p += 2; kind = ATTRIBUTE_UNEQUALS;    break; // non-standard
    case ('=' << 8) + '=':  p += 2; kind = ATTRIBUTE_EQUALS;      break; // non-standard
    case ('=' << 8) + 0:    p += 1; kind = ATTRIBUTE_EQUALS;      break;
    default:                        kind = NONE;                  break;
    }
  if (kind == NONE && (p[0] == ']' || (p[0] == 'i' && p[1] == ']'))) // no matchop
    {
      kind = p[0] == 'i' ? ATTRIBUTE_EXISTS_I : ATTRIBUTE_EXISTS;
      if (p[0] == 'i')
        p++;
      p++;
      SelectorNode node (kind, s);
      chain.push_back (node);
      *stringp = p;
      return true;
    }
  // S* [ IDENT | STRING ] S* 'i'?
  skip_spaces (&p);
  String a;
  if (parse_string (&p, a) ||
      parse_identifier (&p, a))
    {
      skip_spaces (&p);
      if (*p == 'i') // case insensitivity flag
        {
          p++;
          kind = Kind (kind + 1);       // ATTRIBUTE_* -> ATTRIBUTE_*_I
        }
      if (*p == ']')
        {
          p++;
          SelectorNode node (kind, s, a);
          chain.push_back (node);
          *stringp = p;
          return true;
        }
    }
  return false;
}

static bool
parse_special_selector (const char **stringp, SelectorChain &chain)
{ // special_selector : HASH | class | attrib | pseudo | negation
  const char *p = *stringp;
  String s;
  switch (*p)
    {
    case '#': // HASH : '#' identifier
      p++;
      if (parse_identifier (&p, s))
        {
          SelectorNode node (ID, s);
          chain.push_back (node);
          *stringp = p;
          return true;
        }
      return false;
    case '.': // class : '.' identifier
      p++;
      if (parse_identifier (&p, s))
        {
          SelectorNode node (CLASS, s);
          chain.push_back (node);
          *stringp = p;
          return true;
        }
      return false;
    case '[': // attrib : '[' S* [ namespace_prefix ]? IDENT S* [ [ matchvariants... ] S* [ IDENT | STRING ] S*]? ']'
      return parse_attribute (stringp, chain);
    case ':': // pseudo : ':' ':'? [ IDENT | functional_pseudo ] | functional_negation
      p++;
      if (parse_pseudo_selector (&p, chain, 1))
        {
          *stringp = p;
          return true;
        }
      return false;
    }
  return false;
}

static bool
parse_universal_selector (const char **stringp, SelectorChain &chain)
{ // universal : [ namespace_prefix ]? '*'
  const char *p = *stringp;
  if (*p == '*')
    {
      SelectorNode node;
      node.kind = UNIVERSAL;
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
      node.kind = TYPE;
      chain.push_back (node);
      return true;
    }
  return false;
}

static bool
parse_simple_selector_sequence (const char **stringp, SelectorChain &chain)
{ // simple_selector_sequence : [ [ type_selector | universal ] [ special_selector ]* | [ special_selector ]+ ]
  const char *p = *stringp;
  SelectorChain tmpchain;
  bool seen_selector = parse_type_selector (&p, tmpchain) || parse_universal_selector (&p, tmpchain);
  while (parse_special_selector (&p, tmpchain))
    seen_selector = true;
  if (seen_selector)
    {
      chain.insert (chain.end(), tmpchain.begin(), tmpchain.end());
      *stringp = p;
      return true;
    }
  return false;
}

static bool
parse_selector_combinator (const char **stringp, Kind *kind, bool *subjectp)
{ // combinator : '!'? [ S* '+' S* | S* '>' S* | S* '~' S* | S+ ]
  const char *p = *stringp;
  const bool subject = *p == '!';
  p += subject ? 1 : 0;
  const bool seen_spaces = parse_spaces (&p, 1);
  switch (*p)
    {
    case '+':   p++; *kind = ADJACENT;    break;
    case '>':   p++; *kind = CHILD;       break;
    case '~':   p++; *kind = NEIGHBORING; break;
    default:
      if (seen_spaces)
        {
          *kind = DESCENDANT;
          *subjectp = subject;
          *stringp = p;
          return true;
        }
      return false;
    }
  skip_spaces (&p);
  *subjectp = subject;
  *stringp = p;
  return true;
}

bool
SelectorChain::parse (const char **stringp, const bool with_combinators)
{
  return_val_if_fail (stringp != NULL, false);
  const char *p = *stringp;
  /* selector : simple_selector_sequence [ combinator simple_selector_sequence ]* */
  SelectorChain tmpchain;
  if (parse_simple_selector_sequence (&p, tmpchain))
    {
      SelectorChain nextchain;
      const char *q = p;
      SelectorNode combinator_node;
      bool subject;
      while (with_combinators && parse_selector_combinator (&q, &combinator_node.kind, &subject))
        {
          if (parse_simple_selector_sequence (&q, nextchain))
            {
              if (subject)
                tmpchain.push_back (SelectorNode (SUBJECT));
              tmpchain.push_back (combinator_node);
              tmpchain.insert (tmpchain.end(), nextchain.begin(), nextchain.end());
              nextchain.clear();
              p = q;
            }
        }
      this->swap (tmpchain);
      *stringp = p;
      return true;
    }
  return false;
}

String
SelectorChain::string ()
{
  String s;
  for (size_t i = 0; i < size(); i++)
    {
      const SelectorNode &node = operator[] (i);
      const char *flag = "";
      switch (node.kind)
        {
        case NONE:
          s += "<NONE>";
          break;
        case SUBJECT:
          s += "!";
          break;
        case TYPE:
          s += node.ident;
          break;
        case UNIVERSAL:
          s += node.ident;
          break;
        case CLASS:
          s += "." + node.ident;
          break;
        case ID:
          s += "#" + node.ident;
          break;
        case PSEUDO_ELEMENT:
          s += "::" + node.ident;
          break;
        case PSEUDO_CLASS:
          s += ":" + node.ident;
          if (!node.arg.empty())
            s += "(" + node.arg + ")";
          break;
        case ATTRIBUTE_EXISTS_I:
          flag = " i"; // pass through
        case ATTRIBUTE_EXISTS:
          s += "[" + node.ident + flag + "]";
          break;
        case ATTRIBUTE_EQUALS_I:
          flag = " i"; // pass through
        case ATTRIBUTE_EQUALS:
          s += "[" + node.ident + "=" + maybe_quote_identifier (node.arg) + flag + "]";
          break;
        case ATTRIBUTE_UNEQUALS_I:
          flag = " i"; // pass through
        case ATTRIBUTE_UNEQUALS:
          s += "[" + node.ident + "!=" + maybe_quote_identifier (node.arg) + flag + "]";
          break;
        case ATTRIBUTE_PREFIX_I:
          flag = " i"; // pass through
        case ATTRIBUTE_PREFIX:
          s += "[" + node.ident + "^=" + maybe_quote_identifier (node.arg) + flag + "]";
          break;
        case ATTRIBUTE_SUFFIX_I:
          flag = " i"; // pass through
        case ATTRIBUTE_SUFFIX:
          s += "[" + node.ident + "$=" + maybe_quote_identifier (node.arg) + flag + "]";
          break;
        case ATTRIBUTE_DASHSTART_I:
          flag = " i"; // pass through
        case ATTRIBUTE_DASHSTART:
          s += "[" + node.ident + "|=" + maybe_quote_identifier (node.arg) + flag + "]";
          break;
        case ATTRIBUTE_SUBSTRING_I:
          flag = " i"; // pass through
        case ATTRIBUTE_SUBSTRING:
          s += "[" + node.ident + "*=" + maybe_quote_identifier (node.arg) + flag + "]";
          break;
        case ATTRIBUTE_INCLUDES_I:
          flag = " i"; // pass through
        case ATTRIBUTE_INCLUDES:
          s += "[" + node.ident + "~=" + maybe_quote_identifier (node.arg) + flag + "]";
          break;
        case DESCENDANT:
          s += " ";
          break;
        case CHILD:
          s += " > ";
          break;
        case ADJACENT:
          s += " + ";
          break;
        case NEIGHBORING:
          s += " ~ ";
          break;
        }
    }
  return s;
}

bool
Matcher::match_attribute_selector (ItemImpl &item, const SelectorNode &snode)
{
  const Kind kind = snode.kind;
  const bool existing = item.lookup_property (snode.ident);
  if (kind == ATTRIBUTE_EXISTS || kind == ATTRIBUTE_EXISTS_I)
    return existing;
  const String value = existing ? item.get_property (snode.ident) : "";
  const size_t vs = value.size(), as = snode.arg.size(), ms = min (vs, as);
  const char *const vd = value.data(), *const ad = snode.arg.data();
  switch (kind)
    {
      const char *p;
    case ATTRIBUTE_EQUALS:      return value == snode.arg;
    case ATTRIBUTE_EQUALS_I:    return vs == as && strncasecmp (vd, ad, ms) == 0;
    case ATTRIBUTE_UNEQUALS:    return value != snode.arg;
    case ATTRIBUTE_UNEQUALS_I:  return vs != as || strncasecmp (vd, ad, ms) != 0;
    case ATTRIBUTE_PREFIX:      return value.compare (0, as, snode.arg) == 0;
    case ATTRIBUTE_PREFIX_I:    return strncasecmp (vd, ad, ms) == 0;
    case ATTRIBUTE_SUFFIX:      return (vs >= as && value.compare (vs - as, as, snode.arg) == 0);
    case ATTRIBUTE_SUFFIX_I:    return (vs >= as && strncasecmp (vd + vs - as, ad, ms) == 0);
    case ATTRIBUTE_DASHSTART:   return value.compare (0, as, snode.arg) == 0 && (vs == as || value[as] == '-');
    case ATTRIBUTE_DASHSTART_I: return strncasecmp (vd, ad, ms) == 0 && (vs == as || value[as] == '-');
    case ATTRIBUTE_SUBSTRING:   return value.find (snode.arg) != value.npos;
    case ATTRIBUTE_SUBSTRING_I: return strcasestr (vd, ad) != NULL;
    case ATTRIBUTE_INCLUDES:
    case ATTRIBUTE_INCLUDES_I:
      p = kind == ATTRIBUTE_INCLUDES_I ? strcasestr (vd, ad) : strstr (vd, ad);
      if (!p)
        return false;
      if (p > vd && !(p[-1] == ' ' || p[-1] == '\t'))
        return false;
      p += as;
      if (p < vd + vs && !(*p == ' ' || *p == '\t'))
        return false;
      return true;
    default: ;
    }
  return false;
}

bool
Matcher::match_pseudo_element (ItemImpl &item, const SelectorNode &snode)
{
  return false; // FIXME
}

bool
Matcher::match_pseudo_class (ItemImpl &item, const SelectorNode &snode)
{
  if (snode.ident == "not")
    {
      Matcher matcher;
      if (!matcher.parse_selector (snode.arg, false))
        return false;
      return !matcher.match_selector (item);
    }
  else if (snode.ident == "empty")
    {
      ContainerImpl *c = dynamic_cast<ContainerImpl*> (&item);
      return !c || !c->has_children();
    }
  else if (snode.ident == "only-child")
    {
      ContainerImpl *p = item.parent();
      return p && p->n_children() == 1;
    }
  else if (snode.ident == "root")
    {
      ContainerImpl *p = item.parent();
      return !p;
    }
  else if (snode.ident == "first-child")
    {
      ContainerImpl *p = item.parent();
      if (!p)
        return false;
      ContainerImpl::ChildWalker cw = p->local_children();
      return cw.has_next() && &item == &*cw;
    }
  else if (snode.ident == "last-child")
    {
      ContainerImpl *p = item.parent();
      if (!p)
        return false;
      ItemImpl *last = NULL;
      for (ContainerImpl::ChildWalker cw = p->local_children(); cw.has_next(); cw++)
        last = &*cw;
      return &item == last;
    }
  return false; // unknown pseudo class
}

bool
Matcher::match_element_selector (ItemImpl &item, const SelectorNode &snode)
{
  switch (snode.kind)
    {
    case UNIVERSAL:     return true;
    case ID:            return snode.ident == item.name();
    case TYPE:          return snode.ident == Factory::factory_context_type (item.factory_context());
    default:            return false;   // unreached
    case CLASS:         ; // pass through
    }
  // CLASS:
  StringList tags = Factory::factory_context_tags (item.factory_context());
  bool result = false;
  for (StringList::const_iterator it = tags.begin(); it != tags.end() && !result; it++)
    {
      const String &tag = *it;
      size_t i = tag.rfind (snode.ident);
      if (i == String::npos)
        continue;                                     // no match
      if (i + snode.ident.size() == tag.size() and    // tail match
          (i == 0 ||                                  // full match
           tag.data()[i - 1] == ':'))                 // match at namespace boundary
        result = true;
    }
  // DEBUG ("SELECTOR: %s in (%s): %u", snode.ident.c_str(), string_join (" ", tags).c_str(), result);
  return result;
}

template<int CDIR> bool
Matcher::match_selector_stepwise (ItemImpl &item, const size_t chain_index)
{
  return_val_if_fail (chain_index < chain.size(), false);
  RAPICORN_STATIC_ASSERT (CDIR);
  const SelectorNode &snode = chain[chain_index];
  switch (snode.kind)
    {
    default:
    case NONE:
    case SUBJECT:
      break; // automatic match
    case TYPE: case UNIVERSAL: case CLASS: case ID:
      if (match_element_selector (item, snode))
        break;
      return false;
    case PSEUDO_ELEMENT:
      if (match_pseudo_element (item, snode))
        break;
      return false;
    case PSEUDO_CLASS:
      if (match_pseudo_class (item, snode))
        break;
      return false;
    case ATTRIBUTE_EXISTS:    case ATTRIBUTE_EXISTS_I:    case ATTRIBUTE_EQUALS:    case ATTRIBUTE_EQUALS_I:
    case ATTRIBUTE_UNEQUALS:  case ATTRIBUTE_UNEQUALS_I:  case ATTRIBUTE_PREFIX:    case ATTRIBUTE_PREFIX_I:
    case ATTRIBUTE_SUFFIX:    case ATTRIBUTE_SUFFIX_I:    case ATTRIBUTE_DASHSTART: case ATTRIBUTE_DASHSTART_I:
    case ATTRIBUTE_SUBSTRING: case ATTRIBUTE_SUBSTRING_I: case ATTRIBUTE_INCLUDES:  case ATTRIBUTE_INCLUDES_I:
      if (match_attribute_selector (item, snode))
        break;
      return false;
    case CHILD:
      if (CDIR < 0)
        {
          ContainerImpl *p = item.parent();
          if (p && chain_index > 0 && match_selector_stepwise<CDIR> (*p, chain_index - 1))
            return true;
          return false;
        }
      else // CDIR > 0
        {
          ContainerImpl *c = dynamic_cast<ContainerImpl*> (&item);
          if (c && chain_index + 1 < chain.size())
            for (ContainerImpl::ChildWalker cw = c->local_children(); cw.has_next(); cw++)
              {
                if (match_selector_stepwise<CDIR> (*cw, chain_index + 1))
                  return true;
              }
          return false;
        }
    case DESCENDANT:
      if (CDIR < 0)
        {
          if (chain_index < 1)
            return false; // bogus chaining
          ContainerImpl *p = item.parent();
          while (p)
            {
              if (match_selector_stepwise<CDIR> (*p, chain_index - 1))
                return true;
              p = p->parent();
            }
          return false;
        }
      else // CDIR > 0
        {
          ContainerImpl *c = dynamic_cast<ContainerImpl*> (&item);
          if (c && chain_index + 1 < chain.size() &&
              match_selector_children (*c, chain_index + 1))
            return true;
          return false;
        }
    case ADJACENT:
      if (CDIR < 0)
        {
          ContainerImpl *p = item.parent();
          if (p && chain_index > 0)
            {
              ItemImpl *last = NULL;
              for (ContainerImpl::ChildWalker cw = p->local_children(); cw.has_next(); last = &*cw, cw++)
                if (&item == &*cw)
                  {
                    if (!last)
                      break;
                    return match_selector_stepwise<CDIR> (*last, chain_index - 1);
                  }
            }
        }
      else // CDIR > 0
        {
          ContainerImpl *p = item.parent();
          if (p && chain_index + 1 < chain.size())
            for (ContainerImpl::ChildWalker cw = p->local_children(); cw.has_next(); cw++)
              if (&item == &*cw)
                {
                  cw++;
                  if (!cw.has_next())
                    break;
                  return match_selector_stepwise<CDIR> (*cw, chain_index + 1);
                }
        }
      return false; // no adjacent sibling present
    case NEIGHBORING:
      if (CDIR < 0)
        {
          ContainerImpl *p = item.parent();
          if (p && chain_index > 0)
            for (ContainerImpl::ChildWalker cw = p->local_children(); cw.has_next(); cw++)
              if (&item == &*cw)
                break;
              else if (match_selector_stepwise<CDIR> (*cw, chain_index - 1))
                return true;
        }
      else // CDIR > 0
        {
          ContainerImpl *p = item.parent();
          if (p && chain_index + 1 < chain.size())
            {
              ContainerImpl::ChildWalker cw;
              for (cw = p->local_children(); cw.has_next(); cw++)
                if (&item == &*cw)
                  {
                    cw++;
                    break;
                  }
              for (; cw.has_next(); cw++)
                if (match_selector_stepwise<CDIR> (*cw, chain_index + 1))
                  return true;
            }
        }
      return false; // no matching sibling
    }
  if (CDIR < 0 && chain_index > 0)
    return match_selector_stepwise<CDIR> (item, chain_index - 1);
  if (CDIR > 0 && chain_index + 1 < chain.size())
    return match_selector_stepwise<CDIR> (item, chain_index + 1);
  return true;
}

bool
Matcher::match_selector_children (ContainerImpl &container, const size_t chain_index)
{
  for (ContainerImpl::ChildWalker cw = container.local_children(); cw.has_next(); cw++)
    {
      if (match_selector_stepwise<+1> (*cw, chain_index))
        return true;
      ContainerImpl *c = dynamic_cast<ContainerImpl*> (&*cw);
      if (c && match_selector_children (*c, chain_index))
        return true;
    }
  return false;
}

bool
Matcher::parse_selector (const String &selector,
                         bool with_combinators,
                         String       *errorp)
{
  return_val_if_fail (chain.empty(), false);
  const char *s = selector.c_str();
  String error;
  // parse selector string
  if (!chain.parse (&s, with_combinators) || chain.empty())
    error = string_printf ("%s: invalid selector syntax: %s\n", __func__, string_to_cquote (selector).c_str());
  else if (*s)
    error = string_printf ("%s: unexpected junk in selector (%s): %s\n", __func__,
                           string_to_cquote (String (selector.c_str(), s - selector.c_str())).c_str(),
                           string_to_cquote (s).c_str());
  // report errors
  if (!error.empty())
    {
      if (errorp)
        *errorp = error;
      else
        DEBUG ("SELECTOR-PARSE: %s", error.c_str());
      return false;
    }
  // find subject element
  subject_index = chain.size() - 1;
  for (size_t i = 0; i < chain.size(); i++)
    if (chain[i].kind == SUBJECT)
      {
        subject_index = i;
        break;
      }
  return true;
}

bool
Matcher::match_selector (ItemImpl &item)
{
  if (subject_index < chain.size() && !match_selector_stepwise<-1> (item, subject_index))
    return false;
  if (subject_index + 1 < chain.size() && !match_selector_stepwise<+1> (item, subject_index + 1))
    return false;
  return true;
}

template<size_t COUNT> vector<ItemImpl*>
Matcher::recurse_selector (ItemImpl &item)
{
  vector<ItemImpl*> result;
  if (match_selector (item))
    {
      result.push_back (&item);
      if (COUNT == 1)
        return result;
    }
  ContainerImpl *container = dynamic_cast<ContainerImpl*> (&item);
  if (!container)
    return result;
  for (ContainerImpl::ChildWalker cw = container->local_children(); cw.has_next(); cw++)
    {
      vector<ItemImpl*> subs;
      if (COUNT == 2 && result.size() == 1)
        subs = recurse_selector<1> (*cw);
      else // COUNT==0 || result.empty())
        subs = recurse_selector<COUNT> (*cw);
      if (!subs.empty())
        {
          if (result.empty())
            result.swap (subs);
          else
            result.insert (result.end(), subs.begin(), subs.end());
          if (COUNT && result.size() >= COUNT)
            break;
        }
    }
  return result;
}

bool
Matcher::match_selector (const String &selector, ItemImpl &item)
{
  Matcher matcher;
  if (!matcher.parse_selector (selector, true))
    return false;
  if (!matcher.match_selector (item))
    return false;
  return true;
}

vector<ItemImpl*>
Matcher::query_selector_all (const String &selector, ItemImpl &item)
{
  Matcher matcher;
  vector<ItemImpl*> result;
  if (matcher.parse_selector (selector, true))
    result = matcher.recurse_selector<0> (item);
  return result;
}

ItemImpl*
Matcher::query_selector_first (const String &selector, ItemImpl &item)
{
  Matcher matcher;
  vector<ItemImpl*> result;
  if (matcher.parse_selector (selector, true))
    result = matcher.recurse_selector<1> (item);
  return result.empty() ? NULL : result[0];
}

ItemImpl*
Matcher::query_selector_unique (const String &selector, ItemImpl &item)
{
  Matcher matcher;
  vector<ItemImpl*> result;
  if (matcher.parse_selector (selector, true))
    result = matcher.recurse_selector<2> (item);
  return result.size() != 1 ? NULL : result[0];
}

} // Selector
} // Rapicorn
