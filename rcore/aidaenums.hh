// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __RAPICORN_AIDA_ENUMS_HH__
#define __RAPICORN_AIDA_ENUMS_HH__

#include <rcore/aida.hh>
#include <rcore/strings.hh>

namespace Rapicorn { namespace Aida {

/// The EnumInfo contains runtime information about enum values of IDL enumeration types.
struct EnumInfo
{
  struct Value {
    const int64       value;
    const uint        length;
    const char *const name;
  };
  void          list_values     (size_t *n_values, const Value **values) const  { *n_values = n_values_; *values = values_; }
  String        enum_name       () const                                        { return name_; }
  String        enum_namespace  () const                                        { return namespace_; }
  bool          flag_combinable () const                                        { return flag_combinable_; }
  const Value*  find_first      (int64 value) const;
  const Value*  find_first      (const String &value_name) const;
  int64         parse           (const String &value_string, String *error = NULL) const;
  String        string          (int64 value) const;
  EnumInfo&     operator=       (const EnumInfo&);
  explicit      EnumInfo        ();
  template<ssize_t S>
  static void         enlist    (const char *p, const char *n, const Value (&vv)[S]) { enlist (EnumInfo (p, n, vv)); }
  static EnumInfo     from_nsid (const char *enamespace, const char *ename);
private:
  template<ssize_t S> EnumInfo  (const char*, const char*, const Value (&)[S]);
  static void         enlist    (const EnumInfo&);
  const char  *namespace_, *name_;
  const Value *values_;
  uint         n_values_;
  bool         flag_combinable_;
};

/// Template function to retrieve EnumInfo for an IDL enumeration type.
template<class E> inline EnumInfo enum_info ();

// == implementation details ==
template<class E> EnumInfo
enum_info () // Fallback for unspecialized types
{
  static_assert (0 * sizeof (E), "no EnumInfo specialisation for this type");
  return *(EnumInfo*) NULL; // silence compiler
}

template<ssize_t S>
EnumInfo::EnumInfo (const char *const enum_namespace, const char *const enum_name, const Value (&vv)[S]) :
  namespace_ (enum_namespace), name_ (enum_name), values_ (vv), n_values_ (S), flag_combinable_ (0)
{
  static_assert (S > 0, "missing values");
}

#define RAPICORN_AIDA_ENUM_INFO_VALUE(VALUE)    { VALUE, sizeof (#VALUE) - 1, #VALUE }

} } // Rapicorn::Aida

#endif  // __RAPICORN_AIDA_ENUMS_HH__
