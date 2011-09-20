# Copyright (C) 2010 Tim Janik
#
# This work is provided "as is"; see: http://rapicorn.org/LICENSE-AS-IS
"""
Simple Python test for Rapicorn
"""

import sys
import Rapicorn1008 as Rapicorn # Rapicorn modules are versioned

# Define main window Widget Tree
simple_window_widgets = """
  <def:simple-window inherit="Window">
    <Button on-click="CLICK">
      <Label markup-text="Hello Simple World!" />
    </Button>
  </def:simple-window>
"""

# setup application
app = Rapicorn.app_init ("Simple-Python-Test")  # unique application name
app.load_string (simple_window_widgets)         # loads widget tree
window = app.create_window ("simple-window")    # creates main window

# window command handling
seen_click_command = False
def command_handler (cmdname, args):
  global seen_click_command
  seen_click_command |= cmdname == "CLICK"
  ## print "in signal handler, args:", cmdname, args
cid = window.sig_commands_connect (command_handler)

# signal connection testing
cid2 = window.sig_commands_connect (command_handler)
assert cid != 0
assert cid2 != 0
assert cid != cid2
window.sig_commands_disconnect (cid2)

# show window on screen
window.show()

# run synthesized tests
if not max (opt in sys.argv for opt in ('-i','--interactive')):
  testname = "  Simple-Window-Test:"
  print testname,
  # enter window to allow input events
  b = window.synthesize_enter()
  assert b
  # process pending events
  while app.iterate (False, True): pass
  # find button
  button = window.unique_component ('/Button')
  assert button
  # click button
  assert seen_click_command == False
  window.synthesize_click (button, 1)
  window.synthesize_leave()
  while app.iterate (False, True): pass
  if 1:
    import time
    time.sleep (0.1) # FIXME: hack to ensure click is processed remotely
    while app.iterate (False, True): pass
  assert seen_click_command == True
  # delete window
  assert window.closed() == False
  window.synthesize_delete()
  while app.iterate (False, True): pass
  ## assert window.closed() == True
  # FIXME: window.closed() can't be asserted here, because remote
  # object references (from python) are not yet implemented
  print " " * max (0, 75 - len (testname)), "OK"

# main loop to process window events (exits when all windows are gone)
app.loop()
