// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __RAPICORN_SELECTOR_HH__
#define __RAPICORN_SELECTOR_HH__

#include <ui/item.hh>

namespace Rapicorn {
namespace Selector {

enum Kind {
  NONE,
  SUBJECT, TYPE, UNIVERSAL,                     // element selectors
  CLASS, ID, PSEUDO_ELEMENT, PSEUDO_CLASS,      // class, id, pseudo selectors
  DESCENDANT, CHILD, ADJACENT, NEIGHBORING,     // selector combinators
  ATTRIBUTE_EXISTS, ATTRIBUTE_EXISTS_I,         // attributes with match types
  ATTRIBUTE_EQUALS, ATTRIBUTE_EQUALS_I, ATTRIBUTE_UNEQUALS, ATTRIBUTE_UNEQUALS_I,
  ATTRIBUTE_PREFIX, ATTRIBUTE_PREFIX_I, ATTRIBUTE_SUFFIX, ATTRIBUTE_SUFFIX_I,
  ATTRIBUTE_DASHSTART, ATTRIBUTE_DASHSTART_I, ATTRIBUTE_SUBSTRING, ATTRIBUTE_SUBSTRING_I,
  ATTRIBUTE_INCLUDES, ATTRIBUTE_INCLUDES_I,
};
inline bool is_combinator           (Kind kind) { return kind >= DESCENDANT && kind <= NEIGHBORING; }

class CustomPseudoRegistry : protected NonCopyable {
  CustomPseudoRegistry                 *m_next;
  String                                m_ident, m_blurb;
  static CustomPseudoRegistry *volatile stack_head;
public:
  const String& ident                () const { return m_ident; }
  const String& blurb                () const { return m_blurb; }
  explicit      CustomPseudoRegistry (const String &id, const String &b = "") :
    m_next (NULL), m_ident (string_tolower (id)), m_blurb (b)
  { Atomic::stack_push (stack_head, m_next, this); }
  CustomPseudoRegistry*         next () const { return m_next; }
  static CustomPseudoRegistry*  head ()       { return stack_head; }
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
  bool          parse   (const char **stringp, bool with_combinators = true);
  String        string  (size_t first = 0);
};

class Matcher {
  SelectorChain           chain;
  size_t                  subject_index;
  /*ctor*/                Matcher() : subject_index (-1) {}
  bool                    match_attribute_selector (ItemImpl &item, const SelectorNode &snode);
  bool                    match_pseudo_element     (ItemImpl &item, const SelectorNode &snode);
  bool                    match_pseudo_class       (ItemImpl &item, const SelectorNode &snode);
  bool                    match_element_selector   (ItemImpl &item, const SelectorNode &snode);
  template<int CDIR> bool match_selector_stepwise  (ItemImpl &item, const size_t chain_index);
  bool                    match_selector_children  (ContainerImpl &container, const size_t chain_index);
  bool                    match_selector           (ItemImpl &item);
  template<size_t COUNT>
  vector<ItemImpl*>       recurse_selector         (ItemImpl &item);
  bool                    parse_selector           (const String &selector, bool with_combinators, String *errorp = NULL);
public:
  template<class Iter> vector<ItemImpl*>
  static                   match_selector        (const String &selector, Iter first, Iter last, String *errorp = NULL);
  static bool              match_selector        (const String &selector, ItemImpl &item);
  static vector<ItemImpl*> query_selector_all    (const String &selector, ItemImpl &item);
  static ItemImpl*         query_selector_first  (const String &selector, ItemImpl &item);
  static ItemImpl*         query_selector_unique (const String &selector, ItemImpl &item);
};

// Implementations
template<class Iter> vector<ItemImpl*>
Matcher::match_selector (const String &selector, Iter first, Iter last, String *errorp)
{
  Matcher matcher;
  vector<ItemImpl*> result;
  if (matcher.parse_selector (selector, errorp))
    for (Iter it = first; it != last; it++)
      {
        ItemImpl *item = *it;
        if (item && matcher.match_selector (*item))
          result.push_back (item);
      }
  return result;
}

} // Selector
} // Rapicorn

#endif /* __RAPICORN_SELECTOR_HH__ */
