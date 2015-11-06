// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#ifndef __RAPICORN_MATH_HH__
#define __RAPICORN_MATH_HH__

#include <rcore/utilities.hh>
#include <math.h>

namespace Rapicorn {

/* --- double to integer --- */
inline int      dtoi32 (double d) RAPICORN_CONST;
inline int64    dtoi64 (double d) RAPICORN_CONST;
inline int64    iround (double d) RAPICORN_CONST;
inline int64    ifloor (double d) RAPICORN_CONST;
inline int64    iceil  (double d) RAPICORN_CONST;

/* --- implementation bits --- */
inline int RAPICORN_CONST
_dtoi32_generic (double d)
{
  /* this relies on the C++ behaviour of round-to-0 */
  return (int) (d < -0.0 ? d - 0.5 : d + 0.5);
}
inline int RAPICORN_CONST
dtoi32 (double d)
{
  /* this relies on the hardware default round-to-nearest */
#if defined __i386__ && defined __GNUC__
  int r;
  __asm__ volatile ("fistl %0"
                    : "=m" (r)
                    : "t" (d));
  return r;
#endif
  return _dtoi32_generic (d);
}
inline int64 RAPICORN_CONST
_dtoi64_generic (double d)
{
  /* this relies on the C++ behaviour of round-to-0 */
  return (int64) (d < -0.0 ? d - 0.5 : d + 0.5);
}
inline int64 RAPICORN_CONST
dtoi64 (double d)
{
  /* this relies on the hardware default round-to-nearest */
#if defined __i386__ && defined __GNUC__
  int64 r;
  __asm__ volatile ("fistpll %0"
                    : "=m" (r)
                    : "t" (d)
                    : "st");
  return r;
#endif
  return _dtoi64_generic (d);
}
inline int64 RAPICORN_CONST iround (double d) { return dtoi64 (round (d)); }
inline int64 RAPICORN_CONST ifloor (double d) { return dtoi64 (floor (d)); }
inline int64 RAPICORN_CONST iceil  (double d) { return dtoi64 (ceil (d)); }

} // Rapicorn

#endif /* __RAPICORN_MATH_HH__ */
/* vim:set ts=8 sts=2 sw=2: */
