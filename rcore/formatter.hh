// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __RAPICORN_FORMATTER_HH__
#define __RAPICORN_FORMATTER_HH__

#include <rcore/utilities.hh>
#include <sstream>

namespace Rapicorn {
namespace Lib { // Namespace for implementation internals

// == StringFormatter ==
///@cond

/** StringFormatter - printf-like string formatting for C++.
 *
 * See parse_directive() for supported flags, modifiers and conversions.
 * Currently missing are: n I S ls C lc
 */
class StringFormatter {
  typedef long long unsigned int LLUInt;
  typedef long double LDouble;
  struct FormatArg {
    union { LDouble d; LLUInt i; void *p; const char *s; };
    char kind; // i d s p
  };
  inline void assign (FormatArg &farg, bool               arg) { farg.kind = 'i'; farg.i = arg; }
  inline void assign (FormatArg &farg, char               arg) { farg.kind = 'i'; farg.i = arg; }
  inline void assign (FormatArg &farg, signed char        arg) { farg.kind = 'i'; farg.i = arg; }
  inline void assign (FormatArg &farg, unsigned char      arg) { farg.kind = 'i'; farg.i = arg; }
  inline void assign (FormatArg &farg, short              arg) { farg.kind = 'i'; farg.i = arg; }
  inline void assign (FormatArg &farg, unsigned short     arg) { farg.kind = 'i'; farg.i = arg; }
  inline void assign (FormatArg &farg, int                arg) { farg.kind = 'i'; farg.i = arg; }
  inline void assign (FormatArg &farg, unsigned int       arg) { farg.kind = 'i'; farg.i = arg; }
  inline void assign (FormatArg &farg, wchar_t            arg) { farg.kind = 'i'; farg.i = arg; }
  inline void assign (FormatArg &farg, long               arg) { farg.kind = 'i'; farg.i = arg; }
  inline void assign (FormatArg &farg, unsigned long      arg) { farg.kind = 'i'; farg.i = arg; }
  inline void assign (FormatArg &farg, long long          arg) { farg.kind = 'i'; farg.i = arg; }
  inline void assign (FormatArg &farg, unsigned long long arg) { farg.kind = 'i'; farg.i = arg; }
  inline void assign (FormatArg &farg, float              arg) { farg.kind = 'd'; farg.d = arg; }
  inline void assign (FormatArg &farg, double             arg) { farg.kind = 'd'; farg.d = arg; }
  inline void assign (FormatArg &farg, long double        arg) { farg.kind = 'd'; farg.d = arg; }
  inline void assign (FormatArg &farg, const char        *arg) { farg.kind = 's'; farg.s = arg; }
  inline void assign (FormatArg &farg, const std::string &arg) { assign (farg, arg.c_str()); }
  inline void assign (FormatArg &farg, void              *arg) { farg.kind = 'p'; farg.p = arg; }
  template<class T> inline void assign (FormatArg &farg, T *const &arg) { assign (farg, (void*) arg); }
  template<class T> inline void assign (FormatArg &farg, const T &arg)
  {
    std::ostringstream os;
    os << arg;
    temporaries_.push_back (os.str());
    assign (farg, temporaries_[temporaries_.size()-1]);
  }
  uint32_t    arg_as_width     (size_t nth);
  uint32_t    arg_as_precision (size_t nth);
  LLUInt      arg_as_lluint    (size_t nth);
  LDouble     arg_as_ldouble   (size_t nth);
  void*       arg_as_ptr       (size_t nth);
  const char* arg_as_chars     (size_t nth);
  struct Directive {
    char     conversion;
    uint32_t adjust_left : 1, add_sign : 1, use_width : 1, use_precision : 1;
    uint32_t alternate_form : 1, zero_padding : 1, add_space : 1, locale_grouping : 1;
    uint32_t field_width, precision, start, end, value_index, width_index, precision_index;
    Directive() :
      conversion (0), adjust_left (0), add_sign (0), use_width (0), use_precision (0),
      alternate_form (0), zero_padding (0), add_space (0), locale_grouping (0),
      field_width (0), precision (0), start (0), end (0), value_index (0), width_index (0), precision_index (0)
    {}
  };
  FormatArg    *const fargs_;
  const size_t        nargs_;
  const int           locale_context_;
  vector<std::string> temporaries_;
  static std::string            format_error     (const char *err, const char *format, size_t directive);
  static const char*            parse_directive  (const char **stringp, size_t *indexp, Directive *dirp);
  std::string                   locale_format    (size_t last, const char *format);
  std::string                   render_format    (size_t last, const char *format);
  std::string                   render_directive (const Directive &dir);
  template<class A> std::string render_arg       (const Directive &dir, const char *modifier, A arg);
  template<size_t N> inline std::string
  intern_format (const char *format)
  {
    return locale_format (N, format);
  }
  template<size_t N, class A, class ...Args> inline std::string
  intern_format (const char *format, const A &arg, const Args &...args)
  {
    assign (fargs_[N], arg);
    return intern_format<N+1> (format, args...);
  }
  template<size_t N> inline constexpr
  StringFormatter (size_t nargs, FormatArg (&mem)[N], int lc) : fargs_ (mem), nargs_ (nargs), locale_context_ (lc) {}
public:
  enum LocaleContext {
    POSIX_LOCALE,
    CURRENT_LOCALE,
  };
  template<LocaleContext LC = POSIX_LOCALE, class ...Args>
  static __attribute__ ((__format__ (printf, 1, 0), noinline)) std::string
  format (const char *format, const Args &...args)
  {
    constexpr size_t N = sizeof... (Args);
    FormatArg mem[N ? N : 1];
    StringFormatter formatter (N, mem, LC);
    return formatter.intern_format<0> (format, args...);
  }
};

///@endcond

} // Lib
} // Rapicorn

#endif /* __RAPICORN_FORMATTER_HH__ */
