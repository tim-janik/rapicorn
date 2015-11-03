// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#include "unicode.hh"
#include <glib.h>

namespace Rapicorn {

/// The Unicode namespace provides support for the Unicode standard and UTF-8 encoding.
namespace Unicode {

/* --- unichar ctype.h equivalents --- */
bool
isalnum (unichar uc)
{
  return g_unichar_isalnum (uc);
}

bool
isalpha (unichar uc)
{
  return g_unichar_isalpha (uc);
}

bool
iscntrl (unichar uc)
{
  return g_unichar_iscntrl (uc);
}

bool
isdigit (unichar uc)
{
  return g_unichar_isdigit (uc);
}

int
digit_value (unichar uc)
{
  return g_unichar_digit_value (uc);
}

bool
isgraph (unichar uc)
{
  return g_unichar_isgraph (uc);
}

bool
islower (unichar uc)
{
  return g_unichar_islower (uc);
}

unichar
tolower (unichar uc)
{
  return g_unichar_tolower (uc);
}

bool
isprint (unichar uc)
{
  return g_unichar_isprint (uc);
}

bool
ispunct (unichar uc)
{
  return g_unichar_ispunct (uc);
}

bool
isspace (unichar uc)
{
  return g_unichar_isspace (uc);
}

bool
isupper (unichar uc)
{
  return g_unichar_isupper (uc);
}

unichar
toupper (unichar uc)
{
  return g_unichar_toupper (uc);
}

bool
isxdigit (unichar uc)
{
  return g_unichar_isxdigit (uc);
}

int
xdigit_value (unichar uc)
{
  return g_unichar_xdigit_value (uc);
}

bool
istitle (unichar uc)
{
  return g_unichar_istitle (uc);
}

unichar
totitle (unichar uc)
{
  return g_unichar_totitle (uc);
}

bool
isdefined (unichar uc)
{
  return g_unichar_isdefined (uc);
}

bool
iswide (unichar uc)
{
  return g_unichar_iswide (uc);
}

bool
iswide_cjk (unichar uc)
{
#if GLIB_CHECK_VERSION (2, 12, 0)
  return g_unichar_iswide_cjk (uc);
#else
  return false;
#endif
}

Type
get_type (unichar uc)
{
  return Type (g_unichar_type (uc));
}

BreakType
get_break (unichar uc)
{
  return BreakType (g_unichar_break_type (uc));
}

/* --- ensure castable Rapicorn::Unicode::Type --- */
RAPICORN_STATIC_ASSERT (Unicode::CONTROL == (int) G_UNICODE_CONTROL);
RAPICORN_STATIC_ASSERT (Unicode::FORMAT == (int) G_UNICODE_FORMAT);
RAPICORN_STATIC_ASSERT (Unicode::UNASSIGNED == (int) G_UNICODE_UNASSIGNED);
RAPICORN_STATIC_ASSERT (Unicode::PRIVATE_USE == (int) G_UNICODE_PRIVATE_USE);
RAPICORN_STATIC_ASSERT (Unicode::SURROGATE == (int) G_UNICODE_SURROGATE);
RAPICORN_STATIC_ASSERT (Unicode::LOWERCASE_LETTER == (int) G_UNICODE_LOWERCASE_LETTER);
RAPICORN_STATIC_ASSERT (Unicode::MODIFIER_LETTER == (int) G_UNICODE_MODIFIER_LETTER);
RAPICORN_STATIC_ASSERT (Unicode::OTHER_LETTER == (int) G_UNICODE_OTHER_LETTER);
RAPICORN_STATIC_ASSERT (Unicode::TITLECASE_LETTER == (int) G_UNICODE_TITLECASE_LETTER);
RAPICORN_STATIC_ASSERT (Unicode::UPPERCASE_LETTER == (int) G_UNICODE_UPPERCASE_LETTER);
RAPICORN_STATIC_ASSERT (Unicode::COMBINING_MARK == (int) G_UNICODE_COMBINING_MARK);
RAPICORN_STATIC_ASSERT (Unicode::ENCLOSING_MARK == (int) G_UNICODE_ENCLOSING_MARK);
RAPICORN_STATIC_ASSERT (Unicode::NON_SPACING_MARK == (int) G_UNICODE_NON_SPACING_MARK);
RAPICORN_STATIC_ASSERT (Unicode::DECIMAL_NUMBER == (int) G_UNICODE_DECIMAL_NUMBER);
RAPICORN_STATIC_ASSERT (Unicode::LETTER_NUMBER == (int) G_UNICODE_LETTER_NUMBER);
RAPICORN_STATIC_ASSERT (Unicode::OTHER_NUMBER == (int) G_UNICODE_OTHER_NUMBER);
RAPICORN_STATIC_ASSERT (Unicode::CONNECT_PUNCTUATION == (int) G_UNICODE_CONNECT_PUNCTUATION);
RAPICORN_STATIC_ASSERT (Unicode::DASH_PUNCTUATION == (int) G_UNICODE_DASH_PUNCTUATION);
RAPICORN_STATIC_ASSERT (Unicode::CLOSE_PUNCTUATION == (int) G_UNICODE_CLOSE_PUNCTUATION);
RAPICORN_STATIC_ASSERT (Unicode::FINAL_PUNCTUATION == (int) G_UNICODE_FINAL_PUNCTUATION);
RAPICORN_STATIC_ASSERT (Unicode::INITIAL_PUNCTUATION == (int) G_UNICODE_INITIAL_PUNCTUATION);
RAPICORN_STATIC_ASSERT (Unicode::OTHER_PUNCTUATION == (int) G_UNICODE_OTHER_PUNCTUATION);
RAPICORN_STATIC_ASSERT (Unicode::OPEN_PUNCTUATION == (int) G_UNICODE_OPEN_PUNCTUATION);
RAPICORN_STATIC_ASSERT (Unicode::CURRENCY_SYMBOL == (int) G_UNICODE_CURRENCY_SYMBOL);
RAPICORN_STATIC_ASSERT (Unicode::MODIFIER_SYMBOL == (int) G_UNICODE_MODIFIER_SYMBOL);
RAPICORN_STATIC_ASSERT (Unicode::MATH_SYMBOL == (int) G_UNICODE_MATH_SYMBOL);
RAPICORN_STATIC_ASSERT (Unicode::OTHER_SYMBOL == (int) G_UNICODE_OTHER_SYMBOL);
RAPICORN_STATIC_ASSERT (Unicode::LINE_SEPARATOR == (int) G_UNICODE_LINE_SEPARATOR);
RAPICORN_STATIC_ASSERT (Unicode::PARAGRAPH_SEPARATOR == (int) G_UNICODE_PARAGRAPH_SEPARATOR);
RAPICORN_STATIC_ASSERT (Unicode::SPACE_SEPARATOR == (int) G_UNICODE_SPACE_SEPARATOR);

/* --- ensure castable Rapicorn::Unicode::BreakType --- */
RAPICORN_STATIC_ASSERT (Unicode::BREAK_MANDATORY == (int) G_UNICODE_BREAK_MANDATORY);
RAPICORN_STATIC_ASSERT (Unicode::BREAK_CARRIAGE_RETURN == (int) G_UNICODE_BREAK_CARRIAGE_RETURN);
RAPICORN_STATIC_ASSERT (Unicode::BREAK_LINE_FEED == (int) G_UNICODE_BREAK_LINE_FEED);
RAPICORN_STATIC_ASSERT (Unicode::BREAK_COMBINING_MARK == (int) G_UNICODE_BREAK_COMBINING_MARK);
RAPICORN_STATIC_ASSERT (Unicode::BREAK_SURROGATE == (int) G_UNICODE_BREAK_SURROGATE);
RAPICORN_STATIC_ASSERT (Unicode::BREAK_ZERO_WIDTH_SPACE == (int) G_UNICODE_BREAK_ZERO_WIDTH_SPACE);
RAPICORN_STATIC_ASSERT (Unicode::BREAK_INSEPARABLE == (int) G_UNICODE_BREAK_INSEPARABLE);
RAPICORN_STATIC_ASSERT (Unicode::BREAK_NON_BREAKING_GLUE == (int) G_UNICODE_BREAK_NON_BREAKING_GLUE);
RAPICORN_STATIC_ASSERT (Unicode::BREAK_CONTINGENT == (int) G_UNICODE_BREAK_CONTINGENT);
RAPICORN_STATIC_ASSERT (Unicode::BREAK_SPACE == (int) G_UNICODE_BREAK_SPACE);
RAPICORN_STATIC_ASSERT (Unicode::BREAK_AFTER == (int) G_UNICODE_BREAK_AFTER);
RAPICORN_STATIC_ASSERT (Unicode::BREAK_BEFORE == (int) G_UNICODE_BREAK_BEFORE);
RAPICORN_STATIC_ASSERT (Unicode::BREAK_BEFORE_AND_AFTER == (int) G_UNICODE_BREAK_BEFORE_AND_AFTER);
RAPICORN_STATIC_ASSERT (Unicode::BREAK_HYPHEN == (int) G_UNICODE_BREAK_HYPHEN);
RAPICORN_STATIC_ASSERT (Unicode::BREAK_NON_STARTER == (int) G_UNICODE_BREAK_NON_STARTER);
RAPICORN_STATIC_ASSERT (Unicode::BREAK_OPEN_PUNCTUATION == (int) G_UNICODE_BREAK_OPEN_PUNCTUATION);
RAPICORN_STATIC_ASSERT (Unicode::BREAK_CLOSE_PUNCTUATION == (int) G_UNICODE_BREAK_CLOSE_PUNCTUATION);
RAPICORN_STATIC_ASSERT (Unicode::BREAK_QUOTATION == (int) G_UNICODE_BREAK_QUOTATION);
RAPICORN_STATIC_ASSERT (Unicode::BREAK_EXCLAMATION == (int) G_UNICODE_BREAK_EXCLAMATION);
RAPICORN_STATIC_ASSERT (Unicode::BREAK_IDEOGRAPHIC == (int) G_UNICODE_BREAK_IDEOGRAPHIC);
RAPICORN_STATIC_ASSERT (Unicode::BREAK_NUMERIC == (int) G_UNICODE_BREAK_NUMERIC);
RAPICORN_STATIC_ASSERT (Unicode::BREAK_INFIX_SEPARATOR == (int) G_UNICODE_BREAK_INFIX_SEPARATOR);
RAPICORN_STATIC_ASSERT (Unicode::BREAK_SYMBOL == (int) G_UNICODE_BREAK_SYMBOL);
RAPICORN_STATIC_ASSERT (Unicode::BREAK_ALPHABETIC == (int) G_UNICODE_BREAK_ALPHABETIC);
RAPICORN_STATIC_ASSERT (Unicode::BREAK_PREFIX == (int) G_UNICODE_BREAK_PREFIX);
RAPICORN_STATIC_ASSERT (Unicode::BREAK_POSTFIX == (int) G_UNICODE_BREAK_POSTFIX);
RAPICORN_STATIC_ASSERT (Unicode::BREAK_COMPLEX_CONTEXT == (int) G_UNICODE_BREAK_COMPLEX_CONTEXT);
RAPICORN_STATIC_ASSERT (Unicode::BREAK_AMBIGUOUS == (int) G_UNICODE_BREAK_AMBIGUOUS);
RAPICORN_STATIC_ASSERT (Unicode::BREAK_UNKNOWN == (int) G_UNICODE_BREAK_UNKNOWN);
RAPICORN_STATIC_ASSERT (Unicode::BREAK_NEXT_LINE == (int) G_UNICODE_BREAK_NEXT_LINE);
RAPICORN_STATIC_ASSERT (Unicode::BREAK_WORD_JOINER == (int) G_UNICODE_BREAK_WORD_JOINER);
#if GLIB_CHECK_VERSION (2, 10, 0)
RAPICORN_STATIC_ASSERT (Unicode::BREAK_HANGUL_L_JAMO == (int) G_UNICODE_BREAK_HANGUL_L_JAMO);
RAPICORN_STATIC_ASSERT (Unicode::BREAK_HANGUL_V_JAMO == (int) G_UNICODE_BREAK_HANGUL_V_JAMO);
RAPICORN_STATIC_ASSERT (Unicode::BREAK_HANGUL_T_JAMO == (int) G_UNICODE_BREAK_HANGUL_T_JAMO);
RAPICORN_STATIC_ASSERT (Unicode::BREAK_HANGUL_LV_SYLLABLE == (int) G_UNICODE_BREAK_HANGUL_LV_SYLLABLE);
RAPICORN_STATIC_ASSERT (Unicode::BREAK_HANGUL_LVT_SYLLABLE == (int) G_UNICODE_BREAK_HANGUL_LVT_SYLLABLE);
#endif
} // Unicode

/* --- UTF-8 movement --- */
const int8 utf8_skip_table[256] = {
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,4,4,4,4,4,4,4,4,5,5,5,5,6,6,1,1
};

static inline const int8
utf8_char_length (const uint8 c)
{
  return c < 0xfe ? utf8_skip_table[c] : -1;
}
static inline const uint8
utf8_length_bits (const uint8 l)
{
  const uint length_bits[] = { 0x00, 0x00, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc, };
  return l <= 6 ? length_bits[l] : 0;
}

static inline const uint8
utf8_char_mask (const uint8 c)
{
  const uint8 char_masks[8] = { 0x00, 0x7f, 0x1f, 0x0f, 0x07, 0x03, 0x01, 0x00 };
  return char_masks[utf8_skip_table[c]];
}

static inline uint8
utf8_length_from_unichar (unichar uc)
{
  uint8 l = 1;
  l += uc >= 0x00000080; /* 2 */
  l += uc >= 0x00000800; /* 3 */
  l += uc >= 0x00010000; /* 4 */
  l += uc >= 0x00200000; /* 5 */
  l += uc >= 0x04000000; /* 6 */
  return l;
}

unichar
utf8_to_unichar (const char *str)
{
  uint8 clen = utf8_char_length (*str);
  uint8 mask = utf8_char_mask (*str);
  if (clen < 1)
    return 0xffffffff;
  unichar uc = *str & mask;
  for (uint i = 1; i < clen; i++)
    {
      uint8 c = str[i];
      if ((c & 0xc0) != 0x80)
        return 0xffffffff;
      uc = (uc << 6) + (c & 0x3f);
    }
  return uc;
}

int
utf8_from_unichar (unichar uc,
                   char    str[8])
{
  const int l = utf8_length_from_unichar (uc);
  if (!str)
    return l;
  uint i = l;
  str[i] = 0;
  while (--i)
    {
      str[i] = (uc & 0x3f) | 0x80;
      uc >>= 6;
    }
  str[i] = uc | utf8_length_bits (l); /* i == 0 */
  return l;
}

bool
utf8_validate (const String   &strng,
               int            *bound)
{
  const char *c = &strng[0];
  size_t l = strng.size();
  const gchar *end = NULL;
  gboolean gb = g_utf8_validate (c, l, &end);
  if (bound)
    *bound = !gb ? end - c : -1;
  return gb != false;
}

} // Rapicorn

#include <langinfo.h> // avoid conflicts with the use of CURRENCY_SYMBOL above

namespace Rapicorn {

/**
 * Check wether the current locale uses an UTF-8 charset.
 * On most systems nl_langinfo(3) provides the needed codeset information.
 * Otherwise, the charset is extracted from LC_ALL, LC_CTYPE, or LANG.
 * Note that in the latter case per-thread locales may be misidentified.
 */
bool
utf8_is_locale_charset ()
{
  bool isutf8;
  const char *codeset = nl_langinfo (CODESET);
  if (codeset)
    isutf8 = strcasecmp ("UTF-8", codeset) == 0;
  else
    {
      // locale identifiers have this format: [language[_territory][.codeset][@modifier]]
      const char *lid = getenv ("LC_ALL");
      if (!lid || !lid[0])
        lid = getenv ("LC_CTYPE");
      if (!lid || !lid[0])
        lid = getenv ("LANG");
      if (!lid)
        lid = "";
      size_t start = 0, end = strlen (lid);
      const char *sep = strchr (lid + start, '.');
      start = sep ? sep - lid + 1 : start;
      sep = strchr (lid + start, '@');
      end = sep ? sep - lid : end;
      isutf8 = end - start == 5 && strncasecmp ("UTF-8", lid + start, 5) == 0;
    }
  return isutf8;
}

} // Rapicorn
