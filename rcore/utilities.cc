// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#include "utilities.hh"
#include "inout.hh"
#include "main.hh"
#include "strings.hh"
#include "unicode.hh"
#include "thread.hh"
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/types.h>  // gettid
#include <cxxabi.h> // abi::__cxa_demangle
#include <signal.h>
#include <glib.h>
#include <vector>
#include <algorithm>
#include <cstring>
#include <execinfo.h>
#include <stdexcept>

namespace Rapicorn {

/* --- limits.h & float.h checks --- */
/* assert several assumptions the code makes */
RAPICORN_STATIC_ASSERT (CHAR_BIT     == +8);
RAPICORN_STATIC_ASSERT (SCHAR_MIN    == -128);
RAPICORN_STATIC_ASSERT (SCHAR_MAX    == +127);
RAPICORN_STATIC_ASSERT (UCHAR_MAX    == +255);
RAPICORN_STATIC_ASSERT (SHRT_MIN     == -32768);
RAPICORN_STATIC_ASSERT (SHRT_MAX     == +32767);
RAPICORN_STATIC_ASSERT (USHRT_MAX    == +65535);
RAPICORN_STATIC_ASSERT (INT_MIN      == -2147483647 - 1);
RAPICORN_STATIC_ASSERT (INT_MAX      == +2147483647);
RAPICORN_STATIC_ASSERT (UINT_MAX     == +4294967295U);
RAPICORN_STATIC_ASSERT (INT64_MIN    == -9223372036854775807LL - 1);
RAPICORN_STATIC_ASSERT (INT64_MAX    == +9223372036854775807LL);
RAPICORN_STATIC_ASSERT (UINT64_MAX   == +18446744073709551615LLU);
RAPICORN_STATIC_ASSERT (LDBL_MIN     <= 1E-37);
RAPICORN_STATIC_ASSERT (LDBL_MAX     >= 1E+37);
RAPICORN_STATIC_ASSERT (LDBL_EPSILON <= 1E-9);
RAPICORN_STARTUP_ASSERT (FLT_MIN      <= 1E-37);
RAPICORN_STARTUP_ASSERT (FLT_MAX      >= 1E+37);
RAPICORN_STARTUP_ASSERT (FLT_EPSILON  <= 1E-5);
RAPICORN_STARTUP_ASSERT (DBL_MIN      <= 1E-37);
RAPICORN_STARTUP_ASSERT (DBL_MAX      >= 1E+37);
RAPICORN_STARTUP_ASSERT (DBL_EPSILON  <= 1E-9);

/** Demangle a std::typeinfo.name() string into a proper C++ type name.
 * This function uses abi::__cxa_demangle() from <cxxabi.h> to demangle C++ type names,
 * which works for g++, libstdc++, clang++, libc++.
 */
String
cxx_demangle (const char *mangled_identifier)
{
  int status = 0;
  char *malloced_result = abi::__cxa_demangle (mangled_identifier, NULL, NULL, &status);
  String result = malloced_result && !status ? malloced_result : mangled_identifier;
  if (malloced_result)
    free (malloced_result);
  return result;
}

// == Timestamps ==
static clockid_t monotonic_clockid = CLOCK_REALTIME;
static uint64    monotonic_start = 0;
static uint64    monotonic_resolution = 1000;   // assume 1µs resolution for gettimeofday fallback
static uint64    realtime_start = 0;

static void
timestamp_init_ ()
{
  do_once
    {
      realtime_start = timestamp_realtime();
      struct timespec tp = { 0, 0 };
      if (clock_getres (CLOCK_REALTIME, &tp) >= 0)
        monotonic_resolution = tp.tv_sec * 1000000000ULL + tp.tv_nsec;
      uint64 mstart = realtime_start;
#ifdef CLOCK_MONOTONIC
      // CLOCK_MONOTONIC_RAW cannot slew, but doesn't measure SI seconds accurately
      // CLOCK_MONOTONIC may slew, but attempts to accurately measure SI seconds
      if (monotonic_clockid == CLOCK_REALTIME && clock_getres (CLOCK_MONOTONIC, &tp) >= 0)
        {
          monotonic_clockid = CLOCK_MONOTONIC;
          monotonic_resolution = tp.tv_sec * 1000000000ULL + tp.tv_nsec;
          mstart = timestamp_benchmark(); // here, monotonic_start=0 still
        }
#endif
      monotonic_start = mstart;
    }
}
namespace { static struct Timestamper { Timestamper() { timestamp_init_(); } } realtime_startup; } // Anon

/** Provides the timestamp_realtime() value from program startup. */
uint64
timestamp_startup ()
{
  timestamp_init_();
  return realtime_start;
}

/** Return the current time as uint64 in µseconds. */
uint64
timestamp_realtime ()
{
  struct timespec tp = { 0, 0 };
  if (ISLIKELY (clock_gettime (CLOCK_REALTIME, &tp) >= 0))
    return tp.tv_sec * 1000000ULL + tp.tv_nsec / 1000;
  else
    {
      struct timeval now = { 0, 0 };
      gettimeofday (&now, NULL);
      return now.tv_sec * 1000000ULL + now.tv_usec;
    }
}

/** Provides resolution of timestamp_benchmark() in nano-seconds. */
uint64
timestamp_resolution ()
{
  timestamp_init_();
  return monotonic_resolution;
}

/** Returns benchmark timestamp in nano-seconds, clock starts around program startup. */
uint64
timestamp_benchmark ()
{
  struct timespec tp = { 0, 0 };
  uint64 stamp;
  if (ISLIKELY (clock_gettime (monotonic_clockid, &tp) >= 0))
    {
      stamp = tp.tv_sec * 1000000000ULL + tp.tv_nsec;
      stamp -= monotonic_start;                 // reduce number of significant bits
    }
  else
    {
      stamp = timestamp_realtime() * 1000;
      stamp -= MIN (stamp, monotonic_start);    // reduce number of significant bits
    }
  return stamp;
}

String
timestamp_format (uint64 stamp)
{
  const size_t fieldwidth = 8;
  const String fsecs = string_format ("%u", size_t (stamp) / 1000000);
  const String usecs = string_format ("%06u", size_t (stamp) % 1000000);
  String r = fsecs;
  if (r.size() < fieldwidth)
    r += '.';
  if (r.size() < fieldwidth)
    r += usecs.substr (0, fieldwidth - r.size());
  return r;
}

// === Logging ===
String
pretty_file (const char *file_dir, const char *file)
{
  if (!file)
    return "<" + String ("???") + ">";     // cannot use macros here
  if (RAPICORN_IS_ABSPATH (file) || !file_dir || !file_dir[0])
    return file;
  return String (file_dir) + "/" + file;
}

#define BACKTRACE_DEPTH         1024

static std::vector<std::string>
pretty_backtrace_symbols (void **ptrbuffer, const int nptrs, const uint level)
{
  std::vector<std::string> symbols;
  char **btsymbols = backtrace_symbols (ptrbuffer, nptrs);
  if (btsymbols)
    {
      for (int i = 1 + level; i < nptrs; i++) // skip current function plus some
        {
          char dso[1234+1] = "???", fsym[1234+1] = "???"; size_t addr = size_t (ptrbuffer[i]);
          // parse: ./executable(_MangledFunction+0x00b33f) [0x00deadbeef]
          sscanf (btsymbols[i], "%1234[^(]", dso);
          sscanf (btsymbols[i], "%*[^(]%*[(]%1234[^)+-]", fsym);
          std::string func;
          if (fsym[0])
            {
              int status = 0;
              char *demang = abi::__cxa_demangle (fsym, NULL, NULL, &status);
              func = status == 0 && demang ? demang : fsym + std::string (" ()");
              if (demang)
                free (demang);
            }
          const char *dsof = strrchr (dso, '/');
          dsof = dsof ? dsof + 1 : dso;
          char buffer[2048] = { 0 };
          snprintf (buffer, sizeof (buffer), "0x%08zx %-26s %s", addr, dsof, func.c_str());
          symbols.push_back (buffer);
          if (strcmp (fsym, "main") == 0 || strcmp (fsym, "__libc_start_main") == 0)
            break;
        }
      free (btsymbols);
    }
  return symbols;
}

std::vector<std::string>
pretty_backtrace (uint level, size_t *parent_addr)
{
  void *ptrbuffer[BACKTRACE_DEPTH];
  const int nptrs = backtrace (ptrbuffer, ARRAY_SIZE (ptrbuffer));
  if (parent_addr)
    *parent_addr = nptrs >= 1 + int (level) ? size_t (ptrbuffer[1 + level]) : 0;
  std::vector<std::string> symbols = pretty_backtrace_symbols (ptrbuffer, nptrs, level);
  return symbols;
}

// == debug_backtrace_snapshot ==
struct BacktraceBuffer {
  void *ptrbuffer[BACKTRACE_DEPTH];
  int nptrs;
  BacktraceBuffer () :
    nptrs (0)
  {}
};
static Mutex                             backtrace_snapshot_mutex;
static std::map<size_t,BacktraceBuffer> *backtrace_snapshot_map = NULL;

void
debug_backtrace_snapshot (size_t key)
{
  BacktraceBuffer bbuffer;
  bbuffer.nptrs = backtrace (bbuffer.ptrbuffer, ARRAY_SIZE (bbuffer.ptrbuffer));
  ScopedLock<Mutex> locker (backtrace_snapshot_mutex);
  if (!backtrace_snapshot_map)
    backtrace_snapshot_map = new std::map<size_t,BacktraceBuffer>();
  (*backtrace_snapshot_map)[key] = bbuffer;
}

String
debug_backtrace_showshot (size_t key)
{
  BacktraceBuffer bbuffer;
  {
    ScopedLock<Mutex> locker (backtrace_snapshot_mutex);
    if (backtrace_snapshot_map)
      bbuffer = (*backtrace_snapshot_map)[key];
  }
  const vector<String> syms = pretty_backtrace_symbols (bbuffer.ptrbuffer, bbuffer.nptrs, 0);
  String btmsg;
  size_t addr = bbuffer.nptrs >= 1 ? size_t (bbuffer.ptrbuffer[1]) : 0;
  btmsg = string_format ("Backtrace at 0x%08x (stackframe at 0x%08x):\n", addr, size_t (__builtin_frame_address (0)));
  for (size_t i = 0; i < syms.size(); i++)
    btmsg += string_format ("  %s\n", syms[i].c_str());
  return btmsg;
}

// == Development Helpers ==
/**
 * @def RAPICORN_STRLOC()
 * Expand to a string literal, describing the current code location.
 * Returns a string describing the current source code location, such as FILE and LINE number.
 */

/**
 * @def RAPICORN_RETURN_IF(expr [, rvalue])
 * Return if @a expr evaluates to true.
 * Silently return @a rvalue if expression @a expr evaluates to true. Returns void if @a rvalue was not specified.
 */

/**
 * @def RAPICORN_RETURN_UNLESS(expr [, rvalue])
 * Return if @a expr is false.
 * Silently return @a rvalue if expression @a expr evaluates to false. Returns void if @a rvalue was not specified.
 */

/* --- process handle --- */
static struct {
  pid_t pid, ppid;
  uid_t uid;
  struct timeval tv;
  struct timezone tz;
  struct utsname uts;
} process_info = { 0, };
static bool
seed_buffer (vector<uint8> &randbuf)
{
  bool have_random = false;

  static const char *partial_files[] = { "/dev/urandom", "/dev/urandom$", };
  for (uint i = 0; i < ARRAY_SIZE (partial_files) && !have_random; i++)
    {
      FILE *file = fopen (partial_files[i], "r");
      if (file)
        {
          const uint rsz = 16;
          size_t sz = randbuf.size();
          randbuf.resize (randbuf.size() + rsz);
          have_random = rsz == fread (&randbuf[sz], 1, rsz, file);
          fclose (file);
        }
    }

  static const char *full_files[] = { "/proc/stat", "/proc/interrupts", "/proc/uptime", };
  for (uint i = 0; i < ARRAY_SIZE (full_files) && !have_random; i++)
    {
      size_t len = 0;
      char *mem = Path::memread (full_files[i], &len);
      if (mem)
        {
          randbuf.insert (randbuf.end(), mem, mem + len);
          free (mem);
          have_random = len > 0;
        }
    }

#ifdef __OpenBSD__
  if (!have_random)
    {
      randbuf.insert (randbuf.end(), arc4random());
      randbuf.insert (randbuf.end(), arc4random() >> 8);
      randbuf.insert (randbuf.end(), arc4random() >> 16);
      randbuf.insert (randbuf.end(), arc4random() >> 24);
      have_random = true;
    }
#endif

  return have_random;
}

static uint32 process_hash = 0;

static void
process_handle_and_seed_init (const StringVector &args)
{
  /* function should be called only once */
  assert (process_info.pid == 0);

  /* gather process info */
  process_info.pid = getpid();
  process_info.ppid = getppid();
  process_info.uid = getuid();
  gettimeofday (&process_info.tv, &process_info.tz);
  uname (&process_info.uts);

  /* gather random system entropy */
  vector<uint8> randbuf;
  randbuf.insert (randbuf.end(), (uint8*) &process_info, (uint8*) (&process_info + 1));

  /* mangle entropy into system specific hash */
  uint64 phash64 = 0;
  for (uint i = 0; i < randbuf.size(); i++)
    phash64 = (phash64 << 5) - phash64 + randbuf[i];
  process_hash = phash64 % 4294967291U;

  /* gather random seed entropy, this should include the entropy
   * from process_hash, but not be predictable from it.
   */
  seed_buffer (randbuf);

  /* mangle entropy into random seed */
  uint64 rand_seed = phash64;
  for (uint i = 0; i < randbuf.size(); i++)
    rand_seed = (rand_seed << 5) + rand_seed + randbuf[i];

  /* seed random generators */
  union { uint64 seed64; unsigned short us[3]; } u;
  u.seed64 = rand_seed % 0x1000000000015ULL;    // hash entropy into 48bits
  u.seed64 ^= u.seed64 << 48;                   // keep upper bits (us[0]) filled for big-endian
  srand48 (rand_seed); // will be overwritten by seed48() on sane systems
  seed48 (u.us);
  srand (lrand48());
}
static InitHook _process_handle_and_seed_init ("core/01 Init Process Handle and Seeds", process_handle_and_seed_init);

String
process_handle ()
{
  return string_format ("%s/%08x", process_info.uts.nodename, process_hash);
}

namespace Path {

/**
 * @param path  a filename path
 *
 * Return the directory part of a file name.
 */
String
dirname (const String &path)
{
  char *gdir = g_path_get_dirname (path.c_str());
  String dname = gdir;
  g_free (gdir);
  return dname;
}

/**
 * @param path  a filename path
 *
 * Strips all directory components from @a path and returns the resulting file name.
 */
String
basename (const String &path)
{
  char *gbase = g_path_get_basename (path.c_str());
  String bname = gbase;
  g_free (gbase);
  return bname;
}

/**
 * @param path a filename path
 *
 * Resolve links and directory references in @a path and provide
 * a canonicalized absolute pathname.
 */
String
realpath (const String &path)
{
  char *const cpath = ::realpath (path.c_str(), NULL);
  if (cpath)
    {
      const String result = cpath;
      free (cpath);
      errno = 0;
      return result;
    }
  // error case
  return path;
}

/**
 * @param path  a filename path
 * @param incwd optional current working directory
 *
 * Complete @a path to become an absolute file path. If neccessary, @a incwd or
 * the real current working directory is prepended.
 */
String
abspath (const String &path,
         const String &incwd)
{
  if (isabs (path))
    return path;
  if (!incwd.empty())
    return join (incwd, path);
  String pcwd = program_cwd();
  if (!pcwd.empty())
    return join (pcwd, path);
  return join (cwd(), path);
}

/**
 * @param path  a filename path
 *
 * Return wether @a path is an absolute pathname.
 */
bool
isabs (const String &path)
{
  return g_path_is_absolute (path.c_str());
}

/**
 * @param path  a filename path
 *
 * Return wether @a path is pointing to a directory component.
 */
bool
isdirname (const String &path)
{
  uint l = path.size();
  if (path == "." || path == "..")
    return true;
  if (l >= 1 && path[l-1] == RAPICORN_DIR_SEPARATOR)
    return true;
  if (l >= 2 && path[l-2] == RAPICORN_DIR_SEPARATOR && path[l-1] == '.')
    return true;
  if (l >= 3 && path[l-3] == RAPICORN_DIR_SEPARATOR && path[l-2] == '.' && path[l-1] == '.')
    return true;
  return false;
}

String
skip_root (const String &path)
{
  const char *frag = g_path_skip_root (path.c_str());
  return frag;
}

String
join (const String &frag0, const String &frag1,
      const String &frag2, const String &frag3,
      const String &frag4, const String &frag5,
      const String &frag6, const String &frag7,
      const String &frag8, const String &frag9,
      const String &frag10, const String &frag11,
      const String &frag12, const String &frag13,
      const String &frag14, const String &frag15)
{
  char *cpath = g_build_path (RAPICORN_DIR_SEPARATOR_S, frag0.c_str(),
                              frag1.c_str(), frag2.c_str(), frag3.c_str(), frag4.c_str(),
                              frag5.c_str(), frag6.c_str(), frag7.c_str(), frag8.c_str(),
                              frag9.c_str(), frag10.c_str(), frag11.c_str(), frag12.c_str(),
                              frag13.c_str(), frag14.c_str(), frag15.c_str(), NULL);
  String path (cpath);
  g_free (cpath);
  return path;
}

static int
errno_check_file (const char *file_name,
                  const char *mode)
{
  uint access_mask = 0, nac = 0;
  
  if (strchr (mode, 'e'))       /* exists */
    nac++, access_mask |= F_OK;
  if (strchr (mode, 'r'))       /* readable */
    nac++, access_mask |= R_OK;
  if (strchr (mode, 'w'))       /* writable */
    nac++, access_mask |= W_OK;
  bool check_exec = strchr (mode, 'x') != NULL;
  if (check_exec)               /* executable */
    nac++, access_mask |= X_OK;
  
  /* on some POSIX systems, X_OK may succeed for root without any
   * executable bits set, so we also check via stat() below.
   */
  if (nac && access (file_name, access_mask) < 0)
    return -errno;
  
  bool check_file = strchr (mode, 'f') != NULL;     /* open as file */
  bool check_dir  = strchr (mode, 'd') != NULL;     /* open as directory */
  bool check_link = strchr (mode, 'l') != NULL;     /* open as link */
  bool check_char = strchr (mode, 'c') != NULL;     /* open as character device */
  bool check_block = strchr (mode, 'b') != NULL;    /* open as block device */
  bool check_pipe = strchr (mode, 'p') != NULL;     /* open as pipe */
  bool check_socket = strchr (mode, 's') != NULL;   /* open as socket */
  
  if (check_exec || check_file || check_dir || check_link || check_char || check_block || check_pipe || check_socket)
    {
      struct stat st;
      
      if (check_link)
        {
          if (lstat (file_name, &st) < 0)
            return -errno;
        }
      else if (stat (file_name, &st) < 0)
        return -errno;
      
      if (0)
        g_printerr ("file-check(\"%s\",\"%s\"): %s%s%s%s%s%s%s\n",
                    file_name, mode,
                    S_ISREG (st.st_mode) ? "f" : "",
                    S_ISDIR (st.st_mode) ? "d" : "",
                    S_ISLNK (st.st_mode) ? "l" : "",
                    S_ISCHR (st.st_mode) ? "c" : "",
                    S_ISBLK (st.st_mode) ? "b" : "",
                    S_ISFIFO (st.st_mode) ? "p" : "",
                    S_ISSOCK (st.st_mode) ? "s" : "");
      
      if (S_ISDIR (st.st_mode) && (check_file || check_link || check_char || check_block || check_pipe))
        return -EISDIR;
      if (check_file && !S_ISREG (st.st_mode))
        return -EINVAL;
      if (check_dir && !S_ISDIR (st.st_mode))
        return -ENOTDIR;
      if (check_link && !S_ISLNK (st.st_mode))
        return -EINVAL;
      if (check_char && !S_ISCHR (st.st_mode))
        return -ENODEV;
      if (check_block && !S_ISBLK (st.st_mode))
        return -ENOTBLK;
      if (check_pipe && !S_ISFIFO (st.st_mode))
        return -ENXIO;
      if (check_socket && !S_ISSOCK (st.st_mode))
        return -ENOTSOCK;
      if (check_exec && !(st.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH)))
        return -EACCES; /* for root executable, any +x bit is good enough */
    }
  
  return 0;
}

/**
 * @param file  possibly relative filename
 * @param mode  feature string
 * @return      true if @a file adhears to @a mode
 *
 * Perform various checks on @a file and return whether all
 * checks passed. On failure, errno is set appropriately, and
 * FALSE is returned. Available features to be checked for are:
 * @li @c e - @a file must exist
 * @li @c r - @a file must be readable
 * @li @c w - @a file must be writable
 * @li @c x - @a file must be executable
 * @li @c f - @a file must be a regular file
 * @li @c d - @a file must be a directory
 * @li @c l - @a file must be a symbolic link
 * @li @c c - @a file must be a character device
 * @li @c b - @a file must be a block device
 * @li @c p - @a file must be a named pipe
 * @li @c s - @a file must be a socket.
 */
bool
check (const String &file,
       const String &mode)
{
  int err = file.size() && mode.size() ? errno_check_file (file.c_str(), mode.c_str()) : -EFAULT;
  errno = err < 0 ? -err : 0;
  return errno == 0;
}

/**
 * @param file1  possibly relative filename
 * @param file2  possibly relative filename
 * @return       TRUE if @a file1 and @a file2 are equal
 *
 * Check whether @a file1 and @a file2 are pointing to the same inode
 * in the same file system on the same device.
 */
bool
equals (const String &file1,
        const String &file2)
{
  if (!file1.size() || !file2.size())
    return file1.size() == file2.size();
  struct stat st1 = { 0, }, st2 = { 0, };
  int err1 = 0, err2 = 0;
  errno = 0;
  if (stat (file1.c_str(), &st1) < 0 && stat (file1.c_str(), &st1) < 0)
    err1 = errno;
  errno = 0;
  if (stat (file2.c_str(), &st2) < 0 && stat (file2.c_str(), &st2) < 0)
    err2 = errno;
  if (err1 || err2)
    return err1 == err2;
  return (st1.st_dev  == st2.st_dev &&
          st1.st_ino  == st2.st_ino &&
          st1.st_rdev == st2.st_rdev);
}

/// Return the current working directoy, including symlinks used in $PWD if available.
String
cwd ()
{
#ifdef  _GNU_SOURCE
  {
    char *dir = get_current_dir_name();
    if (dir)
      {
        const String result = dir;
        free (dir);
        return result;
      }
  }
#endif
  size_t size = 512;
  do
    {
      char *buf = (char*) malloc (size);
      if (!buf)
        break;
      const char *const dir = getcwd (buf, size);
      if (dir)
        {
          const String result = dir;
          free (buf);
          return result;
        }
      free (buf);
      size *= 2;
    }
  while (errno == ERANGE);
  // system must be in a bad shape if we get here...
  return "./";
}

StringVector
searchpath_split (const String &searchpath)
{
  StringVector sv;
  uint i, l = 0;
  for (i = 0; i < searchpath.size(); i++)
    if (searchpath[i] == RAPICORN_SEARCHPATH_SEPARATOR || // ':' or ';'
        searchpath[i] == ';') // make ';' work under windows and unix
      {
        if (i > l)
          sv.push_back (searchpath.substr (l, i - l));
        l = i + 1;
      }
  if (i > l)
    sv.push_back (searchpath.substr (l, i - l));
  return sv;
}

String
searchpath_find (const String &searchpath, const String &file, const String &mode)
{
  if (isabs (file))
    return check (file, mode) ? file : "";
  StringVector sv = searchpath_split (searchpath);
  for (size_t i = 0; i < sv.size(); i++)
    if (check (join (sv[i], file), mode))
      return join (sv[i], file);
  return "";
}

static String
searchpath_join1 (const String &a, const String &b)
{
  if (a.empty())
    return b;
  if (b.empty())
    return a;
  if (a[a.size()-1] == RAPICORN_SEARCHPATH_SEPARATOR ||
      b[0] == RAPICORN_SEARCHPATH_SEPARATOR)
    return a + b;
  return a + RAPICORN_SEARCHPATH_SEPARATOR_S + b;
}

String
searchpath_join (const String &frag0, const String &frag1, const String &frag2, const String &frag3,
                 const String &frag4, const String &frag5, const String &frag6, const String &frag7,
                 const String &frag8, const String &frag9, const String &frag10, const String &frag11,
                 const String &frag12, const String &frag13, const String &frag14, const String &frag15)
{
  String result = searchpath_join1 (frag0, frag1);
  result = searchpath_join1 (result, frag2);
  result = searchpath_join1 (result, frag3);
  result = searchpath_join1 (result, frag4);
  result = searchpath_join1 (result, frag5);
  result = searchpath_join1 (result, frag6);
  result = searchpath_join1 (result, frag7);
  result = searchpath_join1 (result, frag8);
  result = searchpath_join1 (result, frag9);
  result = searchpath_join1 (result, frag10);
  result = searchpath_join1 (result, frag11);
  result = searchpath_join1 (result, frag12);
  result = searchpath_join1 (result, frag13);
  result = searchpath_join1 (result, frag14);
  result = searchpath_join1 (result, frag15);
  return result;
}

String
vpath_find (const String &file, const String &mode)
{
  String result = searchpath_find (".", file, mode);
  if (!result.empty())
    return result;
  const char *vpath = getenv ("VPATH");
  if (vpath)
    {
      result = searchpath_find (vpath, file, mode);
      if (!result.empty())
        return result;
    }
  return file;
}

const String dir_separator = RAPICORN_DIR_SEPARATOR_S;
const String searchpath_separator = RAPICORN_SEARCHPATH_SEPARATOR_S;

static char* /* return malloc()-ed buffer containing a full read of FILE */
file_memread (FILE   *stream,
              size_t *lengthp)
{
  size_t sz = 256;
  char *malloc_string = (char*) malloc (sz);
  if (!malloc_string)
    return NULL;
  char *start = malloc_string;
  errno = 0;
  while (!feof (stream))
    {
      size_t bytes = fread (start, 1, sz - (start - malloc_string), stream);
      if (bytes <= 0 && ferror (stream) && errno != EAGAIN)
        {
          start = malloc_string; // error/0-data
          break;
        }
      start += bytes;
      if (start == malloc_string + sz)
        {
          bytes = start - malloc_string;
          sz *= 2;
          char *newstring = (char*) realloc (malloc_string, sz);
          if (!newstring)
            {
              start = malloc_string; // error/0-data
              break;
            }
          malloc_string = newstring;
          start = malloc_string + bytes;
        }
    }
  int savederr = errno;
  *lengthp = start - malloc_string;
  if (!*lengthp)
    {
      free (malloc_string);
      malloc_string = NULL;
    }
  errno = savederr;
  return malloc_string;
}

char*
memread (const String &filename,
         size_t       *lengthp)
{
  FILE *file = fopen (filename.c_str(), "r");
  if (!file)
    {
      *lengthp = 0;
      return strdup ("");
    }
  char *contents = file_memread (file, lengthp);
  int savederr = errno;
  fclose (file);
  errno = savederr;
  return contents;
}

void
memfree (char *memread_mem)
{
  if (memread_mem)
    free (memread_mem);
}

bool
memwrite (const String &filename, size_t len, const uint8 *bytes)
{
  FILE *file = fopen (filename.c_str(), "w");
  if (!file)
    return false;
  const size_t nbytes = fwrite (bytes, 1, len, file);
  bool success = ferror (file) == 0 && nbytes == len;
  success = fclose (file) == 0 && success;
  if (!success)
    unlink (filename.c_str());
  return success;
}

} // Path

/* --- DataList --- */
DataList::NodeBase::~NodeBase ()
{}

void
DataList::set_data (NodeBase *node)
{
  /* delete old node */
  NodeBase *it = rip_data (node->key);
  if (it)
    delete it;
  /* prepend node */
  node->next = nodes;
  nodes = node;
}

DataList::NodeBase*
DataList::get_data (DataKey<void> *key) const
{
  NodeBase *it;
  for (it = nodes; it; it = it->next)
    if (it->key == key)
      return it;
  return NULL;
}

DataList::NodeBase*
DataList::rip_data (DataKey<void> *key)
{
  NodeBase *last = NULL, *it;
  for (it = nodes; it; last = it, it = last->next)
    if (it->key == key)
      {
        /* unlink existing node */
        if (last)
          last->next = it->next;
        else
          nodes = it->next;
        it->next = NULL;
        return it;
      }
  return NULL;
}

void
DataList::clear_like_destructor()
{
  while (nodes)
    {
      NodeBase *it = nodes;
      nodes = it->next;
      it->next = NULL;
      delete it;
    }
}

DataList::~DataList()
{
  clear_like_destructor();
}

/* --- url handling --- */
bool
url_show (const char *url)
{
  static struct {
    const char   *prg, *arg1, *prefix, *postfix;
    bool          asyncronous; /* start asyncronously and check exit code to catch launch errors */
    volatile bool disabled;
  } www_browsers[] = {
    /* program */               /* arg1 */      /* prefix+URL+postfix */
    /* configurable, working browser launchers */
    { "gnome-open",             NULL,           "", "", 0 }, /* opens in background, correct exit_code */
    { "exo-open",               NULL,           "", "", 0 }, /* opens in background, correct exit_code */
    /* non-configurable working browser launchers */
    { "kfmclient",              "openURL",      "", "", 0 }, /* opens in background, correct exit_code */
    { "gnome-moz-remote",       "--newwin",     "", "", 0 }, /* opens in background, correct exit_code */
#if 0
    /* broken/unpredictable browser launchers */
    { "browser-config",         NULL,            "", "", 0 }, /* opens in background (+ sleep 5), broken exit_code (always 0) */
    { "xdg-open",               NULL,            "", "", 0 }, /* opens in foreground (first browser) or background, correct exit_code */
    { "sensible-browser",       NULL,            "", "", 0 }, /* opens in foreground (first browser) or background, correct exit_code */
    { "htmlview",               NULL,            "", "", 0 }, /* opens in foreground (first browser) or background, correct exit_code */
#endif
    /* direct browser invocation */
    { "x-www-browser",          NULL,           "", "", 1 }, /* opens in foreground, browser alias */
    { "firefox",                NULL,           "", "", 1 }, /* opens in foreground, correct exit_code */
    { "mozilla-firefox",        NULL,           "", "", 1 }, /* opens in foreground, correct exit_code */
    { "mozilla",                NULL,           "", "", 1 }, /* opens in foreground, correct exit_code */
    { "konqueror",              NULL,           "", "", 1 }, /* opens in foreground, correct exit_code */
    { "opera",                  "-newwindow",   "", "", 1 }, /* opens in foreground, correct exit_code */
    { "galeon",                 NULL,           "", "", 1 }, /* opens in foreground, correct exit_code */
    { "epiphany",               NULL,           "", "", 1 }, /* opens in foreground, correct exit_code */
    { "amaya",                  NULL,           "", "", 1 }, /* opens in foreground, correct exit_code */
    { "dillo",                  NULL,           "", "", 1 }, /* opens in foreground, correct exit_code */
  };
  uint i;
  for (i = 0; i < ARRAY_SIZE (www_browsers); i++)
    if (!www_browsers[i].disabled)
      {
        char *args[128] = { 0, };
        uint n = 0;
        args[n++] = (char*) www_browsers[i].prg;
        if (www_browsers[i].arg1)
          args[n++] = (char*) www_browsers[i].arg1;
        char *string = g_strconcat (www_browsers[i].prefix, url, www_browsers[i].postfix, NULL);
        args[n] = string;
        GError *error = NULL;
        char fallback_error[64] = "Ok";
        bool success;
        if (!www_browsers[i].asyncronous) /* start syncronously and check exit code */
          {
            int exit_status = -1;
            success = g_spawn_sync (NULL, /* cwd */
                                    args,
                                    NULL, /* envp */
                                    G_SPAWN_SEARCH_PATH,
                                    NULL, /* child_setup() */
                                    NULL, /* user_data */
                                    NULL, /* standard_output */
                                    NULL, /* standard_error */
                                    &exit_status,
                                    &error);
            success = success && !exit_status;
            if (exit_status)
              g_snprintf (fallback_error, sizeof (fallback_error), "exitcode: %u", exit_status);
          }
        else
          success = g_spawn_async (NULL, /* cwd */
                                   args,
                                   NULL, /* envp */
                                   G_SPAWN_SEARCH_PATH,
                                   NULL, /* child_setup() */
                                   NULL, /* user_data */
                                   NULL, /* child_pid */
                                   &error);
        g_free (string);
        RAPICORN_KEY_DEBUG ("URL", "show \"%s\": %s: %s", url, args[0], error ? error->message : fallback_error);
        g_clear_error (&error);
        if (success)
          return true;
        www_browsers[i].disabled = true;
      }
  /* reset all disabled states if no browser could be found */
  for (i = 0; i < ARRAY_SIZE (www_browsers); i++)
    www_browsers[i].disabled = false;
  return false;
}

/* --- zintern support --- */
#include <zlib.h>

/**
 * @param decompressed_size exact size of the decompressed data to be returned
 * @param cdata             compressed data block
 * @param cdata_size        exact size of the compressed data block
 * @returns                 decompressed data block or NULL in low memory situations
 *
 * Decompress the data from @a cdata of length @a cdata_size into a newly
 * allocated block of size @a decompressed_size which is returned.
 * The returned block needs to be freed with g_free().
 * This function is intended to decompress data which has been compressed
 * with the rapidres utility, so no errors should occour during decompression.
 * Consequently, if any error occours during decompression or if the resulting
 * data block is of a size other than @a decompressed_size, the program will
 * abort with an appropriate error message.
 * If not enough memory could be allocated for decompression, NULL is returned.
 */
uint8*
zintern_decompress (unsigned int          decompressed_size,
                    const unsigned char  *cdata,
                    unsigned int          cdata_size)
{
  uLongf dlen = decompressed_size;
  uint64 len = dlen + 1;
  uint8 *text = (uint8*) g_try_malloc (len);
  if (!text)
    return NULL;        /* handle ENOMEM gracefully */
  
  int64 result = uncompress (text, &dlen, cdata, cdata_size);
  const char *err;
  switch (result)
    {
    case Z_OK:
      if (dlen == decompressed_size)
        {
          err = NULL;
          break;
        }
      /* fall through */
    case Z_DATA_ERROR:
      err = "internal data corruption";
      break;
    case Z_MEM_ERROR:
      err = "out of memory";
      g_free (text);
      return NULL;      /* handle ENOMEM gracefully */
      break;
    case Z_BUF_ERROR:
      err = "insufficient buffer size";
      break;
    default:
      err = "unknown error";
      break;
    }
  if (err)
    fatal ("failed to decompress (%p, %u): %s", cdata, cdata_size, err);

  text[dlen] = 0;
  return text;          /* success */
}

void
zintern_free (uint8 *dc_data)
{
  g_free (dc_data);
}

} // Rapicorn
