# Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
import sys, re

rapicorn_debug_items, rapicorn_debug_items_set = '', set()
rapicorn_debug_keys, rapicorn_debug_keys_set = '', set()
rapicorn_flippers, rapicorn_flippers_set = '', set()

def process_start ():
  pass

def process_comment (txt):
  pass # print 'C>>>', txt

def process_code (txt):
  cstring = r' " ( (?: [^\\"] | \\ .) * ) " '
  # RAPICORN_DEBUG_OPTION (option, blurb)
  global rapicorn_debug_items, rapicorn_debug_items_set
  pattern = r'\b RAPICORN_DEBUG_OPTION \s* \( \s* ' + cstring + r' \s* , \s* ' + cstring + ' \s* \) \s* ;'
  matches = re.findall (pattern, txt, re.MULTILINE | re.VERBOSE)
  for m in matches:
    if not m[0] in rapicorn_debug_items_set:
      rapicorn_debug_items += '  * - @c %s - %s\n' % (m[0], m[1])
      rapicorn_debug_items_set.add (m[0])
  # RAPICORN_KEY_DEBUG (keys, ...)
  global rapicorn_debug_keys, rapicorn_debug_keys_set
  pattern = r'\b RAPICORN_KEY_DEBUG \s* \( \s* ' + cstring + r' \s* , \s* [^;] *? \)'
  matches = re.findall (pattern, txt, re.MULTILINE | re.VERBOSE)
  for m in matches:
    if not m in rapicorn_debug_keys_set:
      rapicorn_debug_keys += '  * - @c %s - Debugging message key, =1 enable, =0 disable.\n' % m
      rapicorn_debug_keys_set.add (m)
  # RAPICORN_FLIPPER (option, blurb)
  global rapicorn_flippers, rapicorn_flippers_set
  pattern = r'\b RAPICORN_FLIPPER \s* \( \s* ' + cstring + r' \s* , \s* ' + cstring + ' \s* \) \s* ;'
  matches = re.findall (pattern, txt, re.MULTILINE | re.VERBOSE)
  for m in matches:
    if not m[0] in rapicorn_flippers_set:
      rapicorn_flippers += '  * - @c %s - %s\n' % (m[0], m[1])
      rapicorn_flippers_set.add (m[0])

def process_end ():
  if rapicorn_debug_items:
    print '/** @var $RAPICORN_DEBUG'
    print rapicorn_debug_items, '*/'
  if rapicorn_debug_keys:
    print '/** @var $RAPICORN_DEBUG'
    print rapicorn_debug_keys, '*/'
  if rapicorn_flippers:
    print '/** @var $RAPICORN_FLIPPER'
    print rapicorn_flippers, '*/'

def process_specific (text):
  def is_comment (t):
    return t.startswith ('//') or t.startswith ('/*')
  cxx_splitter = r"""(
    "   (?: [^\\"] | \\  .   ) * "        # double quoted string
  | '   (?: [^\\'] | \\  .   ) * '        # single quoted string
  | /\* (?: [^*]   | \* [^/] ) * \*/      # C comment
  | //  (?: [^\n]            ) * \n       # C++ comment
  )"""
  parts = re.split (cxx_splitter, text, 0, re.MULTILINE | re.VERBOSE)
  # parts = filter (len, parts) # strip empty parts
  # concatenate code vs. comment bits
  process_start()
  i = 0
  while i < len (parts):
    s = parts[i]
    isc = is_comment (s)
    i += 1
    while i < len (parts) and isc == is_comment (parts[i]):
      s += parts[i]
      i += 1
    if isc:
      process_comment (s)
    else:
      process_code (s)
  process_end()

# print '/// @file'
process_specific (sys.stdin.read())
