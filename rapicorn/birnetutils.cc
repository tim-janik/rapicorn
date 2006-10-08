/* Birnet Programming Utilities
 * Copyright (C) 2003,2005 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#include "birnetutils.hh"
#include <stdarg.h>
#include <algorithm>
#include <stdexcept>
#include <signal.h>
#include <errno.h>

namespace Birnet {

/* --- exceptions --- */
const std::nothrow_t dothrow = {};

Exception::Exception (const String &s1, const String &s2, const String &s3, const String &s4,
                      const String &s5, const String &s6, const String &s7, const String &s8) :
  reason (NULL)
{
  String s (s1);
  if (s2.size())
    s += s2;
  if (s3.size())
    s += s3;
  if (s4.size())
    s += s4;
  if (s5.size())
    s += s5;
  if (s6.size())
    s += s6;
  if (s7.size())
    s += s7;
  if (s8.size())
    s += s8;
  set (s);
}

Exception::Exception (const Exception &e) :
  reason (e.reason ? strdup (e.reason) : NULL)
{}

void
Exception::set (const String &s)
{
  if (reason)
    free (reason);
  reason = strdup (s.c_str());
}

Exception::~Exception() throw()
{
  if (reason)
    free (reason);
}

void
warning_expr_failed (const char *file, uint line, const char *function, const char *expression)
{
  warning ("%s:%d:%s: assertion failed: (%s)", file, line, function, expression);
}

void
error_expr_failed (const char *file, uint line, const char *function, const char *expression)
{
  error ("%s:%d:%s: assertion failed: (%s)", file, line, function, expression);
}

void
warning_expr_reached (const char *file, uint line, const char *function)
{
  warning ("%s:%d:%s: code location should not be reached", file, line, function);
}

String
string_printf (const char *format,
               ...)
{
  String str;
  va_list args;
  va_start (args, format);
  try {
    str = string_vprintf (format, args);
  } catch (...) {
    va_end (args);
    throw;
  }
  va_end (args);
  return str;
}

String
string_vprintf (const char *format,
                va_list     vargs)
{
  char *str = NULL;
  if (vasprintf (&str, format, vargs) < 0 || !str)
    throw std::length_error (__func__);
  String s (str);
  free (str);
  return s;
}

String
string_strip (const String &str)
{
  const char *cstr = str.c_str();
  uint start = 0, end = str.size();
  while (end and strchr (" \t\n\r", cstr[end-1]))
    end--;
  while (strchr (" \t\n\r", cstr[start]))
    start++;
  return String (cstr + start, end - start);
}

bool
string_to_bool (const String &string)
{
  static const char *spaces = " \t\n\r";
  const char *p = string.c_str();
  /* skip spaces */
  while (*p && strchr (spaces, *p))
    p++;
  /* ignore signs */
  if (p[0] == '-' || p[0] == '+')
    {
      p++;
      /* skip spaces */
      while (*p && strchr (spaces, *p))
        p++;
    }
  /* handle numbers */
  if (p[0] >= '0' && p[0] <= '9')
    return 0 != string_to_uint (p);
  /* handle special words */
  if (strncasecmp (p, "ON", 2) == 0)
    return 1;
  if (strncasecmp (p, "OFF", 3) == 0)
    return 0;
  /* handle non-numbers */
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
  return strtod (string.c_str(), NULL);
}

String
string_from_float (float value)
{
  return string_printf ("%.7g", value);
}

String
string_from_double (double value)
{
  return string_printf ("%.17g", value);
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
  printf ("vector: %d: %s\n", dvec.size(), string_from_vector (dvec).c_str());
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

void
error (const char *format,
       ...)
{
  va_list args;
  va_start (args, format);
  String ers = string_vprintf (format, args);
  va_end (args);
  error (ers);
}

void
error (const String &s)
{
  fflush (stdout);
  String msg ("\nERROR: ");
  msg += s;
  msg += "\naborting...\n";
  fputs (msg.c_str(), stderr);
  fflush (stderr);
  abort();
}

void
warning (const char *format,
         ...)
{
  fflush (stdout);
  va_list args;
  va_start (args, format);
  String ers = string_vprintf (format, args);
  va_end (args);
  warning (ers);
  fflush (stderr);
}

void
warning (const String &s)
{
  fflush (stdout);
  String msg ("\nWARNING: ");
  msg += s;
  msg += '\n';
  fputs (msg.c_str(), stderr);
}

void
diag (const char *format,
      ...)
{
  fflush (stdout);
  va_list args;
  va_start (args, format);
  String ers = string_vprintf (format, args);
  va_end (args);
  diag (ers);
  fflush (stderr);
}

void
diag (const String &s)
{
  String msg ("DIAG: ");
  msg += s;
  msg += '\n';
  fputs (msg.c_str(), stderr);
}

void
errmsg (const String &entity,
        const char *format,
        ...)
{
  fflush (stdout);
  va_list args;
  va_start (args, format);
  String ers = string_vprintf (format, args);
  va_end (args);
  errmsg (entity, ers);
  fflush (stderr);
}

void
errmsg (const String &entity,
        const String &s)
{
  String msg (entity);
  msg += entity.size() ? ": " : "DEBUG: ";
  msg += s;
  msg += '\n';
  fputs (msg.c_str(), stderr);
}

int
uuid_string_test (const char *uuid_string) /* 0: valid, -1: invalid, >0: 1 + invalid index */
{
  if (!uuid_string)
    return -1;
  int i, l = strlen (uuid_string);
  if (l != 36)
    return -1;
  // 00000000-0000-0000-0000-000000000000
  for (i = 0; i < l; i++)
    if (i == 8 || i == 13 || i == 18 || i == 23)
      {
        if (uuid_string[i] != '-')
          return -1;
        continue;
      }
    else if ((uuid_string[i] >= '0' && uuid_string[i] <= '9') ||
             (uuid_string[i] >= 'a' && uuid_string[i] <= 'f') ||
             (uuid_string[i] >= 'A' && uuid_string[i] <= 'F'))
      continue;
    else
      return -1;
  return 0;
}

int
uuid_string_cmp (const char     *uuid_string1,
                 const char     *uuid_string2) /* -1=smaller, 0=equal, +1=greater (assuming valid uuid strings) */
{
  return strcasecmp (uuid_string1, uuid_string2); /* good enough for numeric equality and defines stable order */
}

static bool
unaliased_encoding (std::string &name)
{
  /* first, try upper-casing the encoding name */
  std::string upper = name;
  for (uint i = 0; i < upper.size(); i++)
    if (upper[i] >= 'a' && upper[i] <= 'z')
      upper[i] += 'A' - 'a';
  if (upper != name)
    {
      name = upper;
      return true;
    }
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
    "LATIN5",           "ISO-8859-5",
    "LATIN6",           "ISO-8859-6",
    "LATIN7",           "ISO-8859-7",
    "LATIN8",           "ISO-8859-8",
    "LATIN9",           "ISO-8859-9",
    "LATIN13",          "ISO-8859-13",
    "LATIN15",          "ISO-8859-15",
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
      return encoding_aliases[i + 1];
  /* alias not found */
  return false;
}

static iconv_t
unaliased_iconv_open (const char *tocode,
                      const char *fromcode)
{
  const iconv_t icNONE = (iconv_t) -1;
  iconv_t cd = iconv_open (tocode, fromcode);
  if (cd != icNONE)
    return cd;
  /* lookup destination encoding from alias and retry */
  std::string to_encoding = tocode;
  if (unaliased_encoding (to_encoding))
    {
      cd = iconv_open (to_encoding.c_str(), fromcode);
      if (cd != icNONE)
        return cd;
      /* lookup source and destination encoding from alias and retry */
      std::string from_encoding = fromcode;
      if (unaliased_encoding (from_encoding))
        {
          cd = iconv_open (to_encoding.c_str(), from_encoding.c_str());
          if (cd != icNONE)
            return cd;
        }
    }
  /* lookup source encoding from alias and retry */
  std::string from_encoding = fromcode;
  if (unaliased_encoding (from_encoding))
    {
      cd = iconv_open (tocode, from_encoding.c_str());
      if (cd != icNONE)
        return cd;
    }
  return icNONE; /* encoding not found */
}

bool
text_convert (const char        *to_charset,
              std::string       &output_string,
              const char        *from_charset,
              const std::string &input_string,
              const char        *fallback_charset,
              const std::string &output_mark)
{
  output_string = "";
  const iconv_t icNONE = (iconv_t) -1;
  iconv_t alt_cd = icNONE, cd = unaliased_iconv_open (to_charset, from_charset);
  if (cd == icNONE)
    return false;                       /* failed to perform the requested conversion */
  const char *iptr = input_string.c_str();
  size_t ilength = input_string.size();
  char obuffer[1024];                   /* declared outside loop to spare re-initialization */
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
              if (alt_cd == icNONE && fallback_charset)
                {
                  alt_cd = unaliased_iconv_open (to_charset, fallback_charset);
                  fallback_charset = NULL; /* don't retry iconv_open() */
                }
              size_t former_ilength = ilength;
              if (alt_cd != icNONE)
                {
                  /* convert from fallback_charset */
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

} // Birnet
