// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#include <rcore/testutils.hh>
using namespace Rapicorn;

namespace {
using namespace Rapicorn;

struct Foo { int i; bool operator== (const Foo &o) const { return i == o.i; } };
struct Bar { String s; };       // uncomparable

static void
test_any()
{
  Any a;
  try {
    any_cast<int> (a); // throws bad_cast
    assert_unreached();
  } catch (const std::bad_cast&) {
    ;
  }
  Any f7 (Foo { 7 });
  Any b (Bar { "BAR" });
  TASSERT (f7 == f7);
  TASSERT (f7 != b);
  TASSERT (b != b);     // always fails, beause B is not comparable
  TASSERT (any_cast<Foo> (f7).i == 7);
  TASSERT (any_cast<Bar> (&b)->s == "BAR");
  Any f3 (Foo { 3 });
  TASSERT (f3 == f3);
  TASSERT (f3 != f7);
  any_cast<Foo> (&f3)->i += 4;
  TASSERT (f3 == f7);
  std::swap (f7, b);
  TASSERT (f3 != f7);
  TASSERT (any_cast<Foo> (b).i == 7);
  TASSERT (any_cast<Bar> (f7).s == "BAR");
  const Any c (Foo { 5 });
  TASSERT (any_cast<Foo> (c) == Foo { 5 });
  TASSERT (*any_cast<Foo> (&c) == Foo { 5 });
}
REGISTER_TEST ("Aida/Any", test_any);

} // Anon
