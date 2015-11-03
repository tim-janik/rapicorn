// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#ifndef __RAPICORN_VISITOR_HH__
#define __RAPICORN_VISITOR_HH__

#include <rcore/inifile.hh>
#include <rcore/xmlnode.hh>

namespace Rapicorn {

// == VisitorDispatcher (element type dispatcher) ==
/// Base template for Visitor classes, dispatches operator() to visit_<type>() methods.
template<class DerivedVisitor>
class VisitorDispatcher {
  ~VisitorDispatcher() {} // private dtor prevents undesirable inheritance
  friend DerivedVisitor;  // allow inheritance for DerivedVisitor only (CRTP)
  DerivedVisitor* derived () { return static_cast<DerivedVisitor*> (this); }
protected:
  typedef const char* Name; ///< Member name argument type for visit() methods.
public:
#ifdef    DOXYGEN
  /// The operator() is used to to visit all of a class's members, it dispatches to type dependent visit_*() methods.
  template<class A> void        operator() (A &a, Name n);
#endif // DOXYGEN

  // dispatch for calls like: visitor (field, "field");

  template<class A,
           REQUIRES< IsBool<A>::value > = true> void
  operator() (A &a, Name n)
  { return derived()->visit_bool (a, n); }

  template<class A,
           REQUIRES< (std::is_integral<A>::value && !IsBool<A>::value) > = true> void
  operator() (A &a, Name n)
  { return derived()->visit_integral (a, n); }

  template<class A,
           REQUIRES< std::is_floating_point<A>::value > = true> void
  operator() (A &a, Name n)
  { return derived()->visit_float (a, n); }

  template<class A,
           REQUIRES< std::is_enum<A>::value > = true> void
  operator() (A &a, Name n)
  { return derived()->visit_enum (a, n); }

  template<class A,
           REQUIRES< (!Has__accept__<A, DerivedVisitor>::value &&
                      DerivesString<A>::value) > = true> void
  operator() (A &a, Name n)
  { return derived()->visit_string (a, n); }

  template<class A,
           REQUIRES< (!Has__accept__<A, DerivedVisitor>::value &&
                      DerivesVector<A>::value) > = true> void
  operator() (A &a, Name n)
  { return derived()->visit_vector (a, n); }

  template<class A,
           REQUIRES< Has__accept__<A, DerivedVisitor>::value > = true> void
  operator() (A &a, Name n)
  { return derived()->visit_visitable (a, n); }

  template<class A,
           REQUIRES< (!Has__accept__<A, DerivedVisitor>::value &&
                      !DerivesString<A>::value &&
                      !DerivesVector<A>::value &&
                      std::is_class<A>::value) > = true> void
  operator() (A &a, Name n)
  { return derived()->visit_class (a, n); }

  // dispatch for calls like: visitor (*this, "property", &set_property, &get_property);

  template<class Klass, class Value,
           REQUIRES< IsBool<Value>::value > = true> void
  operator() (Klass &instance, Name n, void (Klass::*setter) (Value), Value (Klass::*getter) () const)
  {
    Value v = (instance .* getter) (), tmp = v;
    derived()->visit_bool (tmp, n);
    if (tmp != v)
      (instance .* setter) (tmp);
  }

  template<class Klass, class Value,
           REQUIRES< (std::is_integral<Value>::value && !IsBool<Value>::value) > = true> void
  operator() (Klass &instance, Name n, void (Klass::*setter) (Value), Value (Klass::*getter) () const)
  {
    Value v = (instance .* getter) (), tmp = v;
    derived()->visit_integral (tmp, n);
    if (tmp != v)
      (instance .* setter) (tmp);
  }

  template<class Klass, class Value,
           REQUIRES< std::is_floating_point<Value>::value > = true> void
  operator() (Klass &instance, Name n, void (Klass::*setter) (Value), Value (Klass::*getter) () const)
  {
    Value v = (instance .* getter) (), tmp = v;
    derived()->visit_float (tmp, n);
    if (tmp != v)
      (instance .* setter) (tmp);
  }

  template<class Klass, class Value,
           REQUIRES< std::is_enum<Value>::value > = true> void
  operator() (Klass &instance, Name n, void (Klass::*setter) (Value), Value (Klass::*getter) () const)
  {
    Value v = (instance .* getter) (), tmp = v;
    derived()->visit_enum (tmp, n);
    if (tmp != v)
      (instance .* setter) (tmp);
  }

  template<class Klass, class Value,
           REQUIRES< (!Has__accept_accessor__<Value, DerivedVisitor>::value &&
                      DerivesString<Value>::value) > = true> void
  operator() (Klass &instance, Name n, void (Klass::*setter) (const Value&), Value (Klass::*getter) () const)
  {
    Value v = (instance .* getter) (), tmp = v;
    derived()->visit_string (tmp, n);
    if (tmp != v)
      (instance .* setter) (tmp);
  }

  template<class Klass, class Value,
           REQUIRES< (!Has__accept_accessor__<Value, DerivedVisitor>::value &&
                      DerivesVector<Value>::value) > = true> void
  operator() (Klass &instance, Name n, void (Klass::*setter) (const Value&), Value (Klass::*getter) () const)
  {
    Value v = (instance .* getter) (), tmp = v;
    derived()->visit_vector (tmp, n);
    if (tmp != v)
      (instance .* setter) (tmp);
  }

  template<class Klass, class Value, // Value can be RemoteHandle
           REQUIRES< Has__accept_accessor__<Value, DerivedVisitor>::value > = true> void
  operator() (Klass &instance, Name n, void (Klass::*setter) (Value), Value (Klass::*getter) () const)
  {
    Value v = (instance .* getter) (), tmp = v;
    derived()->visit_accessor_visitable (tmp, n);
    if (tmp != v)
      (instance .* setter) (tmp);
  }

  template<class Klass, class Value, // Value can be Record
           REQUIRES< Has__accept__<Value, DerivedVisitor>::value > = true> void
  operator() (Klass &instance, Name n, void (Klass::*setter) (const Value&), Value (Klass::*getter) () const)
  {
    Value v = (instance .* getter) (), tmp = v;
    derived()->visit_visitable (tmp, n);
    if (tmp != v)
      (instance .* setter) (tmp);
  }

  template<class Klass, class Value,
           REQUIRES< (!Has__accept_accessor__<Value, DerivedVisitor>::value &&
                      !DerivesString<Value>::value &&
                      !DerivesVector<Value>::value &&
                      std::is_class<Value>::value) > = true> void
  operator() (Klass &instance, Name n, void (Klass::*setter) (Value), Value (Klass::*getter) () const)
  {
    Value v = (instance .* getter) (), tmp = v;
    derived()->visit_class (tmp, n);
    if (tmp != v)
      (instance .* setter) (tmp);
  }
};


// == StdVectorValueHandle ==
/// Special case handling for std::vector<> value references due to std::vector<bool> oddities.
template<class stdvector>
struct StdVectorValueHandle :
  ::std::false_type     /// StdVectorValueHandle<vector<ANYTYPE>>\::value == false
{                       /// is_reference<StdVectorValueHandle<vector<ANYTYPE>>\::type>\::value == true
  typedef typename stdvector::reference type;
};
/// Special case handling for std::vector<bool> elements which provide no bool& references.
template<>
struct StdVectorValueHandle<::std::vector<bool>> :
  ::std::true_type      /// StdVectorValueHandle<vector<bool>>\::value = true
{                       /// is_reference<StdVectorValueHandle<vector<bool>>\::type>\::value == false
  typedef typename ::std::vector<bool>::value_type type;
};


// == ToIniVisitor ==
/// Visitor to generate an INI file from a visitable class.
class ToIniVisitor : public VisitorDispatcher<ToIniVisitor> {
  IniWriter &iwriter_;
  String prefix_;
  String
  key (const char *n)
  {
    return string_format ("%s%s", prefix_, n);
  }
public:
  ToIniVisitor (IniWriter &iwriter, const String &prefix = "") : iwriter_ (iwriter), prefix_ (prefix) {}
  template<class A> void
  visit_bool (A &a, Name name)
  {
    iwriter_.set (key (name), a ? "True" : "False");
  }
  template<class A> void
  visit_integral (A &a, Name name)
  {
    if (a >= 65536)
      iwriter_.set (key (name), string_format ("0x%x", a));
    else
      iwriter_.set (key (name), string_format ("%d", a));
  }
  template<class A> void
  visit_float (A &a, Name name)
  {
    String value;
    if (std::is_same<float, typename std::remove_cv<A>::type>::value)
      value = string_from_float (a);
    else // double
      value = string_from_double (a);
    iwriter_.set (key (name), value);
  }
  template<class A> void
  visit_enum (A &a, Name name)
  {
    iwriter_.set (key (name), Rapicorn::Aida::enum_info<A>().value_to_string (a));
  }
  void
  visit_string (::std::string &a, Name name)
  {
    iwriter_.set (key (name), string_format ("%s", a.c_str()));
  }
  template<class A> void
  visit_vector (::std::vector<A> &a, Name name)
  {
    RAPICORN_RETURN_UNLESS (name);
    for (size_t i = 0; i < a.size(); i++)
      {
        const String nth = string_format ("%c%u", name[0], i);
        ToIniVisitor child_visitor (iwriter_, prefix_ + name + ".");
        typename StdVectorValueHandle<::std::vector<A>>::type value_handle = a[i];
        child_visitor (value_handle, nth.c_str());
        if (StdVectorValueHandle<::std::vector<A>>::value) // copy-by-value
          a[i] = value_handle;
      }
  }
  template<class A> void
  visit_visitable (A &a, Name name)
  {
    ToIniVisitor child_visitor (iwriter_, prefix_ + name + ".");
    a.__accept__ (child_visitor);
  }
  template<class A> void
  visit_accessor_visitable (A &a, Name name)
  {
    ToIniVisitor child_visitor (iwriter_, prefix_ + name + ".");
    a.__accept_accessor__ (child_visitor);
  }
  template<class A> void
  visit_class (A &a, Name name)
  {
    iwriter_.set (key (name), string_format ("- (opaque class)"));
  }
};


// == FromIniVisitor ==
/// Visitor to construct a visitable class from an INI file.
class FromIniVisitor : public VisitorDispatcher<FromIniVisitor> {
  IniFile &ifile_;
  String prefix_;
  String
  key (const char *n)
  {
    return string_format ("%s%s", prefix_, n);
  }
public:
  FromIniVisitor (IniFile &ifile, const String &prefix) : ifile_ (ifile), prefix_ (prefix) {}
  template<class A> void
  visit_bool (A &a, Name name)
  {
    String value;
    const bool hasattr = ifile_.has_value (key (name), &value);
    if (hasattr)
      a = string_to_bool (value);
  }
  template<class A> void
  visit_integral (A &a, Name name)
  {
    String value;
    const bool hasattr = ifile_.has_value (key (name), &value);
    if (hasattr)
      a = string_to_int (value);
  }
  template<class A> void
  visit_float (A &a, Name name)
  {
    String value;
    const bool hasattr = ifile_.has_value (key (name), &value);
    if (hasattr)
      a = string_to_double (value);
  }
  template<class A> void
  visit_enum (A &a, Name name)
  {
    String value;
    if (ifile_.has_value (key (name), &value))
      a = (A) Rapicorn::Aida::enum_info<A>().value_from_string (value);
  }
  void
  visit_string (::std::string &a, Name name)
  {
    String value;
    const bool hasattr = ifile_.has_value (key (name), &value);
    if (hasattr)
      a = value;
  }
  template<class A> void
  visit_vector (::std::vector<A> &a, Name name)
  {
    RAPICORN_RETURN_UNLESS (name);
    a.resize (0);
    for (size_t i = 0; ; i++)
      {
        const String nth = string_format ("%c%u", name[0], i);
        const String next_prefix = prefix_ + name + ".";
        const String next_element = next_prefix + nth;
        if (!ifile_.has_section (next_element) and !ifile_.has_value (next_element))
          break;
        a.resize (i + 1);
        FromIniVisitor child_visitor (ifile_, next_prefix);
        typename StdVectorValueHandle<::std::vector<A>>::type value_handle = a[i];
        child_visitor (value_handle, nth.c_str());
        if (StdVectorValueHandle<::std::vector<A>>::value) // copy-by-value
          a[i] = value_handle;
      }
  }
  template<class A> void
  visit_visitable (A &a, Name name)
  {
    const String next_prefix = prefix_ + name + ".";
    FromIniVisitor child_visitor (ifile_, next_prefix);
    a.__accept__ (child_visitor);
  }
  template<class A> void
  visit_accessor_visitable (A &a, Name name)
  {
    const String next_prefix = prefix_ + name + ".";
    FromIniVisitor child_visitor (ifile_, next_prefix);
    a.__accept_accessor__ (child_visitor);
  }
  template<class A> void
  visit_class (A &a, Name name)
  {
    // "[unvisitable-class]"
  }
};


// == ToXmlVisitor ==
/// Visitor to generate XML nodes from a visitable class.
class ToXmlVisitor : public VisitorDispatcher<ToXmlVisitor> {
  XmlNodeP xnode_;
public:
  explicit ToXmlVisitor (XmlNodeP xnode) : xnode_ (xnode) {}
  template<class A> void
  visit_bool (A &a, Name name)
  {
    xnode_->set_attribute (name, a ? "true" : "false");
  }
  template<class A> void
  visit_integral (A &a, Name name)
  {
    xnode_->set_attribute (name, string_format ("%d", a));
  }
  template<class A> void
  visit_float (A &a, Name name)
  {
    String value;
    if (std::is_same<float, typename std::remove_cv<A>::type>::value)
      value = string_from_float (a);
    else // double
      value = string_from_double (a);
    xnode_->set_attribute (name, value);
  }
  template<class A> void
  visit_enum (A &a, Name name)
  {
    String value = Rapicorn::Aida::enum_info<A>().value_to_string (a);
    xnode_->set_attribute (name, string_tolower (value));
  }
  void
  visit_string (::std::string &a, Name name)
  {
    if (0 && // disabled to handle sequence elements consistently
        a.size() <= 8)
      xnode_->set_attribute (name, string_to_cescape (a));
    else
      {
        XmlNodeP xelem = xnode_->create_child (name, 0, 0, "");
        XmlNodeP xtext = XmlNode::create_text (a, 0, 0, "");
        xelem->add_child (*xtext);
      }
  }
  template<class A> void
  visit_vector (::std::vector<A> &a, Name name)
  {
    RAPICORN_RETURN_UNLESS (name);
    XmlNodeP xseq = xnode_->create_child (name, 0, 0, "");
    for (size_t i = 0; i < a.size(); i++)
      {
        const String nth = string_format ("%c%u", name[0], i);
        ToXmlVisitor child_visitor (xseq);
        typename StdVectorValueHandle<::std::vector<A>>::type value_handle = a[i];
        child_visitor (value_handle, nth.c_str());
        if (StdVectorValueHandle<::std::vector<A>>::value) // copy-by-value
          a[i] = value_handle;
      }
  }
  template<class A> void
  visit_visitable (A &a, Name name)
  {
    XmlNodeP xchild = xnode_->create_child (name, 0, 0, "");
    ToXmlVisitor child_visitor (xchild);
    a.__accept__ (child_visitor);
  }
  template<class A> void
  visit_accessor_visitable (A &a, Name name)
  {
    XmlNodeP xchild = xnode_->create_child (name, 0, 0, "");
    ToXmlVisitor child_visitor (xchild);
    a.__accept_accessor__ (child_visitor);
  }
  template<class A> void
  visit_class (A &a, Name name)
  {
    xnode_->set_attribute (name, "[unvisitable-class]");
  }
};


// == FromXmlVisitor ==
/// Visitor to construct a visitable class from XML nodes.
class FromXmlVisitor : public VisitorDispatcher<FromXmlVisitor> {
  XmlNodeP xnode_;
public:
  explicit FromXmlVisitor (XmlNodeP xnode) : xnode_ (xnode) {}
  template<class A> void
  visit_bool (A &a, Name name)
  {
    String value;
    const bool hasattr = xnode_->has_attribute (name, false, &value);
    if (hasattr)
      a = string_to_bool (value);
  }
  template<class A> void
  visit_integral (A &a, Name name)
  {
    String value;
    const bool hasattr = xnode_->has_attribute (name, false, &value);
    if (hasattr)
      a = string_to_int (value);
  }
  template<class A> void
  visit_float (A &a, Name name)
  {
    String value;
    const bool hasattr = xnode_->has_attribute (name, false, &value);
    if (hasattr)
      a = string_to_double (value);
  }
  template<class A> void
  visit_enum (A &a, Name name)
  {
    String value;
    if (xnode_->has_attribute (name, false, &value))
      a = (A) Rapicorn::Aida::enum_info<A>().value_from_string (value);
  }
  void
  visit_string (::std::string &a, Name name)
  {
    String value;
    const bool hasattr = xnode_->has_attribute (name, false, &value);
    if (hasattr)
      a = value;
    else
      {
        const XmlNodeP xchild = xnode_->find_child (name);
        if (xchild)
          a = xchild->text();
      }
  }
  template<class A> void
  visit_vector (::std::vector<A> &a, Name name)
  {
    RAPICORN_RETURN_UNLESS (name);
    const XmlNodeP xseq = xnode_->find_child (name);
    RAPICORN_RETURN_UNLESS (xseq);
    a.resize (0);
    for (size_t i = 0; ; i++)
      {
        const String nth = string_format ("%c%u", name[0], i);
        if (xseq->has_attribute (nth, false) || xseq->find_child (nth))
          a.resize (i + 1);
        else
          break;
        FromXmlVisitor child_visitor (xseq);
        typename StdVectorValueHandle<::std::vector<A>>::type value_handle = a[i];
        child_visitor (value_handle, nth.c_str());
        if (StdVectorValueHandle<::std::vector<A>>::value) // copy-by-value
          a[i] = value_handle;
      }
  }
  template<class A> void
  visit_visitable (A &a, Name name)
  {
    const XmlNodeP xchild = xnode_->find_child (name);
    if (xchild)
      {
        FromXmlVisitor child_visitor (xchild);
        a.__accept__ (child_visitor);
      }
  }
  template<class A> void
  visit_accessor_visitable (A &a, Name name)
  {
    const XmlNodeP xchild = xnode_->find_child (name);
    if (xchild)
      {
        FromXmlVisitor child_visitor (xchild);
        a.__accept_accessor__ (child_visitor);
      }
  }
  template<class A> void
  visit_class (A &a, Name name)
  {
    // "[unvisitable-class]"
  }
};


// == FromAnyVisitors ==
/// Visitor to construct a visitable class from an Any::FieldVector.
class FromAnyFieldsVisitor {
  const Any::FieldVector &fields_;
public:
  explicit FromAnyFieldsVisitor (const Any::FieldVector &fields) : fields_ (fields) {}

  template<class A,
           REQUIRES< !Has__aida_from_any__<A>::value > = true> void
  operator() (A &a, const char *name)
  {
    for (size_t i = 0; i < fields_.size(); i++)
      if (fields_[i].name == name)
        {
          a = fields_[i].get<A>();
          break;
        }
  }

  template<class A,
           REQUIRES< Has__aida_from_any__<A>::value > = true> void
  operator() (A &a, const char *name)
  {
    for (size_t i = 0; i < fields_.size(); i++)
      if (fields_[i].name == name)
        {
          a.__aida_from_any__ (fields_[i]);
          break;
        }
  }
};

/// Visitor to construct an Any::AnyVector from a visitable class.
class FromAnyVectorVisitor {
  const Any::AnyVector &anys_;
  size_t                index_;
public:
  explicit FromAnyVectorVisitor (const Any::AnyVector &anys) : anys_ (anys), index_ (0) {}

  template<class A,
           REQUIRES< !Has__aida_from_any__<A>::value > = true> void
  operator() (A &a, const char *name)
  {
    if (index_ < anys_.size())
      {
        const size_t i = index_++;
        a = anys_[i].get<A>();
      }
  }

  template<class A,
           REQUIRES< Has__aida_from_any__<A>::value > = true> void
  operator() (A &a, const char *name)
  {
    if (index_ < anys_.size())
      {
        const size_t i = index_++;
        a.__aida_from_any__ (anys_[i]);
      }
  }
};


// == ToAnyVisitors ==
/// Visitor to construct an Any::FieldVector from a visitable class.
class ToAnyFieldsVisitor {
  Any::FieldVector &fields_;
public:
  explicit ToAnyFieldsVisitor (Any::FieldVector &fields) : fields_ (fields) {}

  template<class A,
           REQUIRES< !Has__aida_to_any__<A>::value > = true> void
  operator() (A &a, const char *name)
  {
    Any::Field f (name, Any (a));
    fields_.push_back (f);
  }

  template<class A,
           REQUIRES< Has__aida_to_any__<A>::value > = true> void
  operator() (A &a, const char *name)
  {
    Any::Field f (name, a.__aida_to_any__());
    fields_.push_back (f);
  }
};

/// Visitor to construct an Any::AnyVector from a visitable class.
class ToAnyVectorVisitor {
  Any::AnyVector &anys_;
public:
  explicit ToAnyVectorVisitor (Any::AnyVector &anys) : anys_ (anys) {}

  template<class A,
           REQUIRES< !Has__aida_to_any__<A>::value > = true> void
  operator() (A &a, const char *name)
  {
    anys_.push_back (Any (a));
  }

  template<class A,
           REQUIRES< Has__aida_to_any__<A>::value > = true> void
  operator() (A &a, const char *name)
  {
    anys_.push_back (a.__aida_to_any__());
  }
};


// == Any <-> Visitable ==
template<class Visitable> Any  any_from_visitable (Visitable &visitable);
template<class Visitable> void any_to_visitable   (const Any &any, Visitable &visitable);
template<class Vector>    Any  any_from_sequence  (Vector    &vector);
template<class Vector>    void any_to_sequence    (const Any &any, Vector    &vector);

template<class Visitable> Any
any_from_visitable (Visitable &visitable)
{
  Any::FieldVector fields;
  ToAnyFieldsVisitor visitor (fields);
  static_assert (Has__accept__<Visitable, ToAnyFieldsVisitor>::value, "Visitable provies __accept__");
  visitable.__accept__ (visitor);
  Any any;
  any.set (fields);
  return any;
}

template<class Visitable> void
any_to_visitable (const Any &any, Visitable &visitable)
{
  const Any::FieldVector &fields = any.get<const Any::FieldVector>();
  FromAnyFieldsVisitor visitor (fields);
  static_assert (Has__accept__<Visitable, FromAnyFieldsVisitor>::value, "Visitable provies __accept__");
  visitable.__accept__ (visitor);
}

template<class Vector> Any
any_from_sequence (Vector &vector)
{
  static_assert (DerivesVector<Vector>::value, "Vector derives std::vector");
  Any::AnyVector anys;
  ToAnyVectorVisitor visitor (anys);
  for (size_t i = 0; i < vector.size(); i++)
    {
      typedef StdVectorValueHandle< typename ::std::vector<typename Vector::value_type> > VectorValueHandle;
      typename VectorValueHandle::type value_handle = vector[i];
      visitor (value_handle, "element");
      if (VectorValueHandle::value) // copy-by-value
        vector[i] = value_handle;
    }
  Any any;
  any.set (anys);
  return any;
}

template<class Vector> void
any_to_sequence (const Any &any, Vector &vector)
{
  static_assert (DerivesVector<Vector>::value, "Vector derives std::vector");
  const Any::AnyVector &anys = any.get<const Any::AnyVector>();
  FromAnyVectorVisitor visitor (anys);
  vector.clear();
  vector.resize (anys.size());
  for (size_t i = 0; i < anys.size(); i++)
    {
      typedef StdVectorValueHandle< typename ::std::vector<typename Vector::value_type> > VectorValueHandle;
      typename VectorValueHandle::type value_handle = vector[i];
      visitor (value_handle, "element");
      if (VectorValueHandle::value) // copy-by-value
        vector[i] = value_handle;
    }
}

} // Rapicorn

#endif // __RAPICORN_VISITOR_HH__
