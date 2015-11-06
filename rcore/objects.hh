// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#ifndef __RAPICORN_CORE_OBJECTS_HH__
#define __RAPICORN_CORE_OBJECTS_HH__

#include <rcore/utilities.hh>
#include <rcore/thread.hh>
#include <rcore/aidaprops.hh>

namespace Rapicorn {

// == Misc Helpers ==
typedef Aida::PropertyList PropertyList; /// Import PropertyList from Aida namespace
typedef Aida::Property     Property;     /// Import Property from Aida namespace

class NullInterface : std::exception {};

String          cxx_demangle       (const char *mangled_identifier);

// == ClassDoctor (used for private class copies) ==
#ifdef  __RAPICORN_BUILD__
class ClassDoctor;
#else
class ClassDoctor {};
#endif

// == DataListContainer ==
/**
 * By using a DataKey, DataListContainer objects allow storage and retrieval of custom data members in a typesafe fashion.
 * The custom data members will initially default to DataKey::fallback and are deleted by the DataListContainer destructor.
 * Example: @snippet rcore/tests/datalist.cc DataListContainer-EXAMPLE
 */
class DataListContainer {
  DataList data_list;
public: /// @name Accessing custom data members
  /// Assign @a data to the custom keyed data member, deletes any previously set data.
  template<typename Type> inline void set_data    (DataKey<Type> *key, Type data) { data_list.set (key, data); }
  /// Retrieve contents of the custom keyed data member, returns DataKey::fallback if nothing was set.
  template<typename Type> inline Type get_data    (DataKey<Type> *key) const      { return data_list.get (key); }
  /// Swap @a data with the current contents of the custom keyed data member, returns the current contents.
  template<typename Type> inline Type swap_data   (DataKey<Type> *key, Type data) { return data_list.swap (key, data); }
  /// Removes and returns the current contents of the custom keyed data member without deleting it.
  template<typename Type> inline Type swap_data   (DataKey<Type> *key)            { return data_list.swap (key); }
  /// Delete the current contents of the custom keyed data member, invokes DataKey::destroy.
  template<typename Type> inline void delete_data (DataKey<Type> *key)            { data_list.del (key); }
};

} // Rapicorn

#endif // __RAPICORN_CORE_OBJECTS_HH__
