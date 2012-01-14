// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __RAPICORN_SELECTOR_HH__
#define __RAPICORN_SELECTOR_HH__

#include <ui/primitives.hh>

namespace Rapicorn {
namespace Parser {

bool    parse_spaces            (const char **stringp, int min_spaces = 1);
bool    skip_spaces             (const char **stringp);
bool    parse_case_word         (const char **stringp, const char *word);
bool    parse_unsigned_integer  (const char **stringp, uint64 *up);
bool    parse_signed_integer    (const char **stringp, int64 *ip);
bool    parse_css_nth           (const char **stringp, int64 *ap, int64 *bp);
bool    match_css_nth           (int64 pos, int64 a, int64 b);

bool    parse_identifier        (const char **stringp, String &ident);

struct SelectorNode {
  enum Kind { NONE, TYPE, UNIVERSAL, ATTRIBUTE, CLASS, ID, PSEUDO,
              DESCENDANT, CHILD, NEIGHBOUR, FOLLOWING, };
  Kind  kind;
  String ident;
  String arg;
  SelectorNode (Kind k = NONE) : kind (k) {}
  bool operator== (const SelectorNode &o) const { return kind == o.kind && ident == o.ident && arg == o.arg; }
  bool operator!= (const SelectorNode &o) const { return !operator== (o); }
};
typedef vector<SelectorNode> SelectorChain;

bool    parse_selector_chain    (const char **stringp, SelectorChain &schain);

} // Parser
} // Rapicorn

#endif /* __RAPICORN_SELECTOR_HH__ */
