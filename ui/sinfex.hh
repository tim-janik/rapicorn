// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __RAPICORN_SINFEX_HH__
#define __RAPICORN_SINFEX_HH__

#include <ui/utilities.hh>

namespace Rapicorn {

class Sinfex : public virtual ReferenceCountable, protected NonCopyable {
protected:
  uint          *m_start;
  explicit       Sinfex ();
  virtual       ~Sinfex ();
public:
  static Sinfex* parse_string (const String &expression);
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
    String   m_string;
    double   m_real;
    bool     m_strflag;
    String   real2string () const;
    double   string2real () const;
  public:
    bool     isreal      () const { return !m_strflag; }
    bool     isstring    () const { return m_strflag; }
    double   real        () const { return !m_strflag ? m_real : string2real(); }
    String   string      () const { return m_strflag ? m_string : real2string(); }
    bool     asbool      () const { return (!m_strflag && m_real) || (m_strflag && m_string != ""); }
    explicit Value       (double        d) : m_real (d), m_strflag (0) {}
    explicit Value       (const String &s);
  };
};

} // Rapicorn

#endif  /* __RAPICORN_SINFEX_HH__ */
