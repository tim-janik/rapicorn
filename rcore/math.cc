// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#include "math.hh"

namespace Rapicorn {

struct FPUCheck {
  FPUCheck()
  {
#if defined __i386__ && defined __GNUC__
    /* assert rounding mode round-to-nearest which dtoi32() relies on */
    unsigned short int fpu_state;
    __asm__ ("fnstcw %0"
             : "=m" (*&fpu_state));
    bool rounding_mode_is_round_to_nearest = !(fpu_state & 0x0c00);
#else
    bool rounding_mode_is_round_to_nearest = true;
#endif
    assert (rounding_mode_is_round_to_nearest);
  }
};
static FPUCheck assert_rounding_mode;

} // Rapicorn
