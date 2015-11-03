// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#ifndef __RAPICORN_REGEX_HH__
#define __RAPICORN_REGEX_HH__

#include <rcore/utilities.hh>

namespace Rapicorn {

namespace Regex {

typedef enum { // copied from gregex.h
  CASELESS          = 1 << 0,
  MULTILINE         = 1 << 1,
  DOTALL            = 1 << 2,
  EXTENDED          = 1 << 3,
  ANCHORED          = 1 << 4,
  DOLLAR_ENDONLY    = 1 << 5,
  UNGREEDY          = 1 << 9,
  RAW               = 1 << 11,
  NO_AUTO_CAPTURE   = 1 << 12,
  OPTIMIZE          = 1 << 13,
  DUPNAMES          = 1 << 19,
  NEWLINE_CR        = 1 << 20,
  NEWLINE_LF        = 1 << 21,
  NEWLINE_CRLF      = NEWLINE_CR | NEWLINE_LF
} CompileFlags;
static const CompileFlags COMPILE_NORMAL = CompileFlags (0);
inline CompileFlags  operator&  (CompileFlags  s1, CompileFlags s2) { return CompileFlags (s1 & (uint64) s2); }
inline CompileFlags& operator&= (CompileFlags &s1, CompileFlags s2) { s1 = s1 & s2; return s1; }
inline CompileFlags  operator|  (CompileFlags  s1, CompileFlags s2) { return CompileFlags (s1 | (uint64) s2); }
inline CompileFlags& operator|= (CompileFlags &s1, CompileFlags s2) { s1 = s1 | s2; return s1; }

typedef enum { // copied from gregex.h
  MATCH_ANCHORED      = 1 << 4,
  MATCH_NOTBOL        = 1 << 7,
  MATCH_NOTEOL        = 1 << 8,
  MATCH_NOTEMPTY      = 1 << 10,
  MATCH_PARTIAL       = 1 << 15,
  MATCH_NEWLINE_CR    = 1 << 20,
  MATCH_NEWLINE_LF    = 1 << 21,
  MATCH_NEWLINE_CRLF  = MATCH_NEWLINE_CR | MATCH_NEWLINE_LF,
  MATCH_NEWLINE_ANY   = 1 << 22
} MatchFlags;
static const MatchFlags MATCH_NORMAL = MatchFlags (0);
inline MatchFlags  operator&  (MatchFlags  s1, MatchFlags s2) { return MatchFlags (s1 & (uint64) s2); }
inline MatchFlags& operator&= (MatchFlags &s1, MatchFlags s2) { s1 = s1 & s2; return s1; }
inline MatchFlags  operator|  (MatchFlags  s1, MatchFlags s2) { return MatchFlags (s1 | (uint64) s2); }
inline MatchFlags& operator|= (MatchFlags &s1, MatchFlags s2) { s1 = s1 | s2; return s1; }

bool    match_simple    (const String   &pattern,
                         const String   &utf8string,
                         CompileFlags    compile_flags,
                         MatchFlags      match_flags);
} // Regex

} // Rapicorn

#endif /* __RAPICORN_REGEX_HH__ */
