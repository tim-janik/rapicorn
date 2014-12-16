// This Source Code Form is licensed MPLv2: http://mozilla.org/MPL/2.0
#ifndef __RAPICORN_FACTORY_HH__
#define __RAPICORN_FACTORY_HH__

#include <ui/widget.hh>
#include <list>

namespace Rapicorn {

namespace Factory {

typedef std::vector<String>      ArgumentList;  /* elements: key=utf8string */

// == UI Factory ==
String      parse_ui_data       (const String           &uinamespace,
                                 const String           &data_name,
                                 size_t                  data_length,
                                 const char             *data,
                                 const String           &i18n_domain = "",
                                 vector<String>         *definitions = NULL);
WidgetImplP create_ui_widget    (const String           &widget_identifier,
                                 const ArgumentList     &arguments = ArgumentList());
WidgetImplP create_ui_child     (ContainerImpl &container, const String &widget_identifier,
                                 const ArgumentList &arguments, bool autoadd = true);

bool        check_ui_window     (const String           &widget_identifier);
void        use_ui_namespace    (const String           &uinamespace);

// == Factory Contexts ==

typedef map<String,String>       VariableMap;

String           factory_context_name      (FactoryContext *fc);
String           factory_context_type      (FactoryContext *fc);
const StringSeq& factory_context_tags      (FactoryContext *fc);
UserSource       factory_context_source    (FactoryContext *fc);
String           factory_context_impl_type (FactoryContext *fc);

/* --- widget type registration --- */
struct WidgetTypeFactory {
  const String  qualified_type;
  RAPICORN_CLASS_NON_COPYABLE (WidgetTypeFactory);
protected:
  static void   register_widget_factory (const WidgetTypeFactory    &itfactory);
  static void   sanity_check_identifier (const char                 *namespaced_ident);
  virtual              ~WidgetTypeFactory ();
public:
  explicit              WidgetTypeFactory (const char               *namespaced_ident);
  virtual WidgetImplP   create_widget     (FactoryContext           *fc) const = 0;
  virtual void          type_name_list    (std::vector<const char*> &names) const = 0;
  inline String         type_name         () const                          { return qualified_type; }
};

} // Factory

// namespace Rapicorn

/* --- widget factory template --- */
void temp_window_factory_workaround (ObjectIfaceP o);
template<class Type>
class WidgetFactory : Factory::WidgetTypeFactory {
  virtual WidgetImplP
  create_widget (FactoryContext *fc) const override
  {
    ObjectIfaceP object = (new Type())->temp_factory_workaround();
    temp_window_factory_workaround (object);
    WidgetImplP widget = shared_ptr_cast<WidgetImpl> (object);
    widget->factory_context (fc);
    return widget;
  }
  virtual void
  type_name_list (std::vector<const char*> &names) const override
  {
#define __RAPICORN_SERVERAPI_HH__INTERFACE_NAME(CLASS)  \
    if (std::is_base_of<CLASS ## Iface, Type>::value)   \
      names.push_back ( #CLASS );
    __RAPICORN_SERVERAPI_HH__INTERFACE_LIST ; // enlist all known IDL classes matching Type
#undef __RAPICORN_SERVERAPI_HH__INTERFACE_NAME
    if (std::is_base_of<EventHandler, Type>::value)
      names.push_back ("Rapicorn::EventHandler");
  }
public:
  explicit WidgetFactory (const char *namespaced_ident) :
    WidgetTypeFactory (namespaced_ident)
  {
    sanity_check_identifier (namespaced_ident);
    register_widget_factory (*this);
  }
};

} // Rapicorn

#endif /* __RAPICORN_FACTORY_HH__ */
