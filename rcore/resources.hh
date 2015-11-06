// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#ifndef __RAPICORN_BLOBRES_HH__
#define __RAPICORN_BLOBRES_HH__

#include <rcore/utilities.hh>
#include <memory>

namespace Rapicorn {

// == Blob ==
class BlobResource;
class Blob {
  std::shared_ptr<BlobResource> blob_;
  typedef size_t (Blob::*_UBool) () const;              // unspecified-type-boolean for non-numeric operator bool() result
  static _UBool _ubool1 ()     { return &Blob::size; }  // unspecified-type-boolean true value
  _UBool        _bool () const { return blob_ && size() ? _ubool1() : 0; }
  explicit     Blob   (const std::shared_ptr<BlobResource> &initblob);
protected:
  static Blob  asres  (const String &resource);         ///< Create Blob from internal @a resource.
public:
  explicit     Blob   ();                               ///< Default construct a NULL blob.
  String       name   () const;                         ///< Provide the name of this resource Blob.
  size_t       size   () const;                         ///< Retrieve the size of a Blob in bytes, this may be 0.
  const char*  data   () const;                         ///< Access the data of a Blob.
  const uint8* bytes  () const;                         ///< Access the data of a Blob.
  String       string () const;                         ///< Access data as string, strips trailing 0s.
  operator     _UBool () const { return _bool(); }      ///< Checks if the blob contains accessible data.
  static Blob  load   (const String &filename);         ///< Create a Blob by loading or mmap-ing @a filename, sets errno on error.
  static Blob  from   (const String &blob_string);      ///< Create a Blob containing @a blob_string.
};

// == Res ==
/// Res provides access to resource files at runtime, see also rapidres(1).
class Res {
  const String        res_path_;
protected:
  static void         utest_hook (std::function<Blob (const String&)> hook);
  Blob                resolve    () const;                      ///< Resolve resource into a binary object.
public:
  explicit            Res        (const String &res_path);      /// Construct a resource request.
  String              type       () const;                      ///< Retrieve the resource type tag.
  /// Resolve resource into a specific type.
  template<class T> T as         () { return load<T>(); }
  /// Automatically deduce Type for as<Type>() from lvalue.
  template<class T>   operator T () { return as<T>(); }
  /// Resource type converter for custom overloads.
  template<class Type> Type load () { static_assert (!sizeof (Type), "Unimplemented Resource Type"); }
};

/// Load a Blob resource, e.g.: Blob data = Res ("@res Example/resource.dat");
template<> Blob Res::load<Blob> ();

// == Resource Macros ==

/// Statically declare a ResourceBlob data variable.
#define RAPICORN_RES_STATIC_DATA(IDENT)            \
  static const char __Rapicorn_static_resourceD__##IDENT[] __attribute__ ((__aligned__ (2 * sizeof (size_t))))

/// Statically register a ResourceBlob entry, referring a previously declared RAPICORN_RES_STATIC_DATA(IDENT) variable.
#define RAPICORN_RES_STATIC_ENTRY(IDENT, PATH, ...) \
  static const Rapicorn::ResourceEntry __Rapicorn_static_resourceE__##IDENT = { PATH, __Rapicorn_static_resourceD__##IDENT, __VA_ARGS__ };

// == Internals ==
///@cond
class ResourceEntry {
  ResourceEntry *next;
  const char    *const name;
  const char    *const pdata;
  const size_t   psize, dsize;
  friend         class Blob;
  static const ResourceEntry* find_entry (const String&);
  static void                 reg_add    (ResourceEntry*);
public:
  template<size_t N>  ResourceEntry (const char *res, const char (&idata) [N], size_t data_size = 0) :
    next (NULL), name (res), pdata (idata), psize (N), dsize (data_size)
  { reg_add (this); }
  /*dtor*/           ~ResourceEntry();
};
///@endcond

} // Rapicorn

#endif // __RAPICORN_BLOBRES_HH__
