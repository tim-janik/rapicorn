# Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
import sys, re

rapicorn_debug_items = ''

def process_start ():
  pass

def process_comment (txt):
  pass # print 'C>>>', txt

def process_code (txt):
  cstring = r' " ( (?: [^\\"] | \\ .) * ) " '
  # RAPICORN_DEBUG_OPTION (option, blurb)
  global rapicorn_debug_items
  pattern = r'\b RAPICORN_DEBUG_OPTION \s* \( \s* ' + cstring + r' \s* , \s* ' + cstring + ' \s* \) \s* ;'
  matches = re.findall (pattern, txt, re.MULTILINE | re.VERBOSE)
  for m in matches:
    rapicorn_debug_items += '  * - @c %s - %s\n' % (m[0], m[1])

def process_end ():
  if rapicorn_debug_items:
    print '/** @var $RAPICORN_DEBUG'
    print rapicorn_debug_items
    print '*/'

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
