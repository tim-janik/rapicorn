// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#include "inifile.hh"
#include "unicode.hh"
#include "strings.hh"
#include "blobres.hh"
#include "thread.hh"
#include <cstring>

#define ISASCIINLSPACE(c)    (c == ' ' || (c >= 9 && c <= 13))                  // ' \t\n\v\f\r'
#define ISASCIIWHITESPACE(c) (c == ' ' || c == '\t' || (c >= 11 && c <= 13))    // ' \t\v\f\r'

namespace Rapicorn {

/** @class IniFile
 * Configuration parser for INI files.
 * This class parses configuration files, commonly known as INI files.
 * The files contain "[Section]" markers and "attribute=value" definitions.
 * Comment lines are preceeded by a hash "#" sign.
 * For a detailed reference, see: http://wikipedia.org/wiki/INI_file <BR>
 * Localization of attributes is supported with the "attribute[locale]=value" syntax, in accordance
 * with the desktop file spec: http://freedesktop.org/Standards/desktop-entry-spec <BR>
 * Example:
 * @code
 * [Section]
 *   key = value  # definition of Section.key = "value"
 *   name = "quoted string with \n newlines and spaces"
 * @endcode
 */

/* FIXME: possible IniFile improvements
   - support \xUUUU unicode escapes in strings
   - support \s for space (desktop-entry-spec)
   - support value list parsing, using ';' as delimiters
   - support current locale matching, including locale aliases
   - support merging of duplicates
   - support %(var) interpolation like Pyton's configparser.ConfigParser
   - parse into vector<IniEntry> which are: { kind=(section|assignment|other); String text, comment; }
   - support storage, based on vector<IniEntry>
 */

static bool
parse_whitespaces (const char **stringp, int min_spaces)
{
  const char *p = *stringp;
  while (ISASCIIWHITESPACE (*p))
    p++;
  if (p - *stringp >= min_spaces)
    {
      *stringp = p;
      return true;
    }
  return false;
}

static bool
skip_whitespaces (const char **stringp)
{
  return parse_whitespaces (stringp, 0);
}

static inline bool
scan_escaped (const char **stringp, size_t *linenop, const char term)
{
  const char *p = *stringp;
  size_t lineno = *linenop;
  while (*p)
    if (*p == term)
      {
        p++;
        *stringp = p;
        *linenop = lineno;
        return true;
      }
    else if (p[0] == '\\' && p[1])
      p += 2;
    else // *p != 0
      {
        if (*p == '\n')
          lineno++;
        p++;
      }
  return false;
}

static String
string_rtrim (String &str)
{
  const char *s = str.data();
  const char *e = s + str.size();
  while (e > s && ISASCIINLSPACE (e[-1]))
    e--;
  const size_t l = e - s;
  str.erase (str.begin() + l, str.end());
  return String (str.data(), str.size()); // force copy
}

static bool
scan_value (const char **stringp, size_t *linenop, String *valuep, const char *termchars = "")
{ // read up to newline or comment or termchars, support line continuation and quoted strings
  const char *p = *stringp;
  size_t lineno = *linenop;
  String v;
  v.reserve (16);
  for (;; ++p)
    switch (p[0])
      {
        const char *d;
      case '\\':
        if (p[1] == '\n' || (p[1] == '\r' && p[2] == '\n'))
          {
            p += 1 + (p[1] == '\r');
            lineno++;
            break;
          }
        else if (!p[1])
          break; // ignore
        p++;
        v += "\\" + p[0];
        break;
      case '"': case '\'':
        d = p;
        p++;
        if (scan_escaped (&p, &lineno, d[0]))
          {
            v += String (d, p - d);
            p--; // back off to terminating \" or \'
          }
        else
          return false; // brr, unterminated string
        break;
      case '\r':
        if (p[1] && p[1] != '\n')
          v += ' '; // turn stray '\r' into space
        break;
      default:
        if (!strchr (termchars, p[0]))
          {
            v += p[0];
            break;
          }
        // fall through
      case 0: case '\n': case ';': case '#':
        *stringp = p;
        *linenop = lineno;
        *valuep = string_rtrim (v); // forces copy
        return true;
      }
}

static bool
skip_line (const char **stringp, size_t *linenop, String *textp)
{ // ( !'\n' )* '\n'
  const char *p = *stringp, *const start = p;
  size_t lineno = *linenop;
  while (*p && *p != '\n')
    p++;
  if (textp)
    *textp = String (start, p - start);
  if (*p == '\n')
    {
      lineno++;
      p++;
    }
  *stringp = p;
  *linenop = lineno;
  return true;
}

static bool
skip_commentline (const char **stringp, size_t *linenop, String *commentp = NULL)
{ // S* ( '#' | ';' ) ( !'\n' )* '\n'
  const char *p = *stringp;
  skip_whitespaces (&p);
  if (*p != '#' && *p != ';')
    return false;
  p++;
  *stringp = p;
  return skip_line (stringp, linenop, commentp);
}

static bool
skip_to_eol (const char **stringp, size_t *linenop)
{ // comment? '\r'? '\n', or EOF
  const char *p = *stringp;
  if (*p == '#' || *p == ';')
    return skip_commentline (stringp, linenop);
  if (*p == '\r')
    p++;
  if (*p == '\n')
    {
      p++;
      (*linenop) += 1;
      *stringp = p;
      return true;
    }
  if (!*p)
    {
      *stringp = p;
      return true;
    }
  return false;
}

static bool
parse_assignment (const char **stringp, size_t *linenop, String *keyp, String *localep, String *valuep)
{ // S* KEYCHARS+ S* ( '[' S* LOCALECHARS* S* ']' )? S* ( '=' | ':' ) scan_value comment? EOL
  const char *p = *stringp;
  size_t lineno = *linenop;
  String key, locale, value;
  bool success = true;
  success = success && skip_whitespaces (&p);
  success = success && scan_value (&p, &lineno, &key, "[]=:");
  success = success && skip_whitespaces (&p);
  if (success && *p == '[')
    {
      p++;
      success = success && skip_whitespaces (&p);
      success = success && scan_value (&p, &lineno, &locale, "[]");
      success = success && skip_whitespaces (&p);
      if (*p != ']')
        return false;
      p++;
      success = success && skip_whitespaces (&p);
    }
  if (!success)
    return false;
  if (*p != '=' && *p != ':')
    return false;
  p++;
  success = success && skip_whitespaces (&p);
  success = success && scan_value (&p, &lineno, &value);
  success = success && skip_to_eol (&p, &lineno);
  if (!success)
    return false;
  *stringp = p;
  *linenop = lineno;
  *keyp = key;
  *localep = locale;
  *valuep = value;
  return true;
}

static bool
parse_section (const char **stringp, size_t *linenop, String *sectionp)
{ // S* '[' S* SECTIONCHARS+ S* ']' S* comment? EOL
  const char *p = *stringp;
  size_t lineno = *linenop;
  String section;
  bool success = true;
  success = success && skip_whitespaces (&p);
  if (*p != '[')
    return false;
  p++;
  success = success && skip_whitespaces (&p);
  success = success && scan_value (&p, &lineno, &section, "[]");
  success = success && skip_whitespaces (&p);
  if (*p != ']')
    return false;
  p++;
  success = success && skip_whitespaces (&p);
  success = success && skip_to_eol (&p, &lineno);
  if (!success)
    return false;
  *stringp = p;
  *linenop = lineno;
  *sectionp = section;
  return true;
}

void
IniFile::load_ini (const String &inputname, const String &data)
{
  const char *p = data.c_str();
  size_t nextno = 1;
  String section = "";
  while (*p)
    {
      const size_t lineno = nextno;
      String text, key, locale, *debugp = 0 ? &text : NULL; // DEBUG parsing?
      if (skip_commentline (&p, &nextno, debugp))
        {
          if (debugp)
            printerr ("%s:%zd: #%s\n", inputname.c_str(), lineno, debugp->c_str());
        }
      else if (parse_section (&p, &nextno, &text))
        {
          if (debugp)
            printerr ("%s:%zd: %s\n", inputname.c_str(), lineno, text.c_str());
          section = text;
        }
      else if (parse_assignment (&p, &nextno, &key, &locale, &text))
        {
          if (debugp)
            printerr ("%s:%zd:\t%s[%s] = %s\n", inputname.c_str(), lineno, key.c_str(), locale.c_str(), CQUOTE (text.c_str()));
          String k (key);
          if (!locale.empty())
            k += "[" + locale + "]";
          if (strchr (section.c_str(), '=') || strchr (key.c_str(), '.'))
            DEBUG ("%s:%zd: invalid key name: %s.%s", inputname.c_str(), lineno, section.c_str(), k.c_str());
          else
            m_sections[section].push_back (k + "=" + text);
        }
      else if (skip_line (&p, &nextno, debugp))
        {
          if (debugp)
            printerr ("%s:%zd:~ %s\n", inputname.c_str(), lineno, debugp->c_str());
        }
      else
        break; // EOF if !skip_line
    }
}

IniFile::IniFile (const String &res_ini)
{
  String input;
  errno = ENOENT;
  Blob blob = Blob::load (res_ini);
  if (blob)
    input = blob.string();
  else
    {
      size_t length = 0;
      char *fmem = Path::memread (res_ini, &length);
      if (fmem)
        {
          input = String (fmem, length);
          Path::memfree (fmem);
        }
      else
        DEBUG ("IniFile: failed to locate %s: %s", CQUOTE (res_ini), strerror (errno));
    }
  if (input.size())
    load_ini (res_ini, input);
}

IniFile::IniFile (const String &ini_string, int) // FIXME: use Blob construction instead
{
  load_ini ("<string>", ini_string);
}

const StringVector&
IniFile::section (const String &name) const
{
  SectionMap::const_iterator cit = m_sections.find (name);
  if (cit != m_sections.end())
    return cit->second;
  static const StringVector *dummy = NULL;
  do_once {
    static uint64 space[(sizeof (StringVector) + 7) / 8];
    dummy = new (space) StringVector();
  }
  return *dummy;
}

bool
IniFile::has_section (const String &section) const
{
  SectionMap::const_iterator cit = m_sections.find (section);
  return cit != m_sections.end();
}

StringVector
IniFile::sections () const
{
  StringVector secs;
  for (auto it : m_sections)
    secs.push_back (it.first);
  return secs;
}

StringVector
IniFile::attributes (const String &section) const
{
  StringVector opts;
  SectionMap::const_iterator cit = m_sections.find (section);
  if (cit != m_sections.end())
    for (auto s : cit->second)
      opts.push_back (s.substr (0, s.find ('=')));
  return opts;
}

bool
IniFile::has_attribute (const String &section, const String &key) const
{
  SectionMap::const_iterator cit = m_sections.find (section);
  if (cit == m_sections.end())
    return false;
  for (auto s : cit->second)
    if (s.size() > key.size() && s[key.size()] == '=' && memcmp (s.data(), key.data(), key.size()) == 0)
      return true;
  return false;
}

StringVector
IniFile::raw_values () const
{
  StringVector opts;
  for (auto it : m_sections)
    for (auto s : it.second)
      opts.push_back (it.first + "." + s);
  return opts;
}

String
IniFile::raw_value (const String &dotpath) const
{
  const char *p = dotpath.c_str(), *d = strchr (p, '.');
  if (!d)
    return "";
  const String secname = String (p, d - p);
  d++; // point to key
  const StringVector &sv = section (secname);
  if (!sv.size())
    return "";
  const size_t l = dotpath.size() - (d - p); // key length
  for (auto kv : sv)
    if (kv.size() > l && kv[l] == '=' && memcmp (kv.data(), d, l) == 0)
      return kv.substr (l + 1);
  return "";
}

String
IniFile::cook_string (const String &input)
{
  String v;
  size_t dummy = 0;
  for (const char *p = input.c_str(); *p; p++)
    switch (*p)
      {
        const char *start;
      case '\\':
        switch (p[1])
          {
          case 0:       break; // ignore trailing backslash
          case 'n':     v += '\n';      break;
          case 'r':     v += '\r';      break;
          case 't':     v += '\t';      break;
          case 'b':     v += '\b';      break;
          case 'f':     v += '\f';      break;
          case 'v':     v += '\v';      break;
          default:      v += p[1];      break;
          }
        p++;
        break;
      case '"': case '\'':
        start = ++p;
        if (scan_escaped (&p, &dummy, start[-1]))
          {
            p--; // back off to terminating \" or \'
            v += string_from_cquote (String (start, p - start));
            break;
          }
        // fall through
      default:
        v += p[0];
        break;
      }
  return v;
}


String
IniFile::value_as_string (const String &dotpath) const
{
  String raw = raw_value (dotpath);
  String v = cook_string (raw);
  return v;
}

} // Rapicorn
