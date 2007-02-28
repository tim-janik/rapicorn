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

namespace { // Anon
using namespace Rapicorn;
using namespace Rapicorn::Factory;

static void initialize_standard_gadgets_lazily (void);


/* --- Gadget definition --- */
struct Gadget {
  const String    ident, ancestor, m_input_name;
  int             m_line_number;
  const Gadget   *child_container;
  VariableMap     ancestor_arguments;
  VariableMap     custom_arguments;
  vector<Gadget*> children;
  void            add_child (Gadget *child_gadget) { children.push_back (child_gadget); }
  String          location  () const { return m_input_name + String (m_line_number > INT_MIN ? ":" + string_from_int (m_line_number) : ""); }
  explicit        Gadget    (const String      &cident,
                             const String      &cancestor,
                             const String      &input_name,
                             int                line_number,
                             const VariableMap &cancestor_arguments = VariableMap()) :
    ident (cident),
    ancestor (cancestor),
    m_input_name (input_name), m_line_number (line_number),
    child_container (NULL),
    ancestor_arguments (cancestor_arguments)
  {}
};

/* --- FactoryDomain --- */
struct FactoryDomain {
  const String                domain_name;
  const String                i18n_domain;
  std::map<String,Gadget*>    definitions;
  explicit                    FactoryDomain (const String &cdomain_name,
                                             const String &ci18n_domain) :
    domain_name (cdomain_name),
    i18n_domain (ci18n_domain)
  {}
  explicit                    FactoryDomain ()
  {
    std::map<String,Gadget*>::iterator it;
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
    const Gadget *gadget;
    Item         *item;
    explicit      ChildContainerSlot (const Gadget *cgadget) :
      gadget (cgadget), item (NULL)
    {}
  };
  /* domains */
  std::list<FactoryDomain*>     factory_domains;
  FactoryDomain*                lookup_domain           (const String       &domain_name);
  FactoryDomain*                add_domain              (const String       &domain_name,
                                                         const String       &i18n_domain);
  /* gadgets */
  Gadget*                       lookup_gadget           (const String       &gadget_identifier);
  Item&                         inherit_gadget          (const String       &ancestor_name,
                                                         VariableMap        &call_arguments,
                                                         Evaluator          &env,
                                                         VariableMap        &unused_arguments);
  Item&                         call_gadget             (const Gadget       *gadget,
                                                         VariableMap        &call_arguments,
                                                         Evaluator          &env,
                                                         ChildContainerSlot *ccslot,
                                                         Container          *parent,
                                                         VariableMap        &unused_call_args);
  void                          call_gadget_children    (const Gadget       *gadget,
                                                         Item               &item,
                                                         Evaluator          &env,
                                                         ChildContainerSlot *ccslot);
  /* type registration */
  std::list<const ItemTypeFactory*> types;
  const ItemTypeFactory*        lookup_item_factory     (String                 namespaced_ident);
  Item&                         create_from_item_type   (const String          &ident);
public:
  void                          register_item_factory   (const ItemTypeFactory &itfactory);
  void                          parse_gadget_file       (const String          &file_name,
                                                         const String          &i18n_domain,
                                                         const String          &domain,
                                                         const std::nothrow_t  &nt);
  void                          parse_gadget_data       (const uint             data_length,
                                                         const char            *data,
                                                         const String          &i18n_domain,
                                                         const String          &domain,
                                                         const std::nothrow_t  &nt = dothrow);
  
  Item&                         construct_gadget        (const String          &gadget_identifier,
                                                         const ArgumentList    &arguments);
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
  FactoryDomain      &fdomain;
  std::stack<Gadget*> gadget_stack;
  const Gadget      **child_container_loc;
  String              child_container_name;
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
    // Gadget *top_gadget = !gadget_stack.empty() ? gadget_stack.top() : NULL;
    if (element_name == "rapicorn-gadgets")
      ; // outer element
    else if (element_name.compare (0, 4, "def:") == 0 && gadget_stack.empty())
      {
        String ident = element_name.substr (4);
        String qualified_ident = fdomain.domain_name + "::" + ident;
        Gadget *gadget = fdomain.definitions[ident];
        if (gadget)
          error.set (INVALID_CONTENT, String() + "gadget \"" + qualified_ident + "\" already defined (at " + gadget->location() + ")");
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
              error.set (INVALID_CONTENT, String() + "missing ancestor for gadget \"" + qualified_ident + "\"");
            else
              {
                int line_number, char_number;
                const char *input_name;
                get_position (&line_number, &char_number, &input_name);
                gadget = new Gadget (ident, inherit, input_name, line_number, vmap);
                if (child_container_name[0])
                  child_container_loc = &gadget->child_container;
                fdomain.definitions[ident] = gadget;
                gadget_stack.push (gadget);
              }
          }
      }
    else if (element_name.compare (0, 4, "arg:") == 0 && gadget_stack.size() == 1)
      {
        Gadget *gadget = gadget_stack.top();
        String ident = Evaluator::canonify (element_name.substr (4));
        if (gadget->custom_arguments.find (ident) != gadget->custom_arguments.end())
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
                gadget->custom_arguments[ident] = default_value;
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
        Gadget *gparent = gadget_stack.top();
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
        Gadget *gadget = new Gadget (gadget_name, element_name, input_name, line_number, vmap);
        gparent->add_child (gadget);
        gadget_stack.push (gadget);
        if (child_container_loc && child_container_name == gadget_name)
          *child_container_loc = gadget;
      }
    else
      error.set (INVALID_CONTENT, String() + "invalid element: " + element_name);
  }
  virtual void
  end_element (const String  &element_name,
               Error         &error)
  {
    Gadget *gadget = !gadget_stack.empty() ? gadget_stack.top() : NULL;
    if (element_name.compare (0, 4, "def:") == 0 &&
        gadget && gadget->ident.compare (&element_name[4]) == 0)
      {
        if (child_container_loc && !*child_container_loc)
          error.set (INVALID_CONTENT, element_name + ": missing child container: " + child_container_name);
        child_container_loc = NULL;
        child_container_name = "";
        gadget_stack.pop();
      }
    else if (element_name.compare (0, 4, "arg:") == 0 && gadget)
      {}
    else if (element_name.compare (0, 5, "prop:") == 0 && gadget_stack.size())
      {
        Gadget *gadget = gadget_stack.top();
        String canonified_attribute = canonify_attribute (element_name.substr (5));
        if (!canonified_attribute.size() ||
            canonified_attribute == "name" ||
            canonified_attribute == "child_container" ||
            canonified_attribute == "inherit")
          error.set (INVALID_CONTENT, "invalid property element: " + element_name);
        else
          gadget->ancestor_arguments[canonified_attribute] = recap_string();
      }
    else if (gadget && gadget->ancestor.compare (&element_name[0]) == 0)
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


void
FactorySingleton::parse_gadget_file (const String           &file_name,
                                     const String           &i18n_domain,
                                     const String           &domain,
                                     const std::nothrow_t   &nt)
{
  FactoryDomain *fdomain = lookup_domain (domain);
  if (!fdomain)
    fdomain = add_domain (domain, i18n_domain);
  GadgetParser gp (file_name, *fdomain);
  MarkupParser::Error error;
  FILE *f = fopen (file_name.c_str(), "r");
  if (f)
    {
      char buffer[1024];
      int n = fread (buffer, 1, 1024, f);
      while (n > 0)
        {
          if (!gp.parse (buffer, n, &error))
            break;
          n = fread (buffer, 1, 1024, f);
        }
      if (!error.code)
        gp.end_parse (&error);
    }
  else
    {
      error.code = MarkupParser::READ_FAILED;
      error.message = strerror (errno);
    }
  if (error.code)
    {
      String ers = string_printf ("%s:%d:%d:error(%d): %s", file_name.c_str(), error.line_number, error.char_number, error.code, error.message.c_str());
      if (&nt == &dothrow)
        throw Exception (ers);
      else
        Birnet::error (ers);
    }
}

void
FactorySingleton::parse_gadget_data (const uint             data_length,
                                     const char            *data,
                                     const String          &i18n_domain,
                                     const String          &domain,
                                     const std::nothrow_t  &nt)
{
  String file_name = "-";
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
      String ers = string_printf ("%s:%d:%d:error(%d): %s", file_name.c_str(), error.line_number, error.char_number, error.code, error.message.c_str());
      if (&nt == &dothrow)
        throw Exception (ers);
      else
        Birnet::error (ers);
    }
}

Gadget*
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
        Gadget *gadget = it->definitions[ident];
        if (gadget)
          return gadget;
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
FactorySingleton::construct_gadget (const String           &gadget_identifier,
                                    const ArgumentList     &arguments)
{
  Gadget *gadget = lookup_gadget (gadget_identifier);
  if (!gadget)
    error ("no such gadget: " + gadget_identifier);
  VariableMap args;
  Evaluator::populate_map (args, arguments);
  Evaluator env;
  VariableMap unused_args;
  Item &item = call_gadget (gadget, args, env, NULL, NULL, unused_args);
  for (VariableMap::const_iterator it = unused_args.begin(); it != unused_args.end(); it++)
    error (gadget->location() + ": for <def:" + gadget->ident + "/> - invalid argument: " + it->first + "=" + it->second);
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

Item&
FactorySingleton::inherit_gadget (const String &ancestor_name,
                                  VariableMap  &call_arguments,
                                  Evaluator    &env,
                                  VariableMap  &unused_arguments)
{
  if (ancestor_name[0] == '\177')      /* item factory type */
    {
      Item &item = create_from_item_type (&ancestor_name[1]);
      /* apply arguments */
      try {
        for (ConstVariableIter it = call_arguments.begin(); it != call_arguments.end(); it++)
          if (!item.try_set_property (it->first, env.expand_expression (it->second)))
            unused_arguments[it->first] = it->second;
      } catch (...) {
        sink (&item);
        throw;
      }
      return item;
    }
  else
    {
      Gadget *gadget = lookup_gadget (ancestor_name);
      if (!gadget)
        error ("no such gadget: " + ancestor_name);
      return call_gadget (gadget, call_arguments, env, NULL, NULL, unused_arguments);
    }
}

Item&
FactorySingleton::call_gadget (const Gadget       *gadget,
                               VariableMap        &call_args,
                               Evaluator          &env,
                               ChildContainerSlot *ccslot,
                               Container          *parent,
                               VariableMap        &unused_call_args)
{
  /* filter special arguments */
  String name = gadget->ident;
  name = variable_map_filter (call_args, "name", name);
  /* ignore special arguments (FIXME) */
  variable_map_filter (call_args, "child_container");
  variable_map_filter (call_args, "inherit");
  /* seperate custom arguments */
  VariableMap custom_args;
  for (VariableMap::const_iterator it = gadget->custom_arguments.begin(); it != gadget->custom_arguments.end(); it++)
    {
      VariableMap::iterator ga = call_args.find (it->first);
      if (ga != call_args.end())
        {
          custom_args[ga->first] = ga->second;
          call_args.erase (ga);
        }
    }
  env.push_map (custom_args);
  /* construct argument list for ancestor */
  VariableMap ancestor_args (call_args);
  Evaluator::replenish_map (ancestor_args, gadget->ancestor_arguments);
  /* collect unused arguments */
  VariableMap unused_args;
  /* construct gadget from ancestor */
  Item *itemp;
  try {
    itemp = &inherit_gadget (gadget->ancestor, ancestor_args, env, unused_args);
  } catch (std::exception &exc) {
    error (gadget->location() + ": in <def:" + gadget->ident + "/> - failed to inherit: " + exc.what());
    env.pop_map (custom_args);
    throw;
  } catch (...) {
    env.pop_map (custom_args);
    throw;
  }
  Item &item = *itemp;
  /* construct gadget children */
  try {
    ChildContainerSlot outer_ccslot (gadget->child_container);
    call_gadget_children (gadget, item, env, ccslot ? ccslot : &outer_ccslot);
    /* assign specials */
    item.name (name);
    /* setup child container */
    if (!ccslot) /* outer call */
      {
        Container *container = dynamic_cast<Container*> (outer_ccslot.item);
        Container *item_container = dynamic_cast<Container*> (&item);
        if (item_container && !gadget->child_container)
          item_container->child_container (item_container);
        else if (item_container && container)
          item_container->child_container (container);
        else if (gadget->child_container)
          error ("no such child container: "+ gadget->child_container->ident);
      }
    /* add to parent */
    if (parent)
      {
        /* expand pack args from unused property args */
        VariableMap pack_args;
        for (VariableMap::const_iterator it = unused_args.begin(); it != unused_args.end(); it++)
          pack_args[it->first] = env.expand_expression (it->second);
        /* pack into parent */
        VariableMap unused_pack_args;
        parent->add (item, pack_args, &unused_pack_args);
        /* eliminate args which were used during packing */
        VariableMap::iterator it = unused_args.begin();;
        while (it != unused_args.end())
          {
            VariableMap::iterator current = it++;
            if (unused_pack_args.find (current->first) == unused_pack_args.end())
              unused_args.erase (current);
          }
      }
  } catch (std::exception &exc) {
    error (gadget->location() + ": in <def:" + gadget->ident + "/> - construction failed: " + exc.what());
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
  for (VariableMap::const_iterator it = unused_args.begin(); it != unused_args.end(); it++)
    if (gadget->ancestor_arguments.find (it->first) != gadget->ancestor_arguments.end())
      error (gadget->location() + ": in <def:" + gadget->ident + "/> - no such property: " + it->first + "=" + it->second);
    else
      unused_call_args[it->first] = it->second;
  return item;
}

void
FactorySingleton::call_gadget_children (const Gadget       *gadget,
                                        Item               &item,
                                        Evaluator          &env,
                                        ChildContainerSlot *ccslot)
{
  /* guard against leafs */
  if (!gadget->children.size())
    return;
  /* retrieve container */
  Container *container = item.interface<Container*>();
#if 0
  diag ("children of %s:", gadget->ident.c_str());
  uint nth = 0;
  for (Walker<Gadget*const> cw = walker (gadget->children); cw.has_next(); cw++)
    diag ("%d) %s", nth++, (*cw)->ident.c_str());
#endif
  if (!container)
    error ("parent gadget fails to implement Container interface: "+ gadget->ident);
  /* create children */
  for (Walker<Gadget*const> cw = walker (gadget->children); cw.has_next(); cw++)
    {
      /* create child gadget */
      const Gadget *child_gadget = *cw;
      /* the real call arguments are stored as ancestor arguments of the child */
      VariableMap dummy;
      Item &child = call_gadget (child_gadget, dummy, env, ccslot, container, dummy);
      /* find child container */
      if (ccslot->gadget == child_gadget)
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
  Gadget *gadget = new Gadget (base + 1, String ("\177") + itfactory.qualified_type, "<builtin>", INT_MIN);
  fdomain->definitions[gadget->ident] = gadget;
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
    error ("no such item type: " + ident);
}

} // Anon


namespace Rapicorn {

void
Factory::must_parse_file (const String           &relative_file_name,
                          const String           &i18n_domain,
                          const String            altpath1,
                          const String            altpath2)
{
  if (Path::isabs (relative_file_name))
    parse_file (relative_file_name, i18n_domain, i18n_domain);
  else if (altpath1[0] && altpath2[0])
    {
      String p1 = Path::join (altpath1, relative_file_name);
      if (Path::check (p1, "r"))
        parse_file (p1, i18n_domain, i18n_domain);
      else
        parse_file (Path::join (altpath2, relative_file_name), i18n_domain, i18n_domain);
    }
  else if (altpath1[0])
    parse_file (Path::join (altpath1, relative_file_name), i18n_domain, i18n_domain);
  else if (altpath2[0])
    parse_file (Path::join (altpath2, relative_file_name), i18n_domain, i18n_domain);
  else
    error (STRFUNC + String() + ": failed to locate file without pathname: " + relative_file_name);
}

void
Factory::parse_file (const String           &file_name,
                     const String           &i18n_domain,
                     const String           &domain,
                     const std::nothrow_t   &nt)
{
  initialize_standard_gadgets_lazily();
  FactorySingleton::singleton->parse_gadget_file (file_name, i18n_domain, domain, nt);
}

void
Factory::parse_file (const String           &file_name,
                     const String           &i18n_domain,
                     const std::nothrow_t   &nt)
{
  parse_file (file_name, i18n_domain, i18n_domain, nt);
}

Handle<Item>
Factory::create_item (const String       &gadget_identifier,
                      const ArgumentList &arguments)
{
  initialize_standard_gadgets_lazily();
  Item &item = FactorySingleton::singleton->construct_gadget (gadget_identifier, arguments);
  return item.handle<Item>(); // Handle<> does ref_sink()
}

Handle<Container>
Factory::create_container (const String           &gadget_identifier,
                           const ArgumentList     &arguments)
{
  initialize_standard_gadgets_lazily();
  Item &item = FactorySingleton::singleton->construct_gadget (gadget_identifier, arguments);
  return item.handle<Container>(); // Handle<> does ref_sink()
}

Handle<Root>
Factory::create_root (const String           &gadget_identifier,
                      const ArgumentList     &arguments)
{
  initialize_standard_gadgets_lazily();
  Item &item = FactorySingleton::singleton->construct_gadget (gadget_identifier, arguments);
  return item.handle<Root>(); // Handle<> does ref_sink()
}

Window
Factory::create_window (const String           &gadget_identifier,
                        const ArgumentList     &arguments)
{
  initialize_standard_gadgets_lazily();
  Item &item = FactorySingleton::singleton->construct_gadget (gadget_identifier, arguments);
  Root &root = dynamic_cast<Root&> (item);
  Window win = root.window();
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
#include "zintern.c"    // provides RAPICORN_SIZE and RAPICORN_DATA

static void
initialize_standard_gadgets_lazily (void)
{
  AutoLocker al (rapicorn_mutex());
  static bool initialized = false;
  if (!initialized)
    {
      initialized = true;
      uint8 *data = zintern_decompress (RAPICORN_SIZE, RAPICORN_DATA, sizeof (RAPICORN_DATA) / sizeof (RAPICORN_DATA[0]));
      const char *domain = "Rapicorn";
      FactorySingleton::singleton->parse_gadget_data (RAPICORN_SIZE, (const char*) data, domain, domain);
      zintern_free (data);
    }
}

} // Anon
