# Rapicorn                              -*- Mode: python; -*-
# Copyright (C) 2008 Tim Janik
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# A copy of the GNU Lesser General Public License should ship along
# with this library; if not, see http://www.gnu.org/copyleft/.

"""Rapicorn - experimental UI toolkit

More details at http://www.rapicorn.org/.
"""

from pyRapicorn import *
from py2cpy import *
for s in dir (pyRapicorn):
  if s.startswith ('_PLIC_'):
    setattr (py2cpy, s, getattr (pyRapicorn, s))

app = None
def app_init_with_x11 (application_name = None, cmdline_args = None):
  import sys
  if application_name == None:
    application_name = sys.argv[0]
  if cmdline_args == None:
    cmdline_args = sys.argv
  a = Application()
  a.__plic__object__ = pyRapicorn._init_dispatcher (application_name, cmdline_args)
  global app
  app = a
  return app

app_init_with_x11 ("AppName", [ "--first-arg", "--second-arg"])
del globals()['pyRapicorn']

print "app:", app
apath = app.auto_path (".", ".", True)
print "auto_path:", apath
if not apath.find ('rapicorn/rope/.'):
  raise RuntimeError ('Test procedure failed...')

import time, random

# while True: app.test_counter_inc_fetch() # two way call

niter = 29
bounds = (7000, 7001)
w2samples, w1samples = [], []

for n in range (0, niter):
  app.test_counter_set (0)
  r = random.randrange (*bounds)
  t0 = time.clock()
  for i in range (0, r):
    app.test_counter_inc_fetch() # two way test
  tc = app.test_counter_get()
  t1 = time.clock()
  assert tc == r
  w2samples += [ r / (t1 - t0) ]

mc = max (w2samples)
fs = 1000000 / mc
sl = 1000000 / min (w2samples)
er = (sl - fs) / sl
print "2way: best: %u calls/s; fastest: %.2fus; slowest: %.2fus; err: %.2f%%" %\
    (mc, fs, sl, er * 100)

for n in range (0, niter):
  app.test_counter_set (0)
  r = random.randrange (bounds[0] + 10000, bounds[1] + 10000)
  t0 = time.clock()
  for i in range (0, r):
    app.test_counter_add (1)  # one way test
  tc = app.test_counter_get() # enforce queue flush before timer stops
  t1 = time.clock()
  assert tc == r
  w1samples += [ r / (t1 - t0) ]

mc = max (w1samples)
fs = 1000000 / mc
sl = 1000000 / min (w1samples)
er = (sl - fs) / sl
print "1way: best: %u calls/s; fastest: %.2fus; slowest: %.2fus; err: %.2f%%" %\
    (mc, fs, sl, er * 100)

print 'OK, done.'
