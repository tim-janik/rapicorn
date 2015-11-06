// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#ifndef __RAPICORN_CORE_MEMORY_HH__
#define __RAPICORN_CORE_MEMORY_HH__

#include <rcore/utilities.hh>

namespace Rapicorn {

/// Class for randomized and thread safe ID allocation.
class IdAllocator {
  RAPICORN_CLASS_NON_COPYABLE (IdAllocator);
protected:
  explicit            IdAllocator ();
public:
  virtual            ~IdAllocator ();
  virtual uint        alloc_id    () = 0;
  virtual void        release_id  (uint unique_id) = 0;
  virtual bool        seen_id     (uint unique_id) = 0;
  static IdAllocator* _new        (uint startval = 1);
};

// == Utilities ==
int   fmsb          (uint64  value) RAPICORN_CONST;

// == Aligned Memory ==
void* aligned_alloc (size_t   total_size, size_t alignment, uint8 **free_pointer);
void  aligned_free  (uint8 **free_pointer);

/// Class to maintain an array of aligned memory
template<class T, int ALIGNMENT>
class AlignedArray {
  uint8 *unaligned_mem_;
  T     *data_;
  size_t n_elements_;
  void
  allocate_aligned_data()
  {
    static_assert (ALIGNMENT % sizeof (T) == 0, "ALIGNMENT must exactly fit a multiple of sizeof (T)");
    data_ = reinterpret_cast<T*> (aligned_alloc (n_elements_ * sizeof (T), ALIGNMENT, &unaligned_mem_));
  }
  // disallow copy constructor assignment operator
  RAPICORN_CLASS_NON_COPYABLE (AlignedArray);
public:
  AlignedArray (const vector<T>& elements) :
    n_elements_ (elements.size())
  {
    allocate_aligned_data();
    for (size_t i = 0; i < n_elements_; i++)
      new (data_ + i) T (elements[i]);
  }
  AlignedArray (size_t n_elements) :
    n_elements_ (n_elements)
  {
    allocate_aligned_data();
    for (size_t i = 0; i < n_elements_; i++)
      new (data_ + i) T();
  }
  ~AlignedArray()
  {
    // C++ destruction order: last allocated element is deleted first
    while (n_elements_)
      data_[--n_elements_].~T();
    aligned_free (&unaligned_mem_);
  }
  T&            operator[] (size_t pos)         { return data_[pos]; }
  const T&      operator[] (size_t pos) const   { return data_[pos]; }
  size_t        size       () const             { return n_elements_; }
};

} // Rapicorn

#endif /* __RAPICORN_CORE_MEMORY_HH__ */
