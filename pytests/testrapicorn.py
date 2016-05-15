# Licensed CC0 Public Domain: http://creativecommons.org/publicdomain/zero/1.0
"""
Rapicorn test program for Python
"""
import Rapicorn
import sys

# issue test message
print "  " + __file__,

# Define main window Widget Tree
my_window_xml = """
<interfaces>
  <Window declare="my-window">
    <VBox>
      <Button hexpand="1" on-click="Window::close()">
        <Label markup-text="Quit"/>
      </Button>
      <WidgetList id="CheckWidgetList"/>
    </VBox>
  </Window>
</interfaces>
"""

# setup application
app = Rapicorn.init_app()
app.load_string (my_window_xml)                 # load widget tree
window = app.create_window ('my-window')        # create main window

# property testing
assert window.id == "my-window"
window.id = "frob17"  ; assert window.id == "frob17"    # testing derived property
window.id = "window1" ; assert window.id == "window1"
window.title = "foo"  ; assert window.title == "foo"    # testing narrowed property
window.title = "bar"  ; assert window.title == "bar"
for i in (9999, 22, 778, 0, 1, 2, 3, 123):
  window.width = i ; assert window.width == i
# enum testing
wlist = window.query_selector ("#CheckWidgetList")
wlist.selection_mode = Rapicorn.SelectionMode.NONE      ; assert wlist.selection_mode == Rapicorn.SelectionMode.NONE
wlist.selection_mode = Rapicorn.SelectionMode.SINGLE    ; assert wlist.selection_mode == Rapicorn.SelectionMode.SINGLE
wlist.selection_mode = Rapicorn.SelectionMode.BROWSE    ; assert wlist.selection_mode == Rapicorn.SelectionMode.BROWSE
wlist.selection_mode = Rapicorn.SelectionMode.MULTIPLE  ; assert wlist.selection_mode == Rapicorn.SelectionMode.MULTIPLE

# print positive feedback if we made it here
print " " * max (0, 75 - len ("  " + __file__)), "OK"
