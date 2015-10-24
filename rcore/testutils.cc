// This Source Code Form is licensed MPLv2: http://mozilla.org/MPL/2.0
#include "testutils.hh"
#include "main.hh"
#include "strings.hh"

#include <algorithm>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>

#define TDEBUG(...)     RAPICORN_KEY_DEBUG ("Test", __VA_ARGS__)

namespace Rapicorn {

/** The Test namespace offers utilities for unit tests.
 * The Test namespace is made available by <code> \#include <rapicorn-test.hh> </code> <br/>
 * See also rcore/testutils.hh.
 */
namespace Test {

Timer::Timer (double deadline_in_secs) :
  deadline_ (deadline_in_secs), test_duration_ (0), n_runs_ (0)
{}

Timer::~Timer ()
{}

double
Timer::bench_time ()
{
  /* timestamp_benchmark() counts nano seconds since program start, so
   * it's not going to exceed the 52bit double mantissa too fast.
   */
  return timestamp_benchmark() / 1000000000.0;
}

int64
Timer::loops_needed ()
{
  if (samples_.size() < 7)
    {
      n_runs_ += 1;
      return n_runs_;           // force significant number of test runs
    }
  double resolution = timestamp_resolution() / 1000000000.0;
  const double deadline = MAX (deadline_ == 0.0 ? 0.005 : deadline_, resolution * 10000.0);
  if (test_duration_ < deadline * 0.1)
    {
      // we double the number of tests per run to gain more accuracy
      n_runs_ += n_runs_;
      return n_runs_;
    }
  if (test_duration_ < deadline)
    {
      // we increase the number of tests per run to gain more accuracy
      n_runs_ += 1;
      return n_runs_;
    }
  return 0; // time for testing exceeded
}

void
Timer::reset()
{
  samples_.resize (0);
  test_duration_ = 0;
  n_runs_ = 0;
}

void
Timer::submit (double elapsed, int64 repetitions)
{
  test_duration_ += elapsed;
  double resolution = timestamp_resolution() / 1000000000.0;
  if (elapsed >= resolution * 100.0) // force error below 1%
    samples_.push_back (elapsed / repetitions);
  else
    n_runs_ += repetitions; // double n_runs_ to yield significant times
}

double
Timer::min_elapsed () const
{
  double m = DBL_MAX;
  for (size_t i = 0; i < samples_.size(); i++)
    m = MIN (m, samples_[i]);
  return m;
}

double
Timer::max_elapsed () const
{
  double m = 0;
  for (size_t i = 0; i < samples_.size(); i++)
    m = MAX (m, samples_[i]);
  return m;
}

static String
ensure_newline (const String &s)
{
  if (s.size() && s[s.size()-1] != '\n')
    return s + "\n";
  return s;
}

static __thread char *test_warning = NULL;
static __thread char *test_start = NULL;

void
test_output (int kind, const String &output_msg)
{
  String msg = output_msg;
  String sout, bar;
  constexpr int VERBOSE_TAG = 1000000000;
  switch (verbose() ? VERBOSE_TAG + kind : kind)
    {
    default:
    case 0:                     // TOUT() - ignore when non-verbose
      break;
    case 0 + VERBOSE_TAG:       // TOUT() - literal output
      sout = msg;
      break;
    case 1:                     // TMSG() - unconditional test message
    case 1 + VERBOSE_TAG:
      sout = ensure_newline (msg);
      break;
    case 2:                     // TINFO() - ignore when non-verbose
      break;
    case 2 + VERBOSE_TAG:       // TINFO() - conditional test message
      sout = ensure_newline (msg);
      break;
    case 3:                     // test program title
      if (logging() && slow())
        msg += " (logging,slow)";
      else if (logging())
        msg += " (logging)";
      else if (slow())
        msg += " (slow)";
      sout = "START:   " + ensure_newline (msg);
      break;
    case 3 + VERBOSE_TAG:       // test program title
      bar = "### ** +--" + String (msg.size(), '-') + "--+ ** ###";
      msg = "### ** +  " + msg + "  + ** ###";
      sout = "\n" + bar + "\n" + msg + "\n" + bar + "\n\n";
      break;
    case 4:
    case 4 + VERBOSE_TAG:       // TSTART() - verbose test case start
      sout = "  TEST   " + msg + ":" + String (63 - MIN (63, msg.size()), ' ');
      if (!test_start)
        {
          test_start = strdup (sout.c_str());
          sout = "";
        }
      if (verbose())            // TSTART() - queue msg for later if non-verbose
        sout = "# Test:  " + msg + " ...\n";
      break;
    case 5:
    case 5 + VERBOSE_TAG:       // TDONE() - test case done, issue "OK"
      if (test_start)
        {
          sout = test_start;    // issue delayed TSTART message
          free (test_start);
          test_start = NULL;
        }
      else
        sout = "";
      if (test_warning)
        {
          String w (test_warning);
          free (test_warning);  // issue delayed TWARN message
          test_warning = NULL;
          sout += "WARN\n" + ensure_newline (w);
        }
      else
        sout += "OK\n";
      break;
    case 6:                     // TWARN() - queue test warning for later
      {
        String w;
        if (test_warning)
          {
            w = test_warning;
            free (test_warning);
          }
        test_warning = strdup ((w + ensure_newline (msg)).c_str());
      }
      break;
    case 6 + VERBOSE_TAG:       // TWARN() - immediate warning in verbose mode
      sout = "WARNING: " + ensure_newline (msg);
      break;
    }
  if (!sout.empty())            // actual output to stderr
    {
      fflush (stdout);
      fputs (sout.c_str(), stderr);
      fflush (stderr);
    }
}

static std::function<void()> assertion_hook;

void
set_assertion_hook (const std::function<void()> &hook)
{
  assertion_hook = hook;
}

void
assertion_failed (const char *file, int line, const char *message)
{
  String m;
  if (file)
    {
      m += String (file) + ":";
      if (line >= 0)
        m += string_format ("%u:", line);
    }
  else
    {
      const String pfile = program_file();
      if (!pfile.empty())
        m += pfile + ":";
    }
  m += " assertion failed: ";
  m += message;
  String sout = ensure_newline (m);
  fflush (stdout);
  fputs (sout.c_str(), stderr);
  fflush (stderr);

  if (assertion_hook)
    assertion_hook();

  return Rapicorn::breakpoint();
}

static vector<void(*)(void*)> testfuncs;
static vector<void*>          testdatas;
static vector<String>         testnames;

struct TestEntry {
  void          (*func) (void*);
  void           *data;
  String          name;
  char            kind;
  TestEntry      *next;
};

void
add_internal (const String &testname,
              void        (*test_func) (void*),
              void         *data)
{
  testnames.push_back (testname);
  testfuncs.push_back ((void(*)(void*)) test_func);
  testdatas.push_back ((void*) data);
}

void
add (const String &testname,
     void        (*test_func) (void))
{
  add (testname, (void(*)(void*)) test_func, (void*) 0);
}

static TestEntry *volatile test_entry_list = NULL;

void
RegisterTest::add_test (char kind, const String &testname, void (*test_func) (void*), void *data)
{
  TestEntry *te = new TestEntry;
  te->func = test_func;
  te->data = data;
  te->name = testname;
  te->kind = kind;
  do
    te->next = atomic_load (&test_entry_list);
  while (!atomic_bool_cas (&test_entry_list, te->next, te));
}

static bool flag_test_ui = false;

bool
verbose()
{
  return InitSettings::test_codes() & MODE_VERBOSE;
}

bool
logging()
{
  return InitSettings::test_codes() & MODE_READOUT;
}

bool
slow()
{
  return InitSettings::test_codes() & MODE_SLOW;
}

bool
normal ()
{
  return 0 == (InitSettings::test_codes() & (MODE_READOUT | MODE_SLOW));
}

bool
ui_test()
{
  return flag_test_ui;
}

static RegisterTest::TestTrigger test_trigger = NULL;

static int
test_entry_cmp (const TestEntry *const &v1,
                const TestEntry *const &v2)
{
  return strverscmp (v1->name.c_str(), v2->name.c_str()) < 0;
}

static void
run_tests (void)
{
  vector<TestEntry*> entries;
  for (TestEntry *node = atomic_load (&test_entry_list); node; node = node->next)
    entries.push_back (node);
  stable_sort (entries.begin(), entries.end(), test_entry_cmp);
  char ftype = logging() ? 'l' : (slow() ? 's' : 't');
  if (ui_test())
    ftype = toupper (ftype);
  TDEBUG ("running %u + %u tests", entries.size(), testfuncs.size());
  size_t skipped = 0, passed = 0;
  for (size_t i = 0; i < entries.size(); i++)
    {
      const TestEntry *te = entries[i];
      if (te->kind == ftype)
        {
          TSTART ("%s", te->name.c_str());
          te->func (te->data);
          TDONE();
          passed++;
        }
      else
        skipped++;
    }
  for (uint i = 0; i < testfuncs.size(); i++)
    {
      TSTART ("%s", testnames[i].c_str());
      testfuncs[i] (testdatas[i]);
      TDONE();
      passed++;
    }
  TDEBUG ("passed %u tests", passed);
  TDEBUG ("skipped deselected %u tests", skipped);
}

void
RegisterTest::test_set_trigger (TestTrigger func)
{
  test_trigger = func;
}

int
run (void)
{
  flag_test_ui = false;
  run_tests();
  flag_test_ui = true;
  if (test_trigger)
    test_trigger (run_tests);
  flag_test_ui = false;
  return 0;
}

uint64_t
random_int64 ()
{
  return Rapicorn::random_int64();
}

int64_t
random_irange (int64_t begin, int64_t end)
{
  return Rapicorn::random_irange (begin, end);
}

double
random_float ()
{
  return Rapicorn::random_float();
}

double
random_frange (double begin, double end)
{
  return Rapicorn::random_frange (begin, end);
}

} } // Rapicorn::Test

namespace Rapicorn {

/** Initialize the Rapicorn toolkit core for a test program.
 * Initialize the Rapicorn toolkit core to execute unit tests. Calling this function is
 * equivalent to calling init_core() with args "autonomous=1" and "rapicorn-test-initialization=1".
 * See also #$RAPICORN_TEST.
 */
void
init_core_test (const String &app_ident, int *argcp, char **argv, const StringVector &args)
{
  const char *ivalues[] = { "autonomous=1", "rapicorn-test-initialization=1" };
  StringVector targs = RAPICORN_STRING_VECTOR_FROM_ARRAY (ivalues);
  std::copy (args.begin(), args.end(), std::back_inserter (targs));
  init_core (app_ident, argcp, argv, targs);
}

} // Rapicorn

// Test Traps
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>

#define RETRY_ON_EINTR(expr)    ({ ssize_t __r; do __r = ssize_t (expr); while (__r < 0 && errno == EINTR); __r; })

namespace Rapicorn {
namespace Test {

static int    test_trap_forks = 0;
static int    test_trap_last_status = 0;
static int    test_trap_last_pid = 0;
static String test_trap_last_stdout;
static String test_trap_last_stderr;

static void
test_trap_clear()
{
  test_trap_last_status = 0;
  test_trap_last_pid = 0;
  test_trap_last_stdout = "";
  test_trap_last_stderr = "";
}

static inline ssize_t
string_must_read (String &string, int fd)
{
  ssize_t n_bytes;
  do
    {
      char buf[4096];
      n_bytes = read (fd, buf, sizeof (buf));
      if (n_bytes == 0)
        return 0; // EOF, calling this function assumes data is available
      else if (n_bytes > 0)
        {
          string.append (buf, n_bytes);
          return n_bytes;
        }
    }
  while (n_bytes < 0 && errno == EINTR);
  // n_bytes < 0
  critical ("failed to read() from child process (%d): %s", test_trap_last_pid, strerror (errno));
  return -1;
}

static inline void
string_must_write (const String &string, int outfd, size_t *stringpos)
{
  if (*stringpos < string.size())
    {
      ssize_t n = RETRY_ON_EINTR (write (outfd, string.data() + *stringpos, string.size() - *stringpos));
      *stringpos += MAX (n, 0);
    }
}

static int // 0 on success
kill_child (int pid, int *status, int patience)
{
  int wr;
  if (patience >= 3)    // try graceful reap
    {
      if (waitpid (pid, status, WNOHANG) > 0)
        return 0;
    }
  if (patience >= 2)    // try SIGHUP
    {
      kill (pid, SIGHUP);
      if (waitpid (pid, status, WNOHANG) > 0)
        return 0;
      usleep (20 * 1000); // give it some scheduling/shutdown time
      if (waitpid (pid, status, WNOHANG) > 0)
        return 0;
      usleep (50 * 1000); // give it some scheduling/shutdown time
      if (waitpid (pid, status, WNOHANG) > 0)
        return 0;
      usleep (100 * 1000); // give it some scheduling/shutdown time
      if (waitpid (pid, status, WNOHANG) > 0)
        return 0;
    }
  if (patience >= 1)    // try SIGTERM
    {
      kill (pid, SIGTERM);
      if (waitpid (pid, status, WNOHANG) > 0)
        return 0;
      usleep (200 * 1000); // give it some scheduling/shutdown time
      if (waitpid (pid, status, WNOHANG) > 0)
        return 0;
      usleep (400 * 1000); // give it some scheduling/shutdown time
      if (waitpid (pid, status, WNOHANG) > 0)
        return 0;
    }
  // finish it off
  kill (pid, SIGKILL);
  do
    wr = waitpid (pid, status, 0);
  while (wr < 0 && errno == EINTR);
  return wr;
}

bool
trap_fork (uint64 usec_timeout, uint test_trap_flags)
{
  int stdout_pipe[2] = { -1, -1 };
  int stderr_pipe[2] = { -1, -1 };
  test_trap_clear();
  if (pipe (stdout_pipe) < 0 || pipe (stderr_pipe) < 0)
    fatal ("failed to create pipes to fork test program: %s", strerror (errno));
  signal (SIGCHLD, SIG_DFL);
  test_trap_last_pid = fork ();
  if (test_trap_last_pid < 0)
    fatal ("failed to fork test program: %s", strerror (errno));
  if (test_trap_last_pid == 0)  // child
    {
      int fd0 = -1;
      close (stdout_pipe[0]);
      close (stderr_pipe[0]);
      if (!(test_trap_flags & TRAP_INHERIT_STDIN))
        fd0 = open ("/dev/null", O_RDONLY);
      if (RETRY_ON_EINTR (dup2 (stdout_pipe[1], 1)) < 0 ||
          RETRY_ON_EINTR (dup2 (stderr_pipe[1], 2)) < 0 ||
          (fd0 >= 0 && RETRY_ON_EINTR (dup2 (fd0, 0)) < 0))
        fatal ("failed to dup2() in forked test program: %s", strerror (errno));
      if (fd0 >= 3)
        close (fd0);
      if (stdout_pipe[1] >= 3)
        close (stdout_pipe[1]);
      if (stderr_pipe[1] >= 3)
        close (stderr_pipe[1]);
      if (test_trap_flags & TRAP_NO_FATAL_SYSLOG)
        debug_config_add ("fatal-syslog=0");
      return true;
    }
  else                          // parent
    {
      String sout, serr;
      size_t soutpos = 0, serrpos = 0, wr, need_wait = true;
      test_trap_forks++;
      close (stdout_pipe[1]);
      close (stderr_pipe[1]);
      uint64 sstamp = timestamp_realtime();
      // read data until we get EOF on all pipes
      while (stdout_pipe[0] >= 0 || stderr_pipe[0] >= 0)
        {
          fd_set fds;
          FD_ZERO (&fds);
          if (stdout_pipe[0] >= 0)
            FD_SET (stdout_pipe[0], &fds);
          if (stderr_pipe[0] >= 0)
            FD_SET (stderr_pipe[0], &fds);
          struct timeval tv;
          tv.tv_sec = 0;
          tv.tv_usec = MIN (usec_timeout ? usec_timeout : 1000000, 100 * 1000); // sleep at most 0.5 seconds to catch clock skews, etc.
          int ret = select (MAX (stdout_pipe[0], stderr_pipe[0]) + 1, &fds, NULL, NULL, &tv);
          if (ret < 0 && errno != EINTR)
            {
              critical ("Unexpected error in select() while reading from child process (%d): %s", test_trap_last_pid, strerror (errno));
              break;
            }
          if (stdout_pipe[0] >= 0 && FD_ISSET (stdout_pipe[0], &fds) && string_must_read (sout, stdout_pipe[0]) == 0)
            {
              close (stdout_pipe[0]);
              stdout_pipe[0] = -1;
            }
          if (stderr_pipe[0] >= 0 && FD_ISSET (stderr_pipe[0], &fds) && string_must_read (serr, stderr_pipe[0]) == 0)
            {
              close (stderr_pipe[0]);
              stderr_pipe[0] = -1;
            }
          if (!(test_trap_flags & TRAP_SILENCE_STDOUT))
            string_must_write (sout, 1, &soutpos);
          if (!(test_trap_flags & TRAP_SILENCE_STDERR))
            string_must_write (serr, 2, &serrpos);
          if (usec_timeout)
            {
              uint64 nstamp = timestamp_realtime();
              int status = 0;
              sstamp = MIN (sstamp, nstamp); // guard against backwards clock skews
              if (usec_timeout < nstamp - sstamp)
                {
                  // timeout reached, need to abort the child now
                  kill_child (test_trap_last_pid, &status, 3);
                  test_trap_last_status = 1024; // timeout
                  if (0 && WIFSIGNALED (status))
                    printerr ("%s: child timed out and received: %s\n", STRFUNC, strsignal (WTERMSIG (status)));
                  need_wait = false;
                  break;
                }
            }
        }
      close (stdout_pipe[0]);
      close (stderr_pipe[0]);
      if (need_wait)
        {
          int status = 0;
          do
            wr = waitpid (test_trap_last_pid, &status, 0);
          while (wr < 0 && errno == EINTR);
          if (WIFEXITED (status)) // normal exit
            test_trap_last_status = WEXITSTATUS (status); // 0..255
          else if (WIFSIGNALED (status))
            test_trap_last_status = (WTERMSIG (status) << 12); // signalled
          else // WCOREDUMP (status)
            test_trap_last_status = 512; // coredump
        }
      test_trap_last_stdout = sout;
      test_trap_last_stderr = serr;
      return false;
    }
}

bool
trap_fork_silent ()
{
  return trap_fork (50000000, TRAP_SILENCE_STDOUT | TRAP_SILENCE_STDERR | TRAP_NO_FATAL_SYSLOG);
}

bool
trap_timed_out ()
{
  assert_return (test_trap_last_pid != 0, false);
  return 0 != (test_trap_last_status & 1024); // timeout
}

bool
trap_passed ()
{
  assert_return (test_trap_last_pid != 0, false);
  return test_trap_last_status == 0;
}

bool
trap_aborted ()
{
  assert_return (test_trap_last_pid != 0, false);
  return (test_trap_last_status >> 12) == SIGABRT;
}

bool
trap_sigtrap ()
{
  assert_return (test_trap_last_pid != 0, false);
  return (test_trap_last_status >> 12) == SIGTRAP;
}

String
trap_stdout ()
{
  assert_return (test_trap_last_pid != 0, "");
  return test_trap_last_stdout;
}

String
trap_stderr ()
{
  assert_return (test_trap_last_pid != 0, "");
  return test_trap_last_stderr;
}

} // Test
} // Rapicorn
