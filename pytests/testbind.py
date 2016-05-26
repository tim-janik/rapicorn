# Load and import a versioned Rapicorn module into the 'Rapicorn' namespace
# Licensed CC0 Public Domain: http://creativecommons.org/publicdomain/zero/1.0

import Rapicorn # Load Rapicorn language bindings for Python

# Setup the application object, unsing a unique application name.
app = Rapicorn.init_app ("Test Rapicorn Bindings")

# Define the elements of the dialog window to be displayed.
hello_window = """
  <Window declare="testbind-py">
    <RapicornIdlTestWidget id="test-widget" string-prop="@bind somestring"/>
  </Window>
"""

app.load_string (hello_window)

window = app.create_window ("testbind-py")

@Rapicorn.Bindable
class ObjectModel (object):
  def __init__ (self):
    self.somestring = "initial-somestring"
om = ObjectModel()

window.data_context (om.__aida_relay__)

window.show()

# sync local state with remote state, because of possible binding
# notification ping-pong we have to do this in 2 phases
def sync_remote():
  for i in range (2):                   # settling bindings may require extra round trips
    while app.main_loop().iterate (False):
      pass                              # process signals and events pending locally
    app.test_hook()                     # force round trip, ensure we pickup notifications pending remotely
  while app.main_loop().iterate (False):
    pass                                # process new/remaining batch of signals and events

import sys
testwidget = window.query_selector_unique ('#test-widget')
assert testwidget

sync_remote() # give binding time to initialize

# check value binding from ObjectModel property to UI property
for word in ('test001', 'IxyzI', '3989812241x', 'a', 'aa', 'bbb', 'done'):
  om.somestring = word
  sync_remote() # give binding time to propagate value
  #print "SEE1: " + testwidget.string_prop + ' (expect: ' + word + ')'
  assert testwidget.string_prop == word

sync_remote() # give binding time to complete the last updates

# check value binding from UI property to ObjectModel property
for word in ('uiuiui22', 'HelloHello', 'x7c6g3j9', 'T', 'Te', 'Tes', 'Test'):
  testwidget.string_prop = word
  sync_remote() # give binding time to propagate value
  #print "SEE2: " + om.somestring + ' (expect: ' + word + ')'
  assert om.somestring == word

# OK if we got here
print '  %-6s' % 'CHECK', '%-67s' % __file__, 'OK'
