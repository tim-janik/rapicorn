# Copyright (C) 2010 Tim Janik
#
# Licensed CC0 Public Domain: http://creativecommons.org/publicdomain/zero/1.0
"""
Simple Python test for Rapicorn
"""

import Rapicorn # Load Rapicorn language bindings for Python
import sys

# Define main window Widget Tree
simple_window_widgets = """
  <Window declare="simple-window">
    <Button on-click="CLICK">
      <Label markup-text="Hello Simple World!" />
    </Button>
  </Window>
"""

# setup application
app = Rapicorn.init_app ("Simple PyRapicorn Test") # unique application name
app.load_string (simple_window_widgets)            # loads widget tree
window = app.create_window ("simple-window")       # creates main window

# signal connection testing
def assert_unreachable (*args):
  assert "code unreachable" == True
cid1 = window.sig_commands.connect (assert_unreachable)
cid2 = window.sig_commands.connect (assert_unreachable)
assert cid1 != 0
assert cid2 != 0
assert cid1 != cid2
disconnected = window.sig_commands.disconnect (cid2)
assert disconnected == True
disconnected = window.sig_commands.disconnect (cid2)
assert disconnected == False
disconnected = window.sig_commands.disconnect (cid1)
assert disconnected == True
disconnected = window.sig_commands.disconnect (cid1)
assert disconnected == False

# window command handling
seen_click_command = False
def command_handler (cmdname, args):
  global seen_click_command
  seen_click_command |= cmdname == "CLICK"
  ## print "in signal handler, args:", cmdname, args
  return True # handled
# need one signal connection to test for click
window.sig_commands.connect (command_handler)

# show window on screen
window.show()

# run synthesized tests
if not max (opt in sys.argv for opt in ('-i','--interactive')):
  # enter window to allow input events
  b = window.synthesize_enter()
  assert b
  # find button
  button = window.query_selector_unique ('.Button')
  assert button
  # click button
  assert seen_click_command == False
  window.synthesize_click (button, 1)   # fake "CLICK" command
  window.synthesize_leave()
  while app.main_loop().iterate (False):
    pass # process events and signals
  assert seen_click_command == True     # command_handler should be executed by now
  # delete window
  assert window.closed() == False
  window.synthesize_delete()
  assert window.closed() == True

# run event loop to process window events (exits when all windows are gone)
app.run()
