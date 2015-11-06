// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#ifndef __RAPICORN_FACTORY_HH__
#define __RAPICORN_FACTORY_HH__

#include <ui/widget.hh>
#include <list>

namespace Rapicorn {

namespace Factory {

typedef std::vector<String>      ArgumentList;  /* elements: key=utf8string */

// == UI Factory ==
String      parse_ui_data       (const String           &data_name,
                                 size_t                  data_length,
                                 const char             *data,
                                 const String           &i18n_domain,
                                 const ArgumentList     *arguments,
                                 StringVector           *definitions);
WidgetImplP create_ui_widget    (const String           &widget_identifier,
                                 const ArgumentList     &arguments = ArgumentList());
WidgetImplP create_ui_child     (ContainerImpl &container, const String &widget_identifier,
                                 const ArgumentList &arguments, bool autoadd = true);

bool        check_ui_window     (const String           &widget_identifier);

// == Factory Contexts ==

typedef map<String,String>       VariableMap;

String           factory_context_name      (FactoryContext &fc);
String           factory_context_type      (FactoryContext &fc);
const StringSeq& factory_context_tags      (FactoryContext &fc);
UserSource       factory_context_source    (FactoryContext &fc);
String           factory_context_impl_type (FactoryContext &fc);

// == Object Type Registration ==
struct ObjectTypeFactory {
  const String  qualified_type;
  RAPICORN_CLASS_NON_COPYABLE (ObjectTypeFactory);
protected:
  static void   register_object_factory (const ObjectTypeFactory    &itfactory);
  static void   sanity_check_identifier (const char                 *namespaced_ident);
  virtual              ~ObjectTypeFactory ();
public:
  explicit              ObjectTypeFactory (const char               *namespaced_ident);
  virtual ObjectImplP   create_object     (FactoryContext           &fc) const = 0;
  virtual void          type_name_list    (std::vector<const char*> &names) const = 0;
  inline String         type_name         () const                          { return qualified_type; }
};

} // Factory

// namespace Rapicorn

// == WidgetFactory ==
template<class Type>
class WidgetFactory : Factory::ObjectTypeFactory {
  virtual ObjectImplP
  create_object (FactoryContext &fc) const override
  {
    return ObjectImpl::make_instance<Type> (fc);
  }
  virtual void
  type_name_list (std::vector<const char*> &names) const override
  {
#define RAPICORN_IDL_INTERFACE_NAME(CLASS)  \
    if (std::is_base_of<CLASS ## Iface, Type>::value)   \
      names.push_back ( #CLASS );
    RAPICORN_IDL_INTERFACE_LIST ; // enlist all known IDL classes matching Type
#undef RAPICORN_IDL_INTERFACE_NAME
    if (std::is_base_of<EventHandler, Type>::value)
      names.push_back ("Rapicorn::EventHandler");
  }
public:
  explicit WidgetFactory (const char *namespaced_ident) :
    ObjectTypeFactory (namespaced_ident)
  {
    sanity_check_identifier (namespaced_ident);
    register_object_factory (*this);
  }
};

} // Rapicorn

#endif /* __RAPICORN_FACTORY_HH__ */
