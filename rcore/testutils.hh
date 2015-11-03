// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#ifndef __RAPICORN_TESTUTILS_HH__
#define __RAPICORN_TESTUTILS_HH__

#include <rcore/rcore.hh>

namespace Rapicorn {

void init_core_test (const String &app_ident, int *argcp, char **argv, const StringVector &args = StringVector());

namespace Test {

// Test Macros
#define TTITLE(...)             Rapicorn::Test::test_format (3, __VA_ARGS__) ///< Print out the test program title.
#define TSTART(...)             Rapicorn::Test::test_format (4, __VA_ARGS__) ///< Print message once a test case starts.
#define TDONE()                 Rapicorn::Test::test_format (5, "%s", "")    ///< Print message for test case end.
#define TOUT(...)               Rapicorn::Test::test_format (0, __VA_ARGS__) ///< Test output for verbose mode, like fputs().
#define TMSG(...)               Rapicorn::Test::test_format (1, __VA_ARGS__) ///< Unconditional test message.
#define TINFO(...)              Rapicorn::Test::test_format (2, __VA_ARGS__) ///< Conditional test message (for verbose mode).
#define TWARN(...)              Rapicorn::Test::test_format (6, __VA_ARGS__) ///< Issue a non-fatal test warning.
#define TOK()                   do {} while (0)                 ///< Indicator for successful test progress.
#define TASSERT(cond)           TASSERT__AT (__LINE__, cond)    ///< Unconditional test assertion, enters breakpoint if not fullfilled.
#define TASSERT_AT(LINE, cond)  TASSERT__AT (LINE, cond)        ///< Unconditional test assertion for deputy __LINE__.
#define TCMP(a,cmp,b)           TCMP_op (a,cmp,b,#a,#b,)        ///< Compare @a a and @a b according to operator @a cmp.
#define TCMPS(a,cmp,b)          TCMP_op (a,cmp,b,#a,#b,Rapicorn::Test::_as_strptr) ///< Variant of TCMP() for C strings.

/// @cond
#define TASSERT__AT(LINE,cond)  do { if (RAPICORN_LIKELY (cond)) break; \
    Rapicorn::Test::assertion_failed (RAPICORN_PRETTY_FILE, LINE, #cond); } while (0)
#define TCMP_op(a,cmp,b,sa,sb,cast)  do { if (a cmp b) break;   \
  Rapicorn::String __tassert_va = Rapicorn::Test::stringify_arg (cast (a), #a); \
  Rapicorn::String __tassert_vb = Rapicorn::Test::stringify_arg (cast (b), #b), \
    __tassert_as = Rapicorn::string_format ("'%s %s %s': %s %s %s", \
                                            sa, #cmp, sb, __tassert_va.c_str(), #cmp, __tassert_vb.c_str()); \
  Rapicorn::Test::assertion_failed (RAPICORN_PRETTY_FILE, __LINE__, __tassert_as.c_str()); \
  } while (0)
/// @endcond

/** Class for profiling benchmark tests.
 * UseCase: Benchmarking function implementations, e.g. to compare sorting implementations.
 */
class Timer {
  const double   deadline_;
  vector<double> samples_;
  double         test_duration_;
  int64          n_runs_;
  int64          loops_needed ();
  void           reset        ();
  void           submit       (double elapsed, int64 repetitions);
  static double  bench_time   ();
public:
  /// Create a Timer() instance, specifying an optional upper bound for test durations.
  explicit       Timer        (double deadline_in_secs = 0);
  virtual       ~Timer        ();
  int64          n_runs       () const { return n_runs_; }             ///< Number of benchmark runs executed
  double         test_elapsed () const { return test_duration_; }      ///< Seconds spent in benchmark()
  double         min_elapsed  () const;         ///< Minimum time benchmarked for a @a callee() call.
  double         max_elapsed  () const;         ///< Maximum time benchmarked for a @a callee() call.
  template<typename Callee>
  double         benchmark    (Callee callee);
};

/**
 * @param callee        A callable function or object.
 * Method to benchmark the execution time of @a callee.
 * @returns Minimum runtime in seconds,
 */
template<typename Callee> double
Timer::benchmark (Callee callee)
{
  reset();
  for (int64 runs = loops_needed(); runs; runs = loops_needed())
    {
      int64 n = runs;
      const double start = bench_time();
      while (RAPICORN_LIKELY (n--))
        callee();
      const double stop = bench_time();
      submit (stop - start, runs);
    }
  return min_elapsed();
}

// === test maintenance ===
int     run                ();  ///< Run all registered tests.
bool    verbose            ();  ///< Indicates whether tests should run verbosely.
bool    normal             ();  ///< Indicates whether normal tests should be run.
bool    logging            ();  ///< Indicates whether logging tests should be run.
bool    slow               ();  ///< Indicates whether slow tests should be run.
bool    ui_test            ();  ///< Indicates execution of ui-thread tests.

void    set_assertion_hook (const std::function<void()> &hook);                 ///< Install hook tobe called when assertions fail.
void    assertion_failed   (const char *file, int line, const char *message);   ///< Internal function for failing assertions.

/// @cond
void                        add_internal  (const String &testname, void (*test_func) (void*), void *data);
void                        add           (const String &funcname, void (*test_func) (void));
template<typename D> void   add           (const String &testname, void (*test_func) (D*), D *data)
{ add_internal (testname, (void(*)(void*)) test_func, (void*) data); }
void                        test_output   (int kind, const String &string);
template<class... Args> RAPICORN_PRINTF (2, 0)
void                        test_format   (int kind, const char *format, const Args &...args)
{ test_output (kind, string_format (format, args...)); }
/// @endcond

/// == Stringify Args ==
inline String                   stringify_arg  (const char   *a, const char *str_a) { return a ? string_to_cquote (a) : "(__null)"; }
template<class V> inline String stringify_arg  (const V      *a, const char *str_a) { return string_format ("%p", a); }
template<class A> inline String stringify_arg  (const A      &a, const char *str_a) { return str_a; }
template<> inline String stringify_arg<float>  (const float  &a, const char *str_a) { return string_format ("%.8g", a); }
template<> inline String stringify_arg<double> (const double &a, const char *str_a) { return string_format ("%.17g", a); }
template<> inline String stringify_arg<bool>   (const bool   &a, const char *str_a) { return string_format ("%u", a); }
template<> inline String stringify_arg<int8>   (const int8   &a, const char *str_a) { return string_format ("%d", a); }
template<> inline String stringify_arg<int16>  (const int16  &a, const char *str_a) { return string_format ("%d", a); }
template<> inline String stringify_arg<int32>  (const int32  &a, const char *str_a) { return string_format ("%d", a); }
template<> inline String stringify_arg<int64>  (const int64  &a, const char *str_a) { return string_format ("%lld", a); }
template<> inline String stringify_arg<uint8>  (const uint8  &a, const char *str_a) { return string_format ("0x%02x", a); }
template<> inline String stringify_arg<uint16> (const uint16 &a, const char *str_a) { return string_format ("0x%04x", a); }
template<> inline String stringify_arg<uint32> (const uint32 &a, const char *str_a) { return string_format ("0x%08x", a); }
template<> inline String stringify_arg<uint64> (const uint64 &a, const char *str_a) { return string_format ("0x%08Lx", a); }
template<> inline String stringify_arg<String> (const String &a, const char *str_a) { return string_to_cquote (a); }
inline const char* _as_strptr (const char *s) { return s; } // implementation detail

class RegisterTest {
  static void add_test (char kind, const String &testname, void (*test_func) (void*), void *data);
public:
  RegisterTest (const char k, const String &testname, void (*test_func) (void))
  { add_test (k, testname, (void(*)(void*)) test_func, NULL); }
  RegisterTest (const char k, const String &testname, void (*test_func) (ptrdiff_t), ptrdiff_t data)
  { add_test (k, testname, (void(*)(void*)) test_func, (void*) data); }
  template<typename D>
  RegisterTest (const char k, const String &testname, void (*test_func) (D*), D *data)
  { add_test (k, testname, (void(*)(void*)) test_func, (void*) data); }
  typedef void (*TestTrigger)  (void (*runner) (void));
  static void test_set_trigger (TestTrigger func);
};

// == Deterministic random numbers for tests ===
uint64_t random_int64           ();                                     ///< Return random int for reproduceble tests.
int64_t  random_irange          (int64_t begin, int64_t end);           ///< Return random int within range for reproduceble tests.
double   random_float           ();                                     ///< Return random double for reproduceble tests.
double   random_frange          (double begin, double end);             ///< Return random double within range for reproduceble tests.

enum TrapFlags {
  TRAP_INHERIT_STDIN   = 1 << 0,
  TRAP_SILENCE_STDOUT  = 1 << 1,
  TRAP_SILENCE_STDERR  = 1 << 2,
  TRAP_NO_FATAL_SYSLOG = 1 << 3,
};

bool    trap_fork          (uint64 usec_timeout, uint test_trap_flags);
bool    trap_fork_silent   ();
bool    trap_timed_out     ();
bool    trap_passed        ();
bool    trap_aborted       ();
bool    trap_sigtrap       ();
String  trap_stdout        ();
String  trap_stderr        ();

/// Register a standard test function for execution as unit test.
#define REGISTER_TEST(name, ...)     static const Rapicorn::Test::RegisterTest \
  RAPICORN_CPP_PASTE2 (__Rapicorn_RegisterTest__line, __LINE__) ('t', name, __VA_ARGS__)

/// Register a slow test function for execution as during slow unit testing.
#define REGISTER_SLOWTEST(name, ...) static const Rapicorn::Test::RegisterTest \
  RAPICORN_CPP_PASTE2 (__Rapicorn_RegisterTest__line, __LINE__) ('s', name, __VA_ARGS__)

/// Register a logging test function for output recording and verification.
#define REGISTER_LOGTEST(name, ...) static const Rapicorn::Test::RegisterTest \
  RAPICORN_CPP_PASTE2 (__Rapicorn_RegisterTest__line, __LINE__) ('l', name, __VA_ARGS__)

enum ModeType {
  MODE_TESTING  = 0x1,  ///< Enable execution of test cases.
  MODE_VERBOSE  = 0x2,  ///< Enable extra verbosity during test runs.
  MODE_READOUT  = 0x4,  ///< Execute data driven tests to verify readouts according to a reference.
  MODE_SLOW     = 0x8,  ///< Allow tests to excercise slow code paths or loops.
};

} // Test
} // Rapicorn

#endif /* __RAPICORN_TESTUTILS_HH__ */
