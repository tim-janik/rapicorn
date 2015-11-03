// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#include "memory.hh"
#include "thread.hh"

namespace Rapicorn {

// == IdAllocator ==
IdAllocator::IdAllocator ()
{}

IdAllocator::~IdAllocator ()
{}

class IdAllocatorImpl : public IdAllocator {
  Spinlock     mutex;
  const uint   counterstart, wbuffer_capacity;
  uint         counter, wbuffer_pos, wbuffer_size;
  uint        *wbuffer;
  vector<uint> free_ids;
public:
  explicit     IdAllocatorImpl (uint startval, uint wbuffercap);
  virtual     ~IdAllocatorImpl () { delete[] wbuffer; }
  virtual uint alloc_id        ();
  virtual void release_id      (uint unique_id);
  virtual bool seen_id         (uint unique_id);
};

IdAllocator*
IdAllocator::_new (uint startval)
{
  return new IdAllocatorImpl (startval, 97);
}

IdAllocatorImpl::IdAllocatorImpl (uint startval, uint wbuffercap) :
  counterstart (startval), wbuffer_capacity (wbuffercap),
  counter (counterstart), wbuffer_pos (0), wbuffer_size (0)
{
  wbuffer = new uint[wbuffer_capacity];
}

void
IdAllocatorImpl::release_id (uint unique_id)
{
  assert_return (unique_id >= counterstart && unique_id < counter);
  /* protect */
  mutex.lock();
  /* release oldest withheld id */
  if (wbuffer_size >= wbuffer_capacity)
    free_ids.push_back (wbuffer[wbuffer_pos]);
  /* withhold released id */
  wbuffer[wbuffer_pos++] = unique_id;
  wbuffer_size = MAX (wbuffer_size, wbuffer_pos);
  if (wbuffer_pos >= wbuffer_capacity)
    wbuffer_pos = 0;
  /* cleanup */
  mutex.unlock();
}

uint
IdAllocatorImpl::alloc_id ()
{
  uint64 unique_id;
  mutex.lock();
  if (free_ids.empty())
    unique_id = counter++;
  else
    {
      uint64 randomize = uint64 (this) + (uint64 (&randomize) >> 3); // cheap random data
      randomize += wbuffer[wbuffer_pos] + wbuffer_pos + counter; // add entropy
      uint random_pos = randomize % free_ids.size();
      unique_id = free_ids[random_pos];
      free_ids[random_pos] = free_ids.back();
      free_ids.pop_back();
    }
  mutex.unlock();
  return unique_id;
}

bool
IdAllocatorImpl::seen_id (uint unique_id)
{
  mutex.lock();
  bool inrange = unique_id >= counterstart && unique_id < counter;
  mutex.unlock();
  return inrange;
}

// == memory utils ==
/// Allocate a block of memory aligned to at least @a alignment bytes.
void*
aligned_alloc (size_t total_size, size_t alignment, uint8 **free_pointer)
{
  assert_return (free_pointer != NULL, NULL);
  uint8 *aligned_mem = new uint8[total_size];
  *free_pointer = aligned_mem;
  if (aligned_mem && (!alignment || 0 == size_t (aligned_mem) % alignment))
    return aligned_mem;
  if (aligned_mem)
    delete[] aligned_mem;
  aligned_mem = new uint8[total_size + alignment - 1];
  assert (aligned_mem != NULL);
  *free_pointer = aligned_mem;
  if (size_t (aligned_mem) % alignment)
    aligned_mem += alignment - size_t (aligned_mem) % alignment;
  return aligned_mem;
}

/// Release a block of memory allocated through aligned_malloc().
void
aligned_free (uint8 **free_pointer)
{
  assert_return (free_pointer != NULL);
  if (*free_pointer)
    {
      uint8 *data = *free_pointer;
      *free_pointer = NULL;
      delete[] data;
    }
}

/**
 * The fmsb() function returns the position of the most significant bit set in the word @a val.
 * The least significant bit is position 1 and the most significant position is, for example, 32 or 64.
 * @returns The position of the most significant bit set is returned, or 0 if no bits were set.
 */
int // 0 or 1..64
fmsb (uint64 val)
{
  if (val >> 32)
    return 32 + fmsb (val >> 32);
  int nb = 32;
  do
    {
      nb--;
      if (val & (1U << nb))
        return nb + 1;  /* 1..32 */
    }
  while (nb > 0);
  return 0; /* none found */
}

} // Rapicorn
