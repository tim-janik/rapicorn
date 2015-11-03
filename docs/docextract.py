# This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
import sys, re

rapicorn_debug_items, rapicorn_debug_items_set = '', set()
rapicorn_debug_keys, rapicorn_debug_keys_set = '', set()
rapicorn_flippers, rapicorn_flippers_set = '', set()
rapicorn_todo_lists = {}
rapicorn_bug_lists = {}

def process_start ():
  pass

def process_comment (txt, lines):
  if not txt.startswith (('///', '/**', '/*!')):
    return      # filter non-doxygen comments
  # @TODO @TODOS
  global rapicorn_todo_lists
  pattern = r'[@\\] TODO[Ss]? \s* ( : )'
  match = re.search (pattern, txt, re.MULTILINE | re.VERBOSE)
  if match:
    text = txt[match.end (1):].strip()                  # take todo text, whitespace-stripped
    text = text[:-2] if text.endswith ('*/') else text  # strip comment-closing
    text = re.sub (r'( ^ | \n) \s* \*+',                # match comment prefix at line start
                   r'\1', text, 0, re.X | re.M)         # strip comment prefix from all lines
    pattern = r'\s* ( [*+-] | [0-9]+ \. )'              # pattern for list bullet
    if not re.match (pattern, text, re.X):              # not a list
      l = lines + txt[0:match.start (1)].count ('\n')
      text = ' - %d: @b TODO: ' % l + text              # insert list bullet
    else:
      text = '%d: @b TODOS:\n' % lines + text
    blurb = rapicorn_todo_lists.get (filename, '')
    blurb += text.rstrip() + '\n'
    rapicorn_todo_lists[filename] = blurb
  # @BUG @BUGS
  global rapicorn_bug_lists
  pattern = r'[@\\] BUG[Ss]? \s* ( : )'
  match = re.search (pattern, txt, re.MULTILINE | re.VERBOSE)
  if match:
    text = txt[match.end (1):].strip()                  # take bug text, whitespace-stripped
    text = text[:-2] if text.endswith ('*/') else text  # strip comment-closing
    text = re.sub (r'( ^ | \n) \s* \*+',                # match comment prefix at line start
                   r'\1', text, 0, re.X | re.M)         # strip comment prefix from all lines
    pattern = r'\s* ( [*+-] | [0-9]+ \. )'              # pattern for list bullet
    if not re.match (pattern, text, re.X):              # not a list
      l = lines + txt[0:match.start (1)].count ('\n')
      text = ' - %d: @b BUG: ' % l + text               # insert list bullet
    else:
      text = '%d: @b BUGS:\n' % lines + text
    blurb = rapicorn_bug_lists.get (filename, '')
    blurb += text.rstrip() + '\n'
    rapicorn_bug_lists[filename] = blurb

def process_code (txt, lines):
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

def sanitize_ident (txt):
  return re.sub (r'[^0-9_a-zA-Z]+', '_', txt).strip ('_')

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
  if rapicorn_todo_lists:
    # ('/** @page todo_lists Todo Lists', ' * @section %s %s' % (sanitize_ident (filename), filename), '*/')
    for filename, blurb in rapicorn_todo_lists.items():
      ident = sanitize_ident (filename)
      filename = filename[2:] if filename.startswith ('./') else filename
      print '/** @file %s' % filename
      print '@xrefitem todo "Issues" "Open Issues"' # sync with doxygen.cfg
      print blurb.rstrip()
      print '*/'
  print '/**\n@page todo Open Issues\n*/' # needed to refer to 'todo' page with @subpage
  if rapicorn_bug_lists:
    # ('/** @page bug_lists Bug Lists', ' * @section %s %s' % (sanitize_ident (filename), filename), '*/')
    for filename, blurb in rapicorn_bug_lists.items():
      ident = sanitize_ident (filename)
      filename = filename[2:] if filename.startswith ('./') else filename
      print '/** @file %s' % filename
      print '@xrefitem todo "Issues" "Open Issues"'
      print blurb.rstrip()
      print '*/'
  print '/**\n@page unstable Unstable API\n*/' # needed to refer to 'unstable' page with @subpage

def process_specific (filename, text):
  def is_comment (t):
    return t.startswith ('//') or t.startswith ('/*')
  cxx_splitter = r"""(
    "   (?: [^\\"] | \\  .      )* "        # double quoted string
  | '   (?: [^\\'] | \\  .      )* '        # single quoted string
  | /\* (?: [^*]   | \* (?! / ) )* \*/      # C comment
  | //  (?: [^\n]               )* \n       # C++ comment
  )"""
  parts = re.split (cxx_splitter, text, 0, re.MULTILINE | re.VERBOSE)
  # parts = filter (len, parts) # strip empty parts
  # concatenate code vs. comment bits
  i, l = 0, 1
  while i < len (parts):
    s = parts[i]
    isc = is_comment (s)
    i += 1
    while i < len (parts) and isc == is_comment (parts[i]):
      s += parts[i]
      i += 1
    if isc:
      process_comment (s, l)
    else:
      process_code (s, l)
    l += s.count ('\n')

process_start()
# print '/// @file'
for iline in sys.stdin.readlines():
  filename = iline.rstrip() # removes trailing '\n'
  process_specific (filename, open (filename, 'r').read())
process_end()
