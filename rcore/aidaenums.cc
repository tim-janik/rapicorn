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
  size_t n_values = 0;
  const Value *values;
  list_values (&n_values, &values);
  for (size_t i = 0; i < n_values; i++)
    {
      const size_t length = values[i].length;
      if (length < value_name.size())    // length must be >= value_name for partial matches
        continue;
      if (string_match_identifier_tail (String (values[i].name, length), value_name))
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
