# CC0 Public Domain: http://creativecommons.org/publicdomain/zero/1.0/

# setup Rapicorn test
from Rapicorn1307 import Rapicorn # Rapicorn modules are versioned
app = Rapicorn.app_init()

def show_and_display (win): # show and run main loop until the window is fully displayed
  # FIXME: convenience function for app
  assert app
  assert win.closed() == True
  def window_displayed():
    window_displayed.seen_signal = True
  window_displayed.seen_signal = False
  conid = win.sig_displayed.connect (window_displayed)
  win.show()
  while not window_displayed.seen_signal:
    app.iterate (True, True)
  win.sig_displayed -= conid

# = SizeGroup Tests =
# create wide and a tall buttons that are resized via size groups
decls = """
<tmpl:define id="SGWindow" inherit="Window">
  <VBox>
    <Label markup-text="Horizontal SizeGroup:"/>
    <Frame>
      <HBox>
        <Button id="wide1" hsize-group="HGroupedWidgets">
          <Label markup-text="Wide Button"/></Button>
        <Button id="tall1" hsize-group="HGroupedWidgets">
          <Label markup-text="T<br/>a<br/>l<br/>l<br/> <br/>B<br/>u<br/>t<br/>t<br/>o<br/>n"/></Button>
      </HBox>
    </Frame>
    <Label markup-text="Vertical SizeGroup:"/>
    <Frame>
      <HBox>
        <Button id="wide2" vsize-group="VGroupedWidgets">
          <Label markup-text="Wide Button"/></Button>
        <Button id="tall2" vsize-group="VGroupedWidgets">
          <Label markup-text="T<br/>a<br/>l<br/>l<br/> <br/>B<br/>u<br/>t<br/>t<br/>o<br/>n"/></Button>
      </HBox>
    </Frame>
  </VBox>
</tmpl:define>
"""
app.load_string ("local", decls)
win = app.create_window ("local:SGWindow")
show_and_display (win)  # show & render window

# fetch HSIZE buttons and check the sizes
test = "Horizontal SizeGroups"
wb = win.query_selector ("#wide1")
tb = win.query_selector ("#tall1")
wr = wb.requisition()
tr = tb.requisition()
assert wr.width > 0 and wr.height > 0
assert tr.width > 0 and tr.height > 0
assert wr.height != tr.height
assert wr.width == tr.width # size grouped
print '  %-6s' % 'CHECK', '%-67s' % test, 'OK'

# fetch VSIZE buttons and check the sizes
test = "Vertical SizeGroups"
wb = win.query_selector ("#wide2")
tb = win.query_selector ("#tall2")
wr = wb.requisition()
tr = tb.requisition()
assert wr.width > 0 and wr.height > 0
assert tr.width > 0 and tr.height > 0
assert wr.width != tr.width
assert wr.height == tr.height # size grouped
print '  %-6s' % 'CHECK', '%-67s' % test, 'OK'
