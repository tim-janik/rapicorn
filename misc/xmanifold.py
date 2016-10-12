#!/usr/bin/env python2.7
# xmanifold.py - manifold argument sequences                   -*-mode:python-*-
# Licensed CC0 Public Domain: http://creativecommons.org/publicdomain/zero/1.0

import sys, re

rules = [
  (r'const size_t NARGS = 2;',        'const size_t NARGS = %i;'),
  (r'PyxxCaller2',                    'PyxxCaller%i'),
  # pattern                           strip  repeat         join postfix
  (r'class A1, class A2',             ('',   'class A%i',   ', ', '')),
  (r'A1, A2',                         ('',   'A%i',         ', ', '')),
  (r'a1, a2',                         ('',   'a%i',         ', ', '')),
  (r'typename Type<A1>::T a1; '+
   r'typename Type<A2>::T a2;',       ('',   'typename Type<A%i>::T a%i', '; ', ';')),
  (r'typename ValueType<A1>::T a1; '+
   r'typename ValueType<A2>::T a2;',  ('',   'typename ValueType<A%i>::T a%i', '; ', ';')),
  (r'A1 a1; A2 a2;',                  ('',   'A%i a%i',     '; ', ';')),
  (r'A1 a1, A2 a2',                   ('',   'A%i a%i',     ', ', '')),
  (r'fbr >>= a1; fbr >>= a2;',        ('',   'fbr >>= a%i', '; ', ';')),
  (r'A1 m_a1; A2 m_a2;',              ('',   'A%i m_a%i',   '; ', ';')),
  (r'm_a1, m_a2',                     ('',   'm_a%i',       ', ', '')),
  (r': m_a1 \(a1\), m_a2 \(a2\)',     (': ', 'm_a%i (a%i)', ', ', '')),
]
fixups = [
  [ 0, (', >',  '>') ],
  [ 0, (', \)', ')') ],
]

def reindex (s, i):
  return re.sub (r'%i\b', '%i' % i, s)

def unfold (pair, n):
  m, p = pair
  if isinstance (p, str):
    t = reindex (p, n)
  else:
    pre, pat, joi, post = p                             # pre=': ' pat='m_a%i (a%i)' joi=', ' post='{}'
    l = [reindex (pat, i) for i in range (1, n + 1)]    # l=['A1', 'A2', ...]
    t = pre + joi.join (l) + post                       # t=': m_a1 (a1), m_a2 (a2),... {}'
    if n == 0: t = ''
  return (m, t)

def verify_patterns (n):
  for pair in rules:
    m, t = unfold (pair, n)
    m = re.sub (r'\\(.)', r'\1', m)     # "unescape" backslashes
    if m != t:
      raise AssertionError ('%s == %s' % (repr (m), repr (t)))

def manifold (s, n, filename, line_offset):
  loc = '# %u "%s"\n' % (n * 1000 + line_offset, filename)
  for pair in rules:
    m, t = unfold (pair, n)
    if m[0].isalnum():  m = r'\b' + m
    if m[-1].isalnum(): m = m + r'\b'
    s = re.sub (m, t, s)
  for fixup in [f[1] for f in fixups if f[0] == n]: # pick out fixups for level 'n'
    m, t = fixup
    if m[0].isalnum():  m = r'\b' + m
    if m[-1].isalnum(): m = m + r'\b'
    s = re.sub (m, t, s)
  return s # loc + s

def main():
  verify_patterns (2)
  if len (sys.argv) <= 1:
    print 'Usage: xmanifold.py [-S] <filename> [maxcount] [outfile]'
    sys.exit (1)
  parse_sections = False
  filename = None
  maxfold = 20
  outfile = '/dev/stdout'
  firstarg = 1
  if len (sys.argv) > firstarg and sys.argv[firstarg] == '-S':
    parse_sections = True
    firstarg += 1
  if len (sys.argv) > firstarg:
    filename = sys.argv[firstarg]
    firstarg += 1
  else:
    raise RuntimeError ('Missing input file')
  if len (sys.argv) > firstarg:
    maxfold  = int (sys.argv[firstarg])
    firstarg += 1
  if len (sys.argv) > firstarg:
    outfile  = sys.argv[firstarg]
  txt = open (filename).read()
  line_offset = 1
  if parse_sections:
    m = re.match (r'^(.*)#\s*ifdef\s+XMANIFOLD_SECTION(.*)#\s*endif\s+//\s*XMANIFOLD_SECTION\b', txt, re.MULTILINE | re.DOTALL)
    if not m:
      raise RuntimeError ('%s: failed to identify XMANIFOLD_SECTION' % filename)
    line_offset = 1 + m.group (1).count ('\n')
    txt = m.group (2)
  of = open (outfile, 'w')
  for i in range (0, maxfold + 1):
    print >>of, manifold (txt, i, filename, line_offset)

if __name__ == '__main__':
  main()
