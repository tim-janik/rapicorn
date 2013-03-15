# Copyright (C) 2010 Tim Janik
#
# This work is provided "as is"; see: http://rapicorn.org/LICENSE-AS-IS
"""
PyRapicorn Loop Test
"""

import sys
from Rapicorn1208 import Rapicorn # Rapicorn modules are versioned
import Rapicorn1208.loop as Loop # FIXME: provide by default?

# initialize application
app = Rapicorn.app_init ("PyRapicorn-Loop-Test")  # unique application name

def loop_tests():
  def testfunc(): pass
  loop = Loop.Loop()
  ts = Loop.TimeoutSource (testfunc, 0, 0.25)
  assert ts.state != Loop.DESTROYED
  loop += ts
  assert ts.state != Loop.DESTROYED
  loop -= ts
  assert ts.state == Loop.DESTROYED
  src = Loop.IdleSource (testfunc)
  loop += src
  assert src.state != Loop.DESTROYED
  loop -= testfunc
  assert src.state == Loop.DESTROYED

testname = "  Loop-Test:"
print testname,
loop_tests()
print " " * max (0, 75 - len (testname)), "OK"
