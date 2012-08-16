// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#include "blobres.hh"
#include "thread.hh"
#include <fcntl.h>
#include <sys/stat.h>

namespace Rapicorn {

// == BlobResource ==
struct BlobResource {
  virtual String        name            () = 0;
  virtual size_t        size            () = 0;
  virtual const char*   data            () = 0;
  virtual String        string          () = 0;
  virtual              ~BlobResource    () {}
};

// == ByteBlob ==
template<class Deleter>
struct ByteBlob : public BlobResource {
  const char           *m_data;
  size_t                m_size;
  String                m_name;
  String                m_string;
  Deleter               m_deleter;
  explicit              ByteBlob        (const String &name, size_t dsize, const char *data, const Deleter &deleter);
  virtual String        name            ()      { return m_name; }
  virtual size_t        size            ()      { return m_size; }
  virtual const char*   data            ()      { return m_data; }
  virtual String        string          ();
  virtual              ~ByteBlob        ();
};

template<class Deleter>
ByteBlob<Deleter>::ByteBlob (const String &name, size_t dsize, const char *data, const Deleter &deleter) :
  m_data (data), m_size (dsize), m_name (name), m_deleter (deleter)
{}

template<class Deleter>
ByteBlob<Deleter>::~ByteBlob ()
{
  m_deleter (m_data);
}

template<class Deleter> String
ByteBlob<Deleter>::string ()
{
  if (m_string.empty())
    {
      static Mutex mutex;
      ScopedLock<Mutex> g (mutex);
      if (m_string.empty())
        {
          size_t len = m_size;
          while (len && m_data[len - 1] == 0)
            len--;
          m_string = String (m_data, len);
        }
    }
  return m_string;
}

// == ResourceEntry ==
static ResourceEntry *res_head = NULL;
static Mutex          res_mutex;

void
ResourceEntry::reg_add (ResourceEntry *entry)
{
  assert_return (entry && !entry->next);
  ScopedLock<Mutex> sl (res_mutex);
  entry->next = res_head;
  res_head = entry;
}

ResourceEntry::~ResourceEntry()
{
  ScopedLock<Mutex> sl (res_mutex);
  ResourceEntry **ptr = &res_head;
  while (*ptr != this)
    ptr = &(*ptr)->next;
  *ptr = next;
}

const ResourceEntry*
ResourceEntry::find_entry (const String &res_name)
{
  ScopedLock<Mutex> sl (res_mutex);
  for (const ResourceEntry *e = res_head; e; e = e->next)
    if (res_name == e->name)
      return e;
  return NULL;
}

// == StringBlob ==
struct StringBlob : public BlobResource {
  String                m_name;
  String                m_string;
  explicit              StringBlob      (const String &name, const String &data) : m_name (name), m_string (data) {}
  virtual String        name            ()      { return m_name; }
  virtual size_t        size            ()      { return m_string.size(); }
  virtual const char*   data            ()      { return m_string.c_str(); }
  virtual String        string          ()      { return m_string; }
  virtual              ~StringBlob      ()      {}
};

static String // provides errno on error
string_read (const String &filename, const int fd, size_t guess)
{
  String data;
  if (guess)
    data.resize (guess + 1);            // pad by +1 to detect EOF reads
  else
    data.resize (4096);                 // default buffering for unknown sizes
  size_t stored = 0;
  for (ssize_t l = 1; l > 0; )
    {
      if (stored >= data.size())        // only triggered for unknown sizes
        data.resize (2 * data.size());
      do
        l = read (fd, &data[stored], data.size() - stored);
      while (l < 0 && (errno == EAGAIN || errno == EINTR));
      stored += MAX (0, l);
      if (l < 0)
        DEBUG ("%s: read: %s", filename.c_str(), strerror (errno));
      else
        errno = 0;
    }
  data.resize (stored);
  return data;
}

// == Blob ==
String          Blob::name   () const { return m_blob->name(); }
size_t          Blob::size   () const { return m_blob->size(); }
const char*     Blob::data   () const { return m_blob->data(); }
const uint8*    Blob::bytes  () const { return reinterpret_cast<const uint8*> (m_blob->data()); }
String          Blob::string () const { return m_blob->string(); }

Blob::Blob (const std::shared_ptr<BlobResource> &initblob) :
  m_blob (initblob)
{}

Blob
Blob::load (const String &res_path)
{
  const ResourceEntry *entry = ResourceEntry::find_entry (res_path);
  struct NopDeleter { void operator() (const char*) {} }; // prevent delete on const data
  if (entry && (entry->dsize == entry->psize ||         // uint8[] array
                entry->dsize + 1 == entry->psize))      // string initilization with 0-termination
    {
      if (entry->dsize + 1 == entry->psize)
        assert (entry->pdata[entry->dsize] == 0);
      return Blob (std::make_shared<ByteBlob<NopDeleter>> (res_path, entry->dsize, entry->pdata, NopDeleter()));
    }
  else if (entry && entry->psize && entry->dsize == 0)  // variable length array with automatic size
    return Blob (std::make_shared<ByteBlob<NopDeleter>> (res_path, entry->psize, entry->pdata, NopDeleter()));
  else if (entry && entry->psize < entry->dsize)        // compressed
    {
      const uint8 *u8data = zintern_decompress (entry->dsize, reinterpret_cast<const uint8*> (entry->pdata), entry->psize);
      const char *data = reinterpret_cast<const char*> (u8data);
      struct ZinternDeleter { void operator() (const char *d) { zintern_free ((uint8*) d); } };
      return Blob (std::make_shared<ByteBlob<ZinternDeleter>> (res_path, entry->dsize, data, ZinternDeleter()));
    }
  errno = 0;
  const int fd = open (res_path.c_str(), O_RDONLY | O_NOCTTY, 0);
  if (fd < 0)
    {
      DEBUG ("%s: open: %s", res_path.c_str(), strerror (errno));
      return Blob (std::shared_ptr<BlobResource> (nullptr));
    }
  struct stat sbuf = { 0, };
  size_t guess = 0;
  if (fstat (fd, &sbuf) == 0 && sbuf.st_size)
    guess = sbuf.st_size;
  String iodata = string_read (res_path, fd, guess);
  const int e = errno;
  close (fd);
  errno = e;
  if (errno)
    return Blob (std::shared_ptr<BlobResource> (nullptr));
  else // errno == 0
    return Blob (std::make_shared<StringBlob> (res_path, iodata));
}

/**
 * @class Blob
 * Binary data access for builtin resources and files.
 * A Blob provides access to binary data (BLOB = Binary Large OBject) which can be
 * preassembled and compiled into a program or located in a resource path directory.
 * Locators for resources should generally adhere to the form: @code
 *      res: [relative_path/] resource_name
 * @endcode
 * See also: RAPICORN_STATIC_RESOURCE_DATA(), RAPICORN_STATIC_RESOURCE_ENTRY().
 * <BR> Example: @SNIPPET{rcore/tests/multitest.cc, Blob-EXAMPLE}
 */

} // Rapicorn
