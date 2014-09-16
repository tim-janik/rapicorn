# Licensed CC0 Public Domain: http://creativecommons.org/publicdomain/zero/1.0
# Author: 2014, Tim Janik, see http://rapicorn.org
import sys

N_GC_CYCLES = 7

# Test by asserting the output of: RAPICORN_DEBUG=GCStats python testgc.py -i
if not '-i' in sys.argv:
  import subprocess, os, re
  # capture output from invoking self with GC statistics enabled
  os.environ['RAPICORN_DEBUG'] = os.environ.get ('RAPICORN_DEBUG', '') + ':GCStats:'
  output = subprocess.check_output ([ sys.executable ] + sys.argv + [ '-i' ], stderr = subprocess.STDOUT)
  # check for garbage collection interleaved with this script's activity
  startgc = r'TestGC-Release'
  collected = r'GARBAGE_COLLECTED.*?purged=[1-9]'
  released = r'TestGC-Released'
  # the GC cycle executes asynchronously, so collected/released may be printed in any order
  gccycle = startgc + '.*?' + '(' + collected + '.*?' + released + '|' + released + '.*?' + collected + ')'
  gc_matches = re.findall (gccycle, output, re.S | re.M)
  # OK if we find the required number of GC interactions
  assert len (gc_matches) >= N_GC_CYCLES
  print '  %-6s' % 'CHECK', '%-67s' % __file__, 'OK'
  sys.exit (0)

# Load and import a versioned Rapicorn module into the 'Rapicorn' namespace
from Rapicorn1307 import Rapicorn

# Setup the application object, unsing a unique application name.
app = Rapicorn.app_init ("Test Rapicorn GC")

# Define the elements of the dialog window to be displayed.
hello_window = """
<rapicorn-definitions>
  <tmpl:define id="testgc-py" inherit="Window">
    <Table id="container"/>
  </tmpl:define>
  <tmpl:define id="testwidget" inherit="IdlTestWidget"/>
</rapicorn-definitions>
"""

app.load_string ("T", hello_window) # useless namespace

window = app.create_window ("T:testgc-py")

container = window.query_selector_unique ('#container')
assert container

def sync_remote():
  for i in range (2):                     # settling bindings may require extra round trips
    while app.iterate (False, True): pass # process signals and events pending locally
    app.test_hook()                       # force round trip, ensure we pickup notifications pending remotely
  while app.iterate (False, True): pass   # process new/remaining batch of signals and events

def gc_children():
  total, batch = 100, 20
  print "TestGC-Create %u widgets..." % total
  l = []
  for i in range (total / batch):
    for j in range (batch):
      child = container.create_widget ("T:testwidget")
      l += [ child ]
  assert child
  print "TestGC-Created %u." % len (l)
  print "TestGC-Release %u widgets..." % total
  c = 0
  l.reverse()
  while l:
    child = l.pop()
    container.remove_widget (child)
    c += 1
  del child, l
  print "TestGC-Released %u." % c

for i in range (N_GC_CYCLES):
  gc_children()
  sync_remote()
del container
del window
# window.show()

sync_remote() # give time to shutdown
