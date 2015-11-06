// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#ifndef __RAPICORN_SINFEX_HH__
#define __RAPICORN_SINFEX_HH__

#include <ui/utilities.hh>

namespace Rapicorn {

class Sinfex;
typedef std::shared_ptr<Sinfex> SinfexP;
typedef std::weak_ptr  <Sinfex> SinfexW;

class Sinfex {
  RAPICORN_CLASS_NON_COPYABLE (Sinfex);
protected:
  uint          *start_;
  explicit       Sinfex ();
  virtual       ~Sinfex ();
public:
  static SinfexP parse_string (const String &expression);
  class          Value;
  struct Scope {
    virtual Value resolve_variable        (const String        &entity,
                                           const String        &name) = 0;
    virtual Value call_function           (const String        &entity,
                                           const String        &name,
                                           const vector<Value> &args) = 0;
  };
  virtual Value  eval     (Scope &scope) = 0;
  /* Sinfex::Value implementation */
  class Value {
    String   string_;
    double   real_;
    bool     strflag_;
    String   real2string () const;
    double   string2real () const;
  public:
    bool     isreal      () const { return !strflag_; }
    bool     isstring    () const { return strflag_; }
    double   real        () const { return !strflag_ ? real_ : string2real(); }
    String   string      () const { return strflag_ ? string_ : real2string(); }
    bool     asbool      () const { return (!strflag_ && real_) || (strflag_ && string_ != ""); }
    explicit Value       (double        d) : real_ (d), strflag_ (0) {}
    explicit Value       (const String &s);
  };
};

} // Rapicorn

#endif  /* __RAPICORN_SINFEX_HH__ */
