// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#ifndef __RAPICORN_SELECTOR_HH__
#define __RAPICORN_SELECTOR_HH__

#include <rapicorn-core.hh>

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

class CustomPseudoRegistry {
  CustomPseudoRegistry                 *next_;
  String                                ident_, blurb_;
  static CustomPseudoRegistry* volatile stack_head;
  RAPICORN_CLASS_NON_COPYABLE (CustomPseudoRegistry);
public:
  const String& ident                () const { return ident_; }
  const String& blurb                () const { return blurb_; }
  explicit      CustomPseudoRegistry (const String &id, const String &b = "") :
    next_ (NULL), ident_ (string_tolower (id)), blurb_ (b)
  { atomic_push_link (&stack_head, &next_, this); }
  CustomPseudoRegistry*         next () const { return next_; }
  static CustomPseudoRegistry*  head ()       { return atomic_load (&stack_head); }
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

class Selob { // Matchable Objects
public:
  typedef std::vector<std::string> Strings;
  typedef const StringVector ConstTypes;
  virtual             ~Selob           () {}
  virtual String       get_id          () = 0;
  virtual String       get_type        () = 0;                         // Factory::factory_context_type (selob.factory_context())
  virtual ConstTypes&  get_type_list   () = 0;
  virtual bool         has_property    (const String &name) = 0;       // widget.lookup_property
  virtual String       get_property    (const String &name) = 0;
  virtual Selob*       get_parent      () = 0;
  virtual Selob*       get_sibling     (int64 dir) = 0;
  virtual bool         has_children    () = 0;
  virtual int64        n_children      () = 0;
  virtual Selob*       get_child       (int64 index) = 0;
  virtual bool         is_nth_child    (int64 nth1based) = 0;
  virtual Selob*       pseudo_selector (const String &ident, const String &arg, String &error) = 0;
  static Selob*        true_match      ();
  static bool          is_true_match   (Selob *selob) { return selob == true_match(); }
};

class Matcher {
  SelectorChain             chain;
  uint                      subject_index, last_combinator, first_pseudo_element;
  /*ctor*/                  Matcher() : subject_index (UINT_MAX), last_combinator (UINT_MAX), first_pseudo_element (UINT_MAX) {}
  bool                      match_attribute_selector   (Selob &selob, const SelectorNode &snode);
  Selob*                    match_pseudo_element       (Selob &selob, const SelectorNode &snode);
  bool                      match_pseudo_class         (Selob &selob, const SelectorNode &snode);
  bool                      match_element_selector     (Selob &selob, const SelectorNode &snode);
  template<int CDIR> Selob* match_selector_stepwise    (Selob &selob, const size_t chain_index);
  Selob*                    match_selector_descendants (Selob &selob, const size_t chain_index);
  Selob*                    match_selector_chain       (Selob &selob);
  template<size_t COUNT>
  vector<Selob*>            recurse_selector           (Selob &selob);
  bool                      parse_selector             (const String &selector, bool with_combinators, String *errorp = NULL);
public:
  static bool               query_selector_bool        (const String &selector, Selob &selob, String *errorp = NULL);
  static Selob*             query_selector_first       (const String &selector, Selob &selob, String *errorp = NULL);
  static Selob*             query_selector_unique      (const String &selector, Selob &selob, String *errorp = NULL);
  static vector<Selob*>     query_selector_all         (const String &selector, Selob &selob, String *errorp = NULL);
  template<class Iter>
  static vector<Selob*>     query_selector_objects     (const String &selector, Iter first, Iter last, String *errorp = NULL);
};

// Implementations
template<class Iter> vector<Selob*>
Matcher::query_selector_objects (const String &selector, Iter first, Iter last, String *errorp)
{
  Matcher matcher;
  vector<Selob*> rvector;
  if (matcher.parse_selector (selector, true, errorp))
    for (Iter it = first; it != last; it++)
      {
        Selob *selob = *it;
        if (!selob)
          continue;
        Selob *result = matcher.match_selector_chain (*selob);
        if (result)
          rvector.push_back (result);
      }
  return rvector;
}

} // Selector
} // Rapicorn

#endif /* __RAPICORN_SELECTOR_HH__ */
