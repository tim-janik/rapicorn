# Copyright (C) 2010 Tim Janik
#
# This work is provided "as is"; see: http://rapicorn.org/LICENSE-AS-IS
"""
PyRapicorn Loop Test
"""

import sys
import Rapicorn1008 as Rapicorn  # Rapicorn modules are versioned
import Rapicorn1008.main as Main # FIXME: provide by default?

# initialize application
app = Rapicorn.app_init ("PyRapicorn-Loop-Test")  # unique application name

def loop_tests():
  def testfunc(): pass
  loop = Main.Loop()
  ts = Main.TimeoutSource (testfunc, 0, 0.25)
  assert ts.state != Main.DESTROYED
  loop += ts
  assert ts.state != Main.DESTROYED
  loop -= ts
  assert ts.state == Main.DESTROYED
  src = Main.IdleSource (testfunc)
  loop += src
  assert src.state != Main.DESTROYED
  loop -= testfunc
  assert src.state == Main.DESTROYED

testname = "  Loop-Test:"
print testname,
loop_tests()
print " " * max (0, 75 - len (testname)), "OK"
