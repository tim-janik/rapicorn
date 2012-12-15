// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#include "aidaenums.hh"
#include "thread.hh"
#include <cstring>

namespace Rapicorn { namespace Aida {

static Mutex                       enum_info_mutex;
static std::map<String, EnumInfo> *enum_info_map = NULL;

EnumInfo
EnumInfo::from_nsid (const char *enamespace, const char *ename)
{
  ScopedLock<Mutex> locker (enum_info_mutex);
  if (enum_info_map)
    {
      const String fqen = enamespace + String ("::") + ename;
      auto it = enum_info_map->find (fqen);
      if (it != enum_info_map->end())
        return it->second;
    }
  return *(EnumInfo*)0; // shouldn't happen
}

void
EnumInfo::enlist (const EnumInfo &enum_info)
{
  ScopedLock<Mutex> locker (enum_info_mutex);
  if (!enum_info_map)
    {
      static uint64 space[sizeof (*enum_info_map) / sizeof (uint64)];
      enum_info_map = new (space) typeof (*enum_info_map);
    }
  const String fqen = enum_info.enum_namespace() + String ("::") + enum_info.enum_name();
  (*enum_info_map)[fqen] = enum_info; // dups are simply overwritten
  // dups are common if client and server bindings are linked into the same binary
}

EnumInfo&
EnumInfo::operator= (const EnumInfo &src)
{
  n_values_ = src.n_values_;
  values_ = src.values_;
  name_ = src.name_;
  namespace_ = src.namespace_;
  flag_combinable_ = src.flag_combinable_;
  return *this;
}

EnumInfo::EnumInfo () :
  namespace_ (NULL), name_ (NULL), values_ (NULL), n_values_ (0), flag_combinable_ (0)
{}

static bool
c_isalnum (uint8 c)
{
  return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9');
}

static inline char
char_canon (char c)
{
  if (c >= '0' && c <= '9')
    return c;
  else if (c >= 'A' && c <= 'Z')
    return c - 'A' + 'a';
  else if (c >= 'a' && c <= 'z')
    return c;
  else
    return '-';
}

static inline bool
eval_match (const char *str1, const char *str2)
{
  while (*str1 && *str2)
    {
      uint8 s1 = char_canon (*str1++);
      uint8 s2 = char_canon (*str2++);
      if (s1 != s2)
        return false;
    }
  return *str1 == 0 && *str2 == 0;
}

static bool
name_match_detailed (const char *value_name1, size_t name1_length, const char *value_name2, size_t name2_length)
{
  if (name2_length > name1_length)
    {
      const char *ts = value_name2;
      value_name2 = value_name1;
      value_name1 = ts;
      size_t tl = name2_length;
      name2_length = name1_length;
      name1_length = tl;
    }
  const char *name1 = value_name1 + name1_length - min (name1_length, name2_length);
  const char *name2 = value_name2 + name2_length - min (name1_length, name2_length);
  if (name1 > value_name1)      // allow partial matches on word boundary only
    {
      if (c_isalnum (name1[-1]) && c_isalnum (name1[0])) // no word boundary
        return false;
    }
  return name2[0] && eval_match (name1, name2);
}

bool
EnumInfo::match_partial (const String &value_name1, const String &partial_value_name) const
{
  const size_t length1 = value_name1.size();
  const size_t length2 = partial_value_name.size();
  if (length1 < length2)        // length1 must be >= length2 for partial matches
    return false;
  return name_match_detailed (value_name1.data(), length1, partial_value_name.data(), length2);
}

bool
EnumInfo::match (const String &value_name1, const String &value_name2) const
{
  return name_match_detailed (value_name1.data(), value_name1.size(), value_name2.data(), value_name2.size());
}

const EnumInfo::Value*
EnumInfo::find_first (int64 value) const
{
  // value = constrain (value);
  size_t n_values = 0;
  const Value *values;
  list_values (&n_values, &values);
  for (size_t i = 0; i < n_values; i++)
    if (values[i].value == value)
      return &values[i];
  return NULL;
}

const EnumInfo::Value*
EnumInfo::find_first (const String &value_name) const
{
  const size_t length2 = value_name.size();
  const char *const vname = value_name.c_str();
  size_t n_values = 0;
  const Value *values;
  list_values (&n_values, &values);
  for (size_t i = 0; i < n_values; i++)
    {
      const size_t length1 = values[i].length;
      if (length1 < length2)    // length1 must be >= length2 for partial matches
        continue;
      if (name_match_detailed (values[i].name, length1, vname, length2))
        return &values[i];
    }
  return NULL;
}

int64
EnumInfo::parse (const String &values, String *error_string) const
{
  const char *value_string = values.c_str();
  if (!flag_combinable())
    {
      const Value *v = find_first (value_string);
      if (v)
        return v->value;
      if (error_string)
        *error_string = value_string;
      return 0; // constrain (0);
    }
  // parse flags
  const char *sep = strchr (value_string, '|');
  uint64 value = 0;
  while (sep)
    {
      String s (value_string, sep - value_string);
      value |= parse (s.c_str(), error_string);
      value_string = sep + 1;
    }
  const Value *v = find_first (value_string);
  if (v)
    value |= v->value;
  else if (error_string)
    *error_string = sep;
  return value; // constrain (value);
}

String
EnumInfo::string (int64 value) const
{
  // int64 value = constrain (orig_value);
  if (!flag_combinable())
    {
      const Value *v = find_first (value);
      return v ? v->name : "";
    }
  size_t n_values = 0;
  const Value *values;
  list_values (&n_values, &values);
  String s;
  // combine flags
  while (value)
    {
      const Value *match1 = NULL;
      for (size_t i = 0; i < n_values; i++)
        if (values[i].value == (value & values[i].value))
          // choose the greatest match, needed by mixed flags/enum types (StateMask)
          match1 = match1 && match1->value > values[i].value ? match1 : &values[i];
      if (match1)
        {
          if (s[0])
            s = String (match1->name) + "|" + s;
          else
            s = String (match1->name);
          value &= match1->value;
        }
      else
        value = 0; // no match
    }
  if (!s[0] && value == 0)
    for (uint i = 0; i < n_values; i++)
      if (values[i].value == 0)
        {
          s = values[i].name;
          break;
        }
  return s;
}

} } // Rapicorn::Aida
