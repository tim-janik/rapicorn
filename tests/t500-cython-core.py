# Licensed CC0 Public Domain: http://creativecommons.org/publicdomain/zero/1.0
import Rapicorn # @LINE_REWRITTEN_FOR_INSTALLCHECK@
import collections

# verify that pycallable() raises Exception
def assert_raises (Exception, pycallable, *args, **kwds):
  try:    pycallable (*args, **kwds)
  except Exception: return
  raise AssertionError('%s not raised' % Exception.__name__)

# test MainLoop and EventLoop object identities
ml = Rapicorn.MainLoop()
sl = ml.create_slave()
sm = sl.main_loop()
m3 = Rapicorn.MainLoop()
assert ml == ml and sl == sl and sm == sm and m3 == m3
assert ml != sl and sl != ml and sm != sl and sl != sm
assert ml == sm and sm == ml and ml != m3 and m3 != ml
assert ml.main_loop() == ml  and m3.main_loop() == m3

# test exec_callback
ect_counter = 5
ect_value = ""
def bhandler():
  global ect_counter, ect_value
  if ect_counter:
    ect_value += "*"
    ect_counter -= 1
  return ect_counter
ml = Rapicorn.MainLoop()
ml.exec_callback (bhandler)
ml.iterate_pending()
assert ect_counter == 0
assert ect_value == "*****"

# test exec_dispatcher
prepared   = [ None, None, None ]
checked    = [ None, None, None ]
dispatched = [ None, None, None ]
counter    = 0
ml = Rapicorn.MainLoop()
def dispatcher (loop_state):
  global counter, prepared, checked, dispatched
  if   loop_state.phase == loop_state.PREPARE:
    prepared[counter] = True
    return True
  elif loop_state.phase == loop_state.CHECK:
    checked[counter] = "Yes"
    return True
  elif loop_state.phase == loop_state.DISPATCH:
    dispatched[counter] = counter
    counter += 1
    return counter <= 2
ml.exec_dispatcher (dispatcher)
ml.iterate_pending()
summary = [ counter ] + prepared + checked + dispatched
assert summary == [3, True, True, True, 'Yes', 'Yes', 'Yes', 0, 1, 2]

# Enum Tests
assert Rapicorn.FocusDir
assert Rapicorn.FocusDir.UP and Rapicorn.FocusDir.DOWN
assert Rapicorn.FocusDir.UP != Rapicorn.FocusDir.DOWN
assert Rapicorn.FocusDir.UP.name == 'UP'
assert Rapicorn.FocusDir.DOWN.name == 'DOWN'
assert Rapicorn.FocusDir[Rapicorn.FocusDir.UP.name] == Rapicorn.FocusDir.UP
assert Rapicorn.FocusDir[Rapicorn.FocusDir.DOWN.name] == Rapicorn.FocusDir.DOWN
assert Rapicorn.FocusDir.UP.value > 0 and Rapicorn.FocusDir.DOWN.value > 0
assert Rapicorn.FocusDir.UP.value < Rapicorn.FocusDir.DOWN.value and Rapicorn.FocusDir.DOWN.value > Rapicorn.FocusDir.UP.value
assert Rapicorn.FocusDir.UP.value + Rapicorn.FocusDir.DOWN.value > 0

# List Base Tests
s = Rapicorn.BoolSeq ([ True, False, "A", [], 88, None ])
s = Rapicorn.StringSeq ([ '1', 'B' ])
# assert_raises (TypeError, Rapicorn.StringSeq, [ None ])

# Record Tests
# Pixbuf
p = Rapicorn.Pixbuf()
p.row_length = 2
p.pixels = [ 0x00000000, 0xff000000 ]
p.variables = [ 'meta=foo' ]
assert p.row_length == 2
assert p.pixels == [ 0x00000000, -16777216 ]
assert p.variables == [ 'meta=foo' ]
# UpdateSpan
u = Rapicorn.UpdateSpan()
u.start = 7
u.length = 20
assert u.start == 7 and u.length == 20
assert u._asdict() == collections.OrderedDict ([ ('start', 7), ('length', 20) ])
def invalid_assignment (u): u.no_such_dummy = 999
assert_raises (AttributeError, invalid_assignment, u)
# UpdateRequest
r = Rapicorn.UpdateRequest()
r.kind = Rapicorn.UpdateKind.CHANGE
r.rowspan = u
r.colspan = u
r.variables = p.variables
assert r.kind == Rapicorn.UpdateKind.CHANGE
assert r.colspan == u
assert r.rowspan == r.colspan
assert r.variables == p.variables
# FIXME: r._asdict needs Any
# Requisition
q = Rapicorn.Requisition (width = 7, height = 8)
assert q.width == 7 and q.height == 8
assert list (q) == [ 7, 8 ]
assert q._asdict() == collections.OrderedDict ([ ('width', 7), ('height', 8) ])
assert q[0] == 7 and q[1] == 8
def indexed_access (o, i): return o[i]
assert_raises (IndexError, indexed_access, q, 2)
# Test richcmp
def R (w, h):
  return Rapicorn.Requisition (width = w, height = h)
a = R (17, 33) ; b = R (17, 33) ; assert a == b and a <= b and a >= b and not (a != b) and not (a >  b) and not (a <  b)
a = R (22, 11) ; b = R (22, 11) ; assert a == b and a <= b and a >= b and not (a != b) and not (a >  b) and not (a <  b)
a = R (39, 99) ; b = [ 39, 99 ] ; assert a == b and a <= b and a >= b and not (a != b) and not (a >  b) and not (a <  b)
a = [ 44, 40 ] ; b = R (44, 40) ; assert a == b and a <= b and a >= b and not (a != b) and not (a >  b) and not (a <  b)
a = R (11, 22) ; b = R (11, 21) ; assert a != b and a >  b and a >= b and not (a == b) and not (a <= b) and not (a <  b)
a = R (11, 22) ; b = R (11, 23) ; assert a != b and a <= b and a <  b and not (a == b) and not (a >  b) and not (a >= b)
a = R (11, 22) ; b = [ 11 ]     ; assert a != b and a >= b and a >  b and not (a == b) and not (a <  b) and not (a <= b)
a = R (1, 2)  ; b = [ 1, 2, 3 ] ; assert a != b and a <= b and a <  b and not (a == b) and not (a >  b) and not (a >= b)
a = R (1, 2)  ; b = [ 1 ]       ; assert a != b and a >= b and a >  b and not (a == b) and not (a <  b) and not (a <= b)
a = R (1, 99) ; b = [ 1, 1, 1 ] ; assert a != b and a >= b and a >  b and not (a == b) and not (a <  b) and not (a <= b)
a = R (1, 0)  ; b = [ 1, 1, 1 ] ; assert a != b and a <= b and a <  b and not (a == b) and not (a >  b) and not (a >= b)

# Manual object creation is not possible
assert_raises (TypeError, Rapicorn.Object)
assert_raises (TypeError, Rapicorn.Widget)
assert_raises (TypeError, Rapicorn.Container)
assert_raises (TypeError, Rapicorn.Window)
assert_raises (TypeError, Rapicorn.Application)

# Application setup
app = Rapicorn.init_app()

# Test object handles
w1 = app.create_window ('Window')
wl = app.query_windows ("")
assert wl == []
wl = app.query_windows ("Window")
assert len (wl) == 1

w2 = app.create_window ('Window')
wl = app.query_windows ("Window")
assert len (wl) == 2
assert w2 != app and w1 != app
assert w1 != w2
assert w1 != wl[0] or w1 != wl[1]
assert w2 != wl[0] or w2 != wl[1]
assert w1 in wl and w2 in wl
if w1 == wl[0]: # equal objects must hash to the same value
  assert hash (w1) == hash (wl[0]) and hash (w2) == hash (wl[1])
else:
  assert hash (w2) == hash (wl[0]) and hash (w1) == hash (wl[1])
assert hash (w1) != hash (w2) or hash (w1) != hash (app) # highly likely
a, b = (w1, w2) if w1 < w2 else (w2, w1)
assert a != b and a <= b and a < b and not (a == b) and not (a >= b) and not (a > b)
b = a
assert a == b and a <= b and a >= b and not (a != b) and not (a < b) and not (a > b)

# Show Window
# w1 = app.create_window ('Window', '')
l = w1.create_widget ('Label', ['markup-text=Hello Cython World!'])
seen_window_display = False
def displayed():
  global seen_window_display
  print "Hello, window is being displayed"
  seen_window_display = True
  app.quit()
w1.sig_displayed.connect (displayed)
w1.show()
app.main_loop().exec_timer (app.quit, 2500) # ensures test doesn't hang

# Check for the absence of type erasure
w2 = app.create_window ('Window', '')
hb = w2.create_widget ('HBox', [])
l1 = hb.create_widget ('Label', ['markup-text=Hello 1'])
l2 = hb.create_widget ('Label', ['markup-text=Hello 2'])
l3 = hb.create_widget ('Label', ['markup-text=Hello 3'])
widgets = w2.query_selector_all ('.Widget')
assert Rapicorn.Window in [w.__class__ for w in widgets]
assert Rapicorn.HBox   in [w.__class__ for w in widgets]
assert Rapicorn.Label  in [w.__class__ for w in widgets]

app.run()
assert seen_window_display

# python -ic "import sys, os; sys.path.insert (0, os.path.abspath ('.libs')) ; import Rapicorn"
