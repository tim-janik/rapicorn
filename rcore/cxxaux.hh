// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#ifndef __RAPICORN_CXXAUX_HH__
#define __RAPICORN_CXXAUX_HH__

#include <rcore/buildconfig.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stddef.h>			// NULL
#include <sys/types.h>			// uint, ssize
#include <stdint.h>			// uint64_t
#include <limits.h>                     // {INT|CHAR|...}_{MIN|MAX}
#include <float.h>                      // {FLT|DBL}_{MIN|MAX|EPSILON}
#include <memory>
#include <mutex>
#include <string>
#include <vector>
#include <map>

// == Standard Macros ==
#ifndef FALSE
#  define FALSE					false
#endif
#ifndef TRUE
#  define TRUE					true
#endif
#define RAPICORN_ABS(a)                         ((a) < 0 ? -(a) : (a))
#define RAPICORN_MIN(a,b)                       ((a) <= (b) ? (a) : (b))
#define RAPICORN_MAX(a,b)                       ((a) >= (b) ? (a) : (b))
#define RAPICORN_CLAMP(v,mi,ma)                 ((v) < (mi) ? (mi) : ((v) > (ma) ? (ma) : (v)))
#define RAPICORN_ARRAY_SIZE(array)		(sizeof (array) / sizeof ((array)[0]))
#define RAPICORN_ALIGN(size, base)		((base) * (((size) + (base) - 1) / (base)))
#undef ABS
#define ABS                                     RAPICORN_ABS
#undef MIN
#define MIN                                     RAPICORN_MIN
#undef MAX
#define MAX                                     RAPICORN_MAX
#undef CLAMP
#define CLAMP                                   RAPICORN_CLAMP
#undef ARRAY_SIZE
#define ARRAY_SIZE				RAPICORN_ARRAY_SIZE
#if     !defined (INT64_MAX) || !defined (INT64_MIN) || !defined (UINT64_MAX)
// clang++ and g++ always have __LONG_LONG_MAX__
#  define INT64_MAX     __LONG_LONG_MAX__               //  +9223372036854775807LL
#  define INT64_MIN     (-INT64_MAX - 1LL)              //  -9223372036854775808LL
#  define UINT64_MAX    (INT64_MAX * 2ULL + 1ULL)       // +18446744073709551615LLU
#endif
#ifndef SIZE_T_MAX
#define SIZE_T_MAX              (~size_t (0))
#define SSIZE_T_MAX             (ssize_t (SIZE_T_MAX / 2))
#endif

// == Likelyness Hinting ==
#define	RAPICORN_ISLIKELY(expr)		__builtin_expect (bool (expr), 1)
#define	RAPICORN_UNLIKELY(expr)		__builtin_expect (bool (expr), 0)
#define	RAPICORN_LIKELY			RAPICORN_ISLIKELY

// == Convenience Macros ==
#ifdef  RAPICORN_CONVENIENCE
#define	ISLIKELY		RAPICORN_ISLIKELY       ///< Compiler hint that @a expression is likely to be true.
#define	UNLIKELY		RAPICORN_UNLIKELY       ///< Compiler hint that @a expression is unlikely to be true.
#define	LIKELY			RAPICORN_LIKELY         ///< Compiler hint that @a expression is likely to be true.
#define	STRINGIFY               RAPICORN_CPP_STRINGIFY  ///< Produces a const char C string from the macro @a argument.
#endif
/**
 * @def RAPICORN_CONVENIENCE
 * Compile time configuration macro to enable convenience macros.
 * Defining this before inclusion of rapicorn.hh or rapicorn-core.hh enables several convenience
 * macros that are defined in the global namespace without the usual "RAPICORN_" prefix,
 * see e.g. critical_unless(), UNLIKELY().
 */
#ifdef  DOXYGEN
#  define RAPICORN_CONVENIENCE
#endif // DOXYGEN

// == Preprocessor Convenience ==
#define RAPICORN_CPP_PASTE2_(a,b)               a ## b  // indirection required to expand macros like __LINE__
#define RAPICORN_CPP_PASTE2(a,b)                RAPICORN_CPP_PASTE2_ (a,b)
#define RAPICORN_CPP_STRINGIFY_(s)              #s      // indirection required to expand macros like __LINE__
#define RAPICORN_CPP_STRINGIFY(s)               RAPICORN_CPP_STRINGIFY_ (s)
#define RAPICORN_STATIC_ASSERT(expr)            static_assert (expr, #expr) ///< Shorthand for static_assert (condition, "condition")

// == GCC Attributes ==
#if     __GNUC__ >= 4
#define RAPICORN_PURE                           __attribute__ ((__pure__))
#define RAPICORN_MALLOC                         __attribute__ ((__malloc__))
#define RAPICORN_PRINTF(format_idx, arg_idx)    __attribute__ ((__format__ (__printf__, format_idx, arg_idx)))
#define RAPICORN_SCANF(format_idx, arg_idx)     __attribute__ ((__format__ (__scanf__, format_idx, arg_idx)))
#define RAPICORN_FORMAT(arg_idx)                __attribute__ ((__format_arg__ (arg_idx)))
#define RAPICORN_SENTINEL                       __attribute__ ((__sentinel__))
#define RAPICORN_NORETURN                       __attribute__ ((__noreturn__))
#define RAPICORN_CONST                          __attribute__ ((__const__))
#define RAPICORN_UNUSED                         __attribute__ ((__unused__))
#define RAPICORN_NO_INSTRUMENT                  __attribute__ ((__no_instrument_function__))
#define RAPICORN_DEPRECATED                     __attribute__ ((__deprecated__))
#define RAPICORN_ALWAYS_INLINE			__attribute__ ((always_inline))
#define RAPICORN_NOINLINE			__attribute__ ((noinline))
#define RAPICORN_CONSTRUCTOR			__attribute__ ((constructor,used))      // gcc-3.3 also needs "used" to emit code
#define RAPICORN_MAY_ALIAS                      __attribute__ ((may_alias))
#define	RAPICORN_SIMPLE_FUNCTION	       (::Rapicorn::string_from_pretty_function_name (__PRETTY_FUNCTION__).c_str())
#else   // !__GNUC__
#define RAPICORN_PURE
#define RAPICORN_MALLOC
#define RAPICORN_PRINTF(format_idx, arg_idx)
#define RAPICORN_SCANF(format_idx, arg_idx)
#define RAPICORN_FORMAT(arg_idx)
#define RAPICORN_SENTINEL
#define RAPICORN_NORETURN
#define RAPICORN_CONST
#define RAPICORN_UNUSED
#define RAPICORN_NO_INSTRUMENT
#define RAPICORN_DEPRECATED
#define RAPICORN_ALWAYS_INLINE
#define RAPICORN_NOINLINE
#define RAPICORN_CONSTRUCTOR
#define RAPICORN_MAY_ALIAS
#define	RAPICORN_SIMPLE_FUNCTION	       (__func__)
#error  Failed to detect a recent GCC version (>= 4)
#endif  // !__GNUC__

// == Ensure 'uint' in global namespace ==
#if 	RAPICORN_SIZEOF_SYS_TYPESH_UINT == 0
typedef unsigned int		uint;           // for systems that don't define uint in sys/types.h
#else
RAPICORN_STATIC_ASSERT (RAPICORN_SIZEOF_SYS_TYPESH_UINT == 4);
#endif
RAPICORN_STATIC_ASSERT (sizeof (uint) == 4);

// == Rapicorn Namespace ==
namespace Rapicorn {

// == Provide Canonical Integer Types ==
typedef uint8_t         uint8;          ///< An 8-bit unsigned integer.
typedef uint16_t        uint16;         ///< A 16-bit unsigned integer.
typedef uint32_t        uint32;         ///< A 32-bit unsigned integer.
typedef uint64_t        uint64;         ///< A 64-bit unsigned integer, use PRI*64 in format strings.
typedef int8_t          int8;           ///< An 8-bit signed integer.
typedef int16_t         int16;          ///< A 16-bit signed integer.
typedef int32_t         int32;          ///< A 32-bit signed integer.
typedef int64_t         int64;          ///< A 64-bit unsigned integer, use PRI*64 in format strings.
typedef uint32_t        unichar;        ///< A 32-bit unsigned integer used for Unicode characters.
RAPICORN_STATIC_ASSERT (sizeof (uint8) == 1 && sizeof (uint16) == 2 && sizeof (uint32) == 4 && sizeof (uint64) == 8);
RAPICORN_STATIC_ASSERT (sizeof (int8)  == 1 && sizeof (int16)  == 2 && sizeof (int32)  == 4 && sizeof (int64)  == 8);
RAPICORN_STATIC_ASSERT (sizeof (int) == 4 && sizeof (uint) == 4 && sizeof (unichar) == 4);

///@{
/** LongIffy, ULongIffy, CastIffy, UCastIffy - types for 32bit/64bit overloading.
 * On 64bit, int64_t is aliased to "long int" which is 64 bit wide.
 * On 32bit, int64_t is aliased to "long long int", which is 64 bit wide (and long is 32bit wide).
 * For int-type function overloading, this means that int32, int64 and either "long" or "long long"
 * need to be overloaded, depending on platform. To aid this case, LongIffy and ULongIffy are defined
 * to signed and unsigned "long" (for 32bit) and "long long" (for 64bit). Correspondingly, CastIffy
 * and UCastIffy are defined to signed and unsigned int32 (for 32bit) or int64 (for 64bit), so
 * LongIffy can be cast losslessly into a known type.
 */
#if     __SIZEOF_LONG__ == 8    // 64bit
typedef long long signed int    LongIffy;
typedef long long unsigned int  ULongIffy;
typedef int64_t                 CastIffy;
typedef uint64_t                UCastIffy;
static_assert (__SIZEOF_LONG_LONG__ == 8, "__SIZEOF_LONG_LONG__");
static_assert (__SIZEOF_INT__ == 4, "__SIZEOF_INT__");
#elif   __SIZEOF_LONG__ == 4    // 32bit
typedef long signed int         LongIffy;
typedef long unsigned int       ULongIffy;
typedef int32_t                 CastIffy;
typedef uint32_t                UCastIffy;
static_assert (__SIZEOF_LONG_LONG__ == 8, "__SIZEOF_LONG_LONG__");
static_assert (__SIZEOF_INT__ == 4, "__SIZEOF_INT__");
#else
#error  "Unknown long size:" __SIZEOF_LONG__
#endif
static_assert (sizeof (CastIffy) == sizeof (LongIffy), "CastIffy == LongIffy");
static_assert (sizeof (UCastIffy) == sizeof (ULongIffy), "UCastIffy == ULongIffy");
///@}

// == Convenient stdc++ Types ==
using   std::map;
using   std::vector;
typedef std::string String;             ///< Convenience alias for std::string.
typedef vector<String> StringVector;    ///< Convenience alias for a std::vector<std::string>.

#if DOXYGEN
/// Template to map all type arguments to void, useful for SFINAE.
/// See also: http://open-std.org/jtc1/sc22/wg21/docs/papers/2014/n3911.pdf
template<class...> using void_t = void;
#else // work-around for g++ <= 4.9
template<class...> struct void_t__voider { using type = void; };
template<class... T0toN > using void_t = typename void_t__voider<T0toN...>::type;
#endif

// == File Path Handling ==
#ifdef  _WIN32
#define RAPICORN_DIR_SEPARATOR		  '\\'
#define RAPICORN_DIR_SEPARATOR_S	  "\\"
#define RAPICORN_SEARCHPATH_SEPARATOR	  ';'
#define RAPICORN_SEARCHPATH_SEPARATOR_S	  ";"
#define RAPICORN_IS_ABSPATH(p)          (((p[0] >= 'A' && p[0] <= 'Z') || (p[0] >= 'a' && p[0] <= 'z')) && p[1] == ':' && p[2] == '\\')
#else   // !_WIN32
#define RAPICORN_DIR_SEPARATOR		  '/'
#define RAPICORN_DIR_SEPARATOR_S	  "/"
#define RAPICORN_SEARCHPATH_SEPARATOR	  ':'
#define RAPICORN_SEARCHPATH_SEPARATOR_S	  ":"
#define RAPICORN_IS_ABSPATH(p)            (p[0] == RAPICORN_DIR_SEPARATOR)
#endif  // !_WIN32

// == C++ Macros ==
#define RAPICORN_CLASS_NON_COPYABLE(ClassName)  \
  /*copy-ctor*/ ClassName  (const ClassName&) = delete; \
  ClassName&    operator=  (const ClassName&) = delete
#ifdef __clang__ // clang++-3.8.0: work around 'variable length array of non-POD element type'
#define RAPICORN_DECLARE_VLA(Type, var, count)          std::vector<Type> var (count)
#else // sane c++
#define RAPICORN_DECLARE_VLA(Type, var, count)          Type var[count]
#endif

// == Forward Declarations ==
class Mutex;
template<class> class ScopedLock;

// == C++ Helper Classes ==
/// Simple helper class to call one-line lambda initializers as static constructor.
struct Init {
  explicit Init (void (*f) ()) { f(); }
};

/// Create an instance of @a Class on demand that is constructed and never destructed.
/// DurableInstance<Class*> provides the memory for a @a Class instance and calls it's
/// constructor on demand, but it's destructor is never called (so the memory allocated
/// to the DurableInstance must not be freed). Due to its constexpr ctor and on-demand
/// creation of @a Class, a DurableInstance<> can be accessed at any time during the
/// static ctor (or dtor) phases and will always yield a properly initialized @a Class.
/// A DurableInstance is useful for static variables that need to be accessible from
/// other static ctor/dtor calls.
template<class Class>
class DurableInstance final {
  static_assert (std::is_class<Class>::value, "DurableInstance<Class> requires class template argument");
  Class *ptr_;
  uint64 mem_[(sizeof (Class) + sizeof (uint64) - 1) / sizeof (uint64)];
  void
  initialize() RAPICORN_NOINLINE
  {
    static std::mutex mtx;
    std::unique_lock<std::mutex> lock (mtx);
    if (ptr_ == NULL)
      ptr_ = new (mem_) Class(); // exclusive construction
  }
public:
  constexpr  DurableInstance() : ptr_ (NULL) {}
  /// Retrieve pointer to @a Class instance, always returns the same pointer.
  Class*
  operator->() __attribute__ ((pure))
  {
    if (RAPICORN_UNLIKELY (ptr_ == NULL))
      initialize();
    return ptr_;
  }
  /// Retrieve reference to @a Class instance, always returns the same reference.
  Class&       operator*     () __attribute__ ((pure))       { return *operator->(); }
  const Class* operator->    () const __attribute__ ((pure)) { return const_cast<DurableInstance*> (this)->operator->(); }
  const Class& operator*     () const __attribute__ ((pure)) { return const_cast<DurableInstance*> (this)->operator*(); }
  /// Check if @a this stores a @a Class instance yet.
  explicit     operator bool () const                        { return ptr_ != NULL; }
};

/**
 * A std::make_shared<>() wrapper class to access private ctor & dtor.
 * To call std::make_shared<T>() on a class @a T, its constructor and
 * destructor must be public. For classes with private or protected
 * constructor or destructor, this class can be used as follows:
 * @code{.cc}
 * class Type {
 *   Type (ctor_args...);                // Private ctor.
 *   friend class FriendAllocator<Type>; // Allow access to ctor/dtor of Type.
 * };
 * std::shared_ptr<Type> t = FriendAllocator<Type>::make_shared (ctor_args...);
 * @endcode
 */
template<class T>
class FriendAllocator : public std::allocator<T> {
public:
  /// Construct type @a C object, standard allocator requirement.
  template<typename C, typename... Args> static inline void
  construct (C *p, Args &&... args)
  {
    ::new ((void*) p) C (std::forward<Args> (args)...);
  }
  /// Delete type @a C object, standard allocator requirement.
  template<typename C> static inline void
  destroy (C *p)
  {
    p->~C ();
  }
  /**
   * Construct an object of type @a T that is wrapped into a std::shared_ptr<T>.
   * @param args        The list of arguments to pass into a T() constructor.
   * @return            A std::shared_ptr<T> owning the newly created object.
   */
  template<typename ...Args> static inline std::shared_ptr<T>
  make_shared (Args &&... args)
  {
    return std::allocate_shared<T> (FriendAllocator(), std::forward<Args> (args)...);
  }
};

/** Shorthand for std::dynamic_pointer_cast<>(shared_from_this()).
 * A shared_ptr_cast() takes a std::shared_ptr or a pointer to an @a object that
 * supports std::enable_shared_from_this::shared_from_this().
 * Using std::dynamic_pointer_cast(), the shared_ptr passed in (or retrieved via
 * calling shared_from_this()) is cast into a std::shared_ptr<@a Target>, possibly
 * resulting in an empty (NULL) std::shared_ptr if the underlying dynamic_cast()
 * was not successful or if a NULL @a object was passed in.
 * Note that shared_from_this() can throw a std::bad_weak_ptr exception if
 * the object has no associated std::shared_ptr (usually during ctor and dtor), in
 * which case the exception will also be thrown from shared_ptr_cast<Target>().
 * However a shared_ptr_cast<Target*>() call will not throw and yield an empty
 * (NULL) std::shared_ptr<@a Target>. This is analogous to dynamic_cast<T@amp> which
 * throws, versus dynamic_cast<T*> which yields NULL.
 * @return A std::shared_ptr<@a Target> storing a pointer to @a object or NULL.
 * @throws std::bad_weak_ptr if shared_from_this() throws, unless the @a Target* form is used.
 */
template<class Target, class Source> std::shared_ptr<typename std::remove_pointer<Target>::type>
shared_ptr_cast (Source *object)
{
  if (std::is_pointer<Target>::value)
    {
      if (!object)
        return NULL;
      try {
        return std::dynamic_pointer_cast<typename std::remove_pointer<Target>::type> (object->shared_from_this());
      } catch (const std::bad_weak_ptr&) {
        return NULL;
      }
    }
  else // this may throw bad_weak_ptr
    return !object ? NULL : std::dynamic_pointer_cast<typename std::remove_pointer<Target>::type> (object->shared_from_this());
}
/// See shared_ptr_cast(Source*).
template<class Target, class Source> const std::shared_ptr<typename std::remove_pointer<Target>::type>
shared_ptr_cast (const Source *object)
{
  return shared_ptr_cast<Target> (const_cast<Source*> (object));
}
/// See shared_ptr_cast(Source*).
template<class Target, class Source> std::shared_ptr<typename std::remove_pointer<Target>::type>
shared_ptr_cast (std::shared_ptr<Source> &sptr)
{
  return std::dynamic_pointer_cast<typename std::remove_pointer<Target>::type> (sptr);
}
/// See shared_ptr_cast(Source*).
template<class Target, class Source> const std::shared_ptr<typename std::remove_pointer<Target>::type>
shared_ptr_cast (const std::shared_ptr<Source> &sptr)
{
  return shared_ptr_cast<Target> (const_cast<std::shared_ptr<Source>&> (sptr));
}

// == C++ Traits ==
/// REQUIRES<value> - Simplified version of std::enable_if<> to use SFINAE in function templates.
template<bool value> using REQUIRES = typename ::std::enable_if<value, bool>::type;

/// IsBool<T> - Check if @a T is of type 'bool'.
template<class T> using IsBool = ::std::is_same<bool, typename ::std::remove_cv<T>::type>;

/// IsInteger<T> - Check if @a T is of integral type (except bool).
template<class T> using IsInteger = ::std::integral_constant<bool, !IsBool<T>::value && ::std::is_integral<T>::value>;

/// DerivesString<T> - Check if @a T is of type 'std::string'.
template<class T> using DerivesString = typename std::is_base_of<::std::string, T>;

/// DerivesVector<T> - Check if @a T derives from std::vector<>.
template<class T, typename = void> struct DerivesVector : std::false_type {};
// use void_t to prevent errors for T without vector's typedefs
template<class T> struct DerivesVector<T, void_t< typename T::value_type, typename T::allocator_type > > :
std::is_base_of< std::vector<typename T::value_type, typename T::allocator_type>, T > {};

/// IsComparable<T> - Check if type @a T is comparable for equality.
/// If @a T is a type that can be compared with operator==, provide the member constant @a value equal true, otherwise false.
template<class, class = void> struct IsComparable : std::false_type {}; // IsComparable false case, picked if operator== is missing.
template<class T> struct IsComparable<T, void_t< decltype (std::declval<T>() == std::declval<T>()) >> : std::true_type {};

/// IsSharedPtr<T> - Check if a type @a T is a std::shared_ptr<>.
template<class T> struct IsSharedPtr                      : std::false_type {};
template<class T> struct IsSharedPtr<std::shared_ptr<T> > : std::true_type {};
/// IsWeakPtr<T> - Check if a type @a T is a std::weak_ptr<>.
template<class T> struct IsWeakPtr                        : std::false_type {};
template<class T> struct IsWeakPtr<std::weak_ptr<T> >     : std::true_type {};

/// DerivesSharedPtr<T> - Check if @a T derives from std::shared_ptr<>.
template<class T, typename = void> struct DerivesSharedPtr : std::false_type {};
// use void_t to prevent errors for T without shared_ptr's typedefs
template<class T> struct DerivesSharedPtr<T, Rapicorn::void_t< typename T::element_type > > :
std::is_base_of< std::shared_ptr<typename T::element_type>, T > {};

/// Provide the member typedef type which is the element_type of the shared_ptr type @a T.
template<typename T> struct RemoveSharedPtr                                             { typedef T type; };
template<typename T> struct RemoveSharedPtr<::std::shared_ptr<T>>                       { typedef T type; };
template<typename T> struct RemoveSharedPtr<const ::std::shared_ptr<T>>                 { typedef T type; };
template<typename T> struct RemoveSharedPtr<volatile ::std::shared_ptr<T>>              { typedef T type; };
template<typename T> struct RemoveSharedPtr<const volatile ::std::shared_ptr<T>>        { typedef T type; };

/// Has__accept__<T,Visitor> - Check if @a T provides a member template __accept__<>(Visitor).
template<class, class, class = void> struct Has__accept__ : std::false_type {};
template<class T, class V>
struct Has__accept__<T, V, void_t< decltype (std::declval<T>().template __accept__<V> (*(V*) NULL)) >> : std::true_type {};

/// Has__accept_accessor__<T,Visitor> - Check if @a T provides a member template __accept_accessor__<>(Visitor).
template<class, class, class = void> struct Has__accept_accessor__ : std::false_type {};
template<class T, class V>
struct Has__accept_accessor__<T, V, void_t< decltype (std::declval<T>().template __accept_accessor__<V> (*(V*) NULL)) >> : std::true_type {};

namespace Aida { class Any; } // needed for Has__aida_from_any__

/// Has__aida_from_any__<T> - Check if @a T provides a member __aida_from_any__(const Any&).
template<class, class = void> struct Has__aida_from_any__ : std::false_type {};
template<class T>
struct Has__aida_from_any__<T, void_t< decltype (std::declval<T>().__aida_from_any__ (std::declval<Aida::Any>())) >> : std::true_type {};

/// Has__aida_to_any__<T> - Check if @a T provides a member Has__aida_to_any__().
template<class, class = void> struct Has__aida_to_any__ : std::false_type {};
template<class T>
struct Has__aida_to_any__<T, void_t< decltype (std::declval<T>().__aida_to_any__ ()) >> : std::true_type {};

/// ClassDoctor (used for private class copies), use discouraged.
// TODO: remove ClassDoctor entirely.
#ifdef  __RAPICORN_BUILD__
class ClassDoctor;
#else
class ClassDoctor {};
#endif

// == C++ Helper Functions ==
// cxx_demangle() is here for Aida, the implementation is in utilities.cc.
String cxx_demangle (const char *mangled_identifier);

} // Rapicorn

namespace RapicornInternal {
const char* buildid(); // buildid.cc
} // RapicornInternal

#endif // __RAPICORN_CXXAUX_HH__
