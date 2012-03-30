// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __RAPICORN_FACTORY_HH__
#define __RAPICORN_FACTORY_HH__

#include <ui/item.hh>
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
ItemImpl&   create_ui_item      (const String           &item_identifier,
                                 const ArgumentList     &arguments = ArgumentList());
bool        check_ui_window     (const String           &item_identifier);
void        use_ui_namespace    (const String           &uinamespace);

// == Factory Contexts ==

typedef map<String,String>       VariableMap;

String     factory_context_name    (FactoryContext *fc);
String     factory_context_type    (FactoryContext *fc);
StringList factory_context_tags    (FactoryContext *fc);

/* --- item type registration --- */
struct ItemTypeFactory : protected Deletable, protected NonCopyable {
  const String  qualified_type;
protected:
  static void   register_item_factory   (const ItemTypeFactory  &itfactory);
  static void   sanity_check_identifier (const char             *namespaced_ident);
public:
  explicit      ItemTypeFactory         (const char             *namespaced_ident,
                                         bool _isevh, bool _iscontainer, bool);
  virtual ItemImpl* create_item         (FactoryContext         *fc) const = 0;
  inline String type_name               () const { return qualified_type; }
  const bool iseventhandler, iscontainer;
};

} // Factory

// namespace Rapicorn

/* --- item factory template --- */
template<class Type>
class ItemFactory : Factory::ItemTypeFactory {
  virtual ItemImpl*
  create_item (FactoryContext *fc) const
  {
    ItemImpl *item = new Type();
    item->factory_context (fc);
    return item;
  }
public:
  explicit ItemFactory (const char *namespaced_ident) :
    ItemTypeFactory (namespaced_ident,
                     TraitConvertible<EventHandler,Type>::TRUTH,
                     TraitConvertible<ContainerImpl,Type>::TRUTH,
                     TraitConvertible<int,Type>::TRUTH)
  {
    sanity_check_identifier (namespaced_ident);
    register_item_factory (*this);
  }
};

} // Rapicorn

#endif /* __RAPICORN_FACTORY_HH__ */
