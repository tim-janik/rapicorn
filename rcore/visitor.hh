// This Source Code Form is licensed MPLv2: http://mozilla.org/MPL/2.0
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
    ssize_t v = a;
    if (v >= 65536)
      iwriter_.set (key (name), string_format ("0x%zx", v));
    else
      iwriter_.set (key (name), string_format ("%zd", v));
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
        const String nth = string_format ("%c%zu", name[0], i);
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
        const String nth = string_format ("%c%zu", name[0], i);
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
    xnode_->set_attribute (name, string_format ("%zd", ssize_t (a)));
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
        const String nth = string_format ("%c%zu", name[0], i);
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
        const String nth = string_format ("%c%zu", name[0], i);
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
  visit_class (A &a, Name name)
  {
    // "[unvisitable-class]"
  }
};

} // Rapicorn

#endif // __RAPICORN_VISITOR_HH__
