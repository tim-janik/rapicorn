// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#include "inout.hh"
#include "utilities.hh"
#include "strings.hh"
#include <cstring>

namespace Rapicorn {

// export debug_handler() symbol for debugging stacktraces and gdb
extern void debug_handler (const char, const String&, const String&, const char *key = NULL) __attribute__ ((noinline));

void
debug_assert (const char *file_path, const int line, const char *message)
{
  debug_handler ('C', string_printf ("%s:%d", file_path, line), string_printf ("assertion failed: %s", message));
}

void
debug_fassert (const char *file_path, const int line, const char *message)
{
  debug_handler ('F', string_printf ("%s:%d", file_path, line), string_printf ("assertion failed: %s", message));
  ::abort();
}

void
debug_fatal (const char *file_path, const int line, const char *format, ...)
{
  va_list vargs;
  va_start (vargs, format);
  String msg = string_vprintf (format, vargs);
  va_end (vargs);
  debug_handler ('F', string_printf ("%s:%d", file_path, line), msg);
  ::abort();
}

void
debug_critical (const char *file_path, const int line, const char *format, ...)
{
  va_list vargs;
  va_start (vargs, format);
  String msg = string_vprintf (format, vargs);
  va_end (vargs);
  debug_handler ('C', string_printf ("%s:%d", file_path, line), msg);
}

void
debug_fixit (const char *file_path, const int line, const char *format, ...)
{
  va_list vargs;
  va_start (vargs, format);
  String msg = string_vprintf (format, vargs);
  va_end (vargs);
  debug_handler ('X', string_printf ("%s:%d", file_path, line), msg);
}

// == envkey_ functions ==
static int
cstring_option_sense (const char *option_string, const char *option, char *value, const int offset = 0)
{
  const char *haystack = option_string + offset;
  const char *p = strstr (haystack, option);
  if (p)                                // found possible match
    {
      const int l = strlen (option);
      if (p == haystack || (p > haystack && (p[-1] == ':' || p[-1] == ';')))
        {                               // start matches (word boundary)
          const char *d1 = strchr (p + l, ':'), *d2 = strchr (p + l, ';'), *d = MAX (d1, d2);
          d = d ? d : p + l + strlen (p + l);
          bool match = true;
          if (p[l] == '=')              // found value
            {
              const char *v = p + l + 1;
              strncpy (value, v, d - v);
            }
          else if (p[l] == 0 || p[l] == ':' || p[l] == ';')
            {                           // option present
              strcpy (value, "1");
            }
          else
            match = false;              // no match
          if (match)
            {
              const int pos = d - option_string;
              if (d[0])
                {
                  const int next = cstring_option_sense (option_string, option, value, pos);
                  if (next >= 0)        // found overriding match
                    return next;
                }
              return pos;               // this match is last match
            }
        }                               // unmatched, keep searching
      return cstring_option_sense (option_string, option, value, p + l - option_string);
    }
  return -1;                            // not present in haystack
}

static bool
fast_envkey_check (const char *option_string, const char *key)
{
  const int l = max (size_t (64), strlen (option_string) + 1);
  char kvalue[l];
  strcpy (kvalue, "0");
  const int keypos = !key ? -1 : cstring_option_sense (option_string, key, kvalue);
  char avalue[l];
  strcpy (avalue, "0");
  const int allpos = cstring_option_sense (option_string, "all", avalue);
  if (keypos > allpos)
    return cstring_to_bool (kvalue, false);
  else if (allpos > keypos)
    return cstring_to_bool (avalue, false);
  else
    return false;       // neither key nor "all" found
}

/** Check whether a flipper (feature toggle) is enabled.
 * This function first checks the environment variable @a env_var for @a key, if it is present or if
 * @a with_all_toggle is true and 'all' is present, the function returns @a true, otherwise @a false.
 * The @a cachep argument may point to a caching variable which is reset to 0 if @a env_var is
 * empty (i.e. no features can be enabled), so the caching variable can be used to prevent
 * unneccessary future envkey_flipper_check() calls.
 */
bool
envkey_flipper_check (const char *env_var, const char *key, bool with_all_toggle, volatile bool *cachep)
{
  if (env_var)          // require explicit activation
    {
      const char *val = getenv (env_var);
      if (!val || val[0] == 0)
        {
          if (cachep)
            *cachep = 0;
        }
      else if (key && fast_envkey_check (val, key))
        return true;
    }
  return false;
}

/** Check whether to print debugging message.
 * This function first checks the environment variable @a env_var for @a key, if the key is present,
 * 'all' is present or if @a env_var is NULL, the debugging message will be printed.
 * A NULL @a key checks for general debugging, it's equivalent to passing "debug" as @a key.
 * The @a cachep argument may point to a caching variable which is reset to 0 if @a env_var is
 * empty (so no debugging is enabled), so the caching variable can be used to prevent unneccessary
 * future debugging calls, e.g. to envkey_debug_message().
 */
bool
envkey_debug_check (const char *env_var, const char *key, volatile bool *cachep)
{
  if (!env_var)
    return true;        // unconditional debugging
  return envkey_flipper_check (env_var, key ? key : "debug", true, cachep);
}

/** Conditionally print debugging message.
 * This function first checks whether debugging is enabled via envkey_debug_check() and returns if not.
 * The arguments @a file_path and @a line are used to denote the debugging message source location,
 * @a format and @a va_args are formatting the message analogously to vprintf().
 */
void
envkey_debug_message (const char *env_var, const char *key, const char *file_path, const int line,
                      const char *format, va_list va_args, volatile bool *cachep)
{
  if (!envkey_debug_check (env_var, key, cachep))
    return;
  String msg = string_vprintf (format, va_args);
  debug_handler ('D', string_printf ("%s:%d", file_path, line), msg, key);
}

} // Rapicorn
