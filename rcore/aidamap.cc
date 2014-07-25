// CC0 Public Domain: http://creativecommons.org/publicdomain/zero/1.0/
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <malloc.h>
#include <assert.h>

#define __AIDA_return_EFAULT(v)         do { errno = EFAULT; return (v); } while (0)

namespace Rapicorn { namespace Aida {

const char*
type_kind_name (TypeKind type_kind)
{
  switch (type_kind)
    {
    case UNTYPED:         return "UNTYPED";
    case VOID:            return "VOID";
    case BOOL:            return "BOOL";
    case INT32:           return "INT32";
    case INT64:           return "INT64";
    case FLOAT64:         return "FLOAT64";
    case STRING:          return "STRING";
    case ENUM:            return "ENUM";
    case SEQUENCE:        return "SEQUENCE";
    case RECORD:          return "RECORD";
    case INSTANCE:        return "INSTANCE";
    case FUNC:            return "FUNC";
    case TYPE_REFERENCE:  return "TYPE_REFERENCE";
    case ANY:             return "ANY";
    case LOCAL:           return "LOCAL";
    case REMOTE:          return "REMOTE";
    default:              return NULL;
    }
}

// == AidaTypeMap classes ==
struct InternalList {
  uint32_t      length;         // in members
  uint32_t      items[];        // flexible array member
};
struct InternalString {
  uint32_t      length;         // in bytes
  char          chars[];        // flexible array member
};
struct TypeCode::InternalType {
  uint32_t      tkind;          // type kind
  uint32_t      name;           // string index
  uint32_t      aux_strings;    // index of list<string index>; assignment strings
  uint32_t      custom;         // custom index, used as follows:
  // enum:      index of list<string index>; 3 strings per value (list length is 3 * enum_count())
  // interface: index of list<string index>; prerequisite names
  // record:    index of list<type index>; InternalType per field
  // sequence:  type index; InternalType for sequence field
  // reference: string index; name of referenced type
};
typedef TypeCode::InternalType InternalType;
struct InternalMap {
  uint32_t      magic[4];       // "AidaTypeMap\0\0\0\0\0"
  uint32_t      length;         // typemap length (excludes tail)
  uint32_t      pad0, pad1, pad2;
  uint32_t      seg_types;      // type code segment offset
  uint32_t      seg_lists;      // list segment offset
  uint32_t      seg_strings;    // string segment offset
  uint32_t      pad3;
  uint32_t      types;          // index of list<type index>; public types
  uint32_t      pad4, pad5, pad6;
  bool          check_tail      () { return 0x00000000 == *(uint32_t*) (((char*) this) + length); }
  bool          check_magic     () { return (magic[0] == 0x61646941 && magic[1] == 0x65707954 &&
                                             magic[2] == 0x0070614d && magic[3] == 0x00000000); }
  bool          check_lengths   (size_t memlength)
  {
    return (memlength > sizeof (*this) && length + 4 <= memlength &&
            seg_types >= sizeof (*this) && seg_lists >= seg_types && seg_strings >= seg_lists &&
            length >= seg_strings);
  }
  InternalList*   internal_list    (uint32_t offset) const;
  InternalType*   internal_type    (uint32_t offset) const;
  InternalString* internal_string  (uint32_t offset) const;
  static uint64_t internal_big_int (uint32_t low, uint32_t high) { return low + (uint64_t (high) << 32); }
};
static const char zero_type_or_map[MAX (sizeof (InternalMap), sizeof (InternalType))] = { 0, };
struct TypeCode::MapHandle {
  const InternalMap *const imap;
private:
  volatile uint32_t ref_count_;
  const size_t      length_;
  const bool        needs_free_;
  int               status_;
  Spinlock          mutex_;
  std::map<int32,int> cache_;
public:
  int             status           ()                { return status_; }
  InternalList*   internal_list    (uint32_t offset) { return imap->internal_list (offset); }
  InternalType*   internal_type    (uint32_t offset) { return imap->internal_type (offset); }
  InternalString* internal_string  (uint32_t offset) { return imap->internal_string (offset); }
  uint64_t        internal_big_int (uint32_t low, uint32_t high) { return imap->internal_big_int (low, high); }
  std::string
  simple_string (uint32_t offset)
  {
    InternalString *is = internal_string (offset);
    return AIDA_LIKELY (is) ? std::string (is->chars, is->length) : "";
  }
  const char*
  simple_cstring (uint32_t offset)
  {
    InternalString *is = internal_string (offset);
    if (AIDA_UNLIKELY (!is || !is->chars || (is->length && is->chars[is->length] != 0)))
      return NULL;
    return is->length ? is->chars : "";
  }
  MapHandle*
  ref()
  {
    __sync_fetch_and_add (&ref_count_, +1);
    return this;
  }
  void
  unref()
  {
    uint32_t o = __sync_fetch_and_add (&ref_count_, -1);
    if (o -1 == 0)
      delete this;
  }
  ~MapHandle()
  {
    if (needs_free_)
      free (const_cast<InternalMap*> (imap));
    memset (this, 0, sizeof (*this));
  }
  static TypeMap
  create_type_map (void *addr, size_t length, bool needs_free)
  {
    MapHandle *handle = new MapHandle (addr, length, needs_free);
    assert (handle->ref_count_ == 0);
    TypeMap type_map (handle);
    assert (handle->ref_count_ == 1);
    return type_map;
  }
  static TypeMap
  create_error_type_map (int _status)
  {
    MapHandle *handle = new MapHandle (const_cast<char*> (zero_type_or_map), 0, false);
    assert (handle->ref_count_ == 0);
    handle->status_ = _status;
    TypeMap type_map (handle);
    assert (handle->ref_count_ == 1);
    return type_map;
  }
  TypeCode
  lookup_local (std::string name)
  {
    InternalList *il = internal_list (imap->types);
    const size_t clen = name.size();
    const char* cname = name.data(); // not 0-terminated
    if (AIDA_LIKELY (il))
      for (size_t i = 0; AIDA_LIKELY (i < il->length); i++)
        {
          InternalType *it = internal_type (il->items[i]);
          InternalString *is = AIDA_LIKELY (it) ? internal_string (it->name) : NULL;
          if (AIDA_UNLIKELY (is && clen == is->length && strncmp (cname, is->chars, clen) == 0))
            return TypeCode (this, it);
        }
    return TypeCode::notype (this);
  }
private:
  MapHandle (void *addr, size_t length, bool needs_free) :
    imap ((InternalMap*) addr), ref_count_ (0), length_ (length),
    needs_free_ (needs_free), status_ (0)
  {}
  explicit MapHandle (const MapHandle&);
  void     operator= (const MapHandle&);
};

InternalType*
InternalMap::internal_type (uint32_t offset) const
{
  if (AIDA_UNLIKELY (offset & 0x3 || offset < seg_types || offset + sizeof (InternalType) > seg_lists))
    __AIDA_return_EFAULT (NULL);
  return (InternalType*) (((char*) this) + offset);
}

InternalList*
InternalMap::internal_list (uint32_t offset) const
{
  if (AIDA_UNLIKELY (offset & 0x3 || offset < seg_lists || offset + sizeof (InternalList) > seg_strings))
    __AIDA_return_EFAULT (NULL);
  InternalList *il = (InternalList*) (((char*) this) + offset);
  if (AIDA_UNLIKELY (il->length > seg_strings || offset + sizeof (*il) + il->length * 4 > seg_strings))
    __AIDA_return_EFAULT (NULL);
  return il;
}

InternalString*
InternalMap::internal_string (uint32_t offset) const
{
  if (AIDA_UNLIKELY (offset & 0x3 || offset < seg_strings || offset + sizeof (InternalString) > length))
    __AIDA_return_EFAULT (NULL);
  InternalString *is = (InternalString*) (((char*) this) + offset);
  if (AIDA_UNLIKELY (offset + sizeof (*is) + is->length > length))
    __AIDA_return_EFAULT (NULL);
  return is;
}

TypeMap::TypeMap (TypeCode::MapHandle *handle) :
  handle_ (handle->ref())
{}

TypeMap::TypeMap (const TypeMap &src) :
  handle_ (src.handle_->ref())
{}

TypeMap&
TypeMap::operator= (const TypeMap &src)
{
  TypeCode::MapHandle *old = handle_;
  handle_ = src.handle_->ref();
  old->unref();
  return *this;
}

TypeMap::~TypeMap ()
{
  handle_->unref();
  handle_ = NULL;
}

int
TypeMap::error_status ()
{
  return handle_->status();
}

size_t
TypeMap::type_count () const
{
  InternalList *il = handle_->internal_list (handle_->imap->types);
  return il ? il->length : 0;
}

const TypeCode
TypeMap::type (size_t index) const
{
  InternalList *il = handle_->internal_list (handle_->imap->types);
  if (il && index < il->length)
    {
      Aida::InternalType *it = handle_->internal_type (il->items[index]);
      if (it)
        return TypeCode (handle_, it);
    }
  __AIDA_return_EFAULT (TypeCode::notype (handle_));
}

struct TypeRegistry {
  typedef std::vector<TypeMap> TypeMapVector;
  explicit  TypeRegistry() : builtins (load_builtins()) { pthread_spin_init (&typemap_spinlock, 0 /* pshared */); }
  /*dtor*/ ~TypeRegistry()   { pthread_spin_destroy (&typemap_spinlock); assert_unreached(); }
  size_t    size()           { lock(); size_t sz = typemaps.size(); unlock(); return sz; }
  TypeMap   nth (size_t n)   { lock(); TypeMap tp = n < typemaps.size() ? typemaps[n] : builtins; unlock(); return tp; }
  void      add (TypeMap tp) { lock(); typemaps.push_back (tp); unlock(); }
  TypeMap   standard()       { return builtins; }
private:
  void      lock()           { pthread_spin_lock (&typemap_spinlock); }
  void      unlock()         { pthread_spin_unlock (&typemap_spinlock); }
  static TypeMap     load_builtins ();
  pthread_spinlock_t typemap_spinlock;
  TypeMapVector      typemaps;
  TypeMap            builtins;
};
static TypeRegistry *type_registry = NULL;

static inline void
type_registry_initialize()
{
  if (AIDA_UNLIKELY (!type_registry))
    {
      do_once {
        type_registry = new TypeRegistry();
      }
    }
}

TypeCode
TypeMap::lookup (std::string name)
{
  type_registry_initialize();
  size_t sz = type_registry->size();
  for (size_t i = 0; i < sz; i++)
    {
      TypeMap tp = type_registry->nth (i);
      if (AIDA_UNLIKELY (tp.error_status()))
        break;
      TypeCode tc = tp.lookup_local (name);
      if (AIDA_UNLIKELY (!tc.untyped()))
        return tc;
    }
  return type_registry->standard().lookup_local (name);
}

TypeCode
TypeMap::lookup_local (std::string name) const
{
  return handle_->lookup_local (name);
}

TypeCode
TypeMap::notype ()
{
  type_registry_initialize();
  return TypeCode (type_registry->standard().handle_, (InternalType*) zero_type_or_map);
}

TypeCode::TypeCode (MapHandle *handle, InternalType *itype) :
  handle_ (handle->ref()), type_ (itype)
{}

TypeCode::TypeCode (const TypeCode &clone) :
  handle_ (clone.handle_->ref()), type_ (clone.type_)
{}

TypeCode&
TypeCode::operator= (const TypeCode &clone)
{
  handle_->unref();
  handle_ = clone.handle_->ref();
  type_ = clone.type_;
  return *this;
}

TypeCode::~TypeCode ()
{
  type_ = NULL;
  handle_->unref();
  handle_ = NULL;
}

void
TypeCode::swap (TypeCode &other)
{
  MapHandle *b_handle = other.handle_;
  InternalType *b_type = other.type_;
  other.handle_ = handle_;
  other.type_ = type_;
  handle_ = b_handle;
  type_ = b_type;
}

TypeCode
TypeCode::notype (MapHandle *handle)
{
  return TypeCode (handle, (InternalType*) zero_type_or_map);
}

bool
TypeCode::untyped () const
{
  return handle_ == NULL || type_ == NULL || kind() == UNTYPED;
}

bool
TypeCode::operator!= (const TypeCode &o) const
{
  return !operator== (o);
}

TypeKind
TypeCode::kind () const
{
  if (type_ && name() == "AidaLocal")   // FIXME: bad hack for AidaLocal
    return LOCAL;
  if (type_ && name() == "AidaRemote")   // FIXME: bad hack for AidaRemote
    return REMOTE;
  return AIDA_LIKELY (type_) ? TypeKind (type_->tkind) : UNTYPED;
}

std::string
TypeCode::kind_name () const
{
  return type_kind_name (kind());
}

std::string
TypeCode::name () const
{
  return AIDA_LIKELY (handle_) ? handle_->simple_string (type_->name) : "<broken>";
}

size_t
TypeCode::aux_count () const
{
  if (AIDA_UNLIKELY (!handle_) || AIDA_UNLIKELY (!type_))
    return 0;
  InternalList *il = handle_->internal_list (type_->aux_strings);
  return il ? il->length : 0;
}

std::string
TypeCode::aux_data (size_t index) const // name=utf8data string
{
  InternalList *il = handle_->internal_list (type_->aux_strings);
  if (!il || index > il->length)
    __AIDA_return_EFAULT ("");
  return handle_->simple_string (il->items[index]);
}

std::string
TypeCode::aux_value (std::string key) const // utf8data string
{
  const size_t clen = key.size();
  const char* cname = key.data(); // not 0-terminated
  InternalList *il = handle_->internal_list (type_->aux_strings);
  if (il)
    for (size_t i = 0; i < il->length; i++)
      {
        InternalString *is = handle_->internal_string (il->items[i]);
        if (is && clen < is->length && is->chars[clen] == '=' && strncmp (cname, is->chars, clen) == 0)
          return std::string (is->chars + clen + 1, is->length - clen - 1);
      }
  return "";
}

std::string
TypeCode::hints () const
{
  std::string str = aux_value ("hints");
  if (str.size() == 0 || str[0] != ':')
    str = ":" + str;
  if (str[str.size() - 1] != ':')
    str = str + ":";
  return str;
}

size_t
TypeCode::enum_count () const
{
  if (kind() != ENUM)
    return 0;
  InternalList *il = handle_->internal_list (type_->custom);
  return il && il->length % 5 == 0 ? il->length / 5 : 0;
}

EnumValue
TypeCode::enum_value (const size_t index) const
{
  EnumValue ev;
  if (kind() != ENUM)
    return ev;
  InternalList *il = handle_->internal_list (type_->custom);
  if (!il || il->length == 0 || il->length % 5 != 0 || index >= il->length / 5)
    __AIDA_return_EFAULT (ev);
  ev.value = handle_->internal_big_int (il->items[index * 5 + 0], il->items[index * 5 + 1]);
  ev.ident = handle_->simple_cstring (il->items[index * 5 + 2]);
  ev.label = handle_->simple_cstring (il->items[index * 5 + 3]);
  ev.blurb = handle_->simple_cstring (il->items[index * 5 + 4]);
  return ev;
}

EnumValue
TypeCode::enum_find (int64 value) const
{
  const EnumValue ev;
  if (kind() != ENUM)
    return ev;
  InternalList *il = handle_->internal_list (type_->custom);
  if (!il || il->length == 0 || il->length % 5 != 0)
    __AIDA_return_EFAULT (ev);
  for (size_t i = 0; i < il->length / 5; i++)
    {
      const int64 evalue = handle_->internal_big_int (il->items[i * 5 + 0], il->items[i * 5 + 1]);
      if (value == evalue)
        return enum_value (i);
    }
  return ev;
}

EnumValue
TypeCode::enum_find (const String &name) const
{
  const EnumValue ev;
  if (kind() != ENUM)
    return ev;
  InternalList *il = handle_->internal_list (type_->custom);
  if (!il || il->length == 0 || il->length % 5 != 0)
    __AIDA_return_EFAULT (ev);
  for (size_t i = 0; i < il->length / 5; i++)
    {
      const char *ident = handle_->simple_cstring (il->items[i * 5 + 2]);
      if (ident && string_match_identifier_tail (ident, name))
        return enum_value (i);
    }
  return ev;
}

bool
TypeCode::enum_combinable () const
{
  return string_to_bool (aux_value ("enum_combinable"));
}

String
TypeCode::enum_string (int64 mask) const
{
  if (kind() != ENUM)
    return "";
  if (!enum_combinable())
    {
      const EnumValue ev = enum_find (mask);
      return ev.ident ? ev.ident : "";
    }
  const size_t n_evalues = enum_count();
  EnumValue evalues[n_evalues];
  for (size_t i = 0; i < n_evalues; i++)
    evalues[i] = enum_value (i);
  String s;
  // combine flags
  while (mask)
    {
      const EnumValue *match1 = NULL;
      for (size_t i = 0; i < n_evalues; i++)
        if (evalues[i].value && evalues[i].value == (mask & evalues[i].value))
          // choose the greatest match, needed by mixed flags/enum types (StateMask)
          match1 = match1 && match1->value > evalues[i].value ? match1 : &evalues[i];
      if (match1)
        {
          if (s[0])
            s = s + "|" + String (match1->ident);
          else
            s = String (match1->ident);
          mask &= ~match1->value;
        }
      else
        mask = 0; // no match
    }
  if (!s[0] && mask == 0)
    for (uint i = 0; i < n_evalues; i++)
      if (evalues[i].value == 0)
        {
          s = evalues[i].ident;
          break;
        }
  return s;
}

int64
TypeCode::enum_parse (const String &value_string, String *errorp) const
{
  if (!enum_combinable())
    {
      const EnumValue ev = enum_find (value_string);
      if (ev.ident)
        return ev.value;
      if (errorp)
        *errorp = value_string;
      return 0;
    }
  // parse flags
  uint64 result = 0;
  const char *cstring = value_string.c_str();
  const char *sep = strchr (cstring, '|');
  while (sep)
    {
      String token (cstring, sep - cstring);
      result |= enum_parse (token, errorp);
      cstring = sep + 1; // reminder
      sep = strchr (cstring, '|');
    }
  const EnumValue ev = enum_find (cstring);
  if (ev.ident)
    result |= ev.value;
  else if (errorp)
    *errorp = sep;
  return result;
}

size_t
TypeCode::prerequisite_count () const
{
  if (kind() != INSTANCE)
    return 0;
  InternalList *il = handle_->internal_list (type_->custom);
  return il ? il->length : 0;
}

std::string
TypeCode::prerequisite (size_t index) const
{
  std::string s;
  if (kind() != INSTANCE)
    return s;
  InternalList *il = handle_->internal_list (type_->custom);
  if (!il || index > il->length)
    __AIDA_return_EFAULT (s);
  return handle_->simple_string (il->items[index]);
}

size_t
TypeCode::field_count () const
{
  TypeKind k = kind();
  if (k == SEQUENCE)
    return 1;
  if (k != RECORD)
    return 0;
  InternalList *il = handle_->internal_list (type_->custom);
  return il ? il->length : 0;
}

TypeCode
TypeCode::field (size_t index) const // RECORD or SEQUENCE
{
  TypeKind k = kind();
  uint32_t field_offset;
  if (k == SEQUENCE && index == 0)
    field_offset = type_->custom;
  else if (k == RECORD)
    {
      InternalList *il = handle_->internal_list (type_->custom);
      if (!il || index > il->length)
        __AIDA_return_EFAULT (TypeCode::notype (handle_));
      field_offset = il->items[index];
    }
  else
    return TypeCode::notype (handle_);
  InternalType *it = handle_->internal_type (field_offset);
  return it ? TypeCode (handle_, it) : TypeCode::notype (handle_);
}

std::string
TypeCode::origin () const // type name for TYPE_REFERENCE
{
  std::string s;
  if (kind() != TYPE_REFERENCE)
    return s;
  return handle_->simple_string (type_->custom);
}

TypeCode
TypeCode::resolve () const // type for TYPE_REFERENCE
{
  TypeCode tc = *this;
  while (tc.kind() == TYPE_REFERENCE)
    {
      TypeCode rc = handle_->lookup_local (tc.origin());
      if (rc.kind() == UNTYPED) // local lookup failed
        tc = TypeMap::lookup (tc.origin());
      else
        tc = rc;
    }
  return tc;
}

bool
TypeCode::operator== (const TypeCode &o) const
{
  return o.handle_ == handle_ && o.type_ == type_;
}

std::string
TypeCode::pretty (const std::string &indent) const
{
  std::string s = indent;
  s += name();
  s += std::string (" (") + type_kind_name (kind()) + ")";
  switch (kind())
    {
      char buffer[1024];
    case ENUM:
      snprintf (buffer, sizeof (buffer), "%zu", enum_count());
      s += std::string (": ") + buffer;
      for (uint32_t i = 0; i < enum_count(); i++)
        {
          EnumValue ev = enum_value (i);
          s += std::string ("\n") + indent + indent + string_format ("0x%08llx", ev.value);
          s += String() + ", " + ev.ident + ", " + ev.label + ", " + ev.blurb;
        }
      break;
    case INSTANCE:
      snprintf (buffer, sizeof (buffer), "%zu", prerequisite_count());
      s += std::string (": ") + buffer;
      for (uint32_t i = 0; i < prerequisite_count(); i++)
        {
          std::string sp = prerequisite (i);
          s += std::string ("\n") + indent + indent + sp;
        }
      break;
    case RECORD: case SEQUENCE:
      snprintf (buffer, sizeof (buffer), "%zu", field_count());
      s += std::string (": ") + buffer;
      for (uint32_t i = 0; i < field_count(); i++)
        {
          const TypeCode memb = field (i);
          s += std::string ("\n") + memb.pretty (indent + indent);
        }
      break;
    case TYPE_REFERENCE:
      s += std::string (": ") + origin();
      break;
    default: ;
    }
  if (aux_count())
    {
      s += std::string (" [");
      for (uint32_t i = 0; i < aux_count(); i++)
        {
          std::string aux = aux_data (i);
          s += std::string ("\n") + indent + indent + aux;
          // verify aux_value
          const char *eq = strchr (aux.c_str(), '=');
          if (eq)
            {
              const size_t eqp = eq - aux.data();
              const std::string key = aux.substr (0, eqp);
              std::string val = aux_value (key);
              if (val != aux.substr (eqp + 1))
                {
                  errno = ENXIO;
                  perror (std::string ("aux_value for type: " + name() + ": " + key).c_str()), abort();
                }
            }
        }
      s += "]";
    }
  return s;
}

static char* /* return malloc()-ed buffer containing a full read of FILE */
file_memread (FILE   *stream,
              size_t *lengthp)
{
  size_t sz = 256;
  char *malloc_string = (char*) malloc (sz);
  if (!malloc_string)
    return NULL;
  char *start = malloc_string;
  errno = 0;
  while (!feof (stream))
    {
      size_t bytes = fread (start, 1, sz - (start - malloc_string), stream);
      if (bytes <= 0 && ferror (stream) && errno != EAGAIN)
        {
          start = malloc_string; // error/0-data
          break;
        }
      start += bytes;
      if (start == malloc_string + sz)
        {
          bytes = start - malloc_string;
          sz *= 2;
          char *newstring = (char*) realloc (malloc_string, sz);
          if (!newstring)
            {
              start = malloc_string; // error/0-data
              break;
            }
          malloc_string = newstring;
          start = malloc_string + bytes;
        }
    }
  int savederr = errno;
  *lengthp = start - malloc_string;
  if (!*lengthp)
    {
      free (malloc_string);
      malloc_string = NULL;
    }
  errno = savederr;
  return malloc_string;
}

TypeMap
TypeMap::load (std::string filename)
{
  TypeMap tp = TypeMap::load_local (filename);
  if (!tp.error_status())
    {
      type_registry_initialize();
      type_registry->add (tp);
      errno = 0;
    }
  return tp;
}

TypeMap
TypeRegistry::load_builtins ()
{
  assert (type_registry == NULL);
  TypeMap tp = TypeMap::load_local ("///..builtins..");
  assert (tp.error_status() == 0);
  return tp;
}

TypeMap
TypeMap::load_local (std::string filename)
{
  if (!type_registry && filename == "///..builtins..")
    return builtins();
  FILE *file = fopen (filename.c_str(), "r");
  if (!file)
    {
      errno = errno ? errno : ENOMEM;
      return TypeCode::MapHandle::create_error_type_map (errno);
    }
  size_t length = 0;
  char *contents = file_memread (file, &length);
  int savederr = errno;
  fclose (file);
  errno = savederr;
  InternalMap *imap = (InternalMap*) contents;
  if (!contents || length < sizeof (*imap) + 4)
    {
      if (contents)
        free (contents);
      errno = ENODATA;
      return TypeCode::MapHandle::create_error_type_map (errno);
    }
  if (!imap->check_magic() || !imap->check_lengths (length) || !imap->check_tail())
    {
      free (contents);
      errno = ELIBBAD;
      return TypeCode::MapHandle::create_error_type_map (errno);
    }
  TypeMap type_map = TypeCode::MapHandle::create_type_map (imap, length, true);
  errno = 0;
  return type_map;
}

TypeMap
TypeMap::enlist_map (const size_t length, const char *static_type_map, bool global)
{
  assert (length >= sizeof (InternalMap) + 4);
  assert (static_type_map != NULL);
  assert (static_type_map[length - 1] == 0);
  InternalMap *imap = (InternalMap*) static_type_map;
  const bool valid_type_map_magics = imap->check_magic() && imap->check_lengths (length) && imap->check_tail();
  assert (valid_type_map_magics == true);
  TypeMap type_map = TypeCode::MapHandle::create_type_map (imap, length, false);
  if (!type_map.error_status() && global)
    {
      type_registry_initialize();
      type_registry->add (type_map);
    }
  errno = 0;
  return type_map;
}

#include "aidabuiltins.cc" // defines intern_builtins_typ

TypeMap
TypeMap::builtins()
{
  return enlist_map (sizeof (intern_builtins_typ), intern_builtins_typ, false);
}

} } // Rapicorn::Aida
