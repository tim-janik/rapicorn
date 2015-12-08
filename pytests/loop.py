# Licensed CC0 Public Domain: http://creativecommons.org/publicdomain/zero/1.0
"""
PyRapicorn Loop Test
"""

import Rapicorn # Load Rapicorn language bindings for Python
import sys

# initialize application
app = Rapicorn.init_app ("PyRapicorn Loop Test")

verbose = 0
loop_tchecks = 0
loop_ichecks = 0
test_loop = None
def loop_tests():
  global test_loop
  def tcheck_func():
    global loop_tchecks, loop_ichecks, verbose, test_loop
    loop_tchecks += 1
    if verbose:
      print "%s: %u" % ('tcheck_func', loop_tchecks)
    if loop_tchecks >= 10 and loop_ichecks >= 100:
      test_loop.quit()
    return True # keep running
  def icheck_func():
    global loop_ichecks, verbose
    loop_ichecks += 1
    if verbose:
      print "%s: %u" % ('icheck_func', loop_ichecks)
    return True # keep running
  test_loop = Rapicorn.MainLoop()
  ts = test_loop.exec_timer (tcheck_func, 0, 5)
  src = test_loop.exec_idle (icheck_func)
  test_loop.run()
  assert loop_tchecks >= 10 and loop_ichecks >= 100

testname = "  Loop-Test:"
print testname,
loop_tests()
print " " * max (0, 75 - len (testname)), "OK"
