// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __RAPICORN_CORE_MEMORY_HH__
#define __RAPICORN_CORE_MEMORY_HH__

#include <rcore/utilities.hh>

namespace Rapicorn {

void* aligned_alloc (size_t   total_size, size_t alignment, uint8 **free_pointer);
void  aligned_free  (uint8 **free_pointer);
int   fmsb          (uint64  value) RAPICORN_CONST;

/* --- Id Allocator --- */
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

} // Rapicorn

#endif /* __RAPICORN_CORE_MEMORY_HH__ */
