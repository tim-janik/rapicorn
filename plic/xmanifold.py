#!/usr/bin/env python
# xmanifold.py - manifold argument sequences                   -*-mode:python-*-
# CC0 Public Domain: http://creativecommons.org/publicdomain/zero/1.0/

import sys, re

rules = [
  (r'const size_t NARGS = 2;',        'const size_t NARGS = %i;'),
  (r'class A1, class A2',             ('',   'class A%i',   ', ', '')),
  (r'A1, A2',                         ('',   'A%i',         ', ', '')),
  (r'a1, a2',                         ('',   'a%i',         ', ', '')),
  (r'A1 a1; A2 a2;',                  ('',   'A%i a%i',     '; ', ';')),
  (r'A1 a1, A2 a2',                   ('',   'A%i a%i',     ', ', '')),
  (r'fbr >> a1; fbr >> a2;',          ('',   'fbr >> a%i',  '; ', ';')),
  (r'A1 m_a1; A2 m_a2;',              ('',   'A%i m_a%i',   '; ', ';')),
  (r'm_a1, m_a2',                     ('',   'm_a%i',       ', ', '')),
  (r': m_a1 \(a1\), m_a2 \(a2\)',     (': ', 'm_a%i (a%i)', ', ', '')),
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

def manifold (s, n, filename):
  loc = '' # '# %u "%s"\n' % (100000 + n * 1000 + 1, filename)
  for pair in rules:
    m, t = unfold (pair, n)
    if m[0].isalnum():  m = r'\b' + m
    if m[-1].isalnum(): m = m + r'\b'
    s = re.sub (m, t, s)
  return loc + s

def main():
  verify_patterns (2)
  if len (sys.argv) < 2:
    raise RuntimeError ('missing filename')
  if len (sys.argv) < 3:
    raise RuntimeError ('missing manifold count')
  filename = sys.argv[1]
  maxfold = int (sys.argv[2])
  txt = open (filename).read()
  for i in range (0, maxfold + 1):
    print manifold (txt, i, filename)

if __name__ == '__main__':
  main()
