// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#include <rcore/testutils.hh>
#include <unistd.h>

using namespace Rapicorn;

#if 0 // operator new reporting
void*
operator new (std::size_t size) throw (std::bad_alloc)
{
  void *p = malloc (size);
  printf ("SignalBench: new(%zd)\n", size);
  return p;
}
void*
operator new (std::size_t size, const std::nothrow_t& /*nothrow_constant*/) throw()
{
  void *p = malloc (size);
  printf ("SignalBench: new(%zd)\n", size);
  return p;
}
/* void* operator new (std::size_t size, void* ptr) throw(); */
static int szinfo (const char *msg) { printf ("SignalBench: szinfo: %s\n", msg); return 0; }
#else
static int szinfo (const char *msg) { return 0; }
#endif
// #pragma GCC optimize ("O6")

struct TestCounter {
  static uint64 get     ();
  static void   set     (uint64);
  static void   add     (uint64);
  static void   add2    (void*, uint64);
};

namespace { // Anon
void        (*test_counter_add2) (void*, uint64) = TestCounter::add2; // external symbol to prevent easy inlining
static uint64 test_counter_var = 0;
} // Anon

class BasicSignalTests {
  static std::string accu;
  struct Foo {
    char
    foo_bool (float f, int i, std::string s)
    {
      accu += string_printf ("Foo: %.2f\n", f + i + s.size());
      return true;
    }
  };
  static char
  float_callback (float f, int, std::string)
  {
    accu += string_printf ("float: %.2f\n", f);
    return 0;
  }
public:
  static void
  run()
  {
    accu = "";
    Aida::Signal<char (float, int, std::string)> sig1;
    size_t id1 = sig1 += float_callback;
    size_t id2 = sig1 += [] (float, int i, std::string) { accu += string_printf ("int: %d\n", i); return 0; };
    size_t id3 = sig1 += [] (float, int, const std::string &s) { accu += string_printf ("string: %s\n", s.c_str()); return 0; };
    sig1.emit (.3, 4, "huhu");
    bool success;
    success = sig1 -= id1; assert (success == true);  success = sig1 -= id1; assert (success == false);
    success = sig1 -= id2; assert (success == true);  success = sig1 -= id3; assert (success == true);
    success = sig1 -= id3; assert (success == false); success = sig1 -= id2; assert (success == false);
    Foo foo;
    sig1 += Aida::slot (foo, &Foo::foo_bool);
    sig1 += Aida::slot (&foo, &Foo::foo_bool);
    sig1.emit (.5, 1, "12");

    Aida::Signal<void (std::string, int)> sig2;
    sig2 += [] (std::string msg, int) { accu += string_printf ("msg: %s", msg.c_str()); };
    sig2 += [] (std::string, int d)   { accu += string_printf (" *%d*\n", d); };
    sig2.emit ("in sig2", 17);

    accu += "DONE";

    const char *expected =
      "float: 0,30\n"
      "int: 4\n"
      "string: huhu\n"
      "Foo: 3,50\n"
      "Foo: 3,50\n"
      "msg: in sig2 *17*\n"
      "DONE";
    TCMP (accu, ==, expected);
  }
};
std::string BasicSignalTests::accu;
REGISTER_TEST ("Signal/Basic Tests", BasicSignalTests::run);


class TestCollectorVector {
  static int handler1   ()  { return 1; }
  static int handler42  ()  { return 42; }
  static int handler777 ()  { return 777; }
  public:
  static void
  run ()
  {
    Aida::Signal<int (), Aida::CollectorVector<int>> sig_vector;
    sig_vector += handler777;
    sig_vector += handler42;
    sig_vector += handler1;
    sig_vector += handler42;
    sig_vector += handler777;
    vector<int> results = sig_vector.emit();
    const int reference_array[] = { 777, 42, 1, 42, 777, };
    const vector<int> reference = vector_from_array (reference_array);
    TASSERT (results == reference);
  }
};
REGISTER_TEST ("Signal/CollectorVector", TestCollectorVector::run);

class TestCollectorUntil0 {
  bool check1, check2;
  TestCollectorUntil0() : check1 (0), check2 (0) {}
  bool handler_true  ()  { check1 = true; return true; }
  bool handler_false ()  { check2 = true; return false; }
  bool handler_abort ()  { abort(); }
  public:
  static void
  run ()
  {
    TestCollectorUntil0 self;
    Aida::Signal<bool (), Aida::CollectorUntil0<bool>> sig_until0;
    sig_until0 += Aida::slot (self, &TestCollectorUntil0::handler_true);
    sig_until0 += Aida::slot (self, &TestCollectorUntil0::handler_false);
    sig_until0 += Aida::slot (self, &TestCollectorUntil0::handler_abort);
    TASSERT (!self.check1 && !self.check2);
    const bool result = sig_until0.emit();
    TASSERT (!result && self.check1 && self.check2);
  }
};
REGISTER_TEST ("Signal/CollectorUntil0", TestCollectorUntil0::run);

class TestCollectorWhile0 {
  bool check1, check2;
  TestCollectorWhile0() : check1 (0), check2 (0) {}
  bool handler_0     ()  { check1 = true; return false; }
  bool handler_1     ()  { check2 = true; return true; }
  bool handler_abort ()  { abort(); }
  public:
  static void
  run ()
  {
    TestCollectorWhile0 self;
    Aida::Signal<bool (), Aida::CollectorWhile0<bool>> sig_while0;
    sig_while0 += Aida::slot (self, &TestCollectorWhile0::handler_0);
    sig_while0 += Aida::slot (self, &TestCollectorWhile0::handler_1);
    sig_while0 += Aida::slot (self, &TestCollectorWhile0::handler_abort);
    TASSERT (!self.check1 && !self.check2);
    const bool result = sig_while0.emit();
    TASSERT (result == true && self.check1 && self.check2);
  }
};
REGISTER_TEST ("Signal/CollectorWhile0", TestCollectorWhile0::run);

static void
bench_callback_loop()
{
  void (*counter_increment) (void*, uint64) = test_counter_add2;
  const uint64 start_counter = TestCounter::get();
  const uint64 benchstart = timestamp_benchmark();
  uint64 i;
  for (i = 0; i < 999999; i++)
    {
      counter_increment (NULL, 1);
    }
  const uint64 benchdone = timestamp_benchmark();
  const uint64 end_counter = TestCounter::get();
  assert (end_counter - start_counter == i);
  if (Test::verbose())
    printout ("SignalBench: callback loop: %fns per round\n", size_t (benchdone - benchstart) * 1.0 / size_t (i));
}
REGISTER_TEST ("Signal/SignalBench: callback loop", bench_callback_loop);

static void
bench_aida_signal()
{
  szinfo ("old signal: before init");
  Aida::Signal<void (void*, uint64)> sig_increment;
  szinfo ("old signal: after init");
  sig_increment += test_counter_add2;
  const uint64 start_counter = TestCounter::get();
  const uint64 benchstart = timestamp_benchmark();
  uint64 i;
  for (i = 0; i < 999999; i++)
    {
      sig_increment.emit (NULL, 1);
    }
  const uint64 benchdone = timestamp_benchmark();
  const uint64 end_counter = TestCounter::get();
  TASSERT (end_counter - start_counter == i);
  if (Test::verbose())
    printout ("SignalBench: Aida::Signal: %fns per emission (sz=%zu)\n", size_t (benchdone - benchstart) * 1.0 / size_t (i),
              sizeof (sig_increment));
}
REGISTER_TEST ("Signal/SignalBench: Aida::Signal", bench_aida_signal);

struct DummyObject {
  void  ref()   { /* dummy for signal emission */ }
  void  unref() { /* dummy for signal emission */ }
  int mark1;
  Rapicorn::Signal<DummyObject, void (void*, uint64)> sig_increment;
  int mark2;
  DummyObject() :
    mark1 (szinfo ("old signal: before init")),
    sig_increment (*this),
    mark2 (szinfo ("old signal: after init"))
  {}
  static void
  bench_old_signal()
  {
    DummyObject dummy;
    dummy.sig_increment += test_counter_add2;
    const uint64 start_counter = TestCounter::get();
    const uint64 benchstart = timestamp_benchmark();
    uint64 i;
    for (i = 0; i < 999999; i++)
      {
        dummy.sig_increment.emit (NULL, 1);
      }
    const uint64 benchdone = timestamp_benchmark();
    const uint64 end_counter = TestCounter::get();
    TASSERT (end_counter - start_counter == i);
    if (Test::verbose())
      printout ("SignalBench: Rapicorn::Signal: %fns per emission (sz=%zu)\n", size_t (benchdone - benchstart) * 1.0 / size_t (i),
                sizeof (sig_increment));
  }
};
REGISTER_TEST ("Signal/SignalBench: Rapicorn::Signal", DummyObject::bench_old_signal);

uint64
TestCounter::get ()
{
  return test_counter_var;
}

void
TestCounter::set (uint64 v)
{
  test_counter_var = v;
}

void
TestCounter::add (uint64 v)
{
  test_counter_var += v;
}

void
TestCounter::add2 (void*, uint64 v)
{
  test_counter_var += v;
}
