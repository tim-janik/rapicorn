// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#include "factory.hh"
#include "evaluator.hh"
#include "window.hh"
#include <stdio.h>
#include <stack>
#include <cstring>
#include <algorithm>

#define FDEBUG(...)     RAPICORN_KEY_DEBUG ("Factory", __VA_ARGS__)
#define EDEBUG(...)     RAPICORN_KEY_DEBUG ("Factory-Eval", __VA_ARGS__)

namespace Rapicorn {
namespace Factory {

// == Utilities ==
static String
node_location (const XmlNode *xnode)
{
  return string_format ("%s:%d", xnode->parsed_file().c_str(), xnode->parsed_line());
}

static String
node_location (const XmlNodeP xnode)
{
  return node_location (xnode.get());
}

// == InterfaceFile ==
struct InterfaceFile : public virtual std::enable_shared_from_this<InterfaceFile> {
  const String           file_name;
  const XmlNodeP         root; // <interfaces/>
  Evaluator::VariableMap parser_args; // arguments provided when parsing XML
  explicit               InterfaceFile (const String &f, const XmlNodeP r, const ArgumentList *arguments) :
    file_name (f), root (r), parser_args (variable_map_from_args (arguments))
  {}
  static Evaluator::VariableMap
  variable_map_from_args (const ArgumentList *arguments)
  {
    Evaluator::VariableMap vmap;
    if (arguments)
      Evaluator::populate_map (vmap, *arguments);
    return vmap;
  }
};
typedef std::shared_ptr<InterfaceFile> InterfaceFileP;

static std::vector<InterfaceFileP> interface_file_list;

static String
register_interface_file (String file_name, const XmlNodeP root, const ArgumentList *arguments, StringVector *definitions)
{
  assert_return (file_name.empty() == false, "missing file");
  assert_return (root->name() == "interfaces", "invalid file");
  InterfaceFileP ifile = std::make_shared<InterfaceFile> (file_name, root, arguments);
  const size_t reset_size = definitions ? definitions->size() : 0;
  for (auto dnode : root->children())
    if (dnode->istext() == false)
      {
        const String id = dnode->get_attribute ("id");
        if (id.empty())
          {
            if (definitions)
              definitions->resize (reset_size);
            return string_format ("%s: interface definition without id: <%s/>", node_location (dnode), dnode->name());
          }
        if (definitions)
          definitions->push_back (id);
      }
  interface_file_list.insert (interface_file_list.begin(), ifile);
  FDEBUG ("%s: registering %d interfaces", file_name, root->children().size());
  return "";
}

static const XmlNode*
lookup_interface_node (const String &identifier, InterfaceFileP *ifacepp, const XmlNode *context_node)
{
  for (auto ifile : interface_file_list)
    for (auto node : ifile->root->children())
      {
        const String id = node->get_attribute ("id");
        if (id == identifier)
          {
            if (ifacepp)
              *ifacepp = ifile;
            return node.get();
          }
      }
  return NULL;
}

static bool
is_interface_node (const XmlNode &xnode)
{
  const XmlNode *parent = xnode.parent();
  return parent && !parent->parent() && parent->name() == "interfaces";
}

} // Factory


// == FactoryContext ==
struct FactoryContext {
  const XmlNode *xnode;
  StringSeq     *type_tags;
  String         type;
  StyleIfaceP    style;
  FactoryContext (const XmlNode *xn) : xnode (xn), type_tags (NULL) {}
};
static std::map<const XmlNode*, FactoryContext*> factory_context_map; /// @TODO: Make factory_context_map threadsafe

static void initialize_factory_lazily (void);

namespace Factory {

// == ObjectTypeFactory ==
static std::list<const ObjectTypeFactory*>&
widget_type_list()
{
  static std::list<const ObjectTypeFactory*> *widget_type_factories_p = NULL;
  do_once
    {
      widget_type_factories_p = new std::list<const ObjectTypeFactory*>();
    }
  return *widget_type_factories_p;
}

static const ObjectTypeFactory*
lookup_widget_factory (String namespaced_ident)
{
  std::list<const ObjectTypeFactory*> &widget_type_factories = widget_type_list();
  namespaced_ident = namespaced_ident;
  std::list<const ObjectTypeFactory*>::iterator it;
  for (it = widget_type_factories.begin(); it != widget_type_factories.end(); it++)
    if ((*it)->qualified_type == namespaced_ident)
      return *it;
  return NULL;
}

void
ObjectTypeFactory::register_object_factory (const ObjectTypeFactory &itfactory)
{
  std::list<const ObjectTypeFactory*> &widget_type_factories = widget_type_list();
  const char *ident = itfactory.qualified_type.c_str();
  const char *base = strrchr (ident, ':');
  if (!base || base != ident + 10 - 1 || strncmp (ident, "Rapicorn::", 10) != 0)
    fatal ("ObjectTypeFactory registration with invalid/missing domain name: %s", ident);
  String domain_name;
  domain_name.assign (ident, base - ident - 1);
  widget_type_factories.push_back (&itfactory);
}

ObjectTypeFactory::ObjectTypeFactory (const char *namespaced_ident) :
  qualified_type (namespaced_ident)
{}

ObjectTypeFactory::~ObjectTypeFactory ()
{}

void
ObjectTypeFactory::sanity_check_identifier (const char *namespaced_ident)
{
  if (strncmp (namespaced_ident, "Rapicorn::", 10) != 0)
    fatal ("ObjectTypeFactory: identifier lacks factory qualification: %s", namespaced_ident);
}

// == Public factory_context API ==
String
factory_context_name (FactoryContext &fc)
{
  const XmlNode &xnode = *fc.xnode;
  if (is_interface_node (xnode))
    return xnode.get_attribute ("id");
  else
    return xnode.name();
}

String
factory_context_type (FactoryContext &fc)
{
  const XmlNode *xnode = fc.xnode;
  if (!is_interface_node (*xnode)) // lookup definition node from child node
    {
      xnode = lookup_interface_node (xnode->name(), NULL, xnode);
      assert_return (xnode != NULL, "");
    }
  assert_return (is_interface_node (*xnode), "");
  return xnode->get_attribute ("id");
}

UserSource
factory_context_source (FactoryContext &fc)
{
  const XmlNode *xnode = fc.xnode;
  if (!is_interface_node (*xnode)) // lookup definition node from child node
    {
      xnode = lookup_interface_node (xnode->name(), NULL, xnode);
      assert_return (xnode != NULL, UserSource (""));
    }
  assert_return (is_interface_node (*xnode), UserSource (""));
  return UserSource ("WidgetFactory", xnode->parsed_file(), xnode->parsed_line());
}

StyleIfaceP
factory_context_style (FactoryContext &fc)
{
  if (!fc.style)
    {
      const XmlNode *xnode = fc.xnode;
      if (!is_interface_node (*xnode)) // lookup definition node from child node
        {
          xnode = lookup_interface_node (xnode->name(), NULL, xnode);
          assert_return (xnode && is_interface_node (*xnode), fc.style);
        }
      String factory_svg;
      while (xnode)
        {
          const XmlNode *inode = xnode->parent(); // <interfaces/>
          assert_return (inode && inode->name() == "interfaces", fc.style);
          factory_svg = inode->get_attribute ("factory-svg");
          if (!factory_svg.empty())
            break;
          const String parent_name = xnode->name();
          xnode = lookup_interface_node (parent_name, NULL, xnode);
        }
      fc.style = factory_svg.empty() ? StyleIface::fallback() : StyleIface::load (factory_svg);
    }
  return fc.style;
}

static void
factory_context_list_types (StringVector &types, const XmlNode *xnode, const bool need_ids, const bool need_variants)
{
  assert_return (xnode != NULL);
  if (!is_interface_node (*xnode)) // lookup definition node from child node
    {
      xnode = lookup_interface_node (xnode->name(), NULL, xnode);
      assert_return (xnode != NULL);
    }
  while (xnode)
    {
      assert_return (is_interface_node (*xnode));
      if (need_ids)
        types.push_back (xnode->get_attribute ("id"));
      const String parent_name = xnode->name();
      const XmlNode *last = xnode;
      xnode = lookup_interface_node (parent_name, NULL, xnode);
      if (!xnode && last->name() == "Rapicorn_Factory")
        {
          const StringVector &attributes_names = last->list_attributes(), &attributes_values = last->list_values();
          const ObjectTypeFactory *widget_factory = NULL;
          for (size_t i = 0; i < attributes_names.size(); i++)
            if (attributes_names[i] == "factory-type" || attributes_names[i] == "factory_type")
              {
                widget_factory = lookup_widget_factory (attributes_values[i]);
                break;
              }
          assert_return (widget_factory != NULL);
          types.push_back (widget_factory->type_name());
          std::vector<const char*> variants;
          widget_factory->type_name_list (variants);
          for (auto n : variants)
            if (n)
              types.push_back (n);
        }
    }
}

static void
factory_context_update_cache (FactoryContext &fc)
{
  if (UNLIKELY (!fc.type_tags))
    {
      const XmlNode *xnode = fc.xnode;
      fc.type_tags = new StringSeq;
      StringVector &types = *fc.type_tags;
      factory_context_list_types (types, xnode, false, false);
      fc.type = types.size() ? types[types.size() - 1] : "";
      types.clear();
      factory_context_list_types (types, xnode, true, true);
    }
}

const StringSeq&
factory_context_tags (FactoryContext &fc)
{
  factory_context_update_cache (fc);
  return *fc.type_tags;
}

String
factory_context_impl_type (FactoryContext &fc)
{
  factory_context_update_cache (fc);
  return fc.type;
}

// == Builder ==
class Builder {
  enum Flags { SCOPE_WIDGET = 1, SCOPE_CHILD = 2 };
  InterfaceFileP   interface_file_;             // InterfaceFile for dnode_
  const XmlNode   *const dnode_;                // definition of gadget to be created
  Builder         *const outer_;
  String           child_container_name_;
  ContainerImpl   *child_container_;            // captured child_container_ widget during build phase
  StringVector     scope_names_, scope_values_;
  vector<bool>     scope_consumed_;
  VariableMap      locals_;
  void          eval_args        (Evaluator &env, const StringVector &in_names, const StringVector &in_values, const XmlNode *errnode,
                                  StringVector &out_names, StringVector &out_values, String *child_container_name, const Flags bflags);
  bool          try_set_property (WidgetImpl &widget, const String &property_name, const String &value);
  WidgetImplP   build_scope      (const String &caller_location, const XmlNode *factory_context_node);
  WidgetImplP   build_widget     (const XmlNode *node, Evaluator &env, const XmlNode *factory_context_node, Flags bflags);
  static String canonify_dashes  (const String &key);
public:
  explicit           Builder            (Builder *outer_builder, const String &widget_identifier, const XmlNode *context_node);
  static WidgetImplP eval_and_build     (const String &widget_identifier,
                                         const StringVector &call_names, const StringVector &call_values, const String &call_location);
  static WidgetImplP build_from_factory (const XmlNode *factory_node,
                                         const StringVector &attr_names, const StringVector &attr_values,
                                         const XmlNode *factory_context_node);
  static bool widget_has_ancestor (const String &widget_identifier, const String &ancestor_identifier);
};

Builder::Builder (Builder *outer_builder, const String &widget_identifier, const XmlNode *context_node) :
  interface_file_ (NULL), dnode_ (lookup_interface_node (widget_identifier, &interface_file_, context_node)),
  outer_ (outer_builder), child_container_ (NULL)
{
  if (!dnode_)
    return;
}

String
Builder::canonify_dashes (const String &key)
{
  String s = key;
  for (uint i = 0; i < s.size(); i++)
    if (key[i] == '-')
      s[i] = '_'; // unshares string
  return s;
}

static bool
is_property (const XmlNode &xnode, String *element = NULL, String *attribute = NULL)
{
  // parse property element syntax, e.g. <Button.label>...</Button.label>
  const String &pname = xnode.name();
  const size_t i = pname.rfind (".");
  if (i == String::npos || i + 1 >= pname.size())
    return false;                                       // no match
  if (element)
    *element = pname.substr (0, i);
  if (attribute)
    *attribute = pname.substr (i + 1);
  return true;                                          // matched element.attribute
}

WidgetImplP
Builder::eval_and_build (const String &widget_identifier,
                         const StringVector &call_names, const StringVector &call_values, const String &call_location)
{
  assert_return (call_names.size() == call_values.size(), NULL);
  // initialize and check builder
  initialize_factory_lazily();
  Builder builder (NULL, widget_identifier, NULL);
  if (!builder.dnode_)
    {
      critical ("%s: unknown type identifier: %s", call_location, widget_identifier);
      return NULL;
    }
  // evaluate call arguments
  Evaluator env;
  for (size_t i = 0; i < call_names.size(); i++)
    {
      const String &cname = call_names[i];
      String cvalue = call_values[i];
      if (cname.find (':') == String::npos && // never eval namespaced attributes
          string_startswith (cvalue, "@eval "))
        cvalue = env.parse_eval (cvalue.substr (6));
      builder.scope_names_.push_back (cname);
      builder.scope_values_.push_back (cvalue);
    }
  builder.scope_consumed_.resize (builder.scope_names_.size());
  // build widget
  WidgetImplP widget = builder.build_scope (call_location, builder.dnode_);
  FDEBUG ("%s: built widget '%s': %s", node_location (builder.dnode_), widget_identifier, widget ? widget->name() : "<null>");
  return widget;
}

WidgetImplP
Builder::build_from_factory (const XmlNode *factory_node,
                             const StringVector &attr_names, const StringVector &attr_values,
                             const XmlNode *factory_context_node)
{
  assert_return (factory_context_node != NULL, NULL);
  // extract arguments
  String factory_id, factory_type;
  for (size_t i = 0; i < attr_names.size(); i++)
    if (attr_names[i] == "id")
      factory_id = attr_values[i];
    else if (attr_names[i] == "factory_type")
      factory_type = attr_values[i];
  // lookup widget factory
  const ObjectTypeFactory *widget_factory = lookup_widget_factory (factory_type);
  // sanity check factory node
  if (factory_node->name() != "Rapicorn_Factory" || !widget_factory ||
      attr_names.size() != 2 || factory_node->list_attributes().size() != 2 ||
      factory_id.empty() || factory_id[0] == '@' || factory_type.empty() || factory_type[0] == '@' ||
      factory_node->children().size() != 0)
    {
      critical ("%s: invalid factory type: %s", node_location (factory_node), factory_node->name());
      return NULL;
    }
  // build factory widget
  FactoryContext *fc = factory_context_map[factory_context_node];
  if (!fc)
    {
      fc = new FactoryContext (factory_context_node);
      factory_context_map[factory_context_node] = fc;
    }
  ObjectImplP object = widget_factory->create_object (*fc);
  WidgetImplP widget;
  if (object)
    {
      widget = shared_ptr_cast<WidgetImpl> (object);
      if (widget)
        widget->name (factory_id);
      else
        critical ("%s: %s yields non-widget: %s", node_location (factory_node), factory_node->name(), object->typeid_name());
    }
  return widget;
}

void
Builder::eval_args (Evaluator &env, const StringVector &in_names, const StringVector &in_values, const XmlNode *errnode,
                    StringVector &out_names, StringVector &out_values, String *child_container_name, const Flags bflags)
{
  const bool dissallow_id = bflags & SCOPE_CHILD;
  out_names.reserve (in_names.size());
  out_values.reserve (in_values.size());
  for (size_t i = 0; i < in_names.size(); i++)
    {
      const String cname = canonify_dashes (in_names[i]);
      const String &ivalue = in_values[i];
      String rvalue;
      if (string_startswith (ivalue, "@eval "))
        {
          rvalue = env.parse_eval (ivalue.substr (6));
          EDEBUG ("%s: eval %s=\"%s\": %s", String (errnode ? node_location (errnode) : "Rapicorn:Factory"),
                  in_names[i].c_str(), ivalue.c_str(), rvalue.c_str());
        }
      else
        rvalue = ivalue;
      if (child_container_name && cname == "child_container")
        *child_container_name = rvalue;
      else if (dissallow_id && cname == "id" && errnode)
        critical ("%s: invalid 'id' attribute for inner node: <%s/>", node_location (errnode), errnode->name());
      else
        {
          out_names.push_back (cname);
          out_values.push_back (rvalue);
        }
    }
}

bool
Builder::try_set_property (WidgetImpl &widget, const String &property_name, const String &value)
{
  if (value[0] == '@')
    {
      if (strncmp (value.data(), "@bind ", 6) == 0) /// @TODO: FIXME: implement proper @-string parsing
        {
          widget.add_binding (property_name, value.data() + 6);
          return true;
        }
    }
  return widget.try_set_property (property_name, value);
}

WidgetImplP
Builder::build_scope (const String &caller_location, const XmlNode *factory_context_node)
{
  assert_return (dnode_ != NULL, NULL);
  assert_return (factory_context_node != NULL, NULL);
  // create environment for @eval
  Evaluator env;
  String name_argument; // target name for this scope widget
  // extract <Argument/> defaults from definition
  StringVector argument_names, argument_values;
  for (const XmlNodeP cnode : dnode_->children())
    if (cnode->name() == "Argument")
      {
        const String aname = canonify_dashes (cnode->get_attribute ("name")); // canonify argument name
        if (aname.empty() || aname == "id" || aname == "name")
          critical ("%s: %s argument name: \"%s\"",
                    node_location (cnode),
                    cnode->has_attribute ("name") ? "invalid" : "missing",
                    aname);
        else
          {
            argument_names.push_back (aname);
            String avalue = cnode->get_attribute ("default");
            if (string_startswith (avalue, "@eval "))
              avalue = env.parse_eval (avalue.substr (6));
            argument_values.push_back (avalue);
          }
      }
  // assign Argument values from caller args, remaining values are properties (or 'inherited' arguments)
  for (size_t i = 0; i < scope_names_.size(); i++)
    {
      if (scope_consumed_.at (i))
        continue;
      const String &rawname = scope_names_[i], &cvalue = scope_values_.at (i);
      if (rawname == "name" || rawname == "id")
        {
          name_argument = cvalue;
          scope_consumed_[i] = true;
          continue;
        }
      else if (rawname.find (':') != String::npos)
        {
          scope_consumed_[i] = true;
          continue; // ignore namespaced attributes
        }
      const String cname = canonify_dashes (rawname);
      StringVector::const_iterator it = find (argument_names.begin(), argument_names.end(), cname);
      if (it != argument_names.end())
        {
          const size_t index = it - argument_names.begin();
          argument_values[index] = cvalue;
          scope_consumed_[i] = true;
        }
    }
  // allow outer scopes to override Argument values
  for (Builder *outer = this->outer_; outer; outer = outer->outer_)
    for (size_t i = 0; i < outer->scope_names_.size(); i++)
      if (!outer->scope_consumed_.at (i) && outer->scope_names_[i] != "id" && outer->scope_names_[i] != "name" &&
          outer->scope_names_[i].find (':') == String::npos) // ignore namespaced attributes
        {
          const String outer_cname = canonify_dashes (outer->scope_names_[i]);
          StringVector::const_iterator it = find (argument_names.begin(), argument_names.end(), outer_cname);
          if (it != argument_names.end())
            { // outer Argument values override previous/inner values
              argument_values[it - argument_names.begin()] = outer->scope_values_.at (i);
              outer->scope_consumed_[i] = true;
            }
        }
  // use Argument values to prepare variable map for evaluator
  Evaluator::populate_map (locals_, argument_names, argument_values);
  argument_names.clear(), argument_values.clear();
  // build main definition widget
  env.push_map (interface_file_->parser_args);
  env.push_map (locals_);
  WidgetImplP widget = build_widget (dnode_, env, factory_context_node, SCOPE_WIDGET);
  env.pop_map (locals_);
  env.pop_map (interface_file_->parser_args);
  if (!widget)
    return NULL;
  // assign caller properties ('consumed' values have been used as Argument values)
  if (!name_argument.empty())
    widget->name (name_argument);
  for (size_t i = 0; i < scope_names_.size(); i++)
    if (!scope_consumed_.at (i))
      {
        const String &cname = scope_names_[i], &cvalue = scope_values_.at (i);
        if (try_set_property (*widget, cname, cvalue))
          scope_consumed_[i] = true;
        else
          critical ("%s: widget %s: unknown property: %s", caller_location, widget->name(), cname);
      }
  // assign child container
  if (child_container_)
    {
      ContainerImpl *container = widget->as_container_impl();
      if (!container)
        critical ("%s: invalid type for child container: %s", node_location (dnode_), dnode_->name());
      else
        container->child_container (child_container_);
    }
  else if (!child_container_name_.empty())
    critical ("%s: failed to find child container: %s", node_location (dnode_), child_container_name_);
  return widget;
}

WidgetImplP
Builder::build_widget (const XmlNode *const wnode, Evaluator &env, const XmlNode *const factory_context_node, const Flags bflags)
{
  const bool skip_argument_child = bflags & SCOPE_WIDGET;
  const bool filter_child_container = bflags & SCOPE_WIDGET;
  // collect properties from XML attributes
  StringVector prop_names = wnode->list_attributes(), prop_values = wnode->list_values();
  // collect properties from XML property element syntax
  const size_t chsize = std::max (size_t (1), wnode->children().size());
  bool skip_child[chsize];
  std::fill (skip_child, skip_child + chsize, 0);
  size_t ski = 0; // skip child index
  for (const XmlNodeP cnode : wnode->children())
    {
      String prop_object, pname;
      if (is_property (*cnode, &prop_object, &pname) && wnode->name() == prop_object)
        {
          String eval_element = cnode->get_attribute ("eval-element");
          if (eval_element.empty())
            eval_element = cnode->get_attribute ("eval_element");
          std::function<String (const XmlNode&, size_t, bool, size_t)> node_wrapper =
            [&] (const XmlNode &node, size_t indent, bool include_outer, size_t recursion_depth) -> String {
            if (node.name() == eval_element)
              {
                String node_text = node.xml_string (0, false);
                if (string_startswith (node_text, "@eval "))
                  node_text = env.parse_eval (node_text.substr (6));
                return node_text;
              }
            return node.xml_string (indent, include_outer, recursion_depth, node_wrapper, false);
          };
          const String xml2string = cnode->xml_string (0, false, -1, eval_element.empty() ? NULL : node_wrapper, true);
          pname = canonify_dashes (pname);
          prop_names.push_back (pname);
          prop_values.push_back (xml2string);
          skip_child[ski++] = true;
        }
      else if (cnode->istext() || (skip_argument_child && cnode->name() == "Argument"))
        skip_child[ski++] = true;
      else
        skip_child[ski++] = false;
    }
  assert_return (ski == wnode->children().size(), NULL);
  // evaluate property values
  StringVector eprop_names, eprop_values;
  eval_args (env, prop_names, prop_values, wnode, eprop_names, eprop_values, filter_child_container ? &child_container_name_ : NULL, bflags);
  // create widget and assign properties from attributes and property element syntax
  WidgetImplP widget;
  {
    Builder inner_builder (this, wnode->name(), NULL);
    if (inner_builder.dnode_)
      {
        inner_builder.scope_names_ = std::move (eprop_names);
        inner_builder.scope_values_ = std::move (eprop_values);
        inner_builder.scope_consumed_.resize (inner_builder.scope_names_.size());
        widget = inner_builder.build_scope (node_location (wnode), factory_context_node);
      }
    else
      widget = build_from_factory (wnode, eprop_names, eprop_values, factory_context_node);
  }
  if (!widget)
    {
      critical ("%s: failed to create widget: %s", node_location (wnode), wnode->name());
      return NULL;
    }
  ContainerImpl *container = NULL;
  // find child container within scope
  if (!child_container_name_.empty() && child_container_name_ == widget->name())
    {
      ContainerImpl *cc = widget->as_container_impl();
      if (cc)
        {
          if (child_container_)
            critical ("%s: duplicate child container: %s", node_location (dnode_), node_location (wnode));
          else
            child_container_ = cc;
        }
      else
        critical ("%s: invalid child container type: %s", node_location (dnode_), node_location (wnode));
    }
  // create and add children
  ski = 0;
  for (const XmlNodeP cnode : wnode->children())
    if (skip_child[ski++])
      continue;
    else
      {
        if (!container)
          container = widget->as_container_impl();
        if (!container)
          {
            critical ("%s: invalid container type: %s", node_location (wnode), wnode->name());
            break;
          }
        WidgetImplP child = build_widget (&*cnode, env, &*cnode, SCOPE_CHILD);
        if (child)
          try {
            // be verbose...
            FDEBUG ("%s: built child '%s': %s", node_location (cnode), cnode->name(), child ? child->name() : "<null>");
            container->add (*child);
          } catch (std::exception &exc) {
            critical ("%s: adding %s to parent failed: %s", node_location (cnode), cnode->name(), exc.what());
          }
      }
  return widget;
}

bool
Builder::widget_has_ancestor (const String &widget_identifier, const String &ancestor_identifier)
{
  initialize_factory_lazily();
  const XmlNode *const ancestor_node = lookup_interface_node (ancestor_identifier, NULL, NULL); // may yield NULL
  const ObjectTypeFactory *const ancestor_itfactory = ancestor_node ? NULL : lookup_widget_factory (ancestor_identifier);
  if (widget_identifier == ancestor_identifier && (ancestor_node || ancestor_itfactory))
    return true; // potential fast path
  if (!ancestor_node && !ancestor_itfactory)
    return false; // ancestor_identifier is non-existent
  String identifier = widget_identifier;
  const XmlNode *node = lookup_interface_node (identifier, NULL, NULL), *last = node;
  while (node)
    {
      if (node == ancestor_node)
        return true; // widget ancestor matches
      identifier = node->name();
      last = node;
      node = lookup_interface_node (identifier, NULL, node);
    }
  if (ancestor_node)
    return false; // no node match possible
  if (last && last->name() == "Rapicorn_Factory")
    {
      const StringVector &attributes_names = last->list_attributes(), &attributes_values = last->list_values();
      const ObjectTypeFactory *widget_factory = NULL;
      for (size_t i = 0; i < attributes_names.size(); i++)
        if (attributes_names[i] == "factory-type" || attributes_names[i] == "factory_type")
          {
            widget_factory = lookup_widget_factory (attributes_values[i]);
            break;
          }
      return widget_factory && widget_factory == ancestor_itfactory;
    }
  return false;
}

// == UI Creation API ==
bool
check_ui_window (const String &widget_identifier)
{
  return Builder::widget_has_ancestor (widget_identifier, "Rapicorn::Window");
}

WidgetImplP
create_ui_widget (const String &widget_identifier, const ArgumentList &arguments)
{
  const String call_location = string_format ("Factory::create_ui_widget(%s)", widget_identifier);
  StringVector anames, avalues;
  for (size_t i = 0; i < arguments.size(); i++)
    {
      const String &arg = arguments[i];
      const size_t pos = arg.find ('=');
      if (pos != String::npos)
        anames.push_back (arg.substr (0, pos)), avalues.push_back (arg.substr (pos + 1));
      else
        FDEBUG ("%s: argument without value: %s", call_location, arg);
    }
  WidgetImplP widget = Builder::eval_and_build (widget_identifier, anames, avalues, call_location);
  if (!widget)
    critical ("%s: failed to create widget: %s", call_location, widget_identifier);
  return widget;
}

WidgetImplP
create_ui_child (ContainerImpl &container, const String &widget_identifier, const ArgumentList &arguments, bool autoadd)
{
  // figure XML context
  FactoryContext &fc = container.factory_context();
  assert (NULL != &fc);
  // create child within parent namespace
  //local_namespace_list.push_back (namespace_domain);
  WidgetImplP widget = create_ui_widget (widget_identifier, arguments);
  //local_namespace_list.pop_back();
  // add to parent
  if (autoadd)
    container.add (*widget);
  return widget;
}

// == XML Parsing and Registration ==
static String
parse_ui_data_internal (const String &data_name, size_t data_length, const char *data, const String &i18n_domain,
                        const ArgumentList *arguments, StringVector *definitions)
{
  String pseudoroot; // automatically wrap definitions into root tag <interfaces/>
  const size_t estart = MarkupParser::seek_to_element (data, data_length);
  if (estart + 11 < data_length && strncmp (data + estart, "<interfaces", 11) != 0 && data[estart + 1] != '?')
    pseudoroot = "interfaces";
  MarkupParser::Error perror;
  XmlNodeP xnode = XmlNode::parse_xml (data_name, data, data_length, &perror, pseudoroot);
  String errstr;
  if (perror.code)
    errstr = string_format ("%s:%d:%d: %s", data_name.c_str(), perror.line_number, perror.char_number, perror.message.c_str());
  else
    {
      errstr = register_interface_file (data_name, xnode, arguments, definitions);
    }
  return errstr;
}

String
parse_ui_data (const String &data_name, size_t data_length, const char *data, const String &i18n_domain,
               const ArgumentList *arguments, StringVector *definitions)
{
  initialize_factory_lazily();
  return parse_ui_data_internal (data_name, data_length, data, i18n_domain, arguments, definitions);
}

} // Factory

static void
initialize_factory_lazily (void)
{
  static bool initialized = false;
  if (!initialized)
    {
      initialized++;
      Blob blob;
      String err;
      blob = Res ("@res Rapicorn/foundation.xml");
      err = Factory::parse_ui_data_internal (blob.name(), blob.size(), blob.data(), "", NULL, NULL);
      if (!err.empty())
        user_warning (UserSource ("Factory"), "failed to load '%s': %s", blob.name(), err);
      blob = Res ("@res Rapicorn/standard.xml");
      err = Factory::parse_ui_data_internal (blob.name(), blob.size(), blob.data(), "", NULL, NULL);
      if (!err.empty())
        user_warning (UserSource ("Factory"), "failed to load '%s': %s", blob.name(), err);
      blob = Res ("@res themes/Default.xml");
      err = Factory::parse_ui_data_internal (blob.name(), blob.size(), blob.data(), "", NULL, NULL);
      if (!err.empty())
        user_warning (UserSource ("Factory"), "failed to load '%s': %s", blob.name(), err);
    }
}

} // Rapicorn
