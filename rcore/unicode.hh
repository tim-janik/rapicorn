// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#ifndef __RAPICORN_UNICODE_HH__
#define __RAPICORN_UNICODE_HH__

#include <rcore/utilities.hh>

namespace Rapicorn {

namespace Unicode {
inline bool isvalid      (unichar uc) RAPICORN_CONST;
inline bool isnoncharacter (unichar uc) RAPICORN_CONST;
bool        isalnum      (unichar uc) RAPICORN_CONST;
bool        isalpha      (unichar uc) RAPICORN_CONST;
bool        iscntrl      (unichar uc) RAPICORN_CONST;
bool        isdigit      (unichar uc) RAPICORN_CONST;
int         digit_value  (unichar uc) RAPICORN_CONST;
bool        isgraph      (unichar uc) RAPICORN_CONST;
bool        islower      (unichar uc) RAPICORN_CONST;
unichar     tolower      (unichar uc) RAPICORN_CONST;
bool        isprint      (unichar uc) RAPICORN_CONST;
bool        ispunct      (unichar uc) RAPICORN_CONST;
bool        isspace      (unichar uc) RAPICORN_CONST;
bool        isupper      (unichar uc) RAPICORN_CONST;
unichar     toupper      (unichar uc) RAPICORN_CONST;
bool        isxdigit     (unichar uc) RAPICORN_CONST;
int         xdigit_value (unichar uc) RAPICORN_CONST;
bool        istitle      (unichar uc) RAPICORN_CONST;
unichar     totitle      (unichar uc) RAPICORN_CONST;
bool        isdefined    (unichar uc) RAPICORN_CONST;
bool        iswide       (unichar uc) RAPICORN_CONST;
bool        iswide_cjk   (unichar uc) RAPICORN_CONST;
typedef enum {
  CONTROL,              FORMAT,                 UNASSIGNED,
  PRIVATE_USE,          SURROGATE,              LOWERCASE_LETTER,
  MODIFIER_LETTER,      OTHER_LETTER,           TITLECASE_LETTER,
  UPPERCASE_LETTER,     COMBINING_MARK,         ENCLOSING_MARK,
  NON_SPACING_MARK,     DECIMAL_NUMBER,         LETTER_NUMBER,
  OTHER_NUMBER,         CONNECT_PUNCTUATION,    DASH_PUNCTUATION,
  CLOSE_PUNCTUATION,    FINAL_PUNCTUATION,      INITIAL_PUNCTUATION,
  OTHER_PUNCTUATION,    OPEN_PUNCTUATION,       CURRENCY_SYMBOL,
  MODIFIER_SYMBOL,      MATH_SYMBOL,            OTHER_SYMBOL,
  LINE_SEPARATOR,       PARAGRAPH_SEPARATOR,    SPACE_SEPARATOR
} Type;
Type    get_type     (unichar uc) RAPICORN_CONST;
typedef enum {
  BREAK_MANDATORY,        BREAK_CARRIAGE_RETURN,    BREAK_LINE_FEED,
  BREAK_COMBINING_MARK,   BREAK_SURROGATE,          BREAK_ZERO_WIDTH_SPACE,
  BREAK_INSEPARABLE,      BREAK_NON_BREAKING_GLUE,  BREAK_CONTINGENT,
  BREAK_SPACE,            BREAK_AFTER,              BREAK_BEFORE,
  BREAK_BEFORE_AND_AFTER, BREAK_HYPHEN,             BREAK_NON_STARTER,
  BREAK_OPEN_PUNCTUATION, BREAK_CLOSE_PUNCTUATION,  BREAK_QUOTATION,
  BREAK_EXCLAMATION,      BREAK_IDEOGRAPHIC,        BREAK_NUMERIC,
  BREAK_INFIX_SEPARATOR,  BREAK_SYMBOL,             BREAK_ALPHABETIC,
  BREAK_PREFIX,           BREAK_POSTFIX,            BREAK_COMPLEX_CONTEXT,
  BREAK_AMBIGUOUS,        BREAK_UNKNOWN,            BREAK_NEXT_LINE,
  BREAK_WORD_JOINER,      BREAK_HANGUL_L_JAMO,      BREAK_HANGUL_V_JAMO,
  BREAK_HANGUL_T_JAMO,    BREAK_HANGUL_LV_SYLLABLE, BREAK_HANGUL_LVT_SYLLABLE
} BreakType;
BreakType get_break  (unichar uc) RAPICORN_CONST;

} // Unicode

/* --- UTF-8 movement --- */
inline const char*    utf8_next         (const char     *c);
inline char*          utf8_next         (char           *c);
inline const char*    utf8_prev         (const char     *c);
inline char*          utf8_prev         (char           *c);
inline const char*    utf8_find_next    (const char     *c,
                                         const char     *bound = NULL);
inline char*          utf8_find_next    (char           *current,
                                         const char     *bound = NULL);
inline const char*    utf8_find_prev    (const char     *start,
                                         const char     *current);
inline char*          utf8_find_prev    (const char     *start,
                                         char           *currrent);
inline const char*    utf8_align        (const char     *start,
                                         const char     *current);
inline char*          utf8_align        (const char     *start,
                                         char           *current);
inline bool           utf8_aligned      (const char     *c);
unichar               utf8_to_unichar   (const char     *str);
int                   utf8_from_unichar (unichar         uc,
                                         char            str[8]);
bool                  utf8_validate     (const String   &string,
                                         int            *bound = NULL);
bool                  utf8_is_locale_charset ();

/* --- implementation bits --- */
namespace Unicode {
inline bool
isvalid (unichar uc)
{ // Unicode definition D90, Section 3.9 Unicode Encoding Forms
  if (RAPICORN_UNLIKELY (uc > 0x10ffff))
    return false;
  if (RAPICORN_UNLIKELY ((uc & 0xfffff800) == 0xd800))
    return false;
  return true;
}
inline bool
isnoncharacter (unichar uc)
{ // Unicode definition D14, Section 3.4 Characters and Encoding
  if (RAPICORN_UNLIKELY (uc >= 0xfdd0 && uc <= 0xfdef))
    return true;
  if (RAPICORN_UNLIKELY ((uc & 0xfffe) == 0xfffe && (uc >> 16) <= 0x10))
    return true;
  return false;
}
} // Unicode

extern const int8 utf8_skip_table[256];

inline const char*
utf8_next (const char *c)
{
  return c + utf8_skip_table[(uint8) *c];
}

inline char*
utf8_next (char *c)
{
  return c + utf8_skip_table[(uint8) *c];
}

inline const char*
utf8_prev (const char *c)
{
  do
    c--;
  while ((*c & 0xc0) == 0x80);
  return c;
}

inline char*
utf8_prev (char *c)
{
  do
    c--;
  while ((*c & 0xc0) == 0x80);
  return c;
}

inline const char*
utf8_find_next (const char *c,
                const char *bound)
{
  if (*c)
    do
      c++;
    while ((!bound || c < bound) && (*c & 0xc0) == 0x80);
  return !bound || c < bound ? c : NULL;
}

inline char*
utf8_find_next (char       *c,
                const char *bound)
{
  return const_cast<char*> (utf8_find_next (const_cast<const char*> (c), bound));
}

inline const char*
utf8_find_prev (const char     *start,
                const char     *current)
{
  do
    current--;
  while (current >= start && (*current & 0xc0) == 0x80);
  return current >= start ? current : NULL;
}

inline char*
utf8_find_prev (const char     *start,
                char           *current)
{
  return const_cast<char*> (utf8_find_prev (start, const_cast<const char*> (current)));
}

inline const char*
utf8_align (const char     *start,
            const char     *current)
{
  while (current > start && (*current & 0xc0) == 0x80)
    current--;
  return current;
}

inline char*
utf8_align (const char *start,
            char       *current)
{
  return const_cast<char*> (utf8_align (start, const_cast<const char*> (current)));
}

inline bool
utf8_aligned (const char *c)
{
  return (*c & 0xc0) == 0x80;
}

} // Rapicorn

#endif /* __RAPICORN_UNICODE_HH__ */
