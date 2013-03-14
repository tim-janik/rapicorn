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
<rapicorn-definitions xmlns:tmpl="http://rapicorn.org/xmlns:tmpl">
  <tmpl:define id="FixmeWidgetList" inherit="Rapicorn::Factory::WidgetList"/>
  <tmpl:define id="my-window" inherit="Window">
    <VBox>
      <Button hexpand="1" on-click="Window::close()">
        <Label markup-text="Quit"/>
      </Button>
    <FixmeWidgetList />
    </VBox>
  </tmpl:define>
</rapicorn-definitions>
"""

# setup application
app = Rapicorn.app_init ("testrapicorn.py")     # provide unique application name
app.load_string ("MyTest", my_window_xml)       # load widget tree
window = app.create_window ("MyTest:my-window") # create main window

# property testing
assert window.name == "my-window"
window.name = "frob17"  ; assert window.name == "frob17"        # testing derived property
window.name = "window1" ; assert window.name == "window1"
window.title = "foo"    ; assert window.title == "foo"          # testing narrowed property
window.title = "bar"    ; assert window.title == "bar"
for i in (9999, 22, 778, 0, 1, 2, 3, 123):
  window.width = i ; assert window.width == i
# enum testing
wlist = window.query_selector ("#FixmeWidgetList")
wlist.selection_mode = Rapicorn.SELECTION_NONE          ; assert wlist.selection_mode == Rapicorn.SELECTION_NONE
wlist.selection_mode = Rapicorn.SELECTION_SINGLE        ; assert wlist.selection_mode == Rapicorn.SELECTION_SINGLE
wlist.selection_mode = Rapicorn.SELECTION_BROWSE        ; assert wlist.selection_mode == Rapicorn.SELECTION_BROWSE
wlist.selection_mode = Rapicorn.SELECTION_MULTIPLE      ; assert wlist.selection_mode == Rapicorn.SELECTION_MULTIPLE

# print positive feedback if we made it here
print " " * max (0, 75 - len ("  " + __file__)), "OK"
