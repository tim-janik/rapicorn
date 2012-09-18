// CC0 Public Domain: http://creativecommons.org/publicdomain/zero/1.0/
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <malloc.h>
#include <assert.h>

#define MAX(a, b)                       (((a) >= (b)) ? (a) : (b))
#define __PLIC_return_EFAULT(v)         do { errno = EFAULT; return (v); } while (0)

namespace Aida {

const char*
type_kind_name (TypeKind type_kind)
{
  switch (type_kind)
    {
    case UNTYPED:         return "UNTYPED";
    case VOID:            return "VOID";
    case INT:             return "INT";
    case FLOAT:           return "FLOAT";
    case STRING:          return "STRING";
    case ENUM:            return "ENUM";
    case SEQUENCE:        return "SEQUENCE";
    case RECORD:          return "RECORD";
    case INSTANCE:        return "INSTANCE";
    case FUNC:            return "FUNC";
    case TYPE_REFERENCE:  return "TYPE_REFERENCE";
    case ANY:             return "ANY";
    default:              return NULL;
    }
}

// == PlicTypeMap classes ==
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
  uint32_t      aux_strings;    // list[string index] index; assignment strings
  uint32_t      custom;         // custom index
  // enum:      list[string index] index; 3 strings per value
  // interface: list[string index] index; prerequisite names
  // record:    list[type index] index; InternalType per field
  // sequence:  type index; InternalType for sequence field
  // reference: string index; name of referenced type
};
typedef TypeCode::InternalType InternalType;
struct InternalMap {
  uint32_t      magic[4];       // "PlicTypeMap\0\0\0\0\0"
  uint32_t      length;         // typemap length (excludes tail)
  uint32_t      pad0, pad1, pad2;
  uint32_t      seg_types;      // type code segment offset
  uint32_t      seg_lists;      // list segment offset
  uint32_t      seg_strings;    // string segment offset
  uint32_t      pad3;
  uint32_t      types;          // list[type index] index; public types
  uint32_t      pad4, pad5, pad6;
  bool          check_tail      () { return 0x00000000 == *(uint32_t*) (((char*) this) + length); }
  bool          check_magic     () { return (magic[0] == 0x63696c50 && magic[1] == 0x65707954 &&
                                             magic[2] == 0x0070614d && magic[3] == 0x00000000); }
  bool          check_lengths   (size_t memlength)
  {
    return (memlength > sizeof (*this) && length + 4 <= memlength &&
            seg_types >= sizeof (*this) && seg_lists >= seg_types && seg_strings >= seg_lists &&
            length >= seg_strings);
  }
  InternalList*   internal_list   (uint32_t offset) const;
  InternalType*   internal_type   (uint32_t offset) const;
  InternalString* internal_string (uint32_t offset) const;
};
static const char zero_type_or_map[MAX (sizeof (InternalMap), sizeof (InternalType))] = { 0, };
struct TypeCode::MapHandle {
  const InternalMap *const imap;
private:
  volatile uint32_t m_ref_count;
  const size_t      m_length;
  const bool        m_needs_free;
  int               m_status;
public:
  int             status          ()                { return m_status; }
  InternalList*   internal_list   (uint32_t offset) { return imap->internal_list (offset); }
  InternalType*   internal_type   (uint32_t offset) { return imap->internal_type (offset); }
  InternalString* internal_string (uint32_t offset) { return imap->internal_string (offset); }
  std::string
  simple_string (uint32_t offset)
  {
    InternalString *is = internal_string (offset);
    return PLIC_LIKELY (is) ? std::string (is->chars, is->length) : "";
  }
  MapHandle*
  ref()
  {
    __sync_fetch_and_add (&m_ref_count, +1);
    return this;
  }
  void
  unref()
  {
    uint32_t o = __sync_fetch_and_add (&m_ref_count, -1);
    if (o -1 == 0)
      delete this;
  }
  ~MapHandle()
  {
    if (m_needs_free)
      free (const_cast<InternalMap*> (imap));
    memset (this, 0, sizeof (*this));
  }
  static TypeMap
  create_type_map (void *addr, size_t length, bool needs_free)
  {
    MapHandle *handle = new MapHandle (addr, length, needs_free);
    assert (handle->m_ref_count == 0);
    TypeMap type_map (handle);
    assert (handle->m_ref_count == 1);
    return type_map;
  }
  static TypeMap
  create_error_type_map (int _status)
  {
    MapHandle *handle = new MapHandle (const_cast<char*> (zero_type_or_map), 0, false);
    assert (handle->m_ref_count == 0);
    handle->m_status = _status;
    TypeMap type_map (handle);
    assert (handle->m_ref_count == 1);
    return type_map;
  }
private:
  MapHandle (void *addr, size_t length, bool needs_free) :
    imap ((InternalMap*) addr), m_ref_count (0), m_length (length),
    m_needs_free (needs_free), m_status (0)
  {}
  explicit MapHandle (const MapHandle&);
  void     operator= (const MapHandle&);
};

InternalType*
InternalMap::internal_type (uint32_t offset) const
{
  if (PLIC_UNLIKELY (offset & 0x3 || offset < seg_types || offset + sizeof (InternalType) > seg_lists))
    __PLIC_return_EFAULT (NULL);
  return (InternalType*) (((char*) this) + offset);
}

InternalList*
InternalMap::internal_list (uint32_t offset) const
{
  if (PLIC_UNLIKELY (offset & 0x3 || offset < seg_lists || offset + sizeof (InternalList) > seg_strings))
    __PLIC_return_EFAULT (NULL);
  InternalList *il = (InternalList*) (((char*) this) + offset);
  if (PLIC_UNLIKELY (offset + sizeof (*il) + il->length * 4 > seg_strings))
    __PLIC_return_EFAULT (NULL);
  return il;
}

InternalString*
InternalMap::internal_string (uint32_t offset) const
{
  if (PLIC_UNLIKELY (offset & 0x3 || offset < seg_strings || offset + sizeof (InternalString) > length))
    __PLIC_return_EFAULT (NULL);
  InternalString *is = (InternalString*) (((char*) this) + offset);
  if (PLIC_UNLIKELY (offset + sizeof (*is) + is->length > length))
    __PLIC_return_EFAULT (NULL);
  return is;
}

TypeMap::TypeMap (TypeCode::MapHandle *handle) :
  m_handle (handle->ref())
{}

TypeMap::TypeMap (const TypeMap &src) :
  m_handle (src.m_handle->ref())
{}

TypeMap&
TypeMap::operator= (const TypeMap &src)
{
  TypeCode::MapHandle *old = m_handle;
  m_handle = src.m_handle->ref();
  old->unref();
  return *this;
}

TypeMap::~TypeMap ()
{
  m_handle->unref();
  m_handle = NULL;
}

int
TypeMap::error_status ()
{
  return m_handle->status();
}

size_t
TypeMap::type_count () const
{
  InternalList *il = m_handle->internal_list (m_handle->imap->types);
  return il ? il->length : 0;
}

const TypeCode
TypeMap::type (size_t index) const
{
  InternalList *il = m_handle->internal_list (m_handle->imap->types);
  if (il && index < il->length)
    {
      Aida::InternalType *it = m_handle->internal_type (il->items[index]);
      if (it)
        return TypeCode (m_handle, it);
    }
  __PLIC_return_EFAULT (TypeCode::notype (m_handle));
}

struct TypeRegistry {
  typedef std::vector<TypeMap> TypeMapVector;
  explicit  TypeRegistry() : builtins (load_builtins()) { pthread_spin_init (&typemap_spinlock, 0 /* pshared */); }
  /*dtro*/ ~TypeRegistry()   { pthread_spin_destroy (&typemap_spinlock); }
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
  if (PLIC_UNLIKELY (!type_registry))
    {
      TypeRegistry *tr = new TypeRegistry();
      __sync_synchronize();
      if (!__sync_bool_compare_and_swap (&type_registry, NULL, tr))
        delete tr;
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
      if (PLIC_UNLIKELY (tp.error_status()))
        break;
      TypeCode tc = tp.lookup_local (name);
      if (PLIC_UNLIKELY (!tc.untyped()))
        return tc;
    }
  return type_registry->standard().lookup_local (name);
}

TypeCode
TypeMap::lookup_local (std::string name) const
{
  InternalList *il = m_handle->internal_list (m_handle->imap->types);
  const size_t clen = name.size();
  const char* cname = name.data(); // not 0-terminated
  if (PLIC_LIKELY (il))
    for (size_t i = 0; PLIC_LIKELY (i < il->length); i++)
      {
        InternalType *it = m_handle->internal_type (il->items[i]);
        InternalString *is = PLIC_LIKELY (it) ? m_handle->internal_string (it->name) : NULL;
        if (PLIC_UNLIKELY (is && clen == is->length && strncmp (cname, is->chars, clen) == 0))
          return TypeCode (m_handle, it);
      }
  return TypeCode::notype (m_handle);
}

TypeCode
TypeMap::notype ()
{
  type_registry_initialize();
  return TypeCode (type_registry->standard().m_handle, (InternalType*) zero_type_or_map);
}

TypeCode::TypeCode (MapHandle *handle, InternalType *itype) :
  m_handle (handle->ref()), m_type (itype)
{}

TypeCode::TypeCode (const TypeCode &clone) :
  m_handle (clone.m_handle->ref()), m_type (clone.m_type)
{}

TypeCode&
TypeCode::operator= (const TypeCode &clone)
{
  m_handle->unref();
  m_handle = clone.m_handle->ref();
  m_type = clone.m_type;
  return *this;
}

TypeCode::~TypeCode ()
{
  m_type = NULL;
  m_handle->unref();
  m_handle = NULL;
}

void
TypeCode::swap (TypeCode &other)
{
  MapHandle *b_handle = other.m_handle;
  InternalType *b_type = other.m_type;
  other.m_handle = m_handle;
  other.m_type = m_type;
  m_handle = b_handle;
  m_type = b_type;
}

TypeCode
TypeCode::notype (MapHandle *handle)
{
  return TypeCode (handle, (InternalType*) zero_type_or_map);
}

bool
TypeCode::untyped () const
{
  return m_handle == NULL || m_type == NULL || kind() == UNTYPED;
}

bool
TypeCode::operator!= (const TypeCode &o) const
{
  return !operator== (o);
}

TypeKind
TypeCode::kind () const
{
  return PLIC_LIKELY (m_type) ? TypeKind (m_type->tkind) : UNTYPED;
}

std::string
TypeCode::kind_name () const
{
  return type_kind_name (kind());
}

std::string
TypeCode::name () const
{
  return PLIC_LIKELY (m_handle) ? m_handle->simple_string (m_type->name) : "<broken>";
}

size_t
TypeCode::aux_count () const
{
  if (PLIC_UNLIKELY (!m_handle) || PLIC_UNLIKELY (!m_type))
    return 0;
  InternalList *il = m_handle->internal_list (m_type->aux_strings);
  return il ? il->length : 0;
}

std::string
TypeCode::aux_data (size_t index) const // name=utf8data string
{
  InternalList *il = m_handle->internal_list (m_type->aux_strings);
  if (!il || index > il->length)
    __PLIC_return_EFAULT ("");
  return m_handle->simple_string (il->items[index]);
}

std::string
TypeCode::aux_value (std::string key) const // utf8data string
{
  const size_t clen = key.size();
  const char* cname = key.data(); // not 0-terminated
  InternalList *il = m_handle->internal_list (m_type->aux_strings);
  if (il)
    for (size_t i = 0; i < il->length; i++)
      {
        InternalString *is = m_handle->internal_string (il->items[i]);
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
  InternalList *il = m_handle->internal_list (m_type->custom);
  return il ? il->length / 3 : 0;
}

std::vector<std::string>
TypeCode::enum_value (size_t index) const // (ident,label,blurb) choic
{
  std::vector<String> sv;
  if (kind() != ENUM)
    return sv;
  InternalList *il = m_handle->internal_list (m_type->custom);
  if (!il || index * 3 > il->length)
    __PLIC_return_EFAULT (sv);
  sv.push_back (m_handle->simple_string (il->items[index * 3 + 0]));   // ident
  sv.push_back (m_handle->simple_string (il->items[index * 3 + 1]));   // label
  sv.push_back (m_handle->simple_string (il->items[index * 3 + 2]));   // blurb
  return sv;
}

size_t
TypeCode::prerequisite_count () const
{
  if (kind() != INSTANCE)
    return 0;
  InternalList *il = m_handle->internal_list (m_type->custom);
  return il ? il->length : 0;
}

std::string
TypeCode::prerequisite (size_t index) const
{
  std::string s;
  if (kind() != INSTANCE)
    return s;
  InternalList *il = m_handle->internal_list (m_type->custom);
  if (!il || index > il->length)
    __PLIC_return_EFAULT (s);
  return m_handle->simple_string (il->items[index]);
}

size_t
TypeCode::field_count () const
{
  TypeKind k = kind();
  if (k == SEQUENCE)
    return 1;
  if (k != RECORD)
    return 0;
  InternalList *il = m_handle->internal_list (m_type->custom);
  return il ? il->length : 0;
}

TypeCode
TypeCode::field (size_t index) const // RECORD or SEQUENCE
{
  TypeKind k = kind();
  uint32_t field_offset;
  if (k == SEQUENCE && index == 0)
    field_offset = m_type->custom;
  else if (k == RECORD)
    {
      InternalList *il = m_handle->internal_list (m_type->custom);
      if (!il || index > il->length)
        __PLIC_return_EFAULT (TypeCode::notype (m_handle));
      field_offset = il->items[index];
    }
  else
    return TypeCode::notype (m_handle);
  InternalType *it = m_handle->internal_type (field_offset);
  return it ? TypeCode (m_handle, it) : TypeCode::notype (m_handle);
}

std::string
TypeCode::origin () const // type for TYPE_REFERENCE
{
  std::string s;
  if (kind() != TYPE_REFERENCE)
    return s;
  return m_handle->simple_string (m_type->custom);
}

bool
TypeCode::operator== (const TypeCode &o) const
{
  return o.m_handle == m_handle && o.m_type == m_type;
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
          std::vector<String> sv = enum_value (i);
          s += std::string ("\n") + indent + indent + sv[0] + ", " + sv[1] + ", " + sv[2];
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

#include "./builtins.cc" // defines intern_builtins_cc_typ

TypeMap
TypeMap::builtins ()
{
  InternalMap *imap = (InternalMap*) intern_builtins_cc_typ;
  const size_t length = sizeof (intern_builtins_cc_typ);
  if (!imap || sizeof (intern_builtins_cc_typ) < sizeof (*imap) + 4)
    {
      errno = ENODATA;
      perror ("ERROR: accessing builtin PLIC types");
      abort();
    }
  if (!imap->check_magic() || !imap->check_lengths (length) || !imap->check_tail())
    {
      errno = ELIBBAD;
      perror ("ERROR: accessing builtin PLIC types");
      abort();
    }
  TypeMap type_map = TypeCode::MapHandle::create_type_map (imap, length, true);
  errno = 0;
  return type_map;
}

} // Aida
