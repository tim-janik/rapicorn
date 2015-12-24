// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#include "resources.hh"
#include "thread.hh"
#include "strings.hh"
#include "randomhash.hh"
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>

#define DEBUG(...)      RAPICORN_KEY_DEBUG ("Res", __VA_ARGS__)

namespace Rapicorn {

// == Res ==
Res::Res (const String &res_path) :
  res_path_ (res_path)
{}

static ssize_t
res_path_offset (const String &res_path)
{
  // validate syntax: '@' [a-zA-Z_0-9]+ ' ' <pathname>
  const ssize_t size = res_path.size();
  ssize_t n = 0;
  if (n >= size || res_path[n] != '@')
    return -1;
  n++;                          // '@'
  while (n < size &&
         ((res_path[n] >= 'a' && res_path[n] <= 'z') ||
          (res_path[n] >= 'A' && res_path[n] <= 'Z') ||
          res_path[n] == '_' ||
          (n > 1 && res_path[n] >= '0' && res_path[n] <= '9')))
    n++;                        // [a-zA-Z_0-9]
  if (n >= size || res_path[n] != ' ')
    return -1;
  n++;                          // ' '
  if (n >= size)                // check for non-empty pathname
    return -1;
  return n;
}

String
Res::type () const
{
  ssize_t n = res_path_offset (res_path_);
  return n < 0 ? "" : res_path_.substr (1, n - 2);
}

static std::function<Blob (const String&)> unit_test_hook;

/// Internal function, used for unit tests.
void
Res::utest_hook (std::function<Blob (const String&)> hook)
{
  unit_test_hook = hook;
}

Blob
Res::resolve () const
{
  ssize_t n = res_path_offset (res_path_);
  errno = ENOENT;
  if (n > 0)
    {
      struct Blob_ : Blob { using Blob::asres; };
      Blob blob = Blob_::asres (res_path_.c_str() + n);
      if (blob)
        return blob;
    }
  const int saved_errno = errno;
  DEBUG ("no such resource: '%s'", res_path_);
  errno = saved_errno;
  if (unit_test_hook)
    return unit_test_hook (res_path_);
  return Blob();
}

template<> Blob
Res::load<Blob> ()
{
  return resolve();
}

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
  const char           *data_;
  size_t                size_;
  String                name_;
  String                string_;
  Deleter               deleter_;
  explicit              ByteBlob        (const String &name, size_t dsize, const char *data, const Deleter &deleter);
  virtual String        name            ()      { return name_; }
  virtual size_t        size            ()      { return size_; }
  virtual const char*   data            ()      { return data_; }
  virtual String        string          ();
  virtual              ~ByteBlob        ();
};

template<class Deleter>
ByteBlob<Deleter>::ByteBlob (const String &name, size_t dsize, const char *data, const Deleter &deleter) :
  data_ (data), size_ (dsize), name_ (name), deleter_ (deleter)
{}

template<class Deleter>
ByteBlob<Deleter>::~ByteBlob ()
{
  deleter_ (data_);
}

template<class Deleter> String
ByteBlob<Deleter>::string ()
{
  if (string_.empty() && size_)
    {
      static Mutex mutex;
      ScopedLock<Mutex> g (mutex);
      if (string_.empty())
        {
          size_t len = size_;
          while (len && data_[len - 1] == 0)
            len--;
          string_ = String (data_, len);
        }
    }
  return string_;
}

// == ResourceEntry ==
#ifndef DOXYGEN
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
#endif // !DOXYGEN

// == StringBlob ==
struct StringBlob : public BlobResource {
  String                name_;
  String                string_;
  explicit              StringBlob      (const String &name, const String &data) : name_ (name), string_ (data) {}
  virtual String        name            ()      { return name_; }
  virtual size_t        size            ()      { return string_.size(); }
  virtual const char*   data            ()      { return string_.c_str(); }
  virtual String        string          ()      { return string_; }
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
String          Blob::name   () const { return blob_ ? blob_->name() : ""; }
size_t          Blob::size   () const { return blob_ ? blob_->size() : 0; }
const char*     Blob::data   () const { return blob_ ? blob_->data() : NULL; }
const uint8*    Blob::bytes  () const { return reinterpret_cast<const uint8*> (data()); }
String          Blob::string () const { return blob_ ? blob_->string() : ""; }

Blob::Blob (const std::shared_ptr<BlobResource> &initblob) :
  blob_ (initblob)
{}

Blob::Blob() :
  blob_ (std::shared_ptr<BlobResource> (nullptr))
{}

static Blob
error_result (String url, int fallback_errno = EINVAL, String msg = "failed to load")
{
  const int saved_errno = errno ? errno : fallback_errno;
  DEBUG ("%s %s: %s", msg.c_str(), CQUOTE (url), strerror (saved_errno));
  errno = saved_errno;
  return Blob();
}

Blob
Blob::asres (const String &resource)
{
  // blob from builtin resources
  const ResourceEntry *entry = ResourceEntry::find_entry (resource);
  struct NoDelete { void operator() (const char*) {} }; // prevent delete on const data
  if (entry && (entry->dsize == entry->psize ||         // uint8[] array
                entry->dsize + 1 == entry->psize))      // string initilization with 0-termination
    {
      if (entry->dsize + 1 == entry->psize)
        assert (entry->pdata[entry->dsize] == 0);
      return Blob (std::make_shared<ByteBlob<NoDelete>> (resource, entry->dsize, entry->pdata, NoDelete()));
    }
  else if (entry && entry->psize && entry->dsize == 0)  // variable length array with automatic size
    return Blob (std::make_shared<ByteBlob<NoDelete>> (resource, entry->psize, entry->pdata, NoDelete()));
  // blob from compressed resources
  if (entry && entry->psize < entry->dsize)
    {
      const uint8 *u8data = zintern_decompress (entry->dsize, reinterpret_cast<const uint8*> (entry->pdata), entry->psize);
      const char *data = reinterpret_cast<const char*> (u8data);
      struct ZinternDeleter { void operator() (const char *d) { zintern_free ((uint8*) d); } };
      return Blob (std::make_shared<ByteBlob<ZinternDeleter>> (resource, entry->dsize, data, ZinternDeleter()));
    }
  // handle resource errors
  return error_result (resource, ENOENT, String (entry ? "invalid" : "unknown") + " resource entry");
}

Blob
Blob::load (const String &filename)
{
  // load blob from file
  errno = 0;
  const int fd = open (filename.c_str(), O_RDONLY | O_NOCTTY | O_CLOEXEC, 0);
  struct stat sbuf = { 0, };
  size_t file_size = 0;
  if (fd < 0)
    return error_result (filename, ENOENT);
  if (fstat (fd, &sbuf) == 0 && sbuf.st_size)
    file_size = sbuf.st_size;
  // blob via mmap
  void *maddr;
  if (file_size >= 128 * 1024 &&
      MAP_FAILED != (maddr = mmap (NULL, file_size, PROT_READ, MAP_SHARED | MAP_DENYWRITE | MAP_POPULATE, fd, 0)))
    {
      close (fd); // mmap keeps its own file reference
      struct MunmapDeleter {
        const size_t length; MunmapDeleter (size_t l) : length (l) {}
        void operator() (const char *d) { munmap ((void*) d, length); }
      };
      return Blob (std::make_shared<ByteBlob<MunmapDeleter>> (filename, file_size, (const char*) maddr, MunmapDeleter (file_size)));
    }
  // blob via read
  errno = 0;
  String iodata = string_read (filename, fd, file_size);
  const int saved_errno = errno;
  close (fd);
  errno = saved_errno;
  if (!errno)
    return Blob (std::make_shared<StringBlob> (filename, iodata));
  // handle file errors
  return error_result (filename, ENOENT);
}

Blob
Blob::from (const String &blob_string)
{
  String res_url = string_format ("res:/.tmp/%016x", string_hash64 (blob_string));
  return Blob (std::make_shared<StringBlob> (res_url, blob_string));
}

/**
 * @class Blob
 * Binary data access for builtin resources and files.
 * A Blob provides access to binary data (BLOB = Binary Large OBject) which can be
 * preassembled and compiled into a program or located in a resource path directory.
 * Locators for resources should generally adhere to the form: @code
 *      res: [relative_path/] resource_name
 * @endcode
 * See also: RAPICORN_RES_STATIC_DATA(), RAPICORN_RES_STATIC_ENTRY(), rapidres(1).
 * <BR> Example: @snippet rcore/tests/multitest.cc Blob-EXAMPLE
 */

} // Rapicorn

// rapicorn/res/ hierarchy
#include "zres.cc"
