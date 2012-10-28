// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#include "strings.hh"
#include "rapicornutf8.hh"
#include "main.hh"

#include <cstring>
#include <stdio.h>
#include <stdlib.h>
#include <libintl.h>
#include <iconv.h>
#include <errno.h>
#include <glib.h>

namespace Rapicorn {

// === i18n ===
static const char *rapicorn_i18n_domain = NULL;

const char*
rapicorn_gettext (const char *text)
{
  assert (rapicorn_i18n_domain != NULL);
  return dgettext (rapicorn_i18n_domain, text);
}

static void
init_gettext (const StringVector &args)
{
  // initialize i18n functions
  rapicorn_i18n_domain = RAPICORN_I18N_DOMAIN;
  // bindtextdomain (rapicorn_i18n_domain, package_dirname);
  bind_textdomain_codeset (rapicorn_i18n_domain, "UTF-8");
}
static InitHook _init_gettext ("core/10 Init i18n Translation Domain", init_gettext);

// === String ===
String
string_multiply (const String &s,
                 uint64        count)
{
  if (count == 1)
    return s;
  else if (count & 1)
    {
      String tmp = string_multiply (s, count - 1);
      tmp += s;
      return tmp;
    }
  else if (count)
    {
      String tmp = string_multiply (s, count / 2);
      tmp += tmp;
      return tmp;
    }
  else
    return "";
}

String
string_canonify (const String &s, const String &valid_chars, const String &substitute)
{
  const size_t l = s.size();
  const char *valids = valid_chars.c_str(), *p = s.c_str();
  size_t i;
  for (i = 0; i < l; i++)
    if (!strchr (valids, p[i]))
      goto rewrite_string;
  return s; // only ref increment
 rewrite_string:
  String d = s.substr (0, i);
  d += substitute;
  for (++i; i < l; i++)
    if (strchr (valids, p[i]))
      d += p[i];
    else
      d += substitute;
  return d;
}

String
string_set_a2z ()
{
  return "abcdefghijklmnopqrstuvwxyz";
}

String
string_set_A2Z ()
{
  return "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
}

String
string_set_ascii_alnum ()
{
  return "0123456789" + string_set_A2Z() + string_set_a2z();
}

String
string_tolower (const String &str)
{
  String s (str);
  for (uint i = 0; i < s.size(); i++)
    s[i] = Unicode::tolower (s[i]);
  return s;
}

String
string_toupper (const String &str)
{
  String s (str);
  for (uint i = 0; i < s.size(); i++)
    s[i] = Unicode::toupper (s[i]);
  return s;
}

String
string_totitle (const String &str)
{
  String s (str);
  for (uint i = 0; i < s.size(); i++)
    s[i] = Unicode::totitle (s[i]);
  return s;
}

static locale_t
c_locale()
{
  static locale_t volatile clocale = NULL;
  if (!clocale)
    {
      locale_t tmploc = newlocale (LC_ALL_MASK, "C", NULL);
      if (!__sync_bool_compare_and_swap (&clocale, NULL, tmploc))
        freelocale (tmploc);
    }
  return clocale;
}

#define STACK_BUFFER_SIZE       3072

String
string_printf (const char *format, ...)
{
  va_list args;
  int l;
  {
    char buffer[STACK_BUFFER_SIZE];
    va_start (args, format);
    l = vsnprintf (buffer, sizeof (buffer), format, args);
    va_end (args);
    if (l < 0)
      return format; // error?
    if (size_t (l) < sizeof (buffer))
      return String (buffer, l);
  }
  String string;
  string.resize (l + 1);
  va_start (args, format);
  const int j = vsnprintf (&string[0], string.size(), format, args);
  va_end (args);
  string.resize (std::min (l, std::max (j, 0)));
  return string;
}

String
string_cprintf (const char *format, ...)
{
  va_list args;
  int l;
  {
    char buffer[STACK_BUFFER_SIZE];
    va_start (args, format);
    locale_t olocale = uselocale (c_locale());
    l = vsnprintf (buffer, sizeof (buffer), format, args);
    uselocale (olocale);
    va_end (args);
    if (l < 0)
      return format; // error?
    if (size_t (l) < sizeof (buffer))
      return String (buffer, l);
  }
  String string;
  string.resize (l + 1);
  va_start (args, format);
  locale_t olocale = uselocale (c_locale());
  const int j = vsnprintf (&string[0], string.size(), format, args);
  uselocale (olocale);
  va_end (args);
  string.resize (std::min (l, std::max (j, 0)));
  return string;
}

template<bool CLOCALE> static inline String
local_vprintf (const char *format, va_list vargs)
{
  locale_t olocale;
  if (CLOCALE)
    olocale = uselocale (c_locale());
  va_list pargs;
  char buffer[STACK_BUFFER_SIZE];
  buffer[0] = 0;
  va_copy (pargs, vargs);
  const int l = vsnprintf (buffer, sizeof (buffer), format, pargs);
  va_end (pargs);
  std::string string;
  if (l < 0)
    string = format; // error?
  else if (size_t (l) < sizeof (buffer))
    string = String (buffer, l);
  else
    {
      string.resize (l + 1);
      va_copy (pargs, vargs);
      const int j = vsnprintf (&string[0], string.size(), format, pargs);
      va_end (pargs);
      string.resize (std::min (l, std::max (j, 0)));
    }
  if (CLOCALE)
    uselocale (olocale);
  return string;
}

String
string_vprintf (const char *format, va_list vargs)
{
  return local_vprintf<false> (format, vargs);
}

String
string_vcprintf (const char *format, va_list vargs)
{
  return local_vprintf<true> (format, vargs);
}

static StringVector
string_whitesplit (const String &string)
{
  static const char whitespaces[] = " \t\n\r\f\v";
  StringVector sv;
  uint i, l = 0;
  for (i = 0; i < string.size(); i++)
    if (strchr (whitespaces, string[i]))
      {
        if (i > l)
          sv.push_back (string.substr (l, i - l));
        l = i + 1;
      }
  if (i > l)
    sv.push_back (string.substr (l, i - l));
  return sv;
}

StringVector
string_split (const String       &string,
              const String       &splitter)
{
  if (splitter == "")
    return string_whitesplit (string);
  StringVector sv;
  uint i, l = 0, k = splitter.size();
  for (i = 0; i < string.size(); i++)
    if (string.substr (i, k) == splitter)
      {
        if (i >= l)
          sv.push_back (string.substr (l, i - l));
        l = i + k;
      }
  if (i >= l)
    sv.push_back (string.substr (l, i - l));
  return sv;
}

String
string_join (const String       &junctor,
             const StringVector &strvec)
{
  String s;
  if (strvec.size())
    s = strvec[0];
  for (uint i = 1; i < strvec.size(); i++)
    s += junctor + strvec[i];
  return s;
}

bool
string_to_bool1 (const String &string)
{
  const char *p = string.c_str();
  while (*p && isspace (*p))
    p++;
  if (!*p)
    return true;        // option was present, but empty

  return string.empty() ? true : string_to_bool (string);
}

bool
string_to_bool (const String &string,
                bool          empty_default)
{
  const char *p = string.c_str();
  // skip spaces
  while (*p && isspace (*p))
    p++;
  // ignore signs
  if (p[0] == '-' || p[0] == '+')
    {
      p++;
      // skip spaces
      while (*p && isspace (*p))
        p++;
    }
  // handle numbers
  if (p[0] >= '0' && p[0] <= '9')
    return 0 != string_to_uint (p);
  // handle special words
  if (strncasecmp (p, "ON", 2) == 0)
    return 1;
  if (strncasecmp (p, "OFF", 3) == 0)
    return 0;
  // empty string
  if (!p[0])
    return empty_default;
  // anything else needs to resemble "yes" or "true"
  return strchr ("YyTt", p[0]);
}

String
string_from_bool (bool value)
{
  return String (value ? "1" : "0");
}

uint64
string_to_uint (const String &string,
                uint          base)
{
  const char *p = string.c_str();
  while (*p == ' ' || *p == '\n' || *p == '\t' || *p == '\r')
    p++;
  bool hex = p[0] == '0' && (p[1] == 'X' || p[1] == 'x');
  return strtoull (hex ? p + 2 : p, NULL, hex ? 16 : base);
}

String
string_from_uint (uint64 value)
{
  return string_cprintf ("%llu", value);
}

bool
string_has_int (const String &string)
{
  const char *p = string.c_str();
  while (*p == ' ' || *p == '\n' || *p == '\t' || *p == '\r')
    p++;
  return p[0] >= '0' && p[0] <= '9';
}

int64
string_to_int (const String &string,
               uint          base)
{
  const char *p = string.c_str();
  while (*p == ' ' || *p == '\n' || *p == '\t' || *p == '\r')
    p++;
  bool hex = p[0] == '0' && (p[1] == 'X' || p[1] == 'x');
  return strtoll (hex ? p + 2 : p, NULL, hex ? 16 : base);
}

String
string_from_int (int64 value)
{
  return string_cprintf ("%lld", value);
}

static long double // try strtold in current and C locale
locale_strtold (const char *nptr, char **endptr)
{
  char *fail_pos_1 = NULL;
  const long double val_1 = strtold (nptr, &fail_pos_1);
  if (fail_pos_1 && fail_pos_1[0] != 0)
    {
      char *fail_pos_2 = NULL;
      locale_t olocale = uselocale (c_locale());
      const long double val_2 = strtold (nptr, &fail_pos_2);
      uselocale (olocale);
      if (fail_pos_2 > fail_pos_1)
        {
          if (endptr)
            *endptr = fail_pos_2;
          return val_2;
        }
    }
  if (endptr)
    *endptr = fail_pos_1;
  return val_1;
}

double
string_to_double (const String &string)
{
  return locale_strtold (string.c_str(), NULL);
}

double
string_to_double (const char  *dblstring,
                  const char **endptr)
{
  return locale_strtold (dblstring, (char**) endptr);
}

String
string_from_float (float value)
{
  return string_cprintf ("%.7g", value);
}

String
string_from_double (double value)
{
  return string_cprintf ("%.17g", value);
}

vector<double>
string_to_double_vector (const String &string)
{
  vector<double> dvec;
  const char *spaces = " \t\n";
  const char *obrace = "{([";
  const char *delims = ";";
  const char *cbrace = "])}";
  const char *number = "+-0123456789eE.,";
  const char *s = string.c_str();
  /* skip spaces */
  while (*s && strchr (spaces, *s))
    s++;
  /* skip opening brace */
  if (*s && strchr (obrace, *s))
    s++;
  const char *d = s;
  while (*d && !strchr (cbrace, *d))
    {
      while (*d && strchr (spaces, *d))         /* skip spaces */
        d++;
      s = d;                                    /* start of number */
      if (!*d || (!strchr (number, *d) &&       /* ... if any */
                  !strchr (delims, *d)))
        break;
      while (*d && strchr (number, *d))         /* pass across number */
        d++;
      dvec.push_back (string_to_double (String (s, d - s)));
      while (*d && strchr (spaces, *d))         /* skip spaces */
        d++;
      if (*d && strchr (delims, *d))
        d++;                                    /* eat delimiter */
    }
  // printf ("vector: %d: %s\n", dvec.size(), string_from_double_vector (dvec).c_str());
  return dvec;
}

String
string_from_double_vector (const vector<double> &dvec,
                           const String         &delim)
{
  String s;
  for (uint i = 0; i < dvec.size(); i++)
    {
      if (i > 0)
        s += delim;
      s += string_from_double (dvec[i]);
    }
  return s;
}

String
string_from_errno (int errno_val)
{
  if (errno_val < 0)
    errno_val = -errno_val;     // fixup library return values
  char buffer[1024] = { 0, };
  if (strerror_r (errno_val, buffer, sizeof (buffer)) < 0 || !buffer[0])
    {
      /* strerror_r() may be broken on GNU systems, especially if _GNU_SOURCE is defined, so fall back to strerror() */
      return strerror (errno_val);
    }
  return buffer;
}

bool
string_is_uuid (const String &uuid_string) /* check uuid formatting */
{
  int i, l = uuid_string.size();
  if (l != 36)
    return false;
  // 00000000-0000-0000-0000-000000000000
  for (i = 0; i < l; i++)
    if (i == 8 || i == 13 || i == 18 || i == 23)
      {
        if (uuid_string[i] != '-')
          return false;
        continue;
      }
    else if ((uuid_string[i] >= '0' && uuid_string[i] <= '9') ||
             (uuid_string[i] >= 'a' && uuid_string[i] <= 'f') ||
             (uuid_string[i] >= 'A' && uuid_string[i] <= 'F'))
      continue;
    else
      return false;
  return true;
}

int
string_cmp_uuid (const String &uuid_string1,
                 const String &uuid_string2) /* -1=smaller, 0=equal, +1=greater (assuming valid uuid strings) */
{
  return strcasecmp (uuid_string1.c_str(), uuid_string2.c_str()); /* good enough for numeric equality and defines stable order */
}

String
string_from_pretty_function_name (const char *gnuc_pretty_function)
{
  /* finding the function name is non-trivial in the presence of function pointer
   * return types. the following code assumes the function name preceedes the
   * first opening parenthesis not followed by a star.
   */
  const char *op = strchr (gnuc_pretty_function, '(');
  while (op && op[1] == '*')
    op = strchr (op + 1, '(');
  if (!op)
    return gnuc_pretty_function;
  // *op == '(' && op[1] != '*'
  const char *last = op - 1;
  while (last >= gnuc_pretty_function && strchr (" \t\n", *last))
    last--;
  if (last < gnuc_pretty_function)
    return gnuc_pretty_function;
  const char *first = last;
  while (first >= gnuc_pretty_function && strchr ("0123456789_ABCDEFGHIJKLMNOPQRSTUVWXYZ:abcdefghijklmnopqrstuvwxyz$", *first))
    first--;
  String result = String (first + 1, last - first);
  return result;
}

String
string_to_cescape (const String &str)
{
  String buffer;
  for (String::const_iterator it = str.begin(); it != str.end(); it++)
    {
      uint8 d = *it;
      if (d < 32 || d > 126 || d == '?')
        buffer += string_cprintf ("\\%03o", d);
      else if (d == '\\')
        buffer += "\\\\";
      else if (d == '"')
        buffer += "\\\"";
      else
        buffer += d;
    }
  return buffer;
}

String
string_to_cquote (const String &str)
{
  return String() + "\"" + string_to_cescape (str) + "\"";
}

String
string_from_cquote (const String &input)
{
  uint i = 0;
  if (i < input.size() && (input[i] == '"' || input[i] == '\''))
    {
      const char qchar = input[i];
      i++;
      String out;
      bool be = false;
      while (i < input.size() && (input[i] != qchar || be))
        {
          if (!be && input[i] == '\\')
            be = true;
          else
            {
              if (be)
                switch (input[i])
                  {
                    uint k, oc;
                  case '0': case '1': case '2': case '3':
                  case '4': case '5': case '6': case '7':
                    k = MIN (input.size(), i + 3);
                    oc = input[i++] - '0';
                    while (i < k && input[i] >= '0' && input[i] <= '7')
                      oc = oc * 8 + input[i++] - '0';
                    out += char (oc);
                    i--;
                    break;
                  case 'n':     out += '\n';            break;
                  case 'r':     out += '\r';            break;
                  case 't':     out += '\t';            break;
                  case 'b':     out += '\b';            break;
                  case 'f':     out += '\f';            break;
                  case 'v':     out += '\v';            break;
                  default:      out += input[i];        break;
                  }
              else
                out += input[i];
              be = false;
            }
          i++;
        }
      if (i < input.size() && input[i] == qchar)
        {
          i++;
          if (i < input.size())
            return input; // extraneous characters after string quotes
          return out;
        }
      else
        return input; // unclosed string quotes
    }
  else if (i == input.size())
    return input; // empty string arg: ""
  else
    return input; // missing string quotes
}

static const char *whitespaces = " \t\v\f\n\r";

String
string_lstrip (const String &input)
{
  uint64 i = 0;
  while (i < input.size() && strchr (whitespaces, input[i]))
    i++;
  return i ? input.substr (i) : input;
}

String
string_rstrip (const String &input)
{
  uint64 i = input.size();
  while (i > 0 && strchr (whitespaces, input[i - 1]))
    i--;
  return i < input.size() ? input.substr (0, i) : input;
}

String
string_strip (const String &input)
{
  uint64 a = 0;
  while (a < input.size() && strchr (whitespaces, input[a]))
    a++;
  uint64 b = input.size();
  while (b > 0 && strchr (whitespaces, input[b - 1]))
    b--;
  if (a == 0 && b == input.size())
    return input;
  else if (b == 0)
    return "";
  else
    return input.substr (a, b - a);
}

String
string_substitute_char (const String &input,
                        const char    match,
                        const char    subst)
{
  String output = input;
  if (match != subst)
    for (String::size_type i = 0; i < output.size(); i++)
      if (output.data()[i] == match)
        output[i] = subst; // unshares string
  return output;
}

void
memset4 (guint32        *mem,
         guint32         filler,
         guint           length)
{
  RAPICORN_STATIC_ASSERT (sizeof (*mem) == 4);
  RAPICORN_STATIC_ASSERT (sizeof (filler) == 4);
  RAPICORN_STATIC_ASSERT (sizeof (wchar_t) == 4);
  wmemset ((wchar_t*) mem, filler, length);
}

/**
 * Search for @a key in @a svector and return reminder of the matching string.
 * If multiple matches are possible, the last one is returned.
 * @returns @a fallback if no match was found.
 */
String
string_vector_find (const StringVector &svector,
                    const String       &key,
                    const String       &fallback)
{
  for (size_t i = svector.size(); i > 0; i--)
    {
      const String &s = svector[i-1];
      if (s.size() >= key.size() && strncmp (s.data(), key.data(), key.size()) == 0)
        return s.substr (key.size());
    }
  return fallback;
}

StringVector
cstrings_to_vector (const char *s, ...)
{
  StringVector sv;
  if (s)
    {
      sv.push_back (s);
      va_list args;
      va_start (args, s);
      s = va_arg (args, const char*);
      while (s)
        {
          sv.push_back (s);
          s = va_arg (args, const char*);
        }
      va_end (args);
    }
  return sv;
}

// === String Options ===
#define is_sep(c)               (c == ';' || c == ':')
#define is_spacesep(c)          (isspace (c) || is_sep (c))
#define find_sep(str)           (strpbrk (str, ";:"))

static void
string_option_add (const String   &assignment,
                   vector<String> *option_namesp,
                   vector<String> &option_values,
                   const String   &empty_default,
                   const String   *filter)
{
  assert_return ((option_namesp != NULL) ^ (filter != NULL));
  const char *n = assignment.c_str();
  while (isspace (*n))
    n++;
  const char *p = n;
  while (isalnum (*p) || *p == '-' || *p == '_')
    p++;
  const String name = String (n, p - n);
  if (filter && name != *filter)
    return;
  while (isspace (*p))
    p++;
  const String value = *p == '=' ? String (p + 1) : empty_default;
  if (!name.empty() && (*p == '=' || *p == 0)) // valid name
    {
      if (!filter)
        option_namesp->push_back (name);
      option_values.push_back (value);
    }
}

static void
string_options_split_filtered (const String   &option_string,
                               vector<String> *option_namesp,
                               vector<String> &option_values,
                               const String   &empty_default,
                               const String   *filter)
{
  const char *s = option_string.c_str();
  while (s)
    {
      // find next separator
      const char *b = find_sep (s);
      string_option_add (String (s, b ? b - s : strlen (s)), option_namesp, option_values, empty_default, filter);
      s = b ? b + 1 : NULL;
    }
}

void
string_options_split (const String   &option_string,
                      vector<String> &option_names,
                      vector<String> &option_values,
                      const String   &empty_default)
{
  string_options_split_filtered (option_string, &option_names, option_values, empty_default, NULL);
}

static String
string_option_find_value (const String &option_string,
                          const String &option)
{
  vector<String> option_names, option_values;
  string_options_split_filtered (option_string, NULL, option_values, "1", &option);
  return option_values.empty() ? "0" : option_values[option_values.size() - 1];
}

String
string_option_get (const String   &option_string,
                   const String   &option)
{
  return string_option_find_value (option_string, option);
}

bool
string_option_check (const String   &option_string,
                     const String   &option)
{
  const String value = string_option_find_value (option_string, option);
  return string_to_bool (value, true);
}

// == Strings ==
Strings::Strings (CS &s1)
{ push_back (s1); }
Strings::Strings (CS &s1, CS &s2)
{ push_back (s1); push_back (s2); }
Strings::Strings (CS &s1, CS &s2, CS &s3)
{ push_back (s1); push_back (s2); push_back (s3); }
Strings::Strings (CS &s1, CS &s2, CS &s3, CS &s4)
{ push_back (s1); push_back (s2); push_back (s3); push_back (s4); }
Strings::Strings (CS &s1, CS &s2, CS &s3, CS &s4, CS &s5)
{ push_back (s1); push_back (s2); push_back (s3); push_back (s4); push_back (s5); }
Strings::Strings (CS &s1, CS &s2, CS &s3, CS &s4, CS &s5, CS &s6)
{ push_back (s1); push_back (s2); push_back (s3); push_back (s4); push_back (s5); push_back (s6); }
Strings::Strings (CS &s1, CS &s2, CS &s3, CS &s4, CS &s5, CS &s6, CS &s7)
{ push_back (s1); push_back (s2); push_back (s3); push_back (s4); push_back (s5); push_back (s6); push_back (s7); }
Strings::Strings (CS &s1, CS &s2, CS &s3, CS &s4, CS &s5, CS &s6, CS &s7, CS &s8)
{ push_back (s1); push_back (s2); push_back (s3); push_back (s4); push_back (s5); push_back (s6);
  push_back (s7); push_back (s8); }
Strings::Strings (CS &s1, CS &s2, CS &s3, CS &s4, CS &s5, CS &s6, CS &s7, CS &s8, CS &s9)
{ push_back (s1); push_back (s2); push_back (s3); push_back (s4); push_back (s5); push_back (s6);
  push_back (s7); push_back (s8); push_back (s9); }
Strings::Strings (CS &s1, CS &s2, CS &s3, CS &s4, CS &s5, CS &s6, CS &s7, CS &s8, CS &s9, CS &sA)
{ push_back (s1); push_back (s2); push_back (s3); push_back (s4); push_back (s5); push_back (s6);
  push_back (s7); push_back (s8); push_back (s9); push_back (sA); }
Strings::Strings (CS &s1, CS &s2, CS &s3, CS &s4, CS &s5, CS &s6, CS &s7, CS &s8, CS &s9, CS &sA, CS &sB)
{ push_back (s1); push_back (s2); push_back (s3); push_back (s4); push_back (s5); push_back (s6);
  push_back (s7); push_back (s8); push_back (s9); push_back (sA); push_back (sB); }
Strings::Strings (CS &s1, CS &s2, CS &s3, CS &s4, CS &s5, CS &s6, CS &s7, CS &s8, CS &s9, CS &sA, CS &sB, CS &sC)
{ push_back (s1); push_back (s2); push_back (s3); push_back (s4); push_back (s5); push_back (s6);
  push_back (s7); push_back (s8); push_back (s9); push_back (sA); push_back (sB); push_back (sC); }

// === Charset Conversions ===
static bool
unalias_encoding (String &name)
{
  /* list of common aliases for MIME encodings */
  static const char *encoding_aliases[] = {
    /* alias            MIME (GNU CANONICAL) */
    "UTF8",             "UTF-8",
    /* ascii */
    "646",              "ASCII",
    "ISO_646.IRV:1983", "ASCII",
    "CP20127",          "ASCII",
    /* iso8859 aliases */
    "LATIN1",           "ISO-8859-1",
    "LATIN2",           "ISO-8859-2",
    "LATIN3",           "ISO-8859-3",
    "LATIN4",           "ISO-8859-4",
    "LATIN5",           "ISO-8859-9",
    "LATIN6",           "ISO-8859-10",
    "LATIN7",           "ISO-8859-13",
    "LATIN8",           "ISO-8859-14",
    "LATIN9",           "ISO-8859-15",
    "LATIN10",          "ISO-8859-16",
    "ISO8859-1",        "ISO-8859-1",
    "ISO8859-2",        "ISO-8859-2",
    "ISO8859-3",        "ISO-8859-3",
    "ISO8859-4",        "ISO-8859-4",
    "ISO8859-5",        "ISO-8859-5",
    "ISO8859-6",        "ISO-8859-6",
    "ISO8859-7",        "ISO-8859-7",
    "ISO8859-8",        "ISO-8859-8",
    "ISO8859-9",        "ISO-8859-9",
    "ISO8859-13",       "ISO-8859-13",
    "ISO8859-15",       "ISO-8859-15",
    "CP28591",          "ISO-8859-1",
    "CP28592",          "ISO-8859-2",
    "CP28593",          "ISO-8859-3",
    "CP28594",          "ISO-8859-4",
    "CP28595",          "ISO-8859-5",
    "CP28596",          "ISO-8859-6",
    "CP28597",          "ISO-8859-7",
    "CP28598",          "ISO-8859-8",
    "CP28599",          "ISO-8859-9",
    "CP28603",          "ISO-8859-13",
    "CP28605",          "ISO-8859-15",
    /* EUC aliases */
    "eucCN",            "GB2312",
    "IBM-eucCN",        "GB2312",
    "dechanzi",         "GB2312",
    "eucJP",            "EUC-JP",
    "IBM-eucJP",        "EUC-JP",
    "sdeckanji",        "EUC-JP",
    "eucKR",            "EUC-KR",
    "IBM-eucKR",        "EUC-KR",
    "deckorean",        "EUC-KR",
    "eucTW",            "EUC-TW",
    "IBM-eucTW",        "EUC-TW",
    "CNS11643",         "EUC-TW",
    "CP20866",          "KOI8-R",
    /* misc */
    "PCK",              "SHIFT_JIS",
    "SJIS",             "SHIFT_JIS",
  };
  /* find a MIME encoding from alias list */
  for (uint i = 0; i < sizeof (encoding_aliases) / sizeof (encoding_aliases[0]); i += 2)
    if (strcasecmp (encoding_aliases[i], name.c_str()) == 0)
      {
        name = encoding_aliases[i + 1];
        return true;
      }
  /* last resort, try upper-casing the encoding name */
  String upper = name;
  for (uint i = 0; i < upper.size(); i++)
    if (upper[i] >= 'a' && upper[i] <= 'z')
      upper[i] += 'A' - 'a';
  if (upper != name)
    {
      name = upper;
      return true;
    }
  /* alias not found */
  return false;
}

static iconv_t
aliased_iconv_open (const String &tocode,
                    const String &fromcode)
{
  const iconv_t icNONE = (iconv_t) -1;
  iconv_t cd = iconv_open (tocode.c_str(), fromcode.c_str());
  if (cd != icNONE)
    return cd;
  /* lookup destination encoding from alias and retry */
  String to_encoding = tocode;
  if (unalias_encoding (to_encoding))
    {
      cd = iconv_open (to_encoding.c_str(), fromcode.c_str());
      if (cd != icNONE)
        return cd;
      /* lookup source and destination encoding from alias and retry */
      String from_encoding = fromcode;
      if (unalias_encoding (from_encoding))
        {
          cd = iconv_open (to_encoding.c_str(), from_encoding.c_str());
          if (cd != icNONE)
            return cd;
        }
    }
  /* lookup source encoding from alias and retry */
  String from_encoding = fromcode;
  if (unalias_encoding (from_encoding))
    {
      cd = iconv_open (tocode.c_str(), from_encoding.c_str());
      if (cd != icNONE)
        return cd;
    }
  return icNONE; /* encoding not found */
}

bool
text_convert (const String &to_charset,
              String       &output_string,
              const String &from_charset,
              const String &input_string,
              const String &fallback_charset,
              const String &output_mark)
{
  output_string = "";
  const iconv_t icNONE = (iconv_t) -1;
  iconv_t alt_cd = icNONE, cd = aliased_iconv_open (to_charset, from_charset);
  if (cd == icNONE)
    return false;                       /* failed to perform the requested conversion */
  const char *iptr = input_string.c_str();
  size_t ilength = input_string.size();
  char obuffer[1024];                   /* declared outside loop to spare re-initialization */
  String alt_charset = fallback_charset;
  while (ilength)
    {
      /* convert */
      char *optr = obuffer;
      size_t olength = sizeof (obuffer);
      size_t n = iconv (cd, const_cast<char**> (&iptr), &ilength, &optr, &olength);
      /* transfer output */
      output_string.append (obuffer, optr - obuffer);
      /* handle conversion errors */
      if (ilength &&                    /* ignore past end errors */
          n == (size_t) -1)
        {
          if (errno == EINVAL ||        /* unfinished multibyte sequences follows (near end of string) */
              errno == EILSEQ)          /* invalid multibyte sequence follows */
            {
              /* open alternate converter */
              if (alt_cd == icNONE && alt_charset.size())
                {
                  alt_cd = aliased_iconv_open (to_charset, alt_charset);
                  alt_charset = ""; /* don't retry iconv_open() */
                }
              size_t former_ilength = ilength;
              if (alt_cd != icNONE)
                {
                  /* convert from alt_charset */
                  optr = obuffer;
                  olength = sizeof (obuffer);
                  n = iconv (alt_cd, const_cast<char**> (&iptr), &ilength, &optr, &olength);
                  /* transfer output */
                  output_string.append (obuffer, optr - obuffer);
                }
              if (ilength == former_ilength)
                {
                  /* failed alternate conversion, mark invalid character */
                  output_string += output_mark;
                  iptr++;
                  ilength--;
                }
            }
          else  /* all other errors are considered fatal */
            return false;               /* failed to perform the requested conversion */
        }
    }
  iconv_close (cd);
  if (alt_cd != icNONE)
    iconv_close (alt_cd);
  return true;

}

} // Rapicorn
