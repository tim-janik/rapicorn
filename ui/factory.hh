// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __RAPICORN_FACTORY_HH__
#define __RAPICORN_FACTORY_HH__

#include <ui/widget.hh>
#include <list>

namespace Rapicorn {

namespace Factory {

typedef std::vector<String>      ArgumentList;  /* elements: key=utf8string */

// == UI Factory ==
String      parse_ui_file       (const String           &uinamespace,
                                 const String           &file_name,
                                 const String           &i18n_domain = "",
                                 vector<String>         *definitions = NULL);
String      parse_ui_data       (const String           &uinamespace,
                                 const String           &data_name,
                                 size_t                  data_length,
                                 const char             *data,
                                 const String           &i18n_domain = "",
                                 vector<String>         *definitions = NULL);
WidgetImpl& create_ui_widget    (const String           &widget_identifier,
                                 const ArgumentList     &arguments = ArgumentList());
WidgetImpl& create_ui_child     (ContainerImpl &container, const String &widget_identifier,
                                 const ArgumentList &arguments, bool autoadd = true);

void        create_ui_children  (ContainerImpl          &container,
                                 vector<WidgetImpl*>      *children,
                                 const String           &presuppose,
                                 int64                   max_children = -1);
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
struct WidgetTypeFactory : protected Deletable {
  const String  qualified_type;
  RAPICORN_CLASS_NON_COPYABLE (WidgetTypeFactory);
protected:
  static void   register_widget_factory   (const WidgetTypeFactory  &itfactory);
  static void   sanity_check_identifier (const char             *namespaced_ident);
public:
  explicit      WidgetTypeFactory         (const char             *namespaced_ident,
                                         bool _isevh, bool _iscontainer, bool);
  virtual WidgetImpl* create_widget         (FactoryContext         *fc) const = 0;
  inline String type_name               () const { return qualified_type; }
  const bool iseventhandler, iscontainer;
};

} // Factory

// namespace Rapicorn

/* --- widget factory template --- */
template<class Type>
class WidgetFactory : Factory::WidgetTypeFactory {
  virtual WidgetImpl*
  create_widget (FactoryContext *fc) const
  {
    WidgetImpl *widget = new Type();
    widget->factory_context (fc);
    return widget;
  }
public:
  explicit WidgetFactory (const char *namespaced_ident) :
    WidgetTypeFactory (namespaced_ident,
                     TraitConvertible<EventHandler,Type>::TRUTH,
                     TraitConvertible<ContainerImpl,Type>::TRUTH,
                     TraitConvertible<int,Type>::TRUTH)
  {
    sanity_check_identifier (namespaced_ident);
    register_widget_factory (*this);
  }
};

} // Rapicorn

#endif /* __RAPICORN_FACTORY_HH__ */
