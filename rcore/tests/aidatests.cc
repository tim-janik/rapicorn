// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#include <rcore/testutils.hh>
using namespace Rapicorn;

namespace {
using namespace Rapicorn;
using namespace Rapicorn::Aida;

struct Foo { int i; bool operator== (const Foo &o) const { return i == o.i; } };
struct Bar { String s; };       // uncomparable
// Dummy handles for test purposes
struct TestOrbObject : OrbObject {
  TestOrbObject (ptrdiff_t x) : OrbObject (x) {}
};
struct OneHandle : RemoteHandle {
  OneHandle (OrbObjectP orbo) : RemoteHandle (orbo) {}
  static OneHandle down_cast (RemoteHandle smh) { return make_handle (smh.__aida_orbid__()); }
  static OneHandle make_handle (ptrdiff_t id)
  {
    std::shared_ptr<TestOrbObject> torbo = std::make_shared<TestOrbObject> (id);
    return OneHandle (torbo);
  }
};
class OneIface : public virtual ImplicitBase {
  int64 testid_;
public:
  virtual /*Des*/ ~OneIface () override     {}
  explicit         OneIface (int64 id) : testid_ (id) {}
  typedef std::shared_ptr<OneIface> OneIfaceP;
  // static Rapicorn::Aida::BaseConnection* __aida_connection__();
  virtual const Rapicorn::Aida::PropertyList& __aida_properties__ () override { return *(PropertyList*) NULL; }
  virtual std::string                    __aida_type_name__ () const override { return "Rapicorn::OneIface"; }
  virtual Rapicorn::Aida::TypeHashList   __aida_typelist__  () const override { return TypeHashList(); }
  virtual std::vector<String>            __aida_aux_data__  () const          { return std::vector<String>(); }
  virtual std::vector<String>            __aida_dir__       () const override                    { return std::vector<String>(); }
  virtual Any                            __aida_get__       (const String &name) const           { return Any(); }
  virtual bool                           __aida_set__       (const String &name, const Any &any) { return false; }
  int64 test_id() const { return testid_; }
  static OneIfaceP make_one_iface (int64 id)
  {
    typedef std::shared_ptr<OneIface> OneIfaceP;
    OneIfaceP oiface = std::make_shared<OneIface> (id);
    static std::vector<OneIfaceP> statics;
    statics.push_back (oiface);
    return oiface;
  }
};
typedef OneIface::OneIfaceP OneIfaceP;

static void
test_basics()
{
  const Aida::EnumInfo tke1 = Aida::enum_info<Aida::TypeKind>();
  const Aida::EnumInfo tke = tke1;
  TASSERT (not tke.name().empty() && tke.has_values());
  Aida::EnumValue ev = tke.find_value ("UNTYPED");
  TASSERT (ev.value == Aida::UNTYPED);
  ev = tke.find_value (Aida::STRING);
  TASSERT (ev.ident && String ("STRING") == ev.ident);
  TASSERT (type_kind_name (Aida::VOID) == String ("VOID"));

  // RemoteHandle
  assert (RemoteHandle::__aida_null_handle__() == NULL);
  assert (!RemoteHandle::__aida_null_handle__());
  assert (RemoteHandle::__aida_null_handle__().__aida_orbid__() == 0);
  std::shared_ptr<TestOrbObject> torbo = std::make_shared<TestOrbObject> (1);
  assert (OneHandle (torbo) != NULL);
  assert (OneHandle (torbo));
  assert (OneHandle (torbo).__aida_orbid__() == 1);
  assert (OneHandle (torbo).__aida_null_handle__() == NULL);
  assert (!OneHandle (torbo).__aida_null_handle__());
  assert (OneHandle (torbo).__aida_null_handle__().__aida_orbid__() == 0);
}
REGISTER_TEST ("Aida/Basics", test_basics);

static const double test_double_value = 7.76576e-306;
enum TestEnum { TEST_COFFEE_COFFEE = -0xc0ffeec0ffeeLL };

static void
any_test_set (Any &a, int what)
{
  switch (what)
    {
      typedef unsigned char uchar;
    case 0:  a.set (bool (0));                                     break;
    case 1:  a.set (bool (true));                                  break;
    case 2:  a.set (char (-117));                                  break;
    case 3:  a.set (uchar (250));                                  break;
    case 4:  a.set (int (-134217728));                             break;
    case 5:  a.set (uint (4294967295U));                           break;
    case 6:  a.set (long (-2147483648));                           break;
    case 7:  a.set (ulong (4294967295U));                          break;
    case 8:  a.set (int64_t (-0xc0ffeec0ffeeLL));                  break;
    case 9:  a.set (uint64_t (0xffffffffffffffffULL));             break;
    case 10: a.set ("Test4test");                                  break;
    case 11: a.set (test_double_value);                            break;
    case 12: { Any a2; a2.set ("SecondAny"); a.set (a2); }         break;
    case 13: a.set (TEST_COFFEE_COFFEE);                           break;
    case 14: a.set (OneHandle::make_handle (0x1f1f0c0c));          break;
    case 15: a.set (*OneIface::make_one_iface (0x2eeeabcd).get()); break;
    case 16: a.set (OneIface::make_one_iface (0x37afafaf));        break;
#define ANY_TEST_COUNT    17
    }
  // printerr ("SET: %d) Any=%p %i %s: %u\n", what, &a, a.kind(), type_kind_name(a.kind()), a.get<int64>());
}

static bool
any_test_get (const Any &a, int what)
{
  std::string s;
  EnumValue e;
  // printerr ("GET: %d) Any=%p %i %s: %u\n", what, &a, a.kind(), type_kind_name(a.kind()), a.get<int64>());
  OneHandle thandle (0);
  switch (what)
    {
      typedef unsigned char uchar;
      bool b; char c; uchar uc; int i; uint ui; long l; ulong ul; int64_t i6; uint64_t u6; double d;
    case 0:  b  = a.get<bool>();              assert (b == 0);                      break;
    case 1:  b  = a.get<bool>();              assert (b == true);                   break;
    case 2:  c  = a.get<char>();              assert (c == -117);                   break;
    case 3:  uc = a.get<uchar>();             assert (uc == 250);                   break;
    case 4:  i  = a.get<int>();               assert (i == -134217728);             break;
    case 5:  ui = a.get<uint>();              assert (ui == 4294967295U);           break;
    case 6:  l  = a.get<long>();              assert (l == -2147483648);            break;
    case 7:  ul = a.get<ulong>();             assert (ul == 4294967295U);           break;
    case 8:  i6 = a.get<int64_t>();           assert (i6 == -0xc0ffeec0ffeeLL);     break;
    case 9:  u6 = a.get<uint64_t>();          assert (u6 == 0xffffffffffffffffULL); break;
    case 10: s  = a.get<String>();            assert (s == "Test4test");            break;
    case 11: d  = a.get<double>();            assert (d = test_double_value);       break;
    case 12: s  = a.get<Any>().get<String>(); assert (s == "SecondAny");            break;
    case 13: i6 = a.get<TestEnum>();          assert (i6 == TEST_COFFEE_COFFEE);    break;
    case 14:
      assert (a.kind() == REMOTE);
      thandle = a.get<OneHandle>();
      assert (thandle.__aida_orbid__() == 0x1f1f0c0c);
      break;
    case 15:
      {
        assert (a.kind() == INSTANCE);
        OneIface &oiface = a.get<OneIface>();
        assert (oiface.test_id() == 0x2eeeabcd);
      }
      break;
    case 16:
      {
        assert (a.kind() == INSTANCE);
        OneIfaceP oiface = a.get<OneIfaceP>();
        assert (oiface && oiface->test_id() == 0x37afafaf);
      }
      break;
    }
  return true;
}

static Any any5 () { return Any(5); }

static void
test_any()
{
  {
    Any ax1 = any5();
    TASSERT (ax1 == any5());
    Any ax2 = std::move (any5());
    TASSERT (ax2 == any5());
  }
  if (true) // basics
    {
      Any f7 (Foo { 7 });
      TASSERT (f7 == f7);
      TASSERT (Foo { 7 } == f7.get<Foo>());
      Any b (Bar { "BAR" });
      TASSERT (f7 != b);
      TASSERT (b != b);     // always unequal, beause B is not comparable
      TASSERT (f7.get<Foo>() == Foo { 7 });
      TASSERT (f7.get<Foo>().i == 7);
      TASSERT (b.get<Bar>().s == "BAR");
      Any f3 (Foo { 3 });
      TASSERT (f3 == f3);
      TASSERT (f3 != f7);
      Foo fx = f3.get<Foo>();
      fx.i += 4;
      TASSERT (fx == f7.get<Foo>());
      f3.set (fx);
      TASSERT (f3 == f7);
      f7.swap (b);
      TASSERT (f3 != f7);
      std::swap (f7, b); // uses move assignment
      TASSERT (f3 == f7);
      TASSERT (f7.get<Foo>().i == 7);
      TASSERT (b.get<Bar>().s == "BAR");
      b.clear();
      TASSERT (b.get<Bar>().s == ""); // b is cleared, so get() yields a temporary Bar()
      const Any c (Foo { 5 });
      TASSERT (c.get<Foo>() == Foo { 5 });
      b.set (TEST_COFFEE_COFFEE);
      TASSERT (b.kind() == ENUM);
      TASSERT (b.get<TestEnum>() == TEST_COFFEE_COFFEE);
      printf ("  TEST   Aida basic functions                                            OK\n");
    }
  String s;
  const size_t cases = ANY_TEST_COUNT;
  for (size_t j = 0; j <= cases; j++)
    for (size_t k = 0; k <= cases; k++)
      {
        size_t cs[2] = { j, k };
        for (size_t cc = 0; cc < 2; cc++)
          {
            Any a;
            any_test_set (a, cs[cc]);
            const bool any_getter_successfull = any_test_get (a, cs[cc]);
            assert (any_getter_successfull == true);
            Any a2 (a);
            TASSERT (a2.kind() == a.kind());
            const bool any_copy_successfull = any_test_get (a2, cs[cc]);
            assert (any_copy_successfull == true);
            Any a3;
            a3 = a2;
            const bool any_assignment_successfull = any_test_get (a2, cs[cc]);
            assert (any_assignment_successfull == true);
          }
      }
  printf ("  TEST   Aida Any storage                                                OK\n");
  Any a;
  a.set (bool (0));         assert (a.kind() == BOOL && a.get<int64>() == 0);
  a.set (bool (1));         assert (a.kind() == BOOL && a.get<int64>() == 1);
  a.set (1.);               assert (a.kind() == FLOAT64 && a.get<double>() == +1.0);
  a.set (-1.);              assert (a.kind() == FLOAT64 && a.get<double>() == -1.0);
  a.set (16.5e+6);          assert (a.get<double>() > 16000000.0 && a.get<double>() < 17000000.0);
  a.set (1);                assert (a.kind() == INT64 && a.get<int64>() == 1 && a.get<double>() == 1);
  a.set (-1);               assert (a.kind() == INT64 && a.get<int64>() == -1 && a.get<double>() == -1);
  a.set (int64_t (1));      assert (a.kind() == INT64 && a.get<int64>() == 1 && a.get<double>() == 1);
  a.set (int64_t (-1));     assert (a.kind() == INT64 && a.get<int64>() == -1 && a.get<double>() == -1);
  a.set (0);                assert (a.kind() == INT64 && a.get<int64>() == 0 && a.get<double>() == 0);
  a.set (32767199);         assert (a.kind() == INT64 && a.get<int64>() == 32767199);
  a.set ("");               assert (a.kind() == STRING && a.get<String>() == "" && a.get<bool>() == 0);
  a.set ("f");              assert (a.kind() == STRING && a.get<String>() == "f" && a.get<bool>() == 1);
  a.set ("123456789");      assert (a.kind() == STRING && a.get<String>() == "123456789" && a.get<bool>() == 1);
  printf ("  TEST   Aida Any conversions                                            OK\n");
  Any b, c, d;
  a.set (-3);               assert (a != b); assert (!(a == b));  c.set (a); d.set (b); assert (c != d); assert (!(c == d));
  a.set (Any());            assert (a != b); assert (!(a == b));  c.set (a); d.set (b); assert (c != d); assert (!(c == d));
  a = Any();                assert (a == b); assert (!(a != b));  c.set (a); d.set (b); assert (c == d); assert (!(c != d));
  b.set (Any());            assert (a != b); assert (!(a == b));  c.set (a); d.set (b); assert (c != d); assert (!(c == d));
  a.set (Any());            assert (a == b); assert (!(a != b));  c.set (a); d.set (b); assert (c == d); assert (!(c != d));
  a.set (13);  b.set (13);  assert (a == b); assert (!(a != b));  c.set (a); d.set (b); assert (c == d); assert (!(c != d));
  a.set (14);  b.set (15);  assert (a != b); assert (!(a == b));  c.set (a); d.set (b); assert (c != d); assert (!(c == d));
  a.set ("1"); b.set ("1"); assert (a == b); assert (!(a != b));  c.set (a); d.set (b); assert (c == d); assert (!(c != d));
  a.set ("1"); b.set (1);   assert (a != b); assert (!(a == b));  c.set (a); d.set (b); assert (c != d); assert (!(c == d));
  a.set (1.4); b.set (1.5); assert (a != b); assert (!(a == b));  c.set (a); d.set (b); assert (c != d); assert (!(c == d));
  a.set (1.6); b.set (1.6); assert (a == b); assert (!(a != b));  c.set (a); d.set (b); assert (c == d); assert (!(c != d));
  printf ("  TEST   Aida Any equality                                               OK\n");
}
REGISTER_TEST ("Aida/Any", test_any);

static void
test_dynamics()
{
  // -- FieldVector --
  Any any1 ("any1"), any2;
  any2.set (any1);
  Any::FieldVector fv;
  fv.push_back (Any::Field ("otto", 7.7));
  fv.push_back (Any::Field ("anna", 3));
  fv.push_back (Any::Field ("ida", "ida"));
  fv.push_back (Any::Field ("any2", any2));
  assert (fv[0].name == "otto" && fv[0].get<double>() == 7.7);
  assert (fv[1].name == "anna" && fv[1].get<int64>() == 3);
  assert (fv[2].name == "ida" && fv[2].get<String>() == "ida");
  assert (fv[3].name == "any2" && fv[3].get<Any>().get<String>() == "any1");
  Any::FieldVector gv = fv;
  assert (fv == gv);
  gv[1].set (5);
  assert (fv != gv);
  gv[1].set (int64 (3));
  assert (fv == gv);
  // -- AnyVector --
  Any::AnyVector av;
  av.push_back (Any (7.7));
  av.push_back (Any (3));
  av.push_back (Any ("ida"));
  av.push_back (any2);
  assert (av[0].get<double>() == 7.7);
  assert (av[1].get<int64>() == 3);
  assert (av[2].get<String>() == "ida");
  assert (av[3].kind() == ANY);
  Any::AnyVector bv;
  assert (av != bv);
  for (auto const &f : fv)
    bv.push_back (f);
  if (0)
    for (size_t i = 0; i < av.size(); i++)
      printerr ("%s: av[%d]==bv[%d]) %p %p | %s %s | %d %d\n", __func__, i, i,
                &av[i], &bv[i],
                type_kind_name(av[i].kind()), type_kind_name(bv[i].kind()),
                av[i].get<int64>(), bv[i].get<int64>());
  assert (av == bv);
  // -- FieldVector & AnyVector --
  if (0)        // compare av (DynamicSequence) with fv (DynamicRecord)
    Rapicorn::printerr ("test-compare: %s == %s\n", Any (av).to_string().c_str(), Any (fv).to_string().c_str());
  Any::AnyVector cv (fv.begin(), fv.end());     // initialize AnyVector with { 7.7, 3, "ida" } from FieldVector (Field is-a Any)
  assert (av == cv);                            // as AnyVector (FieldVector) copy, both vectors contain { 7.7, 3, "ida" }
  // -- Any::DynamicSequence & Any::DynamicRecord --
  Any arec (fv), aseq (av);
  assert (arec != aseq);
  const Any::FieldVector *arv = arec.get<const Any::FieldVector*>();
  assert (*arv == fv);
  const Any::AnyVector *asv = aseq.get<const Any::AnyVector*>();
  assert (*asv == av);
  printf ("  TEST   Aida FieldVector & AnyVector                                    OK\n");
}
REGISTER_TEST ("Aida/Any Dynamics", test_dynamics);

template<class, class = void> struct has_complex_member : std::false_type {};      // false case, picked on SFINAE
template<class T> struct has_complex_member<T, void_t<typename T::complex_member>> : std::true_type {}; // !SFINAE

static void
test_cxxaux()
{
  class SimpleType {
    SimpleType () {}
    friend class FriendAllocator<SimpleType>;
  };
  std::shared_ptr<SimpleType> simple = FriendAllocator<SimpleType>::make_shared ();

  class ComplexType {
    ComplexType (int, double, void*) {}        // Private ctor.
    friend class FriendAllocator<ComplexType>; // Allow access to ctor/dtor of ComplexType.
  public: typedef int complex_member;
  };
  std::shared_ptr<ComplexType> complex = FriendAllocator<ComplexType>::make_shared (77, -0.5, (void*) 0);

  static_assert (has_complex_member<SimpleType>::value == false, "testing void_t");
  static_assert (has_complex_member<ComplexType>::value == true, "testing void_t");
  static_assert (IsSharedPtr<SimpleType>::value == false, "testing IsSharedPtr");
  static_assert (IsWeakPtr<std::shared_ptr<SimpleType> >::value == false, "testing IsWeakPtr");
  static_assert (IsSharedPtr<std::shared_ptr<SimpleType> >::value == true, "testing IsSharedPtr");
  static_assert (IsWeakPtr<std::weak_ptr<SimpleType> >::value == true, "testing IsWeakPtr");
  static_assert (IsSharedPtr<std::weak_ptr<SimpleType> >::value == false, "testing IsSharedPtr");
  static_assert (IsComparable<SimpleType>::value == false, "testing IsComparable");
  static_assert (IsComparable<FriendAllocator<SimpleType> >::value == true, "testing IsComparable");
  static_assert (IsComparable<int>::value == true, "testing IsComparable");
  static_assert (IsComparable<ComplexType>::value == false, "testing IsComparable");
}
REGISTER_TEST ("Aida/Cxx Auxillaries", test_cxxaux);

} // Anon

// intrusive BindableIface implementation
struct SimpleInt : BindableIface {
  int a;
  SimpleInt (int init) : a (init) {}
protected: // BindableIface methods
  virtual void bindable_get (const String &bpath, Any &any)
  {
    if (bindable_match (bpath, "a"))
      any.set (a);
  }
  virtual void bindable_set (const String &bpath, const Any &any)
  {
    if (bindable_match (bpath, "a"))
      a = any.get<int>();
  }
};

// non-intrusive BindableIface addon via bindable_accessor_set<> specialisation
struct SimpleString {   // non-BindableIface
  String b;
};

namespace Rapicorn {    // for template specialisation
template<> void
bindable_accessor_get (const BindableIface &bindable, const String &bpath, Any &any, SimpleString &s)
{
  if (bindable.bindable_match (bpath, "b"))
    any.set (s.b);
}
template<> void
bindable_accessor_set (const BindableIface &bindable, const String &bpath, const Any &any, SimpleString &s)
{
  if (bindable.bindable_match (bpath, "b"))
    s.b = any.get<String>();
}
} // Rapicorn           // for template specialisation

// non-intrusive BindableIface addon via BindableAdaptor<D> implementation
struct SimpleDouble {   // non-BindableIface
  double d;
};

namespace Rapicorn {    // for template specialisation
template<>
class BindableAdaptor<SimpleDouble> : virtual public BindableAdaptorBase {
protected:
  SimpleDouble &self_;
public:
  /*ctor*/ BindableAdaptor (SimpleDouble &d) : self_ (d) {}
protected: // BindableIface methods
  virtual void bindable_get (const String &bpath, Any &any)
  {
    if (bindable_match (bpath, "d"))
      any.set (self_.d);
  }
  virtual void bindable_set (const String &bpath, const Any &any)
  {
    if (bindable_match (bpath, "d"))
      self_.d = any.get<double>();
  }
};
} // Rapicorn           // for template specialisation

namespace { // Anon

static void
test_bindings()
{
  Any any;
  // check intrusive BindableIface implementation
  SimpleInt a { 49 };
  BinadableAccessor ba (a);
  any = ba.get_property ("a");
  TASSERT (any.get<int64>() == 49);

  // check non-intrusive BindableIface implementation for shared_ptr
  std::shared_ptr<SimpleString> bshared = std::make_shared<SimpleString> (SimpleString { "T2" });
  BinadableAccessor bb (bshared);
  any = bb.get_property ("b");
  TASSERT (any.get<String>() == "T2");

  // check non-intrusive BindableIface implementation for weak_ptr
  std::weak_ptr<SimpleString> cweak = bshared;
  BinadableAccessor bc (cweak);
  any = bc.get_property ("b");
  TASSERT (any.get<String>() == "T2");

  // check non-intrusive BindableIface implementation via BindableAdaptor<> specialisation
  SimpleDouble d { -0.5 };
  BinadableAccessor bd (d);
  any = bd.get_property ("d");
  TASSERT (any.get<double>() == -0.5);
}
REGISTER_TEST ("Aida/Bindings", test_bindings);

} // Anon
