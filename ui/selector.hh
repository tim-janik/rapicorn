// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __RAPICORN_SELECTOR_HH__
#define __RAPICORN_SELECTOR_HH__

#include <ui/item.hh>

namespace Rapicorn {
namespace Selector {

enum Kind {
  NONE,
  SUBJECT, TYPE, UNIVERSAL,                   // element selectors
  CLASS, ID, PSEUDO_ELEMENT, PSEUDO_CLASS,    // class, id, pseudo selectors
  ATTRIBUTE_EXISTS, ATTRIBUTE_EQUALS,         // attributes with match types
  ATTRIBUTE_PREFIX, ATTRIBUTE_SUFFIX, ATTRIBUTE_DASHSTART, ATTRIBUTE_SUBSTRING, ATTRIBUTE_INCLUDES,
  DESCENDANT, CHILD, NEIGHBOUR, FOLLOWING,    // selector combinators
};

bool    parse_spaces            (const char **stringp, int min_spaces = 1);
bool    skip_spaces             (const char **stringp);
bool    scan_nested             (const char **stringp, const char *pairflags, const char term);
bool    parse_case_word         (const char **stringp, const char *word);
bool    parse_unsigned_integer  (const char **stringp, uint64 *up);
bool    parse_signed_integer    (const char **stringp, int64 *ip);
bool    parse_css_nth           (const char **stringp, int64 *ap, int64 *bp);
bool    match_css_nth           (int64 pos, int64 a, int64 b);

bool    parse_identifier        (const char **stringp, String &ident);
bool    parse_string            (const char **stringp, String &ident);

struct SelectorNode {
  Kind   kind;
  String ident;
  String arg;
  SelectorNode (Kind k = NONE, const String &i = "", const String &a = "") : kind (k), ident (i), arg (a) {}
  bool operator== (const SelectorNode &o) const { return kind == o.kind && ident == o.ident && arg == o.arg; }
  bool operator!= (const SelectorNode &o) const { return !operator== (o); }
};
struct SelectorChain : public vector<SelectorNode> {
  bool          parse   (const char **stringp);
  String        string  ();
};

} // Selector
} // Rapicorn

#endif /* __RAPICORN_SELECTOR_HH__ */
