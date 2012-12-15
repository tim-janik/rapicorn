// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __RAPICORN_ENUMDEFS_HH__
#define __RAPICORN_ENUMDEFS_HH__

#include <rcore/utilities.hh>

namespace Rapicorn {

/* --- enum/flags type descriptions --- */
struct EnumClass {
  struct Value {
    const int64             value;
    const char       *const value_name;
    const uint              name_length;
  };
  virtual            ~EnumClass         () {}
  virtual void        list_values       (uint &n_values, const Value* &values) const = 0;
  virtual const char* enum_name         () const = 0;
  virtual bool        flag_combinable   () const = 0;
  virtual int64       constrain         (int64 value) const = 0;
  bool                match_partial     (const char *value_name1, const char *partial_value_name) const;
  bool                match             (const char *value_name1, const char *value_name2) const;
  const Value*        find_first        (int64 value) const;
  const Value*        find_first        (const String &value_name) const;
  int64               parse             (const char *value_string, String *error = NULL) const;
  String              string            (int64 value) const;
};
template<typename EnumType> EnumType inline enum_type_constrain (EnumType value) { return value; }
template<typename EType>
struct EnumType : public virtual EnumClass {
  typedef EType Type;
  virtual void        list_values       (uint &c_n_values, const Value* &c_values) const { c_n_values = n_values, c_values = values; }
  virtual const char* enum_name         () const { return ename; }
  virtual bool        flag_combinable   () const { return false; }
  virtual int64       constrain         (int64 value) const { return enum_type_constrain<EType> (EType (value)); }
  inline              EnumType          () {}
private:
  static const uint         n_values;
  static const Value *const values;
  static const char        *ename;
};
template<typename EType>
struct FlagsType : public virtual EnumClass {
  typedef EType Type;
  virtual void        list_values       (uint &c_n_values, const Value* &c_values) const { c_n_values = n_values, c_values = values; }
  virtual const char* enum_name         () const { return ename; }
  virtual bool        flag_combinable   () const { return true; }
  virtual int64       constrain         (int64 value) const { return enum_type_constrain<EType> (EType (value)); }
  inline              FlagsType         () {}
private:
  static const uint         n_values;
  static const Value *const values;
  static const char        *ename;
};


} // Rapicorn

#endif  /* __RAPICORN_ENUMDEFS_HH__ */
