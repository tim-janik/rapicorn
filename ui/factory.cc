// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
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

struct FactoryContext {
  const XmlNode *xnode;
  StringSeq     *type_tags;
  String         type;
  FactoryContext (const XmlNode *xn) : xnode (xn), type_tags (NULL) {}
};
static std::map<const XmlNode*, FactoryContext*> factory_context_map; /// @TODO: Make factory_context_map threadsafe

static void initialize_factory_lazily (void);

namespace Factory {

class NodeData {
  void
  setup (XmlNode &xnode)
  {
    const StringVector &attributes_names = xnode.list_attributes(); // &attributes_values = xnode.list_values();
    for (size_t i = 0; i < attributes_names.size(); i++)
      if (attributes_names[i] == "tmpl:presuppose")
        tmpl_presuppose = true;
  }
  static struct NodeDataKey : public DataKey<NodeData*> {
    virtual void destroy (NodeData *data) { delete data; }
  } node_data_key;
public:
  bool tmpl_presuppose;
  String domain;
  NodeData (XmlNode &xnode) : tmpl_presuppose (false) { setup (xnode); }
  static NodeData&
  from_xml_node (XmlNode &xnode)
  {
    NodeData *ndata = xnode.get_data (&node_data_key);
    if (!ndata)
      {
        ndata = new NodeData (xnode);
        xnode.set_data (&node_data_key, ndata);
      }
    return *ndata;
  }
};

NodeData::NodeDataKey NodeData::node_data_key;

static const NodeData&
xml_node_data (const XmlNode &xnode)
{
  return NodeData::from_xml_node (*const_cast<XmlNode*> (&xnode));
}

static std::list<const WidgetTypeFactory*>&
widget_type_list()
{
  static std::list<const WidgetTypeFactory*> *widget_type_factories_p = NULL;
  do_once
    {
      widget_type_factories_p = new std::list<const WidgetTypeFactory*>();
    }
  return *widget_type_factories_p;
}

static const WidgetTypeFactory*
lookup_widget_factory (String namespaced_ident)
{
  std::list<const WidgetTypeFactory*> &widget_type_factories = widget_type_list();
  namespaced_ident = namespaced_ident;
  std::list<const WidgetTypeFactory*>::iterator it;
  for (it = widget_type_factories.begin(); it != widget_type_factories.end(); it++)
    if ((*it)->qualified_type == namespaced_ident)
      return *it;
  return NULL;
}

void
WidgetTypeFactory::register_widget_factory (const WidgetTypeFactory &itfactory)
{
  std::list<const WidgetTypeFactory*> &widget_type_factories = widget_type_list();
  const char *ident = itfactory.qualified_type.c_str();
  const char *base = strrchr (ident, ':');
  if (!base || base <= ident || base[-1] != ':')
    fatal ("WidgetTypeFactory registration with invalid/missing domain name: %s", ident);
  String domain_name;
  domain_name.assign (ident, base - ident - 1);
  widget_type_factories.push_back (&itfactory);
}

typedef map<String, const XmlNode*> GadgetDefinitionMap;
static GadgetDefinitionMap gadget_definition_map;
static vector<String>      local_namespace_list;
static vector<String>      gadget_namespace_list;

static const XmlNode*
gadget_definition_lookup (const String &widget_identifier, const XmlNode *context_node)
{
  GadgetDefinitionMap::iterator it = gadget_definition_map.find (widget_identifier);
  if (it != gadget_definition_map.end())
    return it->second; // non-namespace lookup succeeded
  if (context_node)
    {
      const NodeData &ndata = xml_node_data (*context_node);
      if (!ndata.domain.empty())
        it = gadget_definition_map.find (ndata.domain + ":" + widget_identifier);
      if (it != gadget_definition_map.end())
        return it->second; // lookup in context namespace succeeded
    }
  for (ssize_t i = local_namespace_list.size() - 1; i >= 0; i--)
    {
      it = gadget_definition_map.find (local_namespace_list[i] + ":" + widget_identifier);
      if (it != gadget_definition_map.end())
        return it->second; // namespace searchpath lookup succeeded
    }
  for (ssize_t i = gadget_namespace_list.size() - 1; i >= 0; i--)
    {
      it = gadget_definition_map.find (gadget_namespace_list[i] + ":" + widget_identifier);
      if (it != gadget_definition_map.end())
        return it->second; // namespace searchpath lookup succeeded
    }
#if 0
  printerr ("%s: FAIL, no '%s' in namespaces:", __func__, widget_identifier.c_str());
  if (context_node)
    {
      String context_domain = context_node->get_data (&xml_node_domain_key);
      printerr (" %s", context_domain.c_str());
    }
  for (size_t i = 0; i < local_namespace_list.size(); i++)
    printerr (" %s", local_namespace_list[i].c_str());
  for (size_t i = 0; i < gadget_namespace_list.size(); i++)
    printerr (" %s", gadget_namespace_list[i].c_str());
  printerr ("\n");
#endif
  return NULL; // unmatched
}

void
use_ui_namespace (const String &uinamespace)
{
  initialize_factory_lazily();
  vector<String>::iterator it = find (gadget_namespace_list.begin(), gadget_namespace_list.end(), uinamespace);
  if (it != gadget_namespace_list.end())
    gadget_namespace_list.erase (it);
  gadget_namespace_list.push_back (uinamespace);
}

static void
force_ui_namespace_use (const String &uinamespace)
{
  vector<String>::const_iterator it = find (gadget_namespace_list.begin(), gadget_namespace_list.end(), uinamespace);
  if (it == gadget_namespace_list.end())
    gadget_namespace_list.push_back (uinamespace);
}

WidgetTypeFactory::WidgetTypeFactory (const char *namespaced_ident,
                                  bool _isevh, bool _iscontainer, bool) :
  qualified_type (namespaced_ident),
  iseventhandler (_isevh), iscontainer (_iscontainer)
{}

void
WidgetTypeFactory::sanity_check_identifier (const char *namespaced_ident)
{
  if (strncmp (namespaced_ident, "Rapicorn::Factory::", 19) != 0)
    fatal ("WidgetTypeFactory: identifier lacks factory qualification: %s", namespaced_ident);
}

String
factory_context_name (FactoryContext *fc)
{
  assert_return (fc != NULL, "");
  const XmlNode *xnode = fc->xnode;
  String s = xnode->name();
  if (s == "tmpl:define")
    return xnode->get_attribute ("id");
  else
    return s;
}

String
factory_context_type (FactoryContext *fc)
{
  assert_return (fc != NULL, "");
  const XmlNode *xnode = fc->xnode;
  if (xnode->name() != "tmpl:define") // lookup definition node from child node
    {
      xnode = gadget_definition_lookup (xnode->name(), xnode);
      assert_return (xnode != NULL, "");
    }
  assert_return (xnode->name() == "tmpl:define", "");
  return xnode->get_attribute ("id");
}

UserSource
factory_context_source (FactoryContext *fc)
{
  assert_return (fc != NULL, UserSource (""));
  const XmlNode *xnode = fc->xnode;
  if (xnode->name() != "tmpl:define") // lookup definition node from child node
    {
      xnode = gadget_definition_lookup (xnode->name(), xnode);
      assert_return (xnode != NULL, UserSource (""));
    }
  assert_return (xnode->name() == "tmpl:define", UserSource (""));
  return UserSource ("WidgetFactory", xnode->parsed_file(), xnode->parsed_line());
}

static void
factory_context_list_types (StringVector &types, const XmlNode *xnode, const bool need_ids, const bool need_variants)
{
  assert_return (xnode != NULL);
  if (xnode->name() != "tmpl:define") // lookup definition node from child node
    {
      xnode = gadget_definition_lookup (xnode->name(), xnode);
      assert_return (xnode != NULL);
    }
  while (xnode)
    {
      assert_return (xnode->name() == "tmpl:define");
      if (need_ids)
        types.push_back (xnode->get_attribute ("id"));
      const StringVector &attributes_names = xnode->list_attributes(), &attributes_values = xnode->list_values();
      const XmlNode *cnode = xnode;
      xnode = NULL;
      for (size_t i = 0; i < attributes_names.size(); i++)
        if (attributes_names[i] == "inherit")
          {
            xnode = gadget_definition_lookup (attributes_values[i], cnode);
            if (!xnode)
              {
                const WidgetTypeFactory *itfactory = lookup_widget_factory (attributes_values[i]);
                assert_return (itfactory != NULL);
                types.push_back (itfactory->type_name());
                if (need_variants && itfactory->iseventhandler)
                  types.push_back ("Rapicorn::Factory::EventHandler");
                if (need_variants && itfactory->iscontainer)
                  types.push_back ("Rapicorn::Factory::Container");
                if (need_variants)
                  types.push_back ("Rapicorn::Factory::Widget");
              }
            break;
          }
    }
}

static void
factory_context_update_cache (FactoryContext *fc)
{
  if (UNLIKELY (!fc->type_tags))
    {
      const XmlNode *xnode = fc->xnode;
      fc->type_tags = new StringSeq;
      StringVector &types = *fc->type_tags;
      factory_context_list_types (types, xnode, false, false);
      fc->type = types.size() ? types[types.size() - 1] : "";
      types.clear();
      factory_context_list_types (types, xnode, true, true);
    }
}

const StringSeq&
factory_context_tags (FactoryContext *fc)
{
  assert_return (fc != NULL, *(StringSeq*) NULL);
  factory_context_update_cache (fc);
  return *fc->type_tags;
}

String
factory_context_impl_type (FactoryContext *fc)
{
  assert_return (fc != NULL, "");
  factory_context_update_cache (fc);
  return fc->type;
}

#if 0
static void
dump_args (const String &what, const StringVector &anames, const StringVector &avalues)
{
  printerr ("%s:", what.c_str());
  for (size_t i = 0; i < anames.size(); i++)
    printerr (" %s=%s", anames[i].c_str(), avalues[i].c_str());
  printerr ("\n");
}
#endif

static String
node_location (const XmlNode *xnode)
{
  return string_format ("%s:%d", xnode->parsed_file().c_str(), xnode->parsed_line());
}

class Builder {
  enum Flags { NONE = 0, INHERITED = 1, CHILD = 2 };
  const XmlNode   *const dnode_;               // definition of gadget to be created
  String           child_container_name_;
  ContainerImpl   *child_container_;           // captured child_container_ widget during build phase
  VariableMap      locals_;
  void      eval_args       (const StringVector &in_names, const StringVector &in_values, StringVector &out_names, StringVector &out_values, const XmlNode *caller,
                             String *node_name, String *child_container_name, String *inherit_identifier);
  void      parse_call_args (const StringVector &call_names, const StringVector &call_values, StringVector &rest_names, StringVector &rest_values, String &name, const XmlNode *caller = NULL);
  void      apply_args      (WidgetImpl &widget, const StringVector &arg_names, const StringVector &arg_values, const XmlNode *caller, bool idignore);
  void      apply_props     (const XmlNode *pnode, WidgetImpl &widget);
  void      call_children   (const XmlNode *pnode, WidgetImpl *widget, vector<WidgetImpl*> *vchildren = NULL, const String &presuppose = "", int64 max_children = -1);
  WidgetImpl* call_widget       (const XmlNode *anode, const StringVector &call_names, const StringVector &call_values, const XmlNode *caller, const XmlNode *outmost_caller);
  WidgetImpl* call_child      (const XmlNode *anode, const StringVector &call_names, const StringVector &call_values, const String &name, const XmlNode *caller);
  explicit  Builder         (const XmlNode &definition_node);
public:
  explicit  Builder             (const String &widget_identifier, const XmlNode *context_node);
  static WidgetImpl* build_widget   (const String &widget_identifier, const StringVector &call_names, const StringVector &call_values);
  static WidgetImpl* inherit_widget (const String &widget_identifier, const StringVector &call_names, const StringVector &call_values,
                                 const XmlNode *caller, const XmlNode *derived);
  static void build_children    (ContainerImpl &container, vector<WidgetImpl*> *children, const String &presuppose, int64 max_children);
  static bool widget_has_ancestor (const String &widget_identifier, const String &ancestor_identifier);
};

Builder::Builder (const String &widget_identifier, const XmlNode *context_node) :
  dnode_ (gadget_definition_lookup (widget_identifier, context_node)), child_container_ (NULL)
{
  if (!dnode_)
    return;
}

Builder::Builder (const XmlNode &definition_node) :
  dnode_ (&definition_node), child_container_ (NULL)
{
  assert_return (dnode_->name() == "tmpl:define");
}

void
Builder::build_children (ContainerImpl &container, vector<WidgetImpl*> *children, const String &presuppose, int64 max_children)
{
  assert_return (presuppose != "");
  const XmlNode *dnode, *pnode = container.factory_context()->xnode;
  if (!pnode)
    return;
  while (pnode)
    {
      if (pnode->name() == "tmpl:define")
        dnode = pnode;
      else // lookup definition node from child node
        {
          dnode = gadget_definition_lookup (pnode->name(), pnode);
          assert_return (dnode != NULL);
          assert_return (dnode->name() == "tmpl:define");
        }
      Builder builder (*dnode);
      builder.call_children (pnode, &container, children, presuppose, max_children);
      if (children && max_children >= 0 && children->size() >= size_t (max_children))
        return;
      if (pnode == dnode)
        pnode = gadget_definition_lookup (dnode->get_attribute ("inherit"), dnode);
      else
        pnode = dnode;
    }
}

WidgetImpl*
Builder::build_widget (const String &widget_identifier, const StringVector &call_names, const StringVector &call_values)
{
  initialize_factory_lazily();
  Builder builder (widget_identifier, NULL);
  if (builder.dnode_)
    return builder.call_widget (builder.dnode_, call_names, call_values, NULL, NULL);
  else
    {
      critical ("%s: unknown type identifier: %s", "Builder::build_widget", widget_identifier.c_str());
      return NULL;
    }
}

WidgetImpl*
Builder::inherit_widget (const String &widget_identifier, const StringVector &call_names, const StringVector &call_values,
                       const XmlNode *caller, const XmlNode *derived)
{
  assert_return (derived != NULL, NULL);
  assert_return (caller != NULL, NULL);
  Builder builder (widget_identifier, caller);
  FDEBUG ("lookup %s: dnode=%p itfactory=%p", widget_identifier.c_str(), builder.dnode_, lookup_widget_factory (widget_identifier));
  if (builder.dnode_)
    return builder.call_widget (builder.dnode_, call_names, call_values, caller, derived);
  else
    {
      const WidgetTypeFactory *itfactory = lookup_widget_factory (widget_identifier);
      if (!itfactory)
        {
          critical ("%s: unknown widget type: %s", node_location (caller).c_str(), widget_identifier.c_str());
          return NULL;
        }
      FactoryContext *fc = factory_context_map[derived];
      if (!fc)
        {
          fc = new FactoryContext (derived);
          factory_context_map[derived] = fc;
        }
      WidgetImpl *widget = itfactory->create_widget (fc);
      builder.apply_args (*widget, call_names, call_values, caller, true);
      return widget;
    }
}

static String
canonify_dashes (const String &key) // FIXME: there should be an XmlNode post-processing phase that does this
{
  String s = key;
  for (uint i = 0; i < s.size(); i++)
    if (key[i] == '-')
      s[i] = '_'; // unshares string
  return s;
}

void
Builder::parse_call_args (const StringVector &call_names, const StringVector &call_values, StringVector &rest_names, StringVector &rest_values, String &name, const XmlNode *caller)
{
  StringVector local_names, local_values;
  // setup definition args
  Evaluator env;
  XmlNode::ConstNodes &children = dnode_->children();
  for (XmlNode::ConstNodes::const_iterator it = children.begin(); it != children.end(); it++)
    {
      const XmlNode *cnode = *it;
      if (cnode->name() == "tmpl:argument")
        {
          const String aname = canonify_dashes (cnode->get_attribute ("id")); // canonify argument id
          local_names.push_back (aname);
          String rvalue = cnode->get_attribute ("default");
          if (rvalue.find ('`') != String::npos)
            rvalue = env.parse_eval (rvalue);
          local_values.push_back (rvalue);
        }
    }
  // seperate definition from call args
  rest_names.reserve (call_names.size());
  rest_values.reserve (call_values.size());
  for (size_t i = 0; i < call_names.size(); i++)
    {
      const String cname = canonify_dashes (call_names[i]);
      if (cname == "name" || cname == "id")
        {
          name = call_values[i];
          continue;
        }
      vector<String>::const_iterator it = find (local_names.begin(), local_names.end(), cname);
      if (it != local_names.end())
        local_values[it - local_names.begin()] = call_values[i]; // local argument overriden by call argument
      else if (cname == "name" || cname == "id")
        ; // dname = arg_values[i];
      else
        {
          rest_names.push_back (cname);
          rest_values.push_back (call_values[i]);
        }
    }
  // prepare variable map for evaluator
  Evaluator::populate_map (locals_, local_names, local_values);
}

void
Builder::eval_args (const StringVector &in_names, const StringVector &in_values, StringVector &out_names, StringVector &out_values, const XmlNode *caller,
                    String *node_name, String *child_container_name, String *inherit_identifier)
{
  Evaluator env;
  env.push_map (locals_);
  out_names.reserve (in_names.size());
  out_values.reserve (in_values.size());
  for (size_t i = 0; i < in_names.size(); i++)
    {
      const String cname = canonify_dashes (in_names[i]);
      const String &ivalue = in_values[i];
      String rvalue;
      if (ivalue.find ('`') != String::npos)
        {
          rvalue = env.parse_eval (ivalue);
          EDEBUG ("%s: eval %s=\"%s\": %s", String (caller ? node_location (caller).c_str() : "Rapicorn:Factory").c_str(),
                  in_names[i].c_str(), ivalue.c_str(), rvalue.c_str());
        }
      else
        rvalue = ivalue;
      if (inherit_identifier && cname == "inherit")
        *inherit_identifier = rvalue;
      else if (child_container_name && cname == "child_container")
        *child_container_name = rvalue;
      else if (node_name && (cname == "name" || cname == "id"))
        *node_name = rvalue;
      else
        {
          out_names.push_back (cname);
          out_values.push_back (rvalue);
        }
    }
  env.pop_map (locals_);
}

void
Builder::apply_args (WidgetImpl &widget,
                     const StringVector &prop_names, const StringVector &prop_values, // evaluated args
                     const XmlNode *caller, bool idignore)
{
  for (size_t i = 0; i < prop_names.size(); i++)
    {
      const String aname = canonify_dashes (prop_names[i]);
      if (aname == "name" || aname == "id")
        {
          if (!idignore)
            critical ("%s: internal-error, property should have been filtered: %s", node_location (caller).c_str(), prop_names[i].c_str());
        }
      else if (prop_names[i].find (':') != String::npos)
        ; // ignore namespaced attributes
      else if (widget.try_set_property (aname, prop_values[i]))
        {}
      else
        critical ("%s: widget %s: unknown property: %s", node_location (caller).c_str(), widget.name().c_str(), prop_names[i].c_str());
    }
}

void
Builder::apply_props (const XmlNode *pnode, WidgetImpl &widget)
{
  XmlNode::ConstNodes &children = pnode->children();
  for (XmlNode::ConstNodes::const_iterator it = children.begin(); it != children.end(); it++)
    {
      const XmlNode *cnode = *it;
      if (cnode->istext() || cnode->name() != "tmpl:property")
        continue;
      const String aname = canonify_dashes (cnode->get_attribute ("id"));
      String value = cnode->xml_string (0, false);
      if (value.find ('`') != String::npos && string_to_bool (cnode->get_attribute ("evaluate"), true))
        {
          Evaluator env;
          env.push_map (locals_);
          value = env.parse_eval (value);
          env.push_map (locals_);
        }
      if (aname == "name" || aname == "id")
        critical ("%s: internal-error, property should have been filtered: %s", node_location (cnode).c_str(), cnode->name().c_str());
      else if (widget.try_set_property (aname, value))
        {}
      else
        critical ("%s: widget %s: unknown property: %s", node_location (pnode).c_str(), widget.name().c_str(), aname.c_str());
    }
}

WidgetImpl*
Builder::call_widget (const XmlNode *anode,
                      const StringVector &call_names, const StringVector &call_values, // evaluated args
                      const XmlNode *caller, const XmlNode *outmost_caller)
{
  assert_return (dnode_ != NULL, NULL);
  String name;
  StringVector prop_names, prop_values;
  /// @TODO: Catch error condition: simultaneous use of inherit="..." and child-container="..."
  parse_call_args (call_names, call_values, prop_names, prop_values, name, caller);
  // extract factory attributes and eval attributes
  StringVector parent_names, parent_values;
  String inherit;
  assert (child_container_name_.empty() == true);
  eval_args (anode->list_attributes(), anode->list_values(), parent_names, parent_values, caller, NULL, &child_container_name_, &inherit);
  // create widget
  WidgetImpl *widget = Builder::inherit_widget (inherit, parent_names, parent_values, anode,
                                          outmost_caller ? outmost_caller : (caller ? caller : anode));
  if (!widget)
    return NULL;
  // apply widget arguments
  if (!name.empty())
    widget->name (name);
  apply_args (*widget, prop_names, prop_values, anode, false);
  FDEBUG ("new-widget: %s (%d children) id=%s", anode->name().c_str(), anode->children().size(), widget->name().c_str());
  // apply properties and create children
  if (!anode->children().empty())
    {
      apply_props (anode, *widget);
      call_children (anode, widget);
    }
  // assign child container
  if (child_container_)
    widget->as_container_impl()->child_container (child_container_);
  else if (!child_container_name_.empty())
    critical ("%s: failed to find child container: %s", node_location (dnode_).c_str(), child_container_name_.c_str());
  return widget;
}

WidgetImpl*
Builder::call_child (const XmlNode *anode,
                     const StringVector &call_names, const StringVector &call_values, // evaluated args
                     const String &name, const XmlNode *caller)
{
  assert_return (dnode_ != NULL, NULL);
  // create widget
  WidgetImpl *widget = Builder::inherit_widget (anode->name(), call_names, call_values, anode, caller ? caller : anode);
  if (!widget)
    return NULL;
  // apply widget arguments
  if (!name.empty())
    widget->name (name);
  FDEBUG ("new-widget: %s (%d children) id=%s", anode->name().c_str(), anode->children().size(), widget->name().c_str());
  // apply properties and create children
  if (!anode->children().empty())
    {
      apply_props (anode, *widget);
      call_children (anode, widget);
    }
  // find child container
  if (!child_container_name_.empty() && child_container_name_ == widget->name())
    {
      ContainerImpl *cc = widget->as_container_impl();
      if (cc)
        {
          if (child_container_)
            critical ("%s: duplicate child containers: %s", node_location (dnode_).c_str(), node_location (anode).c_str());
          child_container_ = cc;
          FDEBUG ("assign child-container for %s: %s", dnode_->name().c_str(), child_container_name_.c_str());
          // FIXME: mismatches occour, because child caller args are not passed as call_names/values
        }
    }
  return widget;
}

void
Builder::call_children (const XmlNode *pnode, WidgetImpl *widget, vector<WidgetImpl*> *vchildren, const String &presuppose, int64 max_children)
{
  // add children
  XmlNode::ConstNodes &children = pnode->children();
  ContainerImpl *container = NULL;
  for (XmlNode::ConstNodes::const_iterator it = children.begin(); it != children.end(); it++)
    {
      const XmlNode *cnode = *it;
      if (cnode->istext() || cnode->name() == "tmpl:property")
        continue;
      else if (cnode->name() == "tmpl:argument")
        {
          if (pnode->name() != "tmpl:define")
            critical ("%s: arguments must be declared inside definitions", node_location (cnode).c_str());
          continue;
        }
      // check extended node data
      const NodeData &cdata = xml_node_data (*cnode);
      if ((cdata.tmpl_presuppose ^ !presuppose.empty()) ||
          (cdata.tmpl_presuppose && presuppose != cnode->get_attribute ("tmpl:presuppose")))
        continue;
      if (!container)
        {
          container = widget->as_container_impl();
          if (!container)
            critical ("%s: parent type is not a container: %s:%s", node_location (cnode).c_str(),
                      node_location (pnode).c_str(), pnode->name().c_str());
          return_unless (container != NULL);
        }
      // create child
      StringVector call_names, call_values, arg_names, arg_values;
      String child_name;
      eval_args (cnode->list_attributes(), cnode->list_values(), call_names, call_values, pnode, &child_name, NULL, NULL);
      WidgetImpl *child = call_child (cnode, call_names, call_values, child_name, cnode);
      if (!child)
        {
          critical ("%s: failed to create widget: %s", node_location (cnode).c_str(), cnode->name().c_str());
          continue;
        }
      else if (vchildren)
        vchildren->push_back (child);
      // add to parent
      try {
        container->add (child);
      } catch (std::exception &exc) {
        critical ("%s: adding %s to parent failed: %s", node_location (cnode).c_str(), cnode->name().c_str(), exc.what());
        unref (ref_sink (child));
        throw; /// @TODO: Eliminate exception use
      } catch (...) {
        unref (ref_sink (child));
        throw; // FIXME: eliminate exception use
      }
      // limit amount of children
      if (vchildren && max_children >= 0 && vchildren->size() >= size_t (max_children))
        return;
    }
}

bool
Builder::widget_has_ancestor (const String &widget_identifier, const String &ancestor_identifier)
{
  initialize_factory_lazily();
  const XmlNode *const anode = gadget_definition_lookup (ancestor_identifier, NULL); // maybe NULL
  const WidgetTypeFactory *const ancestor_itfactory = anode ? NULL : lookup_widget_factory (ancestor_identifier);
  if (widget_identifier == ancestor_identifier && (anode || ancestor_itfactory))
    return true; // potential fast path
  else if (!anode && !ancestor_itfactory)
    return false; // ancestor_identifier is non-existent
  String identifier = widget_identifier;
  const XmlNode *node = gadget_definition_lookup (identifier, NULL);
  while (node)
    {
      if (node == anode)
        return true; // widget ancestor matches
      identifier = node->get_attribute ("inherit", true);
      node = gadget_definition_lookup (identifier, node);
    }
  if (anode)
    return false; // no node match possible
  const WidgetTypeFactory *widget_itfactory = lookup_widget_factory (identifier);
  return ancestor_itfactory && widget_itfactory == ancestor_itfactory;
}

bool
check_ui_window (const String &widget_identifier)
{
  return Builder::widget_has_ancestor (widget_identifier, "Rapicorn::Factory::Window");
}

WidgetImpl&
create_ui_widget (const String       &widget_identifier,
                const ArgumentList &arguments)
{
  StringVector anames, avalues;
  for (size_t i = 0; i < arguments.size(); i++)
    {
      const String &arg = arguments[i];
      const size_t pos = arg.find ('=');
      if (pos != String::npos)
        anames.push_back (arg.substr (0, pos)), avalues.push_back (arg.substr (pos + 1));
      else
        FDEBUG ("%s: argument without value: %s", widget_identifier.c_str(), arg.c_str());
    }
  WidgetImpl *widget = Builder::build_widget (widget_identifier, anames, avalues);
  if (!widget)
    fatal ("%s: failed to create widget: %s", "Rapicorn:Factory", widget_identifier.c_str());
  return *widget;
}

WidgetImpl&
create_ui_child (ContainerImpl &container, const String &widget_identifier, const ArgumentList &arguments, bool autoadd)
{
  // figure XML context
  FactoryContext *fc = container.factory_context();
  assert_return (fc != NULL, *(WidgetImpl*) NULL);
  const XmlNode *xnode = fc->xnode;
  const NodeData &ndata = NodeData::from_xml_node (const_cast<XmlNode&> (*xnode));
  // create child within parent namespace
  local_namespace_list.push_back (ndata.domain);
  WidgetImpl &widget = create_ui_widget (widget_identifier, arguments);
  local_namespace_list.pop_back();
  // add to parent
  if (autoadd)
    container.add (widget);
  return widget;
}

void
create_ui_children (ContainerImpl     &container,
                    vector<WidgetImpl*> *children,
                    const String      &presuppose,
                    int64              max_children)
{
  // figure XML context
  FactoryContext *fc = container.factory_context();
  assert_return (fc != NULL);
  const XmlNode *xnode = fc->xnode;
  const NodeData &ndata = NodeData::from_xml_node (const_cast<XmlNode&> (*xnode));
  // create children within parent namespace
  local_namespace_list.push_back (ndata.domain);
  Builder::build_children (container, children, presuppose, max_children);
  local_namespace_list.pop_back();
}

static void
assign_xml_node_data_recursive (XmlNode *xnode, const String &domain)
{
  NodeData &ndata = NodeData::from_xml_node (*xnode);
  ndata.domain = domain;
  XmlNode::ConstNodes &children = xnode->children();
  for (XmlNode::ConstNodes::const_iterator it = children.begin(); it != children.end(); it++)
    {
      const XmlNode *cnode = *it;
      if (cnode->istext() || cnode->name() == "tmpl:property" || cnode->name() == "tmpl:argument")
        continue;
      assign_xml_node_data_recursive (const_cast<XmlNode*> (cnode), domain);
    }
}

static String
register_ui_node (const String   &domain,
                  XmlNode        *xnode,
                  vector<String> *definitions)
{
  if (xnode->name() == "tmpl:define")
    {
      const String &nname = xnode->get_attribute ("id");
      String ident = domain.empty () ? nname : domain + ":" + nname; // FIXME
      GadgetDefinitionMap::iterator it = gadget_definition_map.find (ident);
      if (it != gadget_definition_map.end())
        return string_format ("%s: redefinition of: %s (previously at %s)",
                              node_location (xnode).c_str(), ident.c_str(), node_location (it->second).c_str());
      assign_xml_node_data_recursive (xnode, domain);
      gadget_definition_map[ident] = ref_sink (xnode);
      if (definitions)
        definitions->push_back (ident);
      FDEBUG ("register: %s", ident.c_str());
    }
  else
    return string_format ("%s: invalid element tag: %s", node_location (xnode).c_str(), xnode->name().c_str());
  return ""; // success;
}

static String
register_ui_nodes (const String   &domain,
                   XmlNode        *xnode,
                   vector<String> *definitions)
{
  assert_return (domain.empty() == false, "missing namespace for ui definitions");
  // allow toplevel templates
  if (xnode->name() == "tmpl:define")
    return register_ui_node (domain, xnode, definitions);
  // enforce sane toplevel node
  if (xnode->name() != "rapicorn-definitions")
    return string_format ("%s: invalid root: %s", node_location (xnode).c_str(), xnode->name().c_str());
  // register template children
  XmlNode::ConstNodes children = xnode->children();
  for (XmlNode::ConstNodes::const_iterator it = children.begin(); it != children.end(); it++)
    {
      const XmlNode *cnode = *it;
      if (cnode->istext())
        continue; // ignore top level text
      if (cnode->name() != "tmpl:define")
        fatal ("%s: invalid tag: %s", node_location (cnode).c_str(), cnode->name().c_str());
      const String cerr = register_ui_node (domain, const_cast<XmlNode*> (cnode), definitions);
      if (!cerr.empty())
        return cerr;
    }
  return ""; // success;
}

static String
parse_ui_data_internal (const String   &domain,
                        const String   &data_name,
                        size_t          data_length,
                        const char     *data,
                        const String   &i18n_domain,
                        vector<String> *definitions)
{
  if (domain.empty())
    return "missing namespace for ui definitions";
  MarkupParser::Error perror;
  XmlNode *xnode = XmlNode::parse_xml (data_name, data, data_length, &perror);
  if (xnode)
    ref_sink (xnode);
  String errstr;
  if (perror.code)
    errstr = string_format ("%s:%d:%d: %s", data_name.c_str(), perror.line_number, perror.char_number, perror.message.c_str());
  else
    errstr = register_ui_nodes (domain, xnode, definitions);
  if (xnode)
    unref (xnode);
  return errstr;
}

String
parse_ui_data (const String   &uinamespace,
               const String   &data_name,
               size_t          data_length,
               const char     *data,
               const String   &i18n_domain,
               vector<String> *definitions)
{
  initialize_factory_lazily();
  return parse_ui_data_internal (uinamespace, data_name, data_length, data, "", definitions);
}

String
parse_ui_file (const String   &uinamespace,
               const String   &file_name,
               const String   &i18n_domain,
               vector<String> *definitions)
{
  size_t flen = 0;
  char *fdata = Path::memread (file_name, &flen);
  if (fdata)
    {
      String result = parse_ui_data (uinamespace, file_name, flen, fdata, i18n_domain, definitions);
      Path::memfree (fdata);
      return result;
    }
  else
    return strerror (ENOENT);
}

} // Factory

namespace { // Anon
#include "../res/resources.cc" // various resource files
#include "gen-zintern.c" // compressed foundation.xml & standard.xml
} // Anon

static void
initialize_factory_lazily (void)
{
  do_once
    {
      uint8 *data;
      const char *domain = "Rapicorn";
      Factory::force_ui_namespace_use (domain);
      data = zintern_decompress (FOUNDATION_XML_SIZE, FOUNDATION_XML_DATA,
                                 sizeof (FOUNDATION_XML_DATA) / sizeof (FOUNDATION_XML_DATA[0]));
      Factory::parse_ui_data_internal (domain, FOUNDATION_XML_NAME, FOUNDATION_XML_SIZE, (const char*) data, "", NULL);
      zintern_free (data);
      data = zintern_decompress (STANDARD_XML_SIZE, STANDARD_XML_DATA,
                                 sizeof (STANDARD_XML_DATA) / sizeof (STANDARD_XML_DATA[0]));
      Factory::parse_ui_data_internal (domain, STANDARD_XML_NAME, STANDARD_XML_SIZE, (const char*) data, "", NULL);
      zintern_free (data);
    }
}

} // Rapicorn
