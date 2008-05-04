/* Rapicorn
 * Copyright (C) 2002-2006 Tim Janik
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
#include "factory.hh"
#include "evaluator.hh"
#include "root.hh"
#include <stack>
#include <errno.h>
using namespace std;

namespace Rapicorn {
struct ClassDoctor {
  static void item_constructed (Item &item) { item.constructed(); }
};
} // Rapicorn

namespace { // Anon
using namespace Rapicorn;
using namespace Rapicorn::Factory;

static void initialize_standard_gadgets_lazily (void);


/* --- Gadget definition --- */
struct ChildGadget;

struct BaseGadget {
  const String         ident, ancestor, m_input_name;
  int                  m_line_number;
  VariableMap          ancestor_arguments;
  vector<ChildGadget*> children;
  String          location   () const { return m_input_name + String (m_line_number > INT_MIN ? ":" + string_from_int (m_line_number) : ""); }
  String          definition () const { return location() + ":<def:" + ident + "/>"; }
  void            add_child  (ChildGadget *child_gadget) { children.push_back (child_gadget); }
  explicit        BaseGadget (const String      &cident,
                              const String      &cancestor,
                              const String      &input_name,
                              int                line_number,
                              const VariableMap &cancestor_arguments) :
    ident (cident),
    ancestor (cancestor),
    m_input_name (input_name), m_line_number (line_number),
    ancestor_arguments (cancestor_arguments)
  {}
  virtual        ~BaseGadget() {}
};

struct ChildGadget : public BaseGadget {
  explicit        ChildGadget (const String      &cident,
                               const String      &cancestor,
                               const String      &input_name,
                               int                line_number,
                               const VariableMap &cancestor_arguments = VariableMap()) :
    BaseGadget (cident, cancestor, input_name, line_number, cancestor_arguments)
  {}
};

struct GadgetDef : public BaseGadget {
  const ChildGadget   *child_container;
  VariableMap          custom_arguments;
  explicit        GadgetDef  (const String      &cident,
                              const String      &cancestor,
                              const String      &input_name,
                              int                line_number,
                              const VariableMap &cancestor_arguments = VariableMap()) :
    BaseGadget (cident, cancestor, input_name, line_number, cancestor_arguments),
    child_container (NULL)
  {}
};

/* --- FactoryDomain --- */
struct FactoryDomain {
  const String                domain_name;
  const String                i18n_domain;
  std::map<String,GadgetDef*> definitions;
  explicit                    FactoryDomain (const String &cdomain_name,
                                             const String &ci18n_domain) :
    domain_name (cdomain_name),
    i18n_domain (ci18n_domain)
  {}
  explicit                    FactoryDomain ()
  {
    std::map<String,GadgetDef*>::iterator it;
    for (it = definitions.begin(); it != definitions.end(); it++)
      {
        delete it->second;
        it->second = NULL;
      }
  }
};

/* --- FactorySingleton --- */
class FactorySingleton {
  typedef std::pair<String,String>      ArgumentPair;
  typedef vector<ArgumentPair>          ArgumentVector;
  typedef VariableMap::const_iterator   ConstVariableIter;
  /* ChildContainerSlot - return value slot for the current gadget's "child_container" */
  struct ChildContainerSlot {
    const ChildGadget *cgadget;
    Item              *item;
    explicit           ChildContainerSlot (const ChildGadget *gadget) :
      cgadget (gadget), item (NULL)
    {}
  };
  /* domains */
  std::list<FactoryDomain*>     factory_domains;
  FactoryDomain*                lookup_domain           (const String       &domain_name);
  FactoryDomain*                add_domain              (const String       &domain_name,
                                                         const String       &i18n_domain);
  /* gadgets */
  GadgetDef*                    lookup_gadget           (const String       &gadget_identifier);
  Item*                         inherit_gadget          (const String       &ancestor_name,
                                                         const VariableMap  &call_arguments,
                                                         Evaluator          &env);
  Item&                         call_gadget             (const BaseGadget   *bgadget,
                                                         const VariableMap  &const_ancestor_arguments,
                                                         const VariableMap  &const_call_arguments,
                                                         Evaluator          &env,
                                                         ChildContainerSlot *ccslot,
                                                         Container          *parent);
  void                          call_gadget_children    (const BaseGadget   *bgadget,
                                                         Item               &item,
                                                         Evaluator          &env,
                                                         ChildContainerSlot *ccslot);
  /* type registration */
  std::list<const ItemTypeFactory*> types;
  const ItemTypeFactory*        lookup_item_factory     (String                 namespaced_ident);
  Item&                         create_from_item_type   (const String          &ident);
public:
  void                          register_item_factory   (const ItemTypeFactory &itfactory);
  MarkupParser::Error           parse_gadget_file       (FILE                  *file,
                                                         const String          &i18n_domain,
                                                         const String          &file_name,
                                                         const String          &domain);
  void                          parse_gadget_data       (const char            *input_name,
                                                         const uint             data_length,
                                                         const char            *data,
                                                         const String          &i18n_domain,
                                                         const String          &domain,
                                                         const std::nothrow_t  &nt = dothrow);
  Item&                         construct_gadget        (const String          &gadget_identifier,
                                                         const ArgumentList    &arguments,
                                                         String                *gadget_definition = NULL);
  /* singleton definition & initialization */
  static FactorySingleton      *singleton;
  explicit                      FactorySingleton        ()
  {
    if (singleton)
      error (STRFUNC + String() + ": non-singleton initialization");
    else
      singleton = this;
    /* register backlog */
    ItemTypeFactory::initialize_factories();
  }
};
FactorySingleton *FactorySingleton::singleton = NULL;
static FactorySingleton static_factory_singleton;

/* --- GadgetParser --- */
struct GadgetParser : public MarkupParser {
  FactoryDomain          &fdomain;
  std::stack<BaseGadget*> gadget_stack;
  const ChildGadget     **child_container_loc;
  String                  child_container_name;
public:
  GadgetParser (const String  &input_name,
                FactoryDomain &cfdomain) :
    MarkupParser (input_name),
    fdomain (cfdomain), child_container_loc (NULL)
  {}
  static String
  canonify_element (const String &key)
  {
    /* chars => [A-Za-z0-9_-] */
    String s = key;
    for (uint i = 0; i < s.size(); i++)
      if (!((s[i] >= 'A' && s[i] <= 'Z') ||
            (s[i] >= 'a' && s[i] <= 'z') ||
            (s[i] >= '0' && s[i] <= '9') ||
            s[i] == '_' || s[i] == '-'))
        s[i] = '_';
    return s;
  }
  static String
  canonify_attribute (const String &key)
  {
    /* skip gettext prefix */
    String s = key[0] == '_' ? String (key, 1) : key;
    if (s == "id")
      return "name";
    return Evaluator::canonify (s);
  }
  virtual void
  start_element (const String  &element_name,
                 ConstStrings  &attribute_names,
                 ConstStrings  &attribute_values,
                 Error         &error)
  {
    // GadgetDef *top_gadget = !gadget_stack.empty() ? gadget_stack.top() : NULL;
    if (element_name == "rapicorn-definitions")
      ; // outer element
    else if (element_name.compare (0, 4, "def:") == 0 && gadget_stack.empty())
      {
        String ident = element_name.substr (4);
        String qualified_ident = fdomain.domain_name + "::" + ident;
        GadgetDef *dgadget = fdomain.definitions[ident];
        if (dgadget)
          error.set (INVALID_CONTENT, String() + "gadget \"" + qualified_ident + "\" already defined (at " + dgadget->location() + ")");
        else
          {
            VariableMap vmap;
            String inherit;
            child_container_name = "";
            for (uint i = 0; i < attribute_names.size(); i++)
              {
                String canonified_attribute = canonify_attribute (attribute_names[i]);
                if (canonified_attribute == "name")
                  error.set (INVALID_CONTENT,
                             String() + "invalid attribute for inherited gadget: " +
                             attribute_names[i] + "=\"" + attribute_values[i] + "\"");
                else if (canonified_attribute == "child_container")
                  child_container_name = attribute_values[i];
                else if (canonified_attribute == "inherit")
                  inherit = attribute_values[i];
                else
                  vmap[canonified_attribute] = attribute_values[i];
              }
            if (error.set())
              ;
            else if (inherit[0] == 0)
              error.set (INVALID_CONTENT, String ("missing ancestor in gadget definition: ") + qualified_ident);
            else
              {
                int line_number, char_number;
                const char *input_name;
                get_position (&line_number, &char_number, &input_name);
                dgadget = new GadgetDef (ident, inherit, input_name, line_number, vmap);
                if (child_container_name[0])
                  child_container_loc = &dgadget->child_container;
                fdomain.definitions[ident] = dgadget;
                gadget_stack.push (dgadget);
              }
          }
      }
    else if (element_name.compare (0, 4, "arg:") == 0 && gadget_stack.size() == 1)
      {
        GadgetDef *dgadget = dynamic_cast<GadgetDef*> (gadget_stack.top());
        assert (dgadget != NULL); // must succeed for gadget_stack.top()
        String ident = Evaluator::canonify (element_name.substr (4));
        if (dgadget->custom_arguments.find (ident) != dgadget->custom_arguments.end())
          error.set (INVALID_CONTENT, String() + "redeclaration of argument: " + element_name);
        else
          {
            String default_value = "";
            for (uint i = 0; i < attribute_names.size(); i++)
              {
                String canonified_attribute = canonify_attribute (attribute_names[i]);
                if (canonified_attribute == "default")
                  default_value = attribute_values[i];
                else
                  error.set (INVALID_CONTENT,
                             String() + "invalid attribute for " + element_name + ": " +
                             attribute_names[i] + "=\"" + attribute_values[i] + "\"");
              }
            if (!error.set())
              {
                dgadget->custom_arguments[ident] = default_value;
              }
          }
      }
    else if (element_name.compare (0, 5, "prop:") == 0 && gadget_stack.size())
      {
        if (attribute_names.size())
          error.set (INVALID_CONTENT,
                     String() + "invalid attribute for property element: " +
                     attribute_names[0] + "=\"" + attribute_values[0] + "\"");
        recap_element (element_name, attribute_names, attribute_values, error, false);
      }
    else if (!gadget_stack.empty() && canonify_element (element_name) == element_name)
      {
        BaseGadget *bparent = gadget_stack.top();
        String gadget_name = element_name;
        VariableMap vmap;
        for (uint i = 0; i < attribute_names.size(); i++)
          {
            String canonified_attribute = canonify_attribute (attribute_names[i]);
            if (canonified_attribute == "name")
              gadget_name = attribute_values[i];
            else if (canonified_attribute == "child_container" ||
                     canonified_attribute == "inherit")
              error.set (INVALID_CONTENT,
                         String() + "invalid attribute for gadget construction: " +
                         attribute_names[i] + "=\"" + attribute_values[i] + "\"");
            else
              vmap[canonified_attribute] = attribute_values[i];
          }
        int line_number, char_number;
        const char *input_name;
        get_position (&line_number, &char_number, &input_name);
        ChildGadget *cgadget = new ChildGadget (gadget_name, element_name, input_name, line_number, vmap);
        bparent->add_child (cgadget);
        gadget_stack.push (cgadget);
        if (child_container_loc && child_container_name == gadget_name)
          *child_container_loc = cgadget;
      }
    else
      error.set (INVALID_CONTENT, String() + "invalid element: " + element_name);
  }
  virtual void
  end_element (const String  &element_name,
               Error         &error)
  {
    BaseGadget *bgadget = !gadget_stack.empty() ? gadget_stack.top() : NULL;
    GadgetDef *dgadget = dynamic_cast<GadgetDef*> (bgadget);
    if (element_name.compare (0, 4, "def:") == 0 &&
        dgadget && dgadget->ident.compare (&element_name[4]) == 0)
      {
        if (child_container_loc && !*child_container_loc)
          error.set (INVALID_CONTENT, element_name + ": missing child container: " + child_container_name);
        child_container_loc = NULL;
        child_container_name = "";
        gadget_stack.pop();
      }
    else if (element_name.compare (0, 4, "arg:") == 0 && dgadget)
      {}
    else if (element_name.compare (0, 5, "prop:") == 0 && bgadget)
      {
        String canonified_attribute = canonify_attribute (element_name.substr (5));
        if (!canonified_attribute.size() ||
            canonified_attribute == "name" ||
            canonified_attribute == "child_container" ||
            canonified_attribute == "inherit")
          error.set (INVALID_CONTENT, "invalid property element: " + element_name);
        else
          bgadget->ancestor_arguments[canonified_attribute] = recap_string();
      }
    else if (bgadget && bgadget->ancestor.compare (&element_name[0]) == 0)
      gadget_stack.pop();
  }
  virtual void
  text (const String  &text,
        Error         &error)
  {
    String t (text);
    while (t[0] == ' ' || t[0] == '\t' || t[0] == '\r' || t[0] == '\n')
      t.erase (0, 1);
    if (t.size())
      {
        error.code = INVALID_CONTENT;
        error.message = "unsolicited text: \"" + text + "\"";
      }
  }
  virtual void
  pass_through (const String   &text,
                Error          &error)
  {
    if (text.compare (0, 4, "<!--") == 0 && text.compare (text.size() - 3, 3, "-->") == 0)
      {
        String comment = text.substr (4, text.size() - 4 - 3).c_str();
        if (0)
          printf ("COMMENT: %s\n", comment.c_str());
      }
  }
}; // GadgetParser

/* --- FactorySingleton methods --- */
FactoryDomain*
FactorySingleton::lookup_domain (const String &domain_name)
{
  ValueIterator<std::list<FactoryDomain*>::iterator> it;
  for (it = value_iterator (factory_domains.begin()); it != factory_domains.end(); it++)
    if (domain_name == it->domain_name)
      return &*it;
  return NULL;
}

FactoryDomain*
FactorySingleton::add_domain (const String   &domain_name,
                              const String   &i18n_domain)
{
  FactoryDomain *fdomain = new FactoryDomain (domain_name, i18n_domain);
  factory_domains.push_front (fdomain);
  return fdomain;
}


MarkupParser::Error
FactorySingleton::parse_gadget_file (FILE         *file,
                                     const String &i18n_domain,
                                     const String &file_name,
                                     const String &domain)
{
  FactoryDomain *fdomain = lookup_domain (domain);
  if (!fdomain)
    fdomain = add_domain (domain, i18n_domain);
  GadgetParser gp (file_name, *fdomain);
  MarkupParser::Error merror;
  char buffer[1024];
  int n = fread (buffer, 1, 1024, file);
  while (n > 0)
    {
      if (!gp.parse (buffer, n, &merror))
        break;
      n = fread (buffer, 1, 1024, file);
    }
  if (!merror.code)
    gp.end_parse (&merror);
  return merror;
}

void
FactorySingleton::parse_gadget_data (const char            *input_name,
                                     const uint             data_length,
                                     const char            *data,
                                     const String          &i18n_domain,
                                     const String          &domain,
                                     const std::nothrow_t  &nt)
{
  String file_name = input_name;
  FactoryDomain *fdomain = lookup_domain (domain);
  if (!fdomain)
    fdomain = add_domain (domain, i18n_domain);
  GadgetParser gp (file_name, *fdomain);
  MarkupParser::Error error;
  gp.parse (data, data_length, &error);
  if (!error.code)
    gp.end_parse (&error);
  if (error.code)
    {
      // ("error(%d):", error.code)
      String ers = string_printf ("%s:%d:%d: %s", file_name.c_str(), error.line_number, error.char_number, error.message.c_str());
      if (&nt == &dothrow)
        throw Exception (ers);
      else
        Rapicorn::error (ers);
    }
}

GadgetDef*
FactorySingleton::lookup_gadget (const String &gadget_identifier)
{
  const char *tident = gadget_identifier.c_str();
  const char *sep = strrchr (tident, ':');
  String domain, ident;
  if (sep && sep > tident && sep[-1] == ':') /* detected "::" */
    {
      ident = sep + 1;
      domain.assign (tident, sep - tident - 1);
      if (domain[0] == ':' && domain[1] == ':')
        domain.assign (domain.substr (2));
    }
  else
    ident = tident;
  ValueIterator<std::list<FactoryDomain*>::iterator> it;
  for (it = value_iterator (factory_domains.begin()); it != factory_domains.end(); it++)
    if (!domain[0] || domain == it->domain_name)
      {
        GadgetDef *dgadget = it->definitions[ident];
        if (dgadget)
          return dgadget;
      }
#if 0
  diag ("gadgetlookup: %s :: %s (\"%s\")", domain.c_str(), ident.c_str(), gadget_identifier.c_str());
  for (it = value_iterator (factory_domains.begin()); it != factory_domains.end(); it++)
    if (!domain[0] || domain == it->domain_name)
      diag ("unmatched: %s::%s", it->domain_name.c_str(), ident.c_str());
    else
      diag ("unused: %s::", it->domain_name.c_str());
#endif
  return NULL;
}

Item&
FactorySingleton::construct_gadget (const String          &gadget_identifier,
                                    const ArgumentList    &arguments,
                                    String                *gadget_definition)
{
  GadgetDef *dgadget = lookup_gadget (gadget_identifier);
  if (!dgadget)
    error ("Rapicorn::Factory: unknown gadget type: " + gadget_identifier);
  VariableMap args;
  Evaluator::populate_map (args, arguments);
  Evaluator env;
  Item &item = call_gadget (dgadget, dgadget->ancestor_arguments, args, env, NULL, NULL);
  if (gadget_definition)
    *gadget_definition = dgadget->definition();
  return item;
}

static String
variable_map_filter (VariableMap  &vmap,
                     const String &key,
                     const String &value = "")
{
  String result;
  VariableMap::iterator it = vmap.find (key);
  if (it != vmap.end())
    {
      result = it->second;
      vmap.erase (it);
    }
  else
    result = value;
  return result;
}

Item*
FactorySingleton::inherit_gadget (const String      &ancestor_name,
                                  const VariableMap &call_arguments,
                                  Evaluator         &env)
{
  if (ancestor_name[0] == '\177')      /* item factory type */
    {
      Item *item = &create_from_item_type (&ancestor_name[1]);
      assert (call_arguments.size() == 0); // see FactorySingleton::register_item_factory()
      return item;
    }
  else
    {
      GadgetDef *dgadget = lookup_gadget (ancestor_name);
      Item *item = NULL;
      if (dgadget)
        item = &call_gadget (dgadget, dgadget->ancestor_arguments, call_arguments, env, NULL, NULL);
      return item;
    }
}

Item&
FactorySingleton::call_gadget (const BaseGadget   *bgadget,
                               const VariableMap  &const_ancestor_arguments,
                               const VariableMap  &const_call_arguments,
                               Evaluator          &env,
                               ChildContainerSlot *ccslot,
                               Container          *parent)
{
  const GadgetDef *dgadget = dynamic_cast<const GadgetDef*> (bgadget);
  const GadgetDef *real_dgadget = dgadget;
  if (!real_dgadget)
    {
      real_dgadget = lookup_gadget (bgadget->ancestor);
      if (!real_dgadget)
        error ("%s: invalid or unknown gadget type: %s",
               bgadget->location().c_str(), bgadget->ancestor.c_str());
    }
  VariableMap call_args (const_call_arguments);
  /* filter special arguments */
  String name = bgadget->ident;
  name = variable_map_filter (call_args, "name", name);
  /* ignore special arguments (FIXME) */
  variable_map_filter (call_args, "child_container");
  variable_map_filter (call_args, "inherit");
  /* setup custom arguments (from <arg/> statements) */
  VariableMap custom_args;
  for (VariableMap::const_iterator it = real_dgadget->custom_arguments.begin(); it != real_dgadget->custom_arguments.end(); it++)
    {
      VariableMap::iterator ga = call_args.find (it->first);
      if (ga != call_args.end())
        {
          custom_args[ga->first] = ga->second;
          call_args.erase (ga);
        }
      else
        custom_args[it->first] = it->second;
    }
  env.push_map (custom_args);
  /* construct gadget from ancestor */
  Item *itemp;
  String reason;
  try {
    itemp = inherit_gadget (bgadget->ancestor, const_ancestor_arguments, env);
  } catch (std::exception &exc) {
    itemp = NULL;
    reason = exc.what();
  } catch (...) {
    itemp = NULL;
  }
  if (!itemp)
    error ("%s: failed to inherit: %s", bgadget->definition().c_str(),
           reason.size() ? reason.c_str() : String ("unknown gadget type: " + bgadget->ancestor).c_str());
  Item &item = *itemp;
  /* apply arguments */
  try {
    VariableMap unused_args;
    for (ConstVariableIter it = call_args.begin(); it != call_args.end(); it++)
      if (!item.try_set_property (it->first, env.expand_expression (it->second)))
        unused_args[it->first] = it->second;
    call_args = unused_args;
  } catch (...) {
    error ("%s: failed to assign properties", bgadget->definition().c_str());
    sink (item);
    env.pop_map (custom_args);
    throw;
  }
  /* notify of completed object construction */
  ClassDoctor::item_constructed (item);
  /* construct gadget children */
  try {
    ChildContainerSlot outer_ccslot (dgadget ? dgadget->child_container : NULL);
    call_gadget_children (bgadget, item, env, ccslot ? ccslot : &outer_ccslot);
    /* assign specials */
    item.name (name);
    /* setup child container */
    if (!ccslot) /* outer call */
      {
        Container *container = dynamic_cast<Container*> (outer_ccslot.item);
        Container *item_container = dynamic_cast<Container*> (&item);
        if (item_container && !outer_ccslot.cgadget)
          item_container->child_container (item_container);
        else if (item_container && container)
          item_container->child_container (container);
        else if (outer_ccslot.cgadget)
          error ("%s: failed to find child container: %s", bgadget->definition().c_str(), outer_ccslot.cgadget->ident.c_str());
      }
  } catch (std::exception &exc) {
    error ("%s: construction failed: %s", bgadget->definition().c_str(), exc.what());
    sink (item);
    env.pop_map (custom_args);
    throw;
  } catch (...) {
    sink (item);
    env.pop_map (custom_args);
    throw;
  }
  /* add to parent */
  try {
    if (parent)
      {
        /* expand pack args from unused property args */
        VariableMap pack_args;
        for (VariableMap::const_iterator it = call_args.begin(); it != call_args.end(); it++)
          pack_args[it->first] = env.expand_expression (it->second);
        /* pack into parent */
        VariableMap unused_pack_args;
        parent->add (item, pack_args, &unused_pack_args);
        call_args = unused_pack_args;
      }
  } catch (std::exception &exc) {
    error ("%s: adding to parent (%s) failed: %s", bgadget->definition().c_str(), parent->name().c_str(), exc.what());
    sink (item);
    env.pop_map (custom_args);
    throw;
  } catch (...) {
    sink (item);
    env.pop_map (custom_args);
    throw;
  }
  /* cleanups */
  env.pop_map (custom_args);
  for (VariableMap::const_iterator it = call_args.begin(); it != call_args.end(); it++)
    error ("%s: unknown property: %s", bgadget->definition().c_str(), String (it->first + "=" + it->second).c_str());
  return item;
}

void
FactorySingleton::call_gadget_children (const BaseGadget   *bgadget,
                                        Item               &item,
                                        Evaluator          &env,
                                        ChildContainerSlot *ccslot)
{
  /* guard against leafs */
  if (!bgadget->children.size())
    return;
  /* retrieve container */
  Container *container = item.interface<Container*>();
#if 0
  diag ("children of %s:", gadget->ident.c_str());
  uint nth = 0;
  for (Walker<ChildGadget*const> cw = walker (gadget->children); cw.has_next(); cw++)
    diag ("%d) %s", nth++, (*cw)->ident.c_str());
#endif
  if (!container)
    error ("parent gadget fails to implement Container interface: "+ bgadget->ident);
  /* create children */
  for (Walker<ChildGadget*const> cw = walker (bgadget->children); cw.has_next(); cw++)
    {
      /* create child gadget */
      const ChildGadget *child_gadget = *cw;
      /* the real call arguments are stored as ancestor arguments of the child */
      VariableMap call_args (child_gadget->ancestor_arguments);
      Item &child = call_gadget (child_gadget, VariableMap(), call_args, env, ccslot, container);
      /* find child container */
      if (ccslot->cgadget == child_gadget)
        ccslot->item = &child;
    }
}

void
FactorySingleton::register_item_factory (const ItemTypeFactory &itfactory)
{
  const char *ident = itfactory.qualified_type.c_str();
  const char *base = strrchr (ident, ':');
  if (!base || base <= ident || base[-1] != ':')
    error ("invalid/missing domain name in item type: " + String() + ident);
  String domain_name;
  domain_name.assign (ident, base - ident - 1);
  FactoryDomain *fdomain = lookup_domain (domain_name);
  if (!fdomain)
    fdomain = add_domain (domain_name, domain_name);
  GadgetDef *dgadget = new GadgetDef (base + 1, String ("\177") + itfactory.qualified_type, "<builtin>", INT_MIN);
  assert (dgadget->ancestor_arguments.size() == 0); // assumed by FactorySingleton::inherit_gadget()
  fdomain->definitions[dgadget->ident] = dgadget;
  types.push_back (&itfactory);
}

const ItemTypeFactory*
FactorySingleton::lookup_item_factory (String namespaced_ident)
{
  namespaced_ident = namespaced_ident;
  std::list<const ItemTypeFactory*>::iterator it;
  for (it = types.begin(); it != types.end(); it++)
    if ((*it)->qualified_type == namespaced_ident)
      return *it;
  return NULL;
}

Item&
FactorySingleton::create_from_item_type (const String &ident)
{
  const ItemTypeFactory *itfactory = lookup_item_factory (ident);
  if (itfactory)
    return *itfactory->create_item (ident);
  else
    error ("Rapicorn::Factory: unknown item type: " + ident);
}

} // Anon


namespace Rapicorn {

int
Factory::parse_file (const String           &i18n_domain,
                     const String           &file_name,
                     const String           &domain)
{
  initialize_standard_gadgets_lazily();
  String fdomain = domain == "" ? i18n_domain : domain;
  FILE *file = fopen (file_name.c_str(), "r");
  if (!file)
    return -errno;
  MarkupParser::Error merror = FactorySingleton::singleton->parse_gadget_file (file, i18n_domain, file_name, fdomain);
  if (merror.code)
    {
      // ("error(%d):", merror.code)
      String ers = string_printf ("%s:%d:%d: %s", file_name.c_str(), merror.line_number, merror.char_number, merror.message.c_str());
      error (ers);
    }
  fclose (file);
  return 0;
}

Item&
Factory::create_item (const String       &gadget_identifier,
                      const ArgumentList &arguments)
{
  initialize_standard_gadgets_lazily();
  Item &item = FactorySingleton::singleton->construct_gadget (gadget_identifier, arguments);
  return item; // floating
}

Container&
Factory::create_container (const String           &gadget_identifier,
                           const ArgumentList     &arguments)
{
  initialize_standard_gadgets_lazily();
  String gadget_definition;
  Item &item = FactorySingleton::singleton->construct_gadget (gadget_identifier, arguments, &gadget_definition);
  Container *container = dynamic_cast<Container*> (&item);
  if (!container)
    error ("%s: gadget construction yields non container: %s", gadget_definition.c_str(), item.typeid_pretty_name().c_str());
  return *container; // floating
}

Window
Factory::create_window (const String           &gadget_identifier,
                        const ArgumentList     &arguments)
{
  initialize_standard_gadgets_lazily();
  String gadget_definition;
  Item &item = FactorySingleton::singleton->construct_gadget (gadget_identifier, arguments, &gadget_definition);
  Root *root = dynamic_cast<Root*> (&item);
  if (!root)
    error ("%s: gadget construction yields non window: %s", gadget_definition.c_str(), item.typeid_pretty_name().c_str());
  Window win = root->window();
  /* win does ref_sink(); */
  return win;
}

void
Factory::ItemTypeFactory::register_item_factory (const ItemTypeFactory *itfactory)
{
  struct QueuedEntry { const ItemTypeFactory *itfactory; QueuedEntry *next; };
  static QueuedEntry *queued_entries = NULL;
  if (!FactorySingleton::singleton)
    {
      assert (itfactory);
      QueuedEntry *e = new QueuedEntry;
      e->itfactory = itfactory;
      e->next = queued_entries;
      queued_entries = e;
    }
  else if (itfactory)
    FactorySingleton::singleton->register_item_factory (*itfactory);
  else /* singleton && !itfactory */
    {
      while (queued_entries)
        {
          QueuedEntry *e = queued_entries;
          queued_entries = e->next;
          FactorySingleton::singleton->register_item_factory (*e->itfactory);
          delete e;
        }
    }
}

void
Factory::ItemTypeFactory::initialize_factories ()
{
  register_item_factory (NULL);
}

Factory::ItemTypeFactory::ItemTypeFactory (const char *namespaced_ident) :
  qualified_type (namespaced_ident)
{}

void
Factory::ItemTypeFactory::sanity_check_identifier (const char *namespaced_ident)
{
  if (strncmp (namespaced_ident, "Rapicorn::Factory::", 19) != 0)
    error ("item type not qualified as \"Rapicorn::Factory::\": %s", namespaced_ident);
}

} // Rapicorn

namespace { // Anon
#include "gen-zintern.c" // compressed foundation.xml & standard.xml

static void
initialize_standard_gadgets_lazily (void)
{
  assert (rapicorn_thread_entered());
  static bool initialized = false;
  if (!initialized)
    {
      initialized = true;
      uint8 *data;
      const char *domain = "Rapicorn";
      data = zintern_decompress (FOUNDATION_XML_SIZE, FOUNDATION_XML_DATA,
                                 sizeof (FOUNDATION_XML_DATA) / sizeof (FOUNDATION_XML_DATA[0]));
      FactorySingleton::singleton->parse_gadget_data (FOUNDATION_XML_NAME, FOUNDATION_XML_SIZE, (const char*) data, domain, domain);
      zintern_free (data);
      data = zintern_decompress (STANDARD_XML_SIZE, STANDARD_XML_DATA,
                                 sizeof (STANDARD_XML_DATA) / sizeof (STANDARD_XML_DATA[0]));
      FactorySingleton::singleton->parse_gadget_data (STANDARD_XML_NAME, STANDARD_XML_SIZE, (const char*) data, domain, domain);
      zintern_free (data);
    }
}

} // Anon
