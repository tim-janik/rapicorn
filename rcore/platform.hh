// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#ifndef __RAPICORN_CPU_HH__
#define __RAPICORN_CPU_HH__

#include <rcore/utilities.hh>
#include <rcore/randomhash.hh>

namespace Rapicorn {

String  cpu_info ();

/// Acquire information about a task (process or thread) at runtime.
struct TaskStatus {
  enum State { UNKNOWN = '?', RUNNING = 'R', SLEEPING = 'S', DISKWAIT = 'D', STOPPED = 'T', PAGING = 'W', ZOMBIE = 'Z', DEBUG = 'X', };
  int           process_id;     ///< Process ID.
  int           task_id;        ///< Process ID or thread ID.
  String        name;           ///< Thread name (set by user).
  State         state;          ///< Thread state.
  int           processor;      ///< Rrunning processor number.
  int           priority;       ///< Priority or nice value.
  uint64        utime;          ///< Userspace time.
  uint64        stime;          ///< System time.
  uint64        cutime;         ///< Userspace time of dead children.
  uint64        cstime;         ///< System time of dead children.
  uint64        ac_stamp;       ///< Accounting stamp.
  uint64        ac_utime, ac_stime, ac_cutime, ac_cstime;
  explicit      TaskStatus (int pid, int tid = -1); ///< Construct from process ID and optionally thread ID.
  bool          update     ();  ///< Update status information, might return false if called too frequently.
  String        string     ();  ///< Retrieve string representation of the status information.
};

/// Entropy gathering and provisioning class.
class Entropy {
  static KeccakPRNG& entropy_pool();
protected:
  static void     system_entropy  (KeccakPRNG &pool);
  static void     runtime_entropy (KeccakPRNG &pool);
public:
  /// Add up to 64 bits to entropy pool.
  static void     add_bits      (uint64_t bits)                 { add_data (&bits, sizeof (bits)); }
  /// Add an arbitrary number of bytes to the entropy pool.
  static void     add_data      (const void *bytes, size_t n_bytes);
  /// Gather system statistics (might take several milliseconds) and add to the entropy pool.
  static void     slow_reseed   ();
  /// Get 64 bit of high quality random bits useful for seeding PRNGs.
  static uint64_t get_seed      ();
  /// Fill the range [begin, end) with high quality random seed values.
  template<typename RandomAccessIterator> static void
  generate (RandomAccessIterator begin, RandomAccessIterator end)
  {
    typedef typename std::iterator_traits<RandomAccessIterator>::value_type Value;
    while (begin != end)
      {
        const uint64_t rbits = get_seed();
        *begin++ = Value (rbits);
        if (sizeof (Value) <= 4 && begin != end)
          *begin++ = Value (rbits >> 32);
      }
  }
};

} // Rapicorn

#endif /* __RAPICORN_CPU_HH__ */
