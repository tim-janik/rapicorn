# Copyright (C) 2010 Tim Janik
#
# This work is provided "as is"; see: http://rapicorn.org/LICENSE-AS-IS
"""
Simple Python test for Rapicorn
"""

import sys
import Rapicorn1008 as Rapicorn # Rapicorn modules are versioned

# Define main wind0w Widget Tree
simple_wind0w_widgets = """
  <def:simple-wind0w inherit="Window">
    <Button on-click="CLICK">
      <Label markup-text="Hello Simple World!" />
    </Button>
  </def:simple-wind0w>
"""

# setup application
app = Rapicorn.app_init ("Simple-Python-Test")  # unique application name
app.load_string (simple_wind0w_widgets)         # loads widget tree
wind0w = app.create_wind0w ("simple-wind0w")    # creates main wind0w

# wind0w command handling
seen_click_command = False
def command_handler (cmdname, args):
  global seen_click_command
  seen_click_command |= cmdname == "CLICK"
  ## print "in signal handler, args:", cmdname, args
cid = wind0w.sig_commands_connect (command_handler)

# signal connection testing
cid2 = wind0w.sig_commands_connect (command_handler)
assert cid != 0
assert cid2 != 0
assert cid != cid2
wind0w.sig_commands_disconnect (cid2)

# show wind0w on screen
wind0w.show()

# run synthesized tests
if not max (opt in sys.argv for opt in ('-i','--interactive')):
  testname = "  Simple-Wind0w-Test:"
  print testname,
  # enter wind0w to allow input events
  b = wind0w.synthesize_enter()
  assert b
  # process pending events
  while app.iterate (False, True): pass
  # find button
  button = wind0w.unique_component ('/Button')
  assert button
  # click button
  assert seen_click_command == False
  wind0w.synthesize_click (button, 1)
  wind0w.synthesize_leave()
  while app.iterate (False, True): pass
  if 1:
    import time
    time.sleep (0.1) # FIXME: hack to ensure click is processed remotely
    while app.iterate (False, True): pass
  assert seen_click_command == True
  # delete wind0w
  assert wind0w.closed() == False
  wind0w.synthesize_delete()
  while app.iterate (False, True): pass
  ## assert wind0w.closed() == True
  # FIXME: wind0w.closed() can't be asserted here, because remote
  # object references (from python) are not yet implemented
  print " " * max (0, 75 - len (testname)), "OK"

# main loop to process wind0w events (exits when all wind0ws are gone)
app.loop()
