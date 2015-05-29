# Licensed CC0 Public Domain: http://creativecommons.org/publicdomain/zero/1.0
import Rapicorn
# python -ic "import sys, os; sys.path.insert (0, os.path.abspath ('.libs')) ; import Rapicorn"

# test MainLoop and EventLoop object identities
ml = Rapicorn.MainLoop()
sl = ml.create_slave()
sm = sl.main_loop()
m3 = Rapicorn.MainLoop()
assert ml == ml and sl == sl and sm == sm and m3 == m3
assert ml != sl and sl != ml and sm != sl and sl != sm
assert ml == sm and sm == ml and ml != m3 and m3 != ml
assert ml.main_loop() == ml  and m3.main_loop() == m3

# test exec_callback
ect_counter = 5
ect_value = ""
def bhandler():
  global ect_counter, ect_value
  if ect_counter:
    ect_value += "*"
    ect_counter -= 1
  return ect_counter
ml = Rapicorn.MainLoop()
ml.exec_callback (bhandler)
ml.iterate_pending()
assert ect_counter == 0
assert ect_value == "*****"

# test exec_dispatcher
prepared   = [ None, None, None ]
checked    = [ None, None, None ]
dispatched = [ None, None, None ]
counter    = 0
ml = Rapicorn.MainLoop()
def dispatcher (loop_state):
  global counter, prepared, checked, dispatched
  if   loop_state.phase == loop_state.PREPARE:
    prepared[counter] = True
    return True
  elif loop_state.phase == loop_state.CHECK:
    checked[counter] = "Yes"
    return True
  elif loop_state.phase == loop_state.DISPATCH:
    dispatched[counter] = counter
    counter += 1
    return counter <= 2
ml.exec_dispatcher (dispatcher)
ml.iterate_pending()
summary = [ counter ] + prepared + checked + dispatched
assert summary == [3, True, True, True, 'Yes', 'Yes', 'Yes', 0, 1, 2]

# all done
print '  %-6s' % 'CHECK', '%-67s' % __file__, 'OK'
