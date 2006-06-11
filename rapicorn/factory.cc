/* Rapicorn
 * Copyright (C) 2002-2006 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#include "factory.hh"
#include "birnetmarkup.hh"
#include "root.hh"
#include <stack>
#include <errno.h>
using namespace std;

namespace { // Anon
using Birnet::uint;
using namespace Rapicorn;
using namespace Rapicorn::Factory;


/* --- Gadget definition --- */
struct Gadget {
  const String    ident, ancestor;
  const Gadget   *child_container;
  VariableMap     ancestor_arguments;
  vector<Gadget*> children;
  void            add_child (Gadget *child_gadget) { children.push_back (child_gadget); }
  explicit        Gadget    (const String      &cident,
                             const String      &cancestor,
                             const VariableMap &cancestor_arguments = VariableMap()) :
    ident (cident),
    ancestor (cancestor),
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

/* --- Environment --- */
struct Environment {
  static String
  expand_value (const String      &value,
                const VariableMap &vmap = VariableMap())
  {
    return value;
  }
};

/* --- FactorySingleton --- */
class FactorySingleton {
  typedef std::pair<String,String>      ArgumentPair;
  typedef vector<ArgumentPair>          ArgumentVector;
  typedef VariableMap::const_iterator   ConstVariableIter;
  /* ChildContainerSlot - return value slot for the current gadget's "child-container" */
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
                                                         const VariableMap  &expanded_arguments,
                                                         Environment        &environment);
  Item&                         call_gadget             (const Gadget       *gadget,
                                                         const VariableMap  &canonified_arguments,
                                                         const VariableMap  &fallback_variables,
                                                         Environment        &environment,
                                                         ChildContainerSlot *ccslot,
                                                         Container          *parent);
  void                          call_gadget_children    (const Gadget       *gadget,
                                                         Item               &item,
                                                         const VariableMap  &parent_arguments,
                                                         Environment        &environment,
                                                         ChildContainerSlot *ccslot);
  /* type registration */
  std::list<const ItemTypeFactory*> types;
  const ItemTypeFactory*        lookup_item_factory     (const String          &namespaced_ident);
  Item&                         create_from_item_type   (const String          &ident);
public:
  void                          register_item_factory   (const ItemTypeFactory &itfactory);
  void                          parse_gadget_file       (const String          &file_name,
                                                         const String          &i18n_domain,
                                                         const String          &domain,
                                                         const std::nothrow_t  &nt);
  
  Item&                         construct_gadget        (const String          &gadget_identifier,
                                                         const ArgumentList    &arguments);
  /* singleton definition & initialization */
  static FactorySingleton      *singleton;
  explicit                      FactorySingleton        ()
  {
    if (singleton)
      throw Exception (STRFUNC, ": non-singleton initialization");
    else
      singleton = this;
    /* register backlog */
    ItemTypeFactory::initialize_factories();
  }
};
FactorySingleton *FactorySingleton::singleton = NULL;
static FactorySingleton static_factory_singleton;

/* --- attribute canonification --- */
static inline String
canonify (String s)
{
  for (uint i = 0; i < s.size(); i++)
    if (!((s[i] >= 'A' && s[i] <= 'Z') ||
          (s[i] >= 'a' && s[i] <= 'z') ||
          (s[i] >= '0' && s[i] <= '9') ||
          s[i] == '-'))
      s[i] = '-';
  return s;
}

static inline String
rewrite_canonify_attribute (String s)
{
  if (s == "id")
    return "name";
  else
    return canonify (s);
}

/* --- GadgetParser --- */
struct GadgetParser : public MarkupParser {
  FactoryDomain &fdomain;
  std::stack<Gadget*> gadget_stack;
  const Gadget      **child_container_loc;
  String              child_container_name;
public:
  GadgetParser (FactoryDomain &cfdomain) :
    fdomain (cfdomain), child_container_loc (NULL)
  {}
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
        Gadget *gadget = fdomain.definitions[ident];
        if (!gadget)
          {
            VariableMap vmap;
            String inherit;
            child_container_name = "";
            for (uint i = 0; i < attribute_names.size(); i++)
              {
                String canonified_attribute = rewrite_canonify_attribute (attribute_names[i]);
                if (canonified_attribute == "name")
                  error.set (INVALID_CONTENT,
                             String() + "invalid argument for inherited gadget: " +
                             attribute_names[i] + "=\"" + attribute_values[i] + "\"");
                else if (canonified_attribute == "child-container")
                  child_container_name = attribute_values[i];
                else if (canonified_attribute == "inherit")
                  inherit = attribute_values[i];
                else
                  vmap[canonified_attribute] = attribute_values[i];
              }
            if (error.set())
              ;
            else if (inherit[0] == 0)
              error.set (INVALID_CONTENT, String() + "missing ancestor for gadget \"" + ident + "\"");
            else
              {
                gadget = new Gadget (ident, inherit, vmap);
                if (child_container_name[0])
                  child_container_loc = &gadget->child_container;
                fdomain.definitions[ident] = gadget;
                gadget_stack.push (gadget);
              }
          }
        else
          error.set (INVALID_CONTENT, String() + "gadget \"" + ident + "\" already defined");
      }
    else if (!gadget_stack.empty() && canonify (element_name) == element_name)
      {
        Gadget *gparent = gadget_stack.top();
        String gadget_name = element_name;
        VariableMap vmap;
        for (uint i = 0; i < attribute_names.size(); i++)
          {
            String canonified_attribute = rewrite_canonify_attribute (attribute_names[i]);
            if (canonified_attribute == "name")
              gadget_name = attribute_values[i];
            else if (canonified_attribute == "child-container" ||
                     canonified_attribute == "inherit")
              error.set (INVALID_CONTENT,
                         String() + "invalid argument for gadget construction: " +
                         attribute_names[i] + "=\"" + attribute_values[i] + "\"");
            else
              vmap[canonified_attribute] = attribute_values[i];
          }
        Gadget *gadget = new Gadget (gadget_name, element_name, vmap);
        gparent->add_child (gadget);
        gadget_stack.push (gadget);
        if (child_container_loc && child_container_name == gadget_name)
          *child_container_loc = gadget;
      }
    else
      warning ("ignoring unknown element: " + element_name);
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
  factory_domains.push_back (fdomain);
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
  GadgetParser gp (*fdomain);
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
    throw Exception ("no such gadget: ", gadget_identifier);
  VariableMap vmap;
  for (ArgumentList::const_iterator it = arguments.begin(); it != arguments.end(); it++)
    {
      const char *key_value = it->c_str();
      const char *equal = strchr (key_value, '=');
      if (!equal || equal <= key_value)
        throw Exception ("Invalid argument=value pair: ", *it);
      String key = it->substr (0, equal - key_value);
      vmap[rewrite_canonify_attribute (key)] = equal + 1;
    }
  Environment empty_env;
  return call_gadget (gadget, vmap, VariableMap(), empty_env, NULL, NULL);
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
FactorySingleton::inherit_gadget (const String      &ancestor_name,
                                  const VariableMap &expanded_arguments,
                                  Environment       &environment)
{
  if (ancestor_name[0] == '\177')      /* item factory type */
    {
      Item &item = create_from_item_type (&ancestor_name[1]);
      /* apply arguments */
      try {
        for (ConstVariableIter it = expanded_arguments.begin(); it != expanded_arguments.end(); it++)
          item.set_property (it->first, it->second, nothrow);
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
        throw Exception ("no such gadget: ", ancestor_name);
      return call_gadget (gadget, expanded_arguments, VariableMap(), environment, NULL, NULL);
    }
}

Item&
FactorySingleton::call_gadget (const Gadget       *gadget,
                               const VariableMap  &canonified_arguments,
                               const VariableMap  &fallback_variables,
                               Environment        &environment,
                               ChildContainerSlot *ccslot,
                               Container          *parent)
{
  /* extend environament */
  // FIXME: add gadget->env_variables (not overriding existing vars)
  /* expand call arguments */
  VariableMap expanded_arguments;
  for (ConstVariableIter it = canonified_arguments.begin(); it != canonified_arguments.end(); it++)
    expanded_arguments[it->first] = environment.expand_value (it->second, fallback_variables);
  /* filter special arguments */
  String name = gadget->ident;
  name = variable_map_filter (expanded_arguments, "name", name);
  /* ignore nonsense arguments */
  variable_map_filter (expanded_arguments, "child-container");
  variable_map_filter (expanded_arguments, "inherit");
  /* construct argument list for ancestor */
  VariableMap ancestor_arguments;
  for (ConstVariableIter it = gadget->ancestor_arguments.begin(); it != gadget->ancestor_arguments.end(); it++)
    {
      VariableMap::iterator xt = expanded_arguments.find (it->first);
      if (xt != expanded_arguments.end())
        continue;       /* skip overridden ancestor arguments */
      ancestor_arguments[it->first] = environment.expand_value (it->second, expanded_arguments);
    }
  /* override with call arguments */
  for (ConstVariableIter it = expanded_arguments.begin(); it != expanded_arguments.end(); it++)
    ancestor_arguments[it->first] = it->second;
  /* construct gadget from ancestor */
  Item &item = inherit_gadget (gadget->ancestor, ancestor_arguments, environment);
  /* construct gadget children */
  try {
    ChildContainerSlot outer_ccslot (gadget->child_container);
    call_gadget_children (gadget, item, expanded_arguments, environment, ccslot ? ccslot : &outer_ccslot);
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
          throw Exception ("no such child container: ", gadget->child_container->ident);
      }
    /* add to parent */
    if (parent)
      parent->add (item, ancestor_arguments);
  } catch (...) {
    sink (item);
    throw;
  }
  return item;
}

void
FactorySingleton::call_gadget_children (const Gadget       *gadget,
                                        Item               &item,
                                        const VariableMap  &parent_arguments,
                                        Environment        &environment,
                                        ChildContainerSlot *ccslot)
{
  /* protect leafs */
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
    throw Exception ("parent gadgets must implement Container interface: ", gadget->ident);
  /* create children */
  for (Walker<Gadget*const> cw = walker (gadget->children); cw.has_next(); cw++)
    {
      /* create child gadget */
      Gadget *child_gadget = *cw;
      /* the real call arguments are stored as ancestor arguments of the child */
      VariableMap expanded_arguments;
      Item &child = call_gadget (child_gadget, expanded_arguments, parent_arguments, environment, ccslot, container);
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
    throw Exception ("invalid/missing domain name in item type: ", ident);
  String domain_name;
  domain_name.assign (ident, base - ident - 1);
  FactoryDomain *fdomain = lookup_domain (domain_name);
  if (!fdomain)
    fdomain = add_domain (domain_name, domain_name);
  Gadget *gadget = new Gadget (base + 1, String ("\177") + itfactory.qualified_type);
  fdomain->definitions[gadget->ident] = gadget;
  types.push_back (&itfactory);
}

const ItemTypeFactory*
FactorySingleton::lookup_item_factory (const String &namespaced_ident)
{
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
    throw Exception ("no such item type: ", ident);
}

} // Anon


namespace Rapicorn {

void
Factory::parse_file (const String           &file_name,
                     const String           &i18n_domain,
                     const String           &domain,
                     const std::nothrow_t   &nt)
{
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
  Item &item = FactorySingleton::singleton->construct_gadget (gadget_identifier, arguments);
  return item.handle<Item>(); // Handle<> does ref_sink()
}

Handle<Root>
Factory::create_root (const String           &gadget_identifier,
                      const ArgumentList     &arguments)
{
  Item &item = FactorySingleton::singleton->construct_gadget (gadget_identifier, arguments);
  return item.handle<Root>(); // Handle<> does ref_sink()
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

} // Rapicorn
