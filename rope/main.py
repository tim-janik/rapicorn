# Rapicorn                              -*- Mode: python; -*-
# Copyright (C) 2010 Tim Janik
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
Main.py - Main event loop implementation.
More details at http://www.rapicorn.org/.
"""
import time, select

NEEDS_DISPATCH, PREPARED, UNCHECKED, DESTROYED = 'NpuD'

app = None      # set from __init__.py

class Source (object):
  def __init__ (self, callable, loop = None):
    self.priority = 0
    self.callable = callable
    self.state = UNCHECKED
    self.loop = None
    self.pollfd = -1
    self.pollevents = 0
    self.dispatching = 0
    if loop:
      loop.add_source (self)
  def set_poll (self, fd, pollevents):
    if isinstance (pollevents, str):
      import operator
      pdict = { 'i' : select.POLLIN, 'p' : select.POLLPRI, 'o' : select.POLLOUT,
                'e' : select.POLLERR, 'h' : select.POLLHUP, 'n' : select.POLLNVAL }
      pollevents = reduce (operator.__or__, [pdict.get (c, 0) for c in pollevents])
    self.pollfd = fd
    self.pollevents = pollevents
    if self.loop:
      self.loop.update_poll (self)
  def prepare (self, current_time, timeout):
    return True                                 # returns needs_dispatch
  def check (self, current_time, fdevents):
    return True                                 # returns needs_dispatch
  def dispatch (self, fdevents):
    return self.callable()                      # returs stay_alive
  def destroy (self):
    self.state = DESTROYED
    if self.loop:
      self.loop.update_poll (self)

class IdleSource (Source):
  def __init__ (self, callable):
    super (IdleSource, self).__init__ (callable)

class TimeoutSource (Source):
  def __init__ (self, callable, initial_interval = 0, repeat_interval = 0):
    super (TimeoutSource, self).__init__ (callable)
    self.expiration_time = time.time() + initial_interval
    self.repeat_interval = repeat_interval
  def prepare (self, current_time, timeout):
    if self.expiration_time <= current_time:
      return True
    timeout.expire (self.expiration_time - current_time)
    return False
  def check (self, current_time, fdevents):
    return self.expiration_time <= current_time
  def dispatch (self, fdevents):
    repeat = self.callable()
    if repeat:
      self.expiration_time = time.time() + self.repeat_interval
    return repeat

class Loop:
  class Timeout:
    def __init__ (self, timeout_time):
      self.expire_time = timeout_time
    def expire (self, timeout_time):
      self.expire_time = min (self.expire_time, timeout_time)
    def expire_msecs (self):
      if self.expire_time < 0:
        return 0
      msecs = self.expire_time * 1000
      msecs = min (msecs, 2147483647)
      return int (msecs)
  def __init__ (self):
    self.sources = []
    self.quit_status = None
    self.poll = select.poll()
    self.pollfds = {}
    self.fddict = {}
    self.cached_primary = True
    self.primary_sig = 0
  def update_poll (self, src):
    fd = self.pollfds.get (src, -1)
    if fd >= 0:
      self.poll.unregister (fd)
    if src.pollfd >= 0 and src.state != DESTROYED:
      self.pollfds[src] = src.pollfd
      self.poll.register (src.pollfd, src.pollevents)
  def quit (self, status = 0):
    assert isinstance (status, int)
    if self.quit_status == None:
      self.quit_status = status
  def lost_primary (self):
    self.cached_primary = False
  def loop (self):
    if not self.primary_sig:
      self.primary_sig = app.sig_missing_primary_connect (self.lost_primary)
    while self.iterate (False, True):
      pass # handle all pending events
    self.cached_primary = not app.finishable()
    if not self.cached_primary:
      return self.quit_status
    while self.cached_primary and self.quit_status == None:
      self.iterate (True, True)
    return self.quit_status
  def __iadd__ (self, source):
    assert isinstance (source, Source)
    self.add_source (source)
    return self
  def __isub__ (self, src_callable):
    if callable (src_callable):
      for s in self.sources:
        if s.state != DESTROYED and s.callable == src_callable:
          s.destroy()
          break
    else:
      assert isinstance (src_callable, Source)
      if src_callable.loop == self:
        src_callable.destroy()
    return self
  def add_source (self, src):
    assert src.loop == None
    assert src.state != DESTROYED
    src.loop = self
    self.sources += [ src ]
    src.state = UNCHECKED
    self.update_poll (src)
  def prepare_sources (self, timeout): # returns needs_dispatch
    must_dispatch = False
    current_time = time.time()
    src_list = self.sources
    self.sources = []
    for src in src_list:
      if src.state == DESTROYED:
        del src
        continue
      if src.priority <= self.max_priority:
        needs_dispatch = src.prepare (current_time, timeout)
        if src.state != DESTROYED:
          if needs_dispatch:
            src.state = NEEDS_DISPATCH
            self.max_priority = src.priority # starve lower priority sources
            must_dispatch = True
          else:
            src.state = PREPARED
          self.sources += [ src ]
      else:
        self.sources += [ src ]
    # here, all sources <= max_priority have NEEDS_DISPATCH || PREPARED
    if must_dispatch:
      timeout.expire_time = 0
    return must_dispatch
  def check_sources (self): # returns needs_dispatch
    current_time = time.time()
    must_dispatch = False
    for src in self.sources:
      if src.priority <= self.max_priority:
        if src.state == NEEDS_DISPATCH:
          must_dispatch = True
        elif (src.state == PREPARED and
              src.check (current_time, self.fddict.get (src.pollfd, 0)) and
              src.state != DESTROYED):
          src.state = NEEDS_DISPATCH
          self.max_priority = src.priority # starve lower priority sources
          must_dispatch = True
    # here, all sources <= max_priority have NEEDS_DISPATCH || PREPARED
    return must_dispatch
  def dispatch_sources (self):
    for src in self.sources:
      if src.state == DESTROYED:
        continue # reaped upon prepare
      needs_dispatch = src.priority <= self.max_priority and src.state == NEEDS_DISPATCH
      src.state = UNCHECKED
      if needs_dispatch:
        src.dispatching += 1
        needs_destroy = not src.dispatch (self.fddict.get (src.pollfd, 0))
        src.dispatching -= 1
        if needs_destroy and src.state != DESTROYED:
          src.destroy()
  def iterate (self, may_block, may_dispatch): # returns needs_dispatch
    maximum = 2**32 - 1
    timeout = Loop.Timeout (maximum if may_block else 0)
    self.max_priority = maximum
    needs_dispatch = self.prepare_sources (timeout)
    fdevlist = self.poll.poll (timeout.expire_msecs())
    self.fddict = dict (fdevlist)
    needs_dispatch |= self.check_sources()
    if may_dispatch and needs_dispatch:
      self.dispatch_sources ()
    return needs_dispatch
