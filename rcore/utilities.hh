// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#ifndef __RAPICORN_CORE_UTILITIES_HH__
#define __RAPICORN_CORE_UTILITIES_HH__

#include <rcore/inout.hh>

#if !defined __RAPICORN_CORE_HH__ && !defined __RAPICORN_BUILD__
#error Only <rapicorn-core.hh> can be included directly.
#endif

// === Convenience Macro Abbreviations ===
#ifdef RAPICORN_CONVENIENCE
#include <assert.h>                                             // needed to redefine assert()
#define DIR_SEPARATOR           RAPICORN_DIR_SEPARATOR          ///< Shorthand for RAPICORN_DIR_SEPARATOR.
#define DIR_SEPARATOR_S         RAPICORN_DIR_SEPARATOR_S        ///< Shorthand for RAPICORN_DIR_SEPARATOR_S.
#define SEARCHPATH_SEPARATOR    RAPICORN_SEARCHPATH_SEPARATOR   ///< Shorthand for RAPICORN_SEARCHPATH_SEPARATOR.
#define SEARCHPATH_SEPARATOR_S  RAPICORN_SEARCHPATH_SEPARATOR_S ///< Shorthand for RAPICORN_SEARCHPATH_SEPARATOR_S.
#define __PRETTY_FILE__         RAPICORN_PRETTY_FILE            ///< Shorthand for #RAPICORN_PRETTY_FILE.
#define STRLOC()         RAPICORN_STRLOC()          ///< Shorthand for RAPICORN_STRLOC() if RAPICORN_CONVENIENCE is defined.
#define return_if        RAPICORN_RETURN_IF         ///< Shorthand for RAPICORN_RETURN_IF() if RAPICORN_CONVENIENCE is defined.
#define return_unless    RAPICORN_RETURN_UNLESS     ///< Shorthand for RAPICORN_RETURN_UNLESS() if RAPICORN_CONVENIENCE is defined.
#define BACKTRACE        RAPICORN_BACKTRACE
#endif // RAPICORN_CONVENIENCE

namespace Rapicorn {

// == Convenient Aida Types ==
using Aida::Any;
using Aida::Signal;
using Aida::slot;
using Aida::PropertyList;
using Aida::Property;

// == Common (stdc++) Utilities ==
using ::std::swap;
using ::std::min;
using ::std::max;
#undef abs
template<typename T> inline const T&
abs (const T &value)
{
  return max (value, -value);
}
#undef clamp
template<typename T> inline const T&
clamp (const T &value, const T &minimum, const T &maximum)
{
  if (minimum > value)
    return minimum;
  if (maximum < value)
    return maximum;
  return value;
}
template <class T, size_t S> inline std::vector<T>
vector_from_array (const T (&array_entries)[S]) /// Construct a std::vector<T> from a C array of type T[].
{
  std::vector<T> result;
  for (size_t i = 0; i < S; i++)
    result.push_back (array_entries[i]);
  return result;
}
#if 0 // Already prototyped in cxxaux.hh.
String cxx_demangle (const char *mangled_identifier);
#endif

// === source location strings ===
String  pretty_file                             (const char *file_dir, const char *file);
/** @def RAPICORN_PRETTY_FILE
 * Full source file path name.
 * Macro that expands to __FILE_DIR__ "/" __FILE__, see also #__FILE_DIR__.
 */
#ifdef  __FILE_DIR__
#define RAPICORN_PRETTY_FILE                    (__FILE_DIR__ "/" __FILE__)
#else
#define RAPICORN_PRETTY_FILE                    (__FILE__)
#define __FILE_DIR__                            ""
#endif
#define RAPICORN_STRLOC()                       ((RAPICORN_PRETTY_FILE + ::Rapicorn::String (":") + RAPICORN_STRINGIFY (__LINE__)).c_str()) ///< Return "FILE:LINE"
#define RAPICORN_STRFUNC()                      (std::string (__FUNCTION__).c_str())            ///< Return "FUNCTION()"
#define RAPICORN_STRINGIFY(macro_or_string)     RAPICORN_STRINGIFY_ARG (macro_or_string)        ///< Return stringiified argument
#define RAPICORN_STRINGIFY_ARG(arg)     #arg

// === Control Flow Helpers ===
#define RAPICORN_RETURN_IF(cond, ...)           do { if (RAPICORN_UNLIKELY (cond)) return __VA_ARGS__; } while (0)
#define RAPICORN_RETURN_UNLESS(cond, ...)       do { if (RAPICORN_LIKELY (cond)) break; return __VA_ARGS__; } while (0)

// === Debugging Functions (internal) ===
#define RAPICORN_BACKTRACE_MAXDEPTH  1024
#define RAPICORN_BACKTRACE_STRING()  ({ void *__p_[RAPICORN_BACKTRACE_MAXDEPTH]; const String __s_ = ::Rapicorn::pretty_backtrace (__p_, ::Rapicorn::backtrace_pointers (__p_, sizeof (__p_) / sizeof (__p_[0])), __FILE__, __LINE__, __func__); __s_; })
#define RAPICORN_BACKTRACE()         ({ ::Rapicorn::printerr ("%s", RAPICORN_BACKTRACE_STRING()); })
extern int (*backtrace_pointers)     (void **buffer, int size);
String      pretty_backtrace         (void **ptrs, size_t nptrs, const char *file, int line, const char *func);
void        debug_backtrace_snapshot (size_t key);
String      debug_backtrace_showshot (size_t key);
String      process_handle           ();
inline void breakpoint ();

// === Macro Implementations ===
#define RAPICORN_BREAKPOINT()           Rapicorn::breakpoint()  ///< Cause a debugging breakpoint, for development only.
#if (defined __i386__ || defined __x86_64__)
inline void breakpoint() { __asm__ __volatile__ ("int $03"); }
#elif defined __alpha__ && !defined __osf__
inline void breakpoint() { __asm__ __volatile__ ("bpt"); }
#else   // !__i386__ && !__alpha__
inline void breakpoint() { __builtin_trap(); }
#endif

// == Outwards Indexing ==
// Generate iteration indexes from a loop's midpoint outwards, alternating sides.
static inline constexpr size_t
outwards_index (const size_t pos, const size_t max, const bool right_bias = false)
{
  return (right_bias
          ? (max & 1 || pos + 1 < max // even numbered max needs upper bound guard
             ? ssize_t (max / 2) - ssize_t (pos + 1) / 2 * ((ssize_t (pos) & 1) * -2 + 1)
             : 0)
          :    ssize_t (max / 2) + ssize_t (pos + 1) / 2 * ((ssize_t (pos) & 1) * -2 + 1));
}

// == Timestamp Handling ==
uint64  timestamp_startup    ();        // µseconds
uint64  timestamp_realtime   ();        // µseconds
uint64  timestamp_benchmark  ();        // nseconds
uint64  timestamp_resolution ();        // nseconds
String  timestamp_format     (uint64 stamp);
uint64  monotonic_counter    ();

/// The Path namespace provides functions for file path manipulation and testing.
namespace Path {
String  dirname         (const String &path);
String  basename        (const String &path);
String  realpath        (const String &path);
String  abspath         (const String &path, const String &incwd = "");
bool    isabs           (const String &path);
bool    isdirname       (const String &path);
String  expand_tilde    (const String &path);
String  user_home       (const String &username = "");
String  data_home       ();
String  config_home     ();
String  config_names    ();
void    config_names    (const String &names);
String  cache_home      ();
String  runtime_dir     ();
String  config_dirs     ();
String  data_dirs       ();
String  skip_root       (const String &path);
String  join            (const String &frag0, const String &frag1, const String &frag2 = "", const String &frag3 = "",
                         const String &frag4 = "", const String &frag5 = "", const String &frag6 = "", const String &frag7 = "",
                         const String &frag8 = "", const String &frag9 = "", const String &frag10 = "", const String &frag11 = "",
                         const String &frag12 = "", const String &frag13 = "", const String &frag14 = "", const String &frag15 = "");
bool    check           (const String &file,
                         const String &mode);
bool    equals          (const String &file1,
                         const String &file2);
char*   memread         (const String &filename,
                         size_t       *lengthp);
void    memfree         (char         *memread_mem);
bool    memwrite        (const String &filename, size_t len, const uint8 *bytes);
String  cwd             ();
String       vpath_find           (const String &file, const String &mode = "e");
bool         searchpath_contains  (const String &searchpath, const String &element);
String       searchpath_find      (const String &searchpath, const String &file, const String &mode = "e");
StringVector searchpath_list      (const String &searchpath, const String &mode = "e");
String       searchpath_multiply  (const String &searchpath, const String &postfixes);
StringVector searchpath_split (const String &searchpath);
String       searchpath_join  (const String &frag0, const String &frag1, const String &frag2 = "", const String &frag3 = "",
                               const String &frag4 = "", const String &frag5 = "", const String &frag6 = "", const String &frag7 = "",
                               const String &frag8 = "", const String &frag9 = "", const String &frag10 = "",
                               const String &frag11 = "", const String &frag12 = "", const String &frag13 = "",
                               const String &frag14 = "", const String &frag15 = "");
extern const String     dir_separator;         /* 1char */
extern const String     searchpath_separator;  /* 1char */
} // Path

/* --- url handling --- */
bool url_show                   (const char           *url);

/* --- zintern support --- */
uint8*  zintern_decompress      (unsigned int          decompressed_size,
                                 const unsigned char  *cdata,
                                 unsigned int          cdata_size);
void    zintern_free            (uint8                *dc_data);

/* --- Binary Lookups --- */
template<typename RandIter, class Cmp, typename Arg, int case_lookup_or_sibling_or_insertion>
static inline std::pair<RandIter,bool>
binary_lookup_fuzzy (RandIter  begin,
                     RandIter  end,
                     Cmp       cmp_elements,
                     const Arg &arg)
{
  RandIter current = end;
  size_t n_elements = end - begin, offs = 0;
  const bool want_lookup = case_lookup_or_sibling_or_insertion == 0;
  // const bool want_sibling = case_lookup_or_sibling_or_insertion == 1;
  const bool want_insertion_pos = case_lookup_or_sibling_or_insertion > 1;
  ssize_t cmp = 0;
  while (offs < n_elements)
    {
      size_t i = (offs + n_elements) >> 1;
      current = begin + i;
      cmp = cmp_elements (arg, *current);
      if (cmp == 0)
        return want_insertion_pos ? std::make_pair (current, true) : std::make_pair (current, /*ignored*/ false);
      else if (cmp < 0)
        n_elements = i;
      else /* (cmp > 0) */
        offs = i + 1;
    }
  /* check is last mismatch, cmp > 0 indicates greater key */
  return (want_lookup
          ? std::make_pair (end, /*ignored*/ false)
          : (want_insertion_pos && cmp > 0)
          ? std::make_pair (current + 1, false)
          : std::make_pair (current, false));
}
template<typename RandIter, class Cmp, typename Arg>
static inline std::pair<RandIter,bool>
binary_lookup_insertion_pos (RandIter  begin,
                             RandIter  end,
                             Cmp       cmp_elements,
                             const Arg &arg)
{
  /* return (end,false) for end-begin==0, or return (position,true) for exact match,
   * otherwise return (position,false) where position indicates the location for
   * the key to be inserted (and may equal end).
   */
  return binary_lookup_fuzzy<RandIter,Cmp,Arg,2> (begin, end, cmp_elements, arg);
}
template<typename RandIter, class Cmp, typename Arg>
static inline RandIter
binary_lookup_sibling (RandIter  begin,
                       RandIter  end,
                       Cmp       cmp_elements,
                       const Arg &arg)
{
  /* return end for end-begin==0, otherwise return the exact match element, or,
   * if there's no such element, return the element last visited, which is pretty
   * close to an exact match (will be one off into either direction).
   */
  return binary_lookup_fuzzy<RandIter,Cmp,Arg,1> (begin, end, cmp_elements, arg).first;
}
template<typename RandIter, class Cmp, typename Arg>
static inline RandIter
binary_lookup (RandIter  begin,
               RandIter  end,
               Cmp       cmp_elements,
               const Arg &arg)
{
  /* return end or exact match */
  return binary_lookup_fuzzy<RandIter,Cmp,Arg,0> (begin, end, cmp_elements, arg).first;
}

template<typename Value> static inline int ///< sort lesser items first
compare_lesser (const Value &v1, const Value &v2)
{
  return -(v1 < v2) | (v2 < v1);
}

template<typename Value> static inline int ///< sort greater items first
compare_greater (const Value &v1, const Value &v2)
{
  return (v1 < v2) | -(v2 < v1);
}

// == Custom Keyed Data ==
/// DataKey objects are used to identify and manage custom data members of DataListContainer objects.
template<typename Type> class DataKey {
private:
  /*Copy*/        DataKey    (const DataKey&);
  DataKey&        operator=  (const DataKey&);
public:
  /* explicit */  DataKey    ()                 { }
  virtual Type    fallback   ()                 { Type d = Type(); return d; }  ///< Return the default value for Type.
  virtual void    destroy    (Type data)        { /* destruction hook */ }      ///< Hook invoked when @a data is deleted.
  virtual        ~DataKey    ()                 {}
};

class DataList {
  class NodeBase {
  protected:
    NodeBase      *next;
    DataKey<void> *key;
    explicit       NodeBase (DataKey<void> *k) : next (NULL), key (k) {}
    virtual       ~NodeBase ();
    friend         class DataList;
  };
  template<typename T>
  class Node : public NodeBase {
    T data;
  public:
    T        get_data ()     { return data; }
    T        swap     (T d)  { T result = data; data = d; return result; }
    virtual ~Node()
    {
      if (key)
        {
          DataKey<T> *dkey = reinterpret_cast<DataKey<T>*> (key);
          dkey->destroy (data);
        }
    }
    explicit Node (DataKey<T> *k,
                   T           d) :
      NodeBase (reinterpret_cast<DataKey<void>*> (k)),
      data (d)
    {}
  };
  NodeBase *nodes;
  RAPICORN_CLASS_NON_COPYABLE (DataList);
public:
  DataList() :
    nodes (NULL)
  {}
  template<typename T> void
  set (DataKey<T> *key,
       T           data)
  {
    Node<T> *node = new Node<T> (key, data);
    set_data (node);
  }
  template<typename T> T
  get (DataKey<T> *key) const
  {
    NodeBase *nb = get_data (reinterpret_cast<DataKey<void>*> (key));
    if (nb)
      {
        Node<T> *node = reinterpret_cast<Node<T>*> (nb);
        return node->get_data();
      }
    else
      return key->fallback();
  }
  template<typename T> T
  swap (DataKey<T> *key,
        T           data)
  {
    NodeBase *nb = get_data (reinterpret_cast<DataKey<void>*> (key));
    if (nb)
      {
        Node<T> *node = reinterpret_cast<Node<T>*> (nb);
        return node->swap (data);
      }
    else
      {
        set (key, data);
        return key->fallback();
      }
  }
  template<typename T> T
  swap (DataKey<T> *key)
  {
    NodeBase *nb = rip_data (reinterpret_cast<DataKey<void>*> (key));
    if (nb)
      {
        Node<T> *node = reinterpret_cast<Node<T>*> (nb);
        T d = node->get_data();
        nb->key = NULL; // rip key to prevent data destruction
        delete nb;
        return d;
      }
    else
      return key->fallback();
  }
  template<typename T> void
  del (DataKey<T> *key)
  {
    NodeBase *nb = rip_data (reinterpret_cast<DataKey<void>*> (key));
    if (nb)
      delete nb;
  }
  void clear_like_destructor();
  ~DataList();
private:
  void      set_data (NodeBase      *node);
  NodeBase* get_data (DataKey<void> *key) const;
  NodeBase* rip_data (DataKey<void> *key);
};

/** DataListContainer - typesafe storage and retrieval of arbitrary members.
 * By using a DataKey, DataListContainer objects allow storage and retrieval of custom data members in a typesafe fashion.
 * The custom data members will initially default to DataKey::fallback and are deleted by the DataListContainer destructor.
 * Example: @snippet tests/t201/rcore-basics-datalist.cc DataListContainer-EXAMPLE
 */
class DataListContainer {
  DataList data_list;
public: /// @name Accessing custom data members
  /// Assign @a data to the custom keyed data member, deletes any previously set data.
  template<typename Type> inline void set_data    (DataKey<Type> *key, Type data) { data_list.set (key, data); }
  /// Retrieve contents of the custom keyed data member, returns DataKey::fallback if nothing was set.
  template<typename Type> inline Type get_data    (DataKey<Type> *key) const      { return data_list.get (key); }
  /// Swap @a data with the current contents of the custom keyed data member, returns the current contents.
  template<typename Type> inline Type swap_data   (DataKey<Type> *key, Type data) { return data_list.swap (key, data); }
  /// Removes and returns the current contents of the custom keyed data member without deleting it.
  template<typename Type> inline Type swap_data   (DataKey<Type> *key)            { return data_list.swap (key); }
  /// Delete the current contents of the custom keyed data member, invokes DataKey::destroy.
  template<typename Type> inline void delete_data (DataKey<Type> *key)            { data_list.del (key); }
};

} // Rapicorn

#endif /* __RAPICORN_CORE_UTILITIES_HH__ */
