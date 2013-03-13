# CC0 Public Domain: http://creativecommons.org/publicdomain/zero/1.0/
"""
Rapicorn test program for Python
"""
import Rapicorn1208 as Rapicorn # Rapicorn modules are versioned
import sys

# issue test message
print "  " + __file__,

# Define main window Widget Tree
my_window_xml = """
  <tmpl:define id="my-window" inherit="Window">
    <Button hexpand="1" on-click="Window::close()">
      <Label markup-text="Quit"/>
    </Button>
  </tmpl:define>
"""

# setup application
app = Rapicorn.app_init ("testrapicorn.py")     # provide unique application name
app.load_string ("MyTest", my_window_xml)       # load widget tree
window = app.create_window ("MyTest:my-window") # create main window

# property testing
# FIXME: assert window.name == "my-window"
window.title = "foo"    ; assert window.title == "foo"
window.title = "bar"    ; assert window.title == "bar"



# print positive feedback if we made it here
print " " * max (0, 75 - len ("  " + __file__)), "OK"
