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
string_tolower (const String &str)
{
  String s (str);
  for (uint i = 0; i < s.size(); i++)
    s[i] = Unichar::tolower (s[i]);
  return s;
}

String
string_toupper (const String &str)
{
  String s (str);
  for (uint i = 0; i < s.size(); i++)
    s[i] = Unichar::toupper (s[i]);
  return s;
}

String
string_totitle (const String &str)
{
  String s (str);
  for (uint i = 0; i < s.size(); i++)
    s[i] = Unichar::totitle (s[i]);
  return s;
}

String
string_printf (const char *format,
               ...)
{
  String str;
  va_list args;
  va_start (args, format);
  str = string_vprintf (format, args);
  va_end (args);
  return str;
}

String
string_vprintf (const char *format,
                va_list     vargs)
{
  char *str = NULL;
  if (vasprintf (&str, format, vargs) >= 0 && str)
    {
      String s = str;
      free (str);
      return s;
    }
  else
    return format;
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
string_to_bool (const String &string)
{
  static const char *spaces = " \t\n\r";
  const char *p = string.c_str();
  // special case empty string
  if (!p[0])
    return false;
  // skip spaces
  while (*p && strchr (spaces, *p))
    p++;
  // ignore signs
  if (p[0] == '-' || p[0] == '+')
    {
      p++;
      // skip spaces
      while (*p && strchr (spaces, *p))
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
  // handle non-numbers
  return !(p[0] == 0 ||
           p[0] == 'f' || p[0] == 'F' ||
           p[0] == 'n' || p[0] == 'N');
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
  return string_printf ("%llu", value);
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
  return string_printf ("%lld", value);
}

double
string_to_double (const String &string)
{
  return g_ascii_strtod (string.c_str(), NULL);
}

double
string_to_double (const char  *dblstring,
                  const char **endptr)
{
  return g_ascii_strtod (dblstring, (char**) endptr);
}

String
string_from_float (float value)
{
  char numbuf[G_ASCII_DTOSTR_BUF_SIZE + 1] = { 0, };
  g_ascii_formatd (numbuf, G_ASCII_DTOSTR_BUF_SIZE, "%.7g", value);
  return String (numbuf);
}

String
string_from_double (double value)
{
  char numbuf[G_ASCII_DTOSTR_BUF_SIZE + 1] = { 0, };
  g_ascii_formatd (numbuf, G_ASCII_DTOSTR_BUF_SIZE, "%.17g", value);
  return String (numbuf);
}

vector<double>
string_to_vector (const String &string)
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
  // printf ("vector: %d: %s\n", dvec.size(), string_from_vector (dvec).c_str());
  return dvec;
}

String
string_from_vector (const vector<double> &dvec,
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
        buffer += string_printf ("\\%03o", d);
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

// === String Options ===
static const char*
string_option_find_value (const String   &option_string,
                          const String   &option)
{
  const char *p, *match = NULL;
  int l = strlen (option.c_str());
  return_val_if_fail (l > 0, NULL);
  if (option_string == "")
    return NULL;        /* option not found */

  /* try first match */
  p = strstr (option_string.c_str(), option.c_str());
  if (p &&
      (p == option_string.c_str() || p[-1] == ':') &&
      (p[l] == ':' || p[l] == 0 || p[l] == '=' ||
       ((p[l] == '-' || p[l] == '+') && (p[l + 1] == ':' || p[l + 1] == 0))))
    match = p;
  /* allow later matches to override */
  while (p)
    {
      p = strstr (p + l, option.c_str());
      if (p &&
          p[-1] == ':' &&
          (p[l] == ':' || p[l] == 0 || p[l] == '=' ||
           ((p[l] == '-' || p[l] == '+') && (p[l + 1] == ':' || p[l + 1] == 0))))
        match = p;
    }
  return match ? match + l : NULL;
}

String
string_option_get (const String   &option_string,
                   const String   &option)
{
  const char *value = string_option_find_value (option_string, option);

  if (!value)
    return NULL;                        /* option not present */
  else switch (value[0])
      {
        const char *s;
      case ':':   return "1";           /* option was present, no modifier */
      case 0:     return "1";           /* option was present, no modifier */
      case '+':   return "1";           /* option was present, enable modifier */
      case '-':   return NULL;          /* option was present, disable modifier */
      case '=':                         /* option present with assignment */
        value++;
        s = strchr (value, ':');
        return s ? String (value, s - value) : value;
      default:    return NULL;            /* anything else, undefined */
      }
}

bool
string_option_check (const String   &option_string,
                     const String   &option)
{
  const char *value = string_option_find_value (option_string, option);

  if (!value)
    return false;                       /* option not present */
  else switch (value[0])
    {
      const char *s;
    case ':':   return true;            /* option was present, no modifier */
    case 0:     return true;            /* option was present, no modifier */
    case '+':   return true;            /* option was present, enable modifier */
    case '-':   return false;           /* option was present, disable modifier */
    case '=':                           /* option present with assignment */
      value++;
      s = strchr (value, ':');
      if (!s || s == value)             /* empty assignment */
        return false;
      else switch (s[0])
        {
        case '0': case 'f': case 'F':
        case 'n': case 'N':             /* false assigments */
          return false;
        default: return true;           /* anything else holds true */
        }
    default:    return false;           /* anything else, undefined */
    }
}

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
