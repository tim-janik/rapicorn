# Load and import a versioned Rapicorn module into the 'Rapicorn' namespace
from Rapicorn1307 import Rapicorn

# Setup the application object, unsing a unique application name.
app = Rapicorn.app_init ("Test Rapicorn Bindings")

# Define the elements of the dialog window to be displayed.
hello_window = """
  <tmpl:define id="testbind-py" inherit="Window">
    <IdlTestWidget id="test-widget" string-prop="@bind somestring"/>
  </tmpl:define>
"""

app.load_string ("T", hello_window) # useless namespace

window = app.create_window ("T:testbind-py")

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
  # process events (signals) pending locally or remotely
  while app.iterate (False, True): pass # process signals pending locally
  app.test_hook()                       # force round trip, ensure we pickup notifications pending remotely
  while app.iterate (False, True): pass # process new batch of signals
  # our processing might have queued binding updates for the remote, so sync once more
  app.test_hook()                       # pick up remote notifications again
  while app.iterate (False, True): pass # process final set of signals

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
