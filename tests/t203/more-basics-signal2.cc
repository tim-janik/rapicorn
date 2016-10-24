// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
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
operator new (std::size_t size, const std::nothrow_t& /*nothrow_constant*/) noexcept
{
  void *p = malloc (size);
  printf ("SignalBench: new(%zd)\n", size);
  return p;
}
/* void* operator new (std::size_t size, void* ptr) noexcept; */
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
      accu += string_format ("Foo: %.2f\n", f + i + s.size());
      return true;
    }
  };
  static char
  float_callback (float f, int, std::string)
  {
    accu += string_format ("float: %.2f\n", f);
    return 0;
  }
public:
  static void
  run()
  {
    accu = "";
    Aida::Signal<char (float, int, std::string)> sig1;
    size_t id1 = sig1() += float_callback;
    size_t id2 = sig1() += [] (float, int i, std::string) { accu += string_format ("int: %d\n", i); return 0; };
    size_t id3 = sig1() += [] (float, int, const std::string &s) { accu += string_format ("string: %s\n", s.c_str()); return 0; };
    sig1.emit (.3, 4, "huhu");
    bool success;
    success = sig1() -= id1; assert (success == true);  success = sig1() -= id1; assert (success == false);
    success = sig1() -= id2; assert (success == true);  success = sig1() -= id3; assert (success == true);
    success = sig1() -= id3; assert (success == false); success = sig1() -= id2; assert (success == false);
    Foo foo;
    sig1() += Aida::slot (foo, &Foo::foo_bool);
    sig1() += Aida::slot (&foo, &Foo::foo_bool);
    sig1.emit (.5, 1, "12");

    Aida::Signal<void (std::string, int)> sig2;
    sig2() += [] (std::string msg, int) { accu += string_format ("msg: %s", msg.c_str()); };
    sig2() += [] (std::string, int d)   { accu += string_format (" *%d*\n", d); };
    sig2.emit ("in sig2", 17);

    accu += "DONE";

    const char *expected =
      "float: 0.30\n"
      "int: 4\n"
      "string: huhu\n"
      "Foo: 3.50\n"
      "Foo: 3.50\n"
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
    sig_vector() += handler777;
    sig_vector() += handler42;
    sig_vector() += handler1;
    sig_vector() += handler42;
    sig_vector() += handler777;
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
    sig_until0() += Aida::slot (self, &TestCollectorUntil0::handler_true);
    sig_until0() += Aida::slot (self, &TestCollectorUntil0::handler_false);
    sig_until0() += Aida::slot (self, &TestCollectorUntil0::handler_abort);
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
    sig_while0() += Aida::slot (self, &TestCollectorWhile0::handler_0);
    sig_while0() += Aida::slot (self, &TestCollectorWhile0::handler_1);
    sig_while0() += Aida::slot (self, &TestCollectorWhile0::handler_abort);
    TASSERT (!self.check1 && !self.check2);
    const bool result = sig_while0.emit();
    TASSERT (result == true && self.check1 && self.check2);
  }
};
REGISTER_TEST ("Signal/CollectorWhile0", TestCollectorWhile0::run);

static void
async_signal_tests()
{
  typedef Aida::AsyncSignal<String (const String&, String&, String, double, int)> StringTestSignal;
  String btag = "B";
  // empty emission
  StringTestSignal sig_string_test;
  StringTestSignal::Emission *emi = sig_string_test.emission ("", btag, "", 0.0, 0);
  TASSERT (emi->has_value() == false);
  TASSERT (emi->pending() == false);
  TASSERT (emi->dispatch() == false);
  TASSERT (emi->done() == true);
  delete emi;
  // add simple handler
  auto lambda1 =
    [] (const String &a, String &b, String c, double d, long l) -> String
    {
      return "1" + a + b + c + string_format ("%.0f%d", d, l);
    };
  sig_string_test() += (lambda1);
  // emission with handler
  emi = sig_string_test.emission ("A", btag, "C", -3.0, 9);
  TASSERT (emi->has_value() == false);
  TASSERT (emi->done() == false);
  TASSERT (emi->pending() == true);
  TASSERT (emi->has_value() == false);
  TASSERT (emi->dispatch() == true);          // value unreaped
  TASSERT (emi->has_value() == true);
  TASSERT (emi->get_value() == "1ABC-39");    // fetch and validate value
  TASSERT (emi->has_value() == false);
  TASSERT (emi->pending() == false);
  TASSERT (emi->done() == true);
  TASSERT (emi->dispatch() == false);
  delete emi;
  // add async handler with delay
  Mutex handler2_lock;
  auto lambda2 =
    [&handler2_lock] (const String &a, String &b, String c, double d, int i) -> std::future<String>
    { // need to turn (String &b) into (String b) copy for deferred execution
      auto lambda =
        [&handler2_lock] (const String &a, String b, String c, double d, int i) // -> String
        {
          std::this_thread::sleep_for (std::chrono::milliseconds (1)); // help race detection
          handler2_lock.lock(); handler2_lock.unlock();                // force proper synchronization
          return "2" + a + b + c + string_format ("%.0f%d", d, i);
        };
      return std::async (std::launch::async, lambda, a, b, c, d, i); // execute in seperate thread
    };
  sig_string_test().connect_future (lambda2);
  // emission with handler + async handler
  emi = sig_string_test.emission ("a", btag, "c", -5, 7);
  TASSERT (!emi->done() && !emi->has_value() && emi->pending());
  TASSERT (emi->dispatch() == true);
  TASSERT (!emi->done());
  TASSERT (emi->has_value() == true);         // first handler result
  TASSERT (emi->get_value() == "1aBc-57");    // fetch and validate value
  TASSERT (!emi->done() && !emi->has_value() && emi->pending());
  handler2_lock.lock();                       // block handler2
  TASSERT (emi->dispatch() == true);          // second handler started
  TASSERT (!emi->done());
  TASSERT (emi->has_value() == false);        // asynchronous execution blocked
  if (0) emi->get_value();                    // test deadlock...
  handler2_lock.unlock();                     // unblock handler2
  bool saved_pending = emi->pending();
  while (emi->has_value() == false)           // waiting for async handler2 completion
    {
      TASSERT (saved_pending == false);
      ThisThread::yield();                    // allow for async handler2 completion
      saved_pending = emi->pending();
    }
  TASSERT (emi->pending() == true);           // pending, since has_value() == true
  TASSERT (emi->get_value() == "2aBc-57");    // synchronize, fetch and validate value
  TASSERT (!emi->has_value() && !emi->pending() && emi->done());
  delete emi;
  // add handler with promise
  std::promise<String> result3;
  auto lambda3 =
    [&result3] (const String &a, String &b, String c, double d, int i) // -> std::future<String>
    {
      return result3.get_future();
    };
  sig_string_test().connect_future (lambda3);
  // emission with handler + async handler + promise handler
  emi = sig_string_test.emission ("_", btag, "_", -7, -6);
  TASSERT (!emi->done() && !emi->has_value() && emi->pending());
  TASSERT (emi->dispatch() && !emi->done() && emi->has_value());
  TASSERT (emi->get_value() == "1_B_-7-6");    // fetch and validate value
  TASSERT (!emi->done() && !emi->has_value() && emi->pending());
  TASSERT (emi->dispatch() && !emi->done());
  TASSERT (emi->get_value() == "2_B_-7-6");    // synchronize with handler2, fetch and validate value
  TASSERT (!emi->done() && !emi->has_value() && emi->pending());
  TASSERT (emi->dispatch() && !emi->done());
  TASSERT (emi->has_value() == false);        // promise remains unset
  result3.set_value ("future");
  TASSERT (emi->has_value() == true);         // promise set, future resolved
  TASSERT (emi->get_value() == "future");     // fetch and validate future
  TASSERT (!emi->has_value() && !emi->pending() && emi->done());
  delete emi;
  emi = NULL;
  // parallel and partial emissions
  StringTestSignal::Emission *emi1 = sig_string_test.emission ("(1)", btag, "x", -1, -1);
  StringTestSignal::Emission *emi2 = sig_string_test.emission ("(2)", btag, "x", -1, -1);
  StringTestSignal::Emission *emi3 = sig_string_test.emission ("(3)", btag, "x", -1, -1);
  TASSERT (emi1->dispatch() && emi2->dispatch() && emi3->dispatch());
  TASSERT (emi1->get_value() == "1(1)Bx-1-1" && emi2->get_value() == "1(2)Bx-1-1" && emi3->get_value() == "1(3)Bx-1-1");
  delete emi1;                                // aborting emission
  TASSERT (emi2->dispatch() && emi3->dispatch());
  TASSERT (emi2->get_value() == "2(2)Bx-1-1" && emi3->get_value() == "2(3)Bx-1-1");
  delete emi3;
  if (0) TASSERT (emi2->dispatch());          // avoid future_already_retrieved exception
  delete emi2;
}
REGISTER_TEST ("Signal/AsyncSignal tests", async_signal_tests);

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
  sig_increment() += test_counter_add2;
  szinfo ("old signal: after connect");
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
    printout ("SignalBench: Aida::Signal: %fns per emission (sz=%u)\n", size_t (benchdone - benchstart) * 1.0 / size_t (i),
              sizeof (sig_increment));
}
REGISTER_TEST ("Signal/SignalBench: Aida::Signal", bench_aida_signal);

static void
bench_async_signal()
{
  typedef Aida::AsyncSignal<void (void*, uint64)> TestSignal;
  szinfo ("asnyc signal: before init");
  TestSignal sig_increment;
  szinfo ("async signal: after init");
  sig_increment() += test_counter_add2;
  szinfo ("async signal: after connect");
  {
    TestSignal::Emission *tmp = sig_increment.emission (NULL, 1);
    szinfo ("async signal: after emission() creation");
    delete tmp;
  }
  const uint64 start_counter = TestCounter::get();
  const uint64 benchstart = timestamp_benchmark();
  uint64 i;
  for (i = 0; i < 999999; i++)
    {
      TestSignal::Emission *emi = sig_increment.emission (NULL, 1);
      emi->dispatch();
      emi->get_value();
      delete emi;
    }
  const uint64 benchdone = timestamp_benchmark();
  const uint64 end_counter = TestCounter::get();
  TASSERT (end_counter - start_counter == i);
  if (Test::verbose())
    printout ("SignalBench: AsyncSignal: %fns per emission (sz=%u)\n", size_t (benchdone - benchstart) * 1.0 / size_t (i),
              sizeof (sig_increment));
}
REGISTER_TEST ("Signal/SignalBench: AsyncSignal", bench_async_signal);

#if 0
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
    dummy.sig_increment() += test_counter_add2;
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
      printout ("SignalBench: Rapicorn::Signal: %fns per emission (sz=%u)\n", size_t (benchdone - benchstart) * 1.0 / size_t (i),
                sizeof (sig_increment));
  }
};
REGISTER_TEST ("Signal/SignalBench: Rapicorn::Signal", DummyObject::bench_old_signal);
#endif

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
