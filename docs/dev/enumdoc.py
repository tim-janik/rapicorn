# Licensed GNU GPLv3 or later: http://www.gnu.org/licenses/gpl.html
import re, sys

# patterns
enum_value_end        = r'(\w(?:[^,{}]*))([,}]|$)'
doxygen_comment       = r'/(?:\*[*!]|/[/!])'
member_comment        = doxygen_comment + r'<'
no_look_ahead_comment = r'(?![^\n]*' + member_comment + r')'
re_enum_value = re.compile (enum_value_end + no_look_ahead_comment)
re_enum_def   = re.compile (r'((?<!_)\benum(?=[\s{])[^{}]*{[^}]*})')
re_dcomment   = re.compile (doxygen_comment)
re_keyword    = re.compile (r'\b(?:typedef|enum)\b')
re_dnl        = re.compile (r'(?:^\s*#.*$|//.*$|/\*([^*]|\*[^/])*(\*/)?)') # C++ delete until newline (comment or CPP directive)
re_ocomment   = re.compile (r'(?://[^\n]*|(?<!/)/\*([^*]|\*[^/])*)\Z')     # open/unfinished C/C++ comment

def fixline (txt):
  if re_dcomment.search (txt):                  # has existing Doxygen comment
    return txt
  s = re_keyword.sub ('', txt)
  s = re_dnl.sub ('', s)                        # strip keywords and comments
  if not re.search (r'\w', s):                  # anything left for marking?
    return txt
  # mark all enum value definitions as documented
  return re_enum_value.sub (r'\1/**< */\2', txt)

def fixenum (txt):
  lines = txt.split ('\n')                      # process line wise
  return '\n'.join ([fixline (ll) for ll in lines])

# Usage: python enumdoc.py [iofiles...]
for f in sys.argv[1:]:
  l = re_enum_def.split (open (f).read())       # find all enum definitions
  open_comment = False
  for i in range (0, len (l)):
    if i % 2 == 0:                              # non-enum
      open_comment = re_ocomment.search (l[i])
    elif not open_comment:                      # ignore commented enums
      l[i] = fixenum (l[i])                     # convert real, non-commented enum
  open (f, 'w').write (''.join (l))
