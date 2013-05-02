// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __RAPICORN_CXXAUX_HH__
#define __RAPICORN_CXXAUX_HH__

#include <rcore/rapicornconfig.h>       // _GNU_SOURCE
#include <stdbool.h>
#include <stdarg.h>
#include <stddef.h>			// NULL
#include <sys/types.h>			// uint, ssize
#include <stdint.h>			// uint64_t
#include <limits.h>                     // {INT|CHAR|...}_{MIN|MAX}
#include <float.h>                      // {FLT|DBL}_{MIN|MAX|EPSILON}

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
#undef EXTERN_C
#ifdef	__cplusplus
#define EXTERN_C                                extern "C"
#else
#define EXTERN_C                                extern
#endif
#undef STRFUNC
#define STRFUNC				        RAPICORN_SIMPLE_FUNCTION
#if     !defined (INT64_MAX) || !defined (INT64_MIN) || !defined (UINT64_MAX)
#ifdef  LLONG_MAX       // some gcc versions ship limits.h that fail to define LLONG_MAX for C99
#  define INT64_MAX     LLONG_MAX       // +9223372036854775807LL
#  define INT64_MIN     LLONG_MIN       // -9223372036854775808LL
#  define UINT64_MAX    ULLONG_MAX      // +18446744073709551615LLU
#else   // !LLONG_MAX but gcc always has __LONG_LONG_MAX__
#  define INT64_MAX     __LONG_LONG_MAX__
#  define INT64_MIN     (-INT64_MAX - 1LL)
#  define UINT64_MAX    (INT64_MAX * 2ULL + 1ULL)
#endif
#endif
#ifndef SIZE_T_MAX
#define SIZE_T_MAX              (~size_t (0))
#define SSIZE_T_MAX             (ssize_t (SIZE_T_MAX / 2))
#endif

// == Likelyness Hinting ==
#define	RAPICORN__BOOL(expr)		__extension__ ({ bool _rapicorn__bool; if (expr) _rapicorn__bool = 1; else _rapicorn__bool = 0; _rapicorn__bool; })
#define	RAPICORN_ISLIKELY(expr)		__builtin_expect (RAPICORN__BOOL (expr), 1)
#define	RAPICORN_UNLIKELY(expr)		__builtin_expect (RAPICORN__BOOL (expr), 0)
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
 * Configuration macro to enable convenience macros.
 * Defining this before inclusion of rapicorn.hh or rapicorn-core.hh enables several convenience
 * macros that are defined in the global namespace without the usual "RAPICORN_" prefix,
 * see e.g. critical_unless(), UNLIKELY().
 */
#ifdef  RAPICORN_DOXYGEN
#  define RAPICORN_CONVENIENCE
#endif // RAPICORN_DOXYGEN

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
#define RAPICORN_NEVER_INLINE			__attribute__ ((noinline))
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
#define RAPICORN_NEVER_INLINE
#define RAPICORN_CONSTRUCTOR
#define RAPICORN_MAY_ALIAS
#define	RAPICORN_SIMPLE_FUNCTION	       (__func__)
#error  Failed to detect a recent GCC version (>= 4)
#endif  // !__GNUC__

// == C++11 Keywords ==
#if __GNUC__ == 4 && __GNUC_MINOR__ < 7
#define override        /* unimplemented */
#define final           /* unimplemented */
#endif // GCC < 4.7

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
typedef uint8_t                 uint8;          ///< An 8-bit unsigned integer.
typedef uint16_t                uint16;         ///< A 16-bit unsigned integer.
typedef uint32_t                uint32;         ///< A 32-bit unsigned integer.
typedef unsigned long long int  uint64;         ///< A 64-bit unsigned integer, use "%llu" in format strings.
// typedef uint64_t             uint64;         // int64_t / uint64_t are longs on AMD64 which breaks printf
typedef int8_t                  int8;           ///< An 8-bit signed integer.
typedef int16_t                 int16;          ///< A 16-bit signed integer.
typedef int32_t                 int32;          ///< A 32-bit signed integer.
typedef signed long long int    int64;          ///< A 64-bit unsigned integer, use "%lld" in format strings.
typedef uint32_t                unichar;        ///< A 32-bit unsigned integer used for Unicode characters.
RAPICORN_STATIC_ASSERT (sizeof (uint8) == 1 && sizeof (uint16) == 2 && sizeof (uint32) == 4 && sizeof (uint64) == 8);
RAPICORN_STATIC_ASSERT (sizeof (int8)  == 1 && sizeof (int16)  == 2 && sizeof (int32)  == 4 && sizeof (int64)  == 8);
RAPICORN_STATIC_ASSERT (sizeof (int) == 4 && sizeof (uint) == 4 && sizeof (unichar) == 4);

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
#define RAPICORN_CLASS_NON_COPYABLE(ClassName)                                  private: \
  /*copy-ctor*/ ClassName  (const ClassName&) __attribute__ ((error ("NON_COPYABLE"))) = delete; \
  ClassName&    operator=  (const ClassName&) __attribute__ ((error ("NON_COPYABLE"))) = delete

// == C++ Helper Classes ==
/// Simple helper class to call one-line lambda initializers as static constructor.
struct Init {
  explicit Init (void (*f) ()) { f(); }
};

// == X86 Architecture ==
#if defined __i386__ || defined __x86_64__
#define RAPICORN_HAVE_X86_RDTSC  1
#define RAPICORN_X86_RDTSC()     ({ Rapicorn::uint32 __l_, __h_, __s_; \
                                    __asm__ __volatile__ ("rdtsc" : "=a" (__l_), "=d" (__h_));  \
                                    __s_ = __l_ + (Rapicorn::uint64 (__h_) << 32); __s_; })
#else
#define RAPICORN_HAVE_X86_RDTSC  0
#define RAPICORN_X86_RDTSC()    (0)
#endif

} // Rapicorn

#endif // __RAPICORN_CXXAUX_HH__
