// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#include "objects.hh"
#include "thread.hh"
#include <typeinfo>
#include <cxxabi.h>

namespace Rapicorn {

// == VirtualTypeid ==
VirtualTypeid::~VirtualTypeid ()
{ /* virtual destructor ensures vtable */ }

String
VirtualTypeid::typeid_name ()
{
  return cxx_demangle (typeid (*this).name());
}

String
VirtualTypeid::cxx_demangle (const char *mangled_identifier)
{
  int status = 0;
  char *malloced_result = abi::__cxa_demangle (mangled_identifier, NULL, NULL, &status);
  String result = malloced_result && !status ? malloced_result : mangled_identifier;
  if (malloced_result)
    free (malloced_result);
  return result;
}

// == Deletable ==
/**
 * @class Deletable
 * Classes derived from @a Deletable need to have a virtual destructor.
 * Handlers can be registered with class instances that are called during
 * instance deletion. This is most useful for automated memory keeping of
 * custom resources attached to the lifetime of a particular instance.
 */
Deletable::~Deletable ()
{
  invoke_deletion_hooks();
}

/**
 * @param deletable     possible Deletable* handle
 * @return              TRUE if the hook was added
 *
 * Adds the deletion hook to @a deletable if it is non NULL.
 * The deletion hook is asserted to be so far uninstalled.
 * This function is MT-safe and may be called from any thread.
 */
bool
Deletable::DeletionHook::deletable_add_hook (Deletable *deletable)
{
  if (deletable)
    {
      deletable->add_deletion_hook (this);
      return true;
    }
  return false;
}

/**
 * @param deletable     possible Deletable* handle
 * @return              TRUE if the hook was removed
 *
 * Removes the deletion hook from @a deletable if it is non NULL.
 * The deletion hook is asserted to be installed on @a deletable.
 * This function is MT-safe and may be called from any thread.
 */
bool
Deletable::DeletionHook::deletable_remove_hook (Deletable *deletable)
{
  if (deletable)
    {
      deletable->remove_deletion_hook (this);
      return true;
    }
  return false;
}

Deletable::DeletionHook::~DeletionHook ()
{
  if (this->next || this->prev)
    fatal ("hook is being destroyed but not unlinked: %p", this);
}

#define DELETABLE_MAP_HASH (19) /* use prime size for hashing, sums up to roughly 1k (use 83 for 4k) */
struct DeletableAuxData {
  Deletable::DeletionHook* hooks;
  DeletableAuxData() : hooks (NULL) {}
  ~DeletableAuxData() { assert (hooks == NULL); }
};
struct DeletableMap {
  Mutex                                 mutex;
  std::map<Deletable*,DeletableAuxData> dmap;
};
typedef std::map<Deletable*,DeletableAuxData>::iterator DMapIterator;
static Atomic<DeletableMap*> deletable_maps = NULL;

static inline void
auto_init_deletable_maps (void)
{
  if (UNLIKELY (deletable_maps.load() == NULL))
    {
      DeletableMap *dmaps = new DeletableMap[DELETABLE_MAP_HASH];
      if (!deletable_maps.cas (NULL, dmaps))
        delete dmaps;
      // ensure threading works
      deletable_maps[0].mutex.lock();
      deletable_maps[0].mutex.unlock();
    }
}

/**
 * @param hook  valid deletion hook
 *
 * Add an uninstalled deletion hook to the deletable.
 * This function is MT-safe and may be called from any thread.
 */
void
Deletable::add_deletion_hook (DeletionHook *hook)
{
  auto_init_deletable_maps();
  const uint32 hashv = ((size_t) (void*) this) % DELETABLE_MAP_HASH;
  deletable_maps[hashv].mutex.lock();
  RAPICORN_ASSERT (hook);
  RAPICORN_ASSERT (!hook->next);
  RAPICORN_ASSERT (!hook->prev);
  DeletableAuxData &ad = deletable_maps[hashv].dmap[this];
  if (!ad.hooks)
    ad.hooks = hook->prev = hook->next = hook;
  else
    {
      hook->prev = ad.hooks->prev;
      hook->next = ad.hooks;
      hook->prev->next = hook;
      hook->next->prev = hook;
      ad.hooks = hook;
    }
  deletable_maps[hashv].mutex.unlock();
  hook->monitoring_deletable (*this);
  //g_printerr ("DELETABLE-ADD(%p,%p)\n", this, hook);
}

/**
 * @param hook  valid deletion hook
 *
 * Remove a previously added deletion hook.
 * This function is MT-safe and may be called from any thread.
 */
void
Deletable::remove_deletion_hook (DeletionHook *hook)
{
  auto_init_deletable_maps();
  const uint32 hashv = ((size_t) (void*) this) % DELETABLE_MAP_HASH;
  deletable_maps[hashv].mutex.lock();
  RAPICORN_ASSERT (hook);
  RAPICORN_ASSERT (hook->next && hook->prev);
  hook->next->prev = hook->prev;
  hook->prev->next = hook->next;
  DMapIterator it = deletable_maps[hashv].dmap.find (this);
  RAPICORN_ASSERT (it != deletable_maps[hashv].dmap.end());
  DeletableAuxData &ad = it->second;
  if (ad.hooks == hook)
    ad.hooks = hook->next != hook ? hook->next : NULL;
  hook->prev = NULL;
  hook->next = NULL;
  deletable_maps[hashv].mutex.unlock();
  //g_printerr ("DELETABLE-REM(%p,%p)\n", this, hook);
}

/**
 * Invoke all deletion hooks installed on this deletable.
 */
void
Deletable::invoke_deletion_hooks()
{
  /* upon program exit, we may get here without deletable maps or even
   * threading being initialized. to avoid calling into a NULL threading
   * table, we'll detect the case and return
   */
  if (NULL == deletable_maps.load())
    return;
  auto_init_deletable_maps();
  const uint32 hashv = ((size_t) (void*) this) % DELETABLE_MAP_HASH;
  while (TRUE)
    {
      /* lookup hook list */
      deletable_maps[hashv].mutex.lock();
      DeletionHook *hooks;
      DMapIterator it = deletable_maps[hashv].dmap.find (this);
      if (it != deletable_maps[hashv].dmap.end())
        {
          DeletableAuxData &ad = it->second;
          hooks = ad.hooks;
          ad.hooks = NULL;
          deletable_maps[hashv].dmap.erase (it);
        }
      else
        hooks = NULL;
      deletable_maps[hashv].mutex.unlock();
      /* we're done if all hooks have been procesed */
      if (!hooks)
        break;
      /* process hooks */
      while (hooks)
        {
          DeletionHook *hook = hooks;
          hook->next->prev = hook->prev;
          hook->prev->next = hook->next;
          hooks = hook->next != hook ? hook->next : NULL;
          hook->prev = hook->next = NULL;
          //g_printerr ("DELETABLE-DISMISS(%p,%p)\n", this, hook);
          hook->dismiss_deletable();
        }
    }
}

// == BaseObject ==
/// Static unref function for std::shared_ptr<BaseObject> Deleter.
static void
base_object_unref (BaseObject *object)
{
  object->unref();
}

std::shared_ptr<BaseObject>
BaseObject::shared_ptr (BaseObject *object)
{
  if (object)
    return std::shared_ptr<BaseObject> (ref (object), base_object_unref);
  else
    return std::shared_ptr<BaseObject>();
}

void
BaseObject::dispose ()
{}

// == ReferenceCountable ==
static __attribute__ ((noinline)) size_t
stack_ptrdiff (const void *stackvariable, const void *data)
{
  size_t d = size_t (data);
  size_t s = size_t (stackvariable);
  return MAX (d, s) - MIN (d, s);
}

ReferenceCountable::ReferenceCountable (uint allow_stack_magic) :
  ref_field (FLOATING_FLAG + 1)
{
  if (allow_stack_magic != 0xbadbad)
    {
      // check for stack allocations
      const void *data = this;
      static const size_t stack_proximity_threshold = 4096; ///< page_size on most systems
      int stackvariable = 0;
      /* this check for ReferenceCountable instance allocations on the stack isn't
       * perfect, but should catch the most common cases for growing and shrinking stacks
       */
      if (stack_ptrdiff (&stackvariable, data) < stack_proximity_threshold)
        fatal ("ReferenceCountable object allocated on stack instead of heap: %u > %u (%p - %p)",
               stack_proximity_threshold, stack_ptrdiff (&stackvariable, data), data, &stackvariable);
    }
}

void
ReferenceCountable::ref_sink() const
{
  ref();
  uint32 old_ref = ref_get();
  uint32 new_ref = old_ref & ~FLOATING_FLAG;
  if (RAPICORN_UNLIKELY (old_ref != new_ref))
    {
      while (RAPICORN_UNLIKELY (!ref_cas (old_ref, new_ref)))
        {
          old_ref = ref_get();
          new_ref = old_ref & ~FLOATING_FLAG;
        }
      if (old_ref & FLOATING_FLAG)
        unref();
    }
}

void
ReferenceCountable::real_unref () const
{
  uint32 old_ref = ref_get();
  if (RAPICORN_UNLIKELY (1 == (old_ref & ~FLOATING_FLAG)))
    {
      ReferenceCountable *self = const_cast<ReferenceCountable*> (this);
      self->pre_finalize();
      old_ref = ref_get();
    }
  RAPICORN_ASSERT (old_ref & ~FLOATING_FLAG);         // old_ref > 1 ?
  while (RAPICORN_UNLIKELY (!ref_cas (old_ref, old_ref - 1)))
    {
      old_ref = ref_get();
      RAPICORN_ASSERT (old_ref & ~FLOATING_FLAG);     // catch underflow
    }
  if (RAPICORN_UNLIKELY (1 == (old_ref & ~FLOATING_FLAG)))
    {
      ReferenceCountable *self = const_cast<ReferenceCountable*> (this);
      self->finalize();
      self->delete_this();                            // usually: delete this;
    }
}

void
ReferenceCountable::ref_diag (const char *msg) const
{
  fprintf (stderr, "%s: this=%p ref_count=%d floating=%d", msg ? msg : "ReferenceCountable", this, ref_count(), floating());
}

void
ReferenceCountable::pre_finalize ()
{}

void
ReferenceCountable::finalize ()
{}

void
ReferenceCountable::delete_this ()
{
  delete this;
}

ReferenceCountable::~ReferenceCountable ()
{
  RAPICORN_ASSERT (ref_count() == 0);
}

} // Rapicorn
