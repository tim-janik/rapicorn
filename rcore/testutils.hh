/* Rapicorn
 * Copyright (C) 2008 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * A copy of the GNU Lesser General Public License should ship along
 * with this library; if not, see http://www.gnu.org/copyleft/.
 */
#ifndef __RAPICORN_TESTUTILS_HH__
#define __RAPICORN_TESTUTILS_HH__

#include <rcore/rcore.hh>

// Test Macros
#define TTITLE(...)             Rapicorn::Test::test_output (3, __VA_ARGS__)
#define TSTART(...)             Rapicorn::Test::test_output (4, __VA_ARGS__)
#define TDONE()                 Rapicorn::Test::test_output (5, "%s", "")
#define TEXIT()                 ({ exit (0); 0; })
#define TOUT(...)               Rapicorn::Test::test_output (0, __VA_ARGS__)
#define TMSG(...)               Rapicorn::Test::test_output (1, __VA_ARGS__)
#define TINFO(...)              Rapicorn::Test::test_output (2, __VA_ARGS__)
#define TWARN(...)              Rapicorn::Test::test_output (6, __VA_ARGS__)
#define TRUN(name, func)        ({ TSTART (name); func(); TDONE(); })
#define TRUN3(name, func, d)    ({ TSTART (name); func (d); TDONE(); })
#define TCMP(a,cmp,b)           TCMP_implf (a,cmp,b)
#define TCMPHEX(a,cmp,b)        TCMP_implx (a,cmp,b)
#define TCMPSIGNED(a,cmp,b)     TCMP_impls (a,cmp,b)
#define TCHECK(code)            TCHECK_impl (code)
#define TASSERT(code)           TCHECK_impl (code)
#define TOK()           do {} while (0) // printerr (".")
#define TCHECK_impl(code)       do { if (code) TOK(); else              \
      Rapicorn::Logging::message ("ASSERT", RAPICORN__FILE__, __LINE__, RAPICORN__FUNC__.c_str(), \
                                  "assertion failed: %s", #code);       \
  } while (0)
#define TCMP_implf(a,cmp,b)     do { if (a cmp b) TOK(); else { \
  double __tassert_va = a; double __tassert_vb = b;             \
  Rapicorn::Logging::message ("ASSERT", RAPICORN__FILE__, __LINE__, RAPICORN__FUNC__.c_str(), \
                              "assertion failed: %s %s %s: %.17g %s %.17g", \
                              #a, #cmp, #b, __tassert_va, #cmp, __tassert_vb); \
    } } while (0)
#define TCMP_implx(a,cmp,b)     do { if (a cmp b) TOK(); else { \
  uint64 __tassert_va = a; uint64 __tassert_vb = b;             \
  Rapicorn::Logging::message ("ASSERT", RAPICORN__FILE__, __LINE__, RAPICORN__FUNC__.c_str(), \
                              "assertion failed: %s %s %s: 0x%08Lx %s 0x%08Lx", \
                              #a, #cmp, #b, __tassert_va, #cmp, __tassert_vb); \
    } } while (0)
#define TCMP_impls(a,cmp,b)     do { if (a cmp b) TOK(); else { \
  int64 __tassert_va = a; int64 __tassert_vb = b;               \
  Rapicorn::Logging::message ("ASSERT", RAPICORN__FILE__, __LINE__, RAPICORN__FUNC__.c_str(), \
    "assertion failed: %s %s %s: %lld %s %lld",                         \
                              #a, #cmp, #b, __tassert_va, #cmp, __tassert_vb); \
    } } while (0)

namespace Rapicorn {

void    rapicorn_init_logtest (int *argc, char **argv);
void    rapicorn_init_test    (int *argc, char **argv);

namespace Test {

/**
 * Class for profiling benchmark tests.
 * UseCase: Benchmarking function implementations, e.g. to compare sorting implementations.
 */
class Timer {
  const double   m_deadline;
  vector<double> m_samples;
  double         m_test_duration;
  int64          m_n_runs;
  int64          loops_needed () const;
  void           reset        ();
  void           submit       (double elapsed, int64 repetitions);
  static double  bench_time   ();
public:
  /// Create a Timer() instance, specifying an optional upper bound for test durations.
  explicit       Timer        (double deadline_in_secs = 0);
  virtual       ~Timer        ();
  int64          n_runs       () const { return m_n_runs; }             ///< Number of benchmark runs executed
  double         test_elapsed () const { return m_test_duration; }      ///< Seconds spent in benchmark()
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

/* test maintenance */
int     run             (void);
bool    verbose         (void);
bool    quick           (void);
bool    slow            (void);
bool    thorough        (void);

void    test_output     (int kind, const char *format, ...) RAPICORN_PRINTF (2, 3);

void    add_internal    (const String &testname,
                         void        (*test_func) (void*),
                         void         *data);
void    add             (const String &funcname,
                         void (*test_func) (void));
template<typename D>
void    add             (const String &testname,
                         void        (*test_func) (D*),
                         D            *data)
{
  add_internal (testname, (void(*)(void*)) test_func, (void*) data);
}

struct RegisterTest { RegisterTest (const String &testname, void (*test_func) (void)) { add (testname, test_func); } };
#define REGISTER_TEST(name,func) static const Rapicorn::Test::RegisterTest \
  RAPICORN_CPP_PASTE2 (__Rapicorn_RegisterTest__line, __LINE__) (name,func)

/* random numbers */
char    rand_bit                (void);
int32   rand_int                (void);
int32   rand_int_range          (int32          begin,
                                 int32          end);
double  test_rand_double        (void);
double  test_rand_double_range  (double          range_start,
                                 double          range_end);

} // Test
} // Rapicorn

#endif /* __RAPICORN_TESTUTILS_HH__ */
