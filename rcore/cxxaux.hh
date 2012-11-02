// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __RAPICORN_CXXAUX_HH__
#define __RAPICORN_CXXAUX_HH__

#include <rcore/rapicornconfig.h> /* _GNU_SOURCE */
#include <stdbool.h>
#include <stdarg.h>
#include <stddef.h>			/* NULL */
#include <sys/types.h>			/* uint, ssize */
#include <stdint.h>			/* uint64_t */
#include <limits.h>                     /* {INT|CHAR|...}_{MIN|MAX} */
#include <float.h>                      /* {FLT|DBL}_{MIN|MAX|EPSILON} */

RAPICORN_EXTERN_C_BEGIN();

/* --- standard macros --- */
#ifndef FALSE
#  define FALSE					false
#endif
#ifndef TRUE
#  define TRUE					true
#endif
#define RAPICORN_ABS(a)                       	((a) > -(a) ? (a) : -(a))
#define RAPICORN_MIN(a,b)                         ((a) <= (b) ? (a) : (b))
#define RAPICORN_MAX(a,b)                         ((a) >= (b) ? (a) : (b))
#define RAPICORN_CLAMP(v,mi,ma)                   ((v) < (mi) ? (mi) : ((v) > (ma) ? (ma) : (v)))
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
#ifdef  LLONG_MAX       /* some gcc versions ship limits.h that fail to define LLONG_MAX for C99 */
#  define INT64_MAX     LLONG_MAX       // +9223372036854775807LL
#  define INT64_MIN     LLONG_MIN       // -9223372036854775808LL
#  define UINT64_MAX    ULLONG_MAX      // +18446744073709551615LLU
#else /* !LLONG_MAX but gcc always has __LONG_LONG_MAX__ */
#  define INT64_MAX     __LONG_LONG_MAX__
#  define INT64_MIN     (-INT64_MAX - 1LL)
#  define UINT64_MAX    (INT64_MAX * 2ULL + 1ULL)
#endif
#endif

/* --- likelyness hinting --- */
#define	RAPICORN__BOOL(expr)		__extension__ ({ bool _rapicorn__bool; if (expr) _rapicorn__bool = 1; else _rapicorn__bool = 0; _rapicorn__bool; })
#define	RAPICORN_ISLIKELY(expr)		__builtin_expect (RAPICORN__BOOL (expr), 1)
#define	RAPICORN_UNLIKELY(expr)		__builtin_expect (RAPICORN__BOOL (expr), 0)
#define	RAPICORN_LIKELY			RAPICORN_ISLIKELY

/* --- assertions and runtime errors --- */
#ifndef __cplusplus
#define RAPICORN_RETURN_IF_FAIL(e)	 do { if (RAPICORN_ISLIKELY (e)) break; RAPICORN__RUNTIME_PROBLEM ('R', RAPICORN_LOG_DOMAIN, __FILE__, __LINE__, RAPICORN_SIMPLE_FUNCTION, "%s", #e); return; } while (0)
#define RAPICORN_RETURN_VAL_IF_FAIL(e,v) do { if (RAPICORN_ISLIKELY (e)) break; RAPICORN__RUNTIME_PROBLEM ('R', RAPICORN_LOG_DOMAIN, __FILE__, __LINE__, RAPICORN_SIMPLE_FUNCTION, "%s", #e); return v; } while (0)
#define RAPICORN_ASSERT(e)		 do { if (RAPICORN_ISLIKELY (e)) break; RAPICORN__RUNTIME_PROBLEM ('A', RAPICORN_LOG_DOMAIN, __FILE__, __LINE__, RAPICORN_SIMPLE_FUNCTION, "%s", #e); while (1) *(void*volatile*)0; } while (0)
#define RAPICORN_ASSERT_UNREACHED()	 do { RAPICORN__RUNTIME_PROBLEM ('N', RAPICORN_LOG_DOMAIN, __FILE__, __LINE__, RAPICORN_SIMPLE_FUNCTION, NULL); RAPICORN_ABORT_NORETURN(); } while (0)
#define RAPICORN_WARNING(...)		 do { RAPICORN__RUNTIME_PROBLEM ('W', RAPICORN_LOG_DOMAIN, __FILE__, __LINE__, RAPICORN_SIMPLE_FUNCTION, __VA_ARGS__); } while (0)
#define RAPICORN_ERROR(...)		 do { RAPICORN__RUNTIME_PROBLEM ('E', RAPICORN_LOG_DOMAIN, __FILE__, __LINE__, RAPICORN_SIMPLE_FUNCTION, __VA_ARGS__); RAPICORN_ABORT_NORETURN(); } while (0)
#define RAPICORN_ABORT_NORETURN()	 rapicorn_abort_noreturn()
#endif

/* --- convenient aliases --- */
#ifdef  RAPICORN_CONVENIENCE
#define	ISLIKELY		RAPICORN_ISLIKELY
#define	UNLIKELY		RAPICORN_UNLIKELY
#define	LIKELY			RAPICORN_LIKELY
#define	STRINGIFY               RAPICORN_CPP_STRINGIFY
#endif

/* --- preprocessor pasting --- */
#define RAPICORN_CPP_PASTE4i(a,b,c,d)             a ## b ## c ## d
#define RAPICORN_CPP_PASTE4(a,b,c,d)              RAPICORN_CPP_PASTE4i (a,b,c,d)
#define RAPICORN_CPP_PASTE3i(a,b,c)               a ## b ## c   /* indirection required to expand __LINE__ etc */
#define RAPICORN_CPP_PASTE3(a,b,c)                RAPICORN_CPP_PASTE3i (a,b,c)
#define RAPICORN_CPP_PASTE2i(a,b)                 a ## b        /* indirection required to expand __LINE__ etc */
#define RAPICORN_CPP_PASTE2(a,b)                  RAPICORN_CPP_PASTE2i (a,b)
#define RAPICORN_CPP_STRINGIFYi(s)                #s            /* indirection required to expand __LINE__ etc */
#define RAPICORN_CPP_STRINGIFY(s)                 RAPICORN_CPP_STRINGIFYi (s)
#define RAPICORN_STATIC_ASSERT_NAMED(expr,asname) typedef struct { char asname[(expr) ? 1 : -1]; } RAPICORN_CPP_PASTE2 (Rapicorn_StaticAssertion_LINE, __LINE__)
#define RAPICORN_STATIC_ASSERT(expr)              RAPICORN_STATIC_ASSERT_NAMED (expr, compile_time_assertion_failed)

/* --- attributes --- */
#if     __GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 3)
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
#define RAPICORN_CONSTRUCTOR			__attribute__ ((constructor,used)) /* gcc-3.3 also needs "used" to emit code */
#define RAPICORN_MAY_ALIAS                      __attribute__ ((may_alias))
#else   /* !__GNUC__ */
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
#error  Failed to detect a recent GCC version (>= 3.3)
#endif  /* !__GNUC__ */
#if     defined __GNUC__ && defined __cplusplus
#define	RAPICORN_SIMPLE_FUNCTION			(::Rapicorn::string_from_pretty_function_name (__PRETTY_FUNCTION__).c_str())
#else
#define	RAPICORN_SIMPLE_FUNCTION			(__func__)
#endif

/* --- provide canonical integer types --- */
#if 	RAPICORN_SIZEOF_SYS_TYPESH_UINT == 0
typedef unsigned int		uint;	/* for systems that don't define uint in types.h */
#else
RAPICORN_STATIC_ASSERT (RAPICORN_SIZEOF_SYS_TYPESH_UINT == 4);
#endif
RAPICORN_STATIC_ASSERT (sizeof (uint) == 4);
typedef uint8_t                 RapicornUInt8;  // __attribute__ ((__mode__ (__QI__)))
typedef uint16_t                RapicornUInt16; // __attribute__ ((__mode__ (__HI__)))
typedef uint32_t                RapicornUInt32; // __attribute__ ((__mode__ (__SI__)))
typedef unsigned long long int  RapicornUInt64; // __attribute__ ((__mode__ (__DI__)))
// typedef uint64_t RapicornUInt64; // uint64_t is a long on AMD64 which breaks printf
// Using stdint.h defines here for type compatibility with standard APIs (references, printf, ...)
RAPICORN_STATIC_ASSERT (sizeof (RapicornUInt8)  == 1);
RAPICORN_STATIC_ASSERT (sizeof (RapicornUInt16) == 2);
RAPICORN_STATIC_ASSERT (sizeof (RapicornUInt32) == 4);
RAPICORN_STATIC_ASSERT (sizeof (RapicornUInt64) == 8);
typedef int8_t                  RapicornInt8;  // __attribute__ ((__mode__ (__QI__)))
typedef int16_t                 RapicornInt16; // __attribute__ ((__mode__ (__HI__)))
typedef int32_t                 RapicornInt32; // __attribute__ ((__mode__ (__SI__)))
typedef signed long long int    RapicornInt64; // __attribute__ ((__mode__ (__DI__)))
// typedef int64_t RapicornInt64; // int64_t is a long on AMD64 which breaks printf
// provided by compiler       int;
RAPICORN_STATIC_ASSERT (sizeof (RapicornInt8)  == 1);
RAPICORN_STATIC_ASSERT (sizeof (RapicornInt16) == 2);
RAPICORN_STATIC_ASSERT (sizeof (RapicornInt32) == 4);
RAPICORN_STATIC_ASSERT (sizeof (RapicornInt64) == 8);
typedef RapicornUInt32		RapicornUnichar;
RAPICORN_STATIC_ASSERT (sizeof (RapicornUnichar) == 4);


/* --- path handling --- */
#ifdef	RAPICORN_OS_WIN32
#define RAPICORN_DIR_SEPARATOR		  '\\'
#define RAPICORN_DIR_SEPARATOR_S		  "\\"
#define RAPICORN_SEARCHPATH_SEPARATOR	  ';'
#define RAPICORN_SEARCHPATH_SEPARATOR_S	  ";"
#else	/* !RAPICORN_OS_WIN32 */
#define RAPICORN_DIR_SEPARATOR		  '/'
#define RAPICORN_DIR_SEPARATOR_S		  "/"
#define RAPICORN_SEARCHPATH_SEPARATOR	  ':'
#define RAPICORN_SEARCHPATH_SEPARATOR_S	  ":"
#endif	/* !RAPICORN_OS_WIN32 */
#define	RAPICORN_IS_DIR_SEPARATOR(c)    	  ((c) == RAPICORN_DIR_SEPARATOR)
#define RAPICORN_IS_SEARCHPATH_SEPARATOR(c) ((c) == RAPICORN_SEARCHPATH_SEPARATOR)


// == X86 Architecture ==
#if defined __i386__ || defined __x86_64__
#define RAPICORN_HAVE_X86_RDTSC  1
#define RAPICORN_X86_RDTSC()            ({ RapicornUInt32 __l_, __h_, __s_;                           \
                                           __asm__ __volatile__ ("rdtsc" : "=a" (__l_), "=d" (__h_)); \
                                           __s_ = __l_ + (RapicornUInt64 (__h_) << 32); __s_; })
#else
#define RAPICORN_HAVE_X86_RDTSC  0
#define RAPICORN_X86_RDTSC()    (0)
#endif

/* --- implementation bits --- */
/* the above macros rely on a problem handler macro: */
// RAPICORN__RUNTIME_PROBLEM(ErrorWarningReturnAssertNotreach,domain,file,line,funcname,exprformat,...); // noreturn cases: 'E', 'A', 'N'
extern inline void rapicorn_abort_noreturn (void) RAPICORN_NORETURN;
extern inline void rapicorn_abort_noreturn (void) { while (1) *(void*volatile*)0; }
RAPICORN_EXTERN_C_END();

#endif /* __RAPICORN_CXXAUX_HH__ */
