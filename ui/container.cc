// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#include "container.hh"
#include "container.hh"
#include "window.hh"
#include "factory.hh"
#include <algorithm>
#include <stdio.h>

#define DEBUG_RESIZE(...)     RAPICORN_KEY_DEBUG ("Resize", __VA_ARGS__)

namespace Rapicorn {

struct ClassDoctor {
  static void item_set_parent (ItemImpl &item, ContainerImpl *parent) { item.set_parent (parent); }
};

/* --- CrossLinks --- */
struct CrossLink {
  ItemImpl                             *owner, *link;
  Signals::Trampoline1<void,ItemImpl&> *uncross;
  CrossLink                            *next;
  CrossLink (ItemImpl *o, ItemImpl *l, Signals::Trampoline1<void,ItemImpl&> &it) :
    owner (o), link (l),
    uncross (ref_sink (&it)), next (NULL)
  {}
  ~CrossLink()
  {
    unref (uncross);
  }
  RAPICORN_CLASS_NON_COPYABLE (CrossLink);
};
struct CrossLinks {
  ContainerImpl *container;
  CrossLink *links;
};
static inline void      container_uncross_link_R        (ContainerImpl *container,
                                                         CrossLink **clinkp,
                                                         bool        notify_callback = true);
struct CrossLinksKey : public DataKey<CrossLinks*> {
  virtual void
  destroy (CrossLinks *clinks)
  {
    while (clinks->links)
      container_uncross_link_R (clinks->container, &clinks->links);
    delete (clinks);
  }
};
static CrossLinksKey cross_links_key;

struct UncrossNode {
  UncrossNode *next;
  ContainerImpl   *mutable_container;
  CrossLink   *clink;
  UncrossNode (ContainerImpl *xcontainer, CrossLink *xclink) :
    next (NULL), mutable_container (xcontainer), clink (xclink)
  {}
  RAPICORN_CLASS_NON_COPYABLE (UncrossNode);
};
static UncrossNode *uncross_callback_stack = NULL;
static Mutex        uncross_callback_stack_mutex;

void
ContainerImpl::item_cross_link (ItemImpl       &owner,
                                ItemImpl       &link,
                                const ItemSlot &uncross)
{
  assert (&owner != &link);
  assert (owner.common_ancestor (link) == this); // could be disabled for performance
  CrossLinks *clinks = get_data (&cross_links_key);
  if (!clinks)
    {
      clinks =  new CrossLinks();
      clinks->container = this;
      clinks->links = NULL;
      set_data (&cross_links_key, clinks);
    }
  CrossLink *clink = new CrossLink (&owner, &link, *uncross.get_trampoline());
  clink->next = clinks->links;
  clinks->links = clink;
}

void
ContainerImpl::item_cross_unlink (ItemImpl       &owner,
                                  ItemImpl       &link,
                                  const ItemSlot &uncross)
{
  bool found_one = false;
  ref (this);
  ref (owner);
  ref (link);
  /* _first_ check whether a currently uncrossing link (recursing from
   * container_uncross_link_R()) needs to be unlinked.
   */
  uncross_callback_stack_mutex.lock();
  for (UncrossNode *unode = uncross_callback_stack; unode; unode = unode->next)
    if (unode->mutable_container == this &&
        unode->clink->owner == &owner &&
        unode->clink->link == &link &&
        *unode->clink->uncross == *uncross.get_trampoline())
      {
        unode->mutable_container = NULL; /* prevent more cross_unlink() calls */
        found_one = true;
        break;
      }
  uncross_callback_stack_mutex.unlock();
  if (!found_one)
    {
      CrossLinks *clinks = get_data (&cross_links_key);
      for (CrossLink *last = NULL, *clink = clinks ? clinks->links : NULL; clink; last = clink, clink = last->next)
        if (clink->owner == &owner &&
            clink->link == &link &&
            *clink->uncross == *uncross.get_trampoline())
          {
            container_uncross_link_R (clinks->container, last ? &last->next : &clinks->links, false);
            found_one = true;
            break;
          }
    }
  if (!found_one)
    throw Exception ("no cross link from \"" + owner.name() + "\" to \"" + link.name() + "\" on \"" + name() + "\" to remove");
  unref (link);
  unref (owner);
  unref (this);
}

void
ContainerImpl::item_uncross_links (ItemImpl &owner,
                                   ItemImpl &link)
{
  ref (this);
  ref (owner);
  ref (link);
 restart_search:
  CrossLinks *clinks = get_data (&cross_links_key);
  for (CrossLink *last = NULL, *clink = clinks ? clinks->links : NULL; clink; last = clink, clink = last->next)
    if (clink->owner == &owner &&
        clink->link == &link)
      {
        container_uncross_link_R (clinks->container, last ? &last->next : &clinks->links);
        clinks = get_data (&cross_links_key);
        goto restart_search;
      }
  unref (link);
  unref (owner);
  unref (this);
}

static inline bool
item_has_ancestor (const ItemImpl *item,
                   const ItemImpl *ancestor)
{
  /* this duplicates item->has_ancestor() to optimize speed and
   * to cover the case where item == ancestor.
   */
  do
    if (item == ancestor)
      return true;
    else
      item = item->parent();
  while (item);
  return false;
}

void
ContainerImpl::uncross_descendant (ItemImpl &descendant)
{
  assert (descendant.has_ancestor (*this)); // could be disabled for performance
  ItemImpl *item = &descendant;
  ref (this);
  ref (item);
  ContainerImpl *cc = dynamic_cast<ContainerImpl*> (item);
 restart_search:
  CrossLinks *clinks = get_data (&cross_links_key);
  if (!cc || !cc->has_children()) /* suppress tree walks where possible */
    for (CrossLink *last = NULL, *clink = clinks ? clinks->links : NULL; clink; last = clink, clink = last->next)
      {
        if (clink->owner == item || clink->link == item)
          {
            container_uncross_link_R (clinks->container, last ? &last->next : &clinks->links);
            goto restart_search;
          }
      }
  else /* need to check whether item is ancestor of any of our cross-link items */
    {
      /* we do some minor hackery here, for optimization purposes. since item
       * is a descendant of this container, we don't need to walk ->owner's or
       * ->link's ancestor lists any further than up to reaching this container.
       * to suppress extra checks in item_has_ancestor() in this regard, we
       * simply set parent() to NULL temporarily and with that cause
       * item_has_ancestor() to return earlier.
       */
      ContainerImpl *saved_parent = *_parent_loc();
      *_parent_loc() = NULL;
      for (CrossLink *last = NULL, *clink = clinks ? clinks->links : NULL; clink; last = clink, clink = last->next)
        if (item_has_ancestor (clink->owner, item) ||
            item_has_ancestor (clink->link, item))
          {
            *_parent_loc() = saved_parent;
            container_uncross_link_R (clinks->container, last ? &last->next : &clinks->links);
            goto restart_search;
          }
      *_parent_loc() = saved_parent;
    }
  unref (item);
  unref (this);
}

static inline void
container_uncross_link_R (ContainerImpl *container,
                          CrossLink **clinkp,
                          bool        notify_callback)
{
  CrossLink *clink = *clinkp;
  /* remove cross link */
  *clinkp = clink->next;
  /* notify */
  if (notify_callback)
    {
      /* record execution */
      UncrossNode unode (container, clink);
      uncross_callback_stack_mutex.lock();
      unode.next = uncross_callback_stack;
      uncross_callback_stack = &unode;
      uncross_callback_stack_mutex.unlock();
      /* exec callback, note that this may recurse */
      (*clink->uncross) (*clink->link);
      /* unrecord execution */
      uncross_callback_stack_mutex.lock();
      UncrossNode *walk, *last = NULL;
      for (walk = uncross_callback_stack; walk; last = walk, walk = last->next)
        if (walk == &unode)
          {
            if (!last)
              uncross_callback_stack = unode.next;
            else
              last->next = unode.next;
            break;
          }
      uncross_callback_stack_mutex.unlock();
      assert (walk != NULL); /* paranoid */
    }
  /* delete cross link */
  delete clink;
}

/* --- ContainerImpl --- */
ContainerImpl::~ContainerImpl ()
{}

const CommandList&
ContainerImpl::list_commands()
{
  static Command *commands[] = {
  };
  static const CommandList command_list (commands, ItemImpl::list_commands());
  return command_list;
}

static DataKey<ContainerImpl*> child_container_key;

void
ContainerImpl::child_container (ContainerImpl *child_container)
{
  if (child_container && !child_container->has_ancestor (*this))
    throw Exception ("child container is not descendant of container \"", name(), "\": ", child_container->name());
  set_data (&child_container_key, child_container);
}

ContainerImpl&
ContainerImpl::child_container ()
{
  ContainerImpl *container = get_data (&child_container_key);
  if (!container)
    container = this;
  return *container;
}

void
ContainerImpl::add (ItemImpl &item)
{
  if (item.parent())
    throw Exception ("not adding item with parent: ", item.name());
  ContainerImpl &container = child_container();
  if (this != &container)
    {
      container.add (item);
      return;
    }
  item.ref();
  try {
    container.add_child (item);
    const PackInfo &pa = item.pack_info();
    PackInfo po = pa;
    po.hspan = po.vspan = 0; // indicate initial repack_child()
    container.repack_child (item, po, pa);
  } catch (...) {
    item.unref();
    throw;
  }
  /* can invalidate etc. the fully setup item now */
  item.invalidate();
  invalidate();
  item.unref();
}

void
ContainerImpl::add (ItemImpl *item)
{
  if (!item)
    throw NullPointer();
  add (*item);
}

void
ContainerImpl::remove (ItemImpl &item)
{
  ContainerImpl *container = item.parent();
  if (!container)
    throw NullPointer();
  item.ref();
  if (item.visible())
    {
      item.invalidate();
      invalidate();
    }
  ContainerImpl *dcontainer = container;
  while (dcontainer)
    {
      dcontainer->dispose_item (item);
      dcontainer = dcontainer->parent();
    }
  container->remove_child (item);
  item.invalidate();
  item.unref();
}

Affine
ContainerImpl::child_affine (const ItemImpl &item)
{
  return Affine(); // Identity
}

void
ContainerImpl::hierarchy_changed (ItemImpl *old_toplevel)
{
  ItemImpl::hierarchy_changed (old_toplevel);
  for (ChildWalker cw = local_children(); cw.has_next(); cw++)
    cw->sig_hierarchy_changed.emit (old_toplevel);
}

void
ContainerImpl::dispose_item (ItemImpl &item)
{
  if (&item == get_data (&child_container_key))
    child_container (NULL);
}

void
ContainerImpl::repack_child (ItemImpl       &item,
                             const PackInfo &orig,
                             const PackInfo &pnew)
{
  item.invalidate_parent();
}

static DataKey<ItemImpl*> focus_child_key;

void
ContainerImpl::unparent_child (ItemImpl &item)
{
  ref (this);
  if (&item == get_data (&focus_child_key))
    delete_data (&focus_child_key);
  ContainerImpl *ancestor = this;
  do
    {
      ancestor->uncross_descendant (item);
      ancestor = ancestor->parent();
    }
  while (ancestor);
  unref (this);
}

void
ContainerImpl::set_focus_child (ItemImpl *item)
{
  if (!item)
    delete_data (&focus_child_key);
  else
    {
      assert (item->parent() == this);
      set_data (&focus_child_key, item);
    }
}

ItemImpl*
ContainerImpl::get_focus_child () const
{
  return get_data (&focus_child_key);
}

struct LesserItemByHBand {
  bool
  operator() (ItemImpl *const &i1,
              ItemImpl *const &i2) const
  {
    const Allocation &a1 = i1->allocation();
    const Allocation &a2 = i2->allocation();
    // sort items by horizontal bands first
    if (a1.y + a1.height <= a2.y)
      return true;
    if (a1.y >= a2.y + a2.height)
      return false;
    // sort vertically overlapping items by horizontal position
    if (a1.x != a2.x)
      return a1.x < a2.x;
    // resort to center
    Point m1 (a1.x + a1.width * 0.5, a1.y + a1.height * 0.5);
    Point m2 (a2.x + a2.width * 0.5, a2.y + a2.height * 0.5);
    if (m1.y != m2.y)
      return m1.y < m2.y;
    else
      return m1.x < m2.x;
  }
};

struct LesserItemByDirection {
  FocusDirType dir;
  Point        anchor;
  LesserItemByDirection (FocusDirType d,
                         const Point &p) :
    dir (d), anchor (p)
  {}
  double
  directional_distance (const Allocation &a) const
  {
    switch (dir)
      {
      case FOCUS_RIGHT:
        return a.x - anchor.x;
      case FOCUS_UP:
        return anchor.y - (a.y + a.height);
      case FOCUS_LEFT:
        return anchor.x - (a.x + a.width);
      case FOCUS_DOWN:
        return a.y - anchor.y;
      default:
        return -1;      // unused
      }
  }
  bool
  operator() (ItemImpl *const &i1,
              ItemImpl *const &i2) const
  {
    // calculate item distances along dir, dist >= 0 lies ahead
    const Allocation &a1 = i1->allocation();
    const Allocation &a2 = i2->allocation();
    double dd1 = directional_distance (a1);
    double dd2 = directional_distance (a2);
    // sort items along dir
    if (dd1 != dd2)
      return dd1 < dd2;
    // same horizontal/vertical band distance, sort by closest edge distance
    dd1 = a1.dist (anchor);
    dd2 = a2.dist (anchor);
    if (dd1 != dd2)
      return dd1 < dd2;
    // same edge distance, resort to center distance
    dd1 = anchor.dist (Point (a1.x + a1.width * 0.5, a1.y + a1.height * 0.5));
    dd2 = anchor.dist (Point (a2.x + a2.width * 0.5, a2.y + a2.height * 0.5));
    return dd1 < dd2;
  }
};

static inline Point
rect_center (const Allocation &a)
{
  return Point (a.x + a.width * 0.5, a.y + a.height * 0.5);
}

bool
ContainerImpl::move_focus (FocusDirType fdir)
{
  /* check focus ability */
  if (!visible() || !sensitive())
    return false;
  /* focus self */
  if (!has_focus() && can_focus())
    return grab_focus();
  ItemImpl *last_child = get_data (&focus_child_key);
  /* let last focus descendant handle movement */
  if (last_child && last_child->move_focus (fdir))
    return true;
  /* copy children */
  vector<ItemImpl*> children;
  ChildWalker lw = local_children();
  while (lw.has_next())
    children.push_back (&*lw++);
  /* sort children according to direction and current focus */
  const Allocation &area = allocation();
  Point upper_left (area.x, area.y + area.height);
  Point lower_right (area.x + area.width, area.y);
  Point refpoint;
  switch (fdir)
    {
      ItemImpl *current;
    case FOCUS_NEXT:
      stable_sort (children.begin(), children.end(), LesserItemByHBand());
      break;
    case FOCUS_PREV:
      stable_sort (children.begin(), children.end(), LesserItemByHBand());
      reverse (children.begin(), children.end());
      break;
    case FOCUS_UP:
    case FOCUS_LEFT:
      current = get_window()->get_focus();
      refpoint = current ? rect_center (current->allocation()) : lower_right;
      { /* filter items with negative distance (not ahead in focus direction) */
        LesserItemByDirection lesseribd = LesserItemByDirection (fdir, refpoint);
        vector<ItemImpl*> children2;
        for (vector<ItemImpl*>::const_iterator it = children.begin(); it != children.end(); it++)
          if (lesseribd.directional_distance ((*it)->allocation()) >= 0)
            children2.push_back (*it);
        children.swap (children2);
        stable_sort (children.begin(), children.end(), lesseribd);
      }
      break;
    case FOCUS_RIGHT:
    case FOCUS_DOWN:
      current = get_window()->get_focus();
      refpoint = current ? rect_center (current->allocation()) : upper_left;
      { /* filter items with negative distance (not ahead in focus direction) */
        LesserItemByDirection lesseribd = LesserItemByDirection (fdir, refpoint);
        vector<ItemImpl*> children2;
        for (vector<ItemImpl*>::const_iterator it = children.begin(); it != children.end(); it++)
          if (lesseribd.directional_distance ((*it)->allocation()) >= 0)
            children2.push_back (*it);
        children.swap (children2);
        stable_sort (children.begin(), children.end(), lesseribd);
      }
      break;
    }
  /* skip children beyond last focus descendant */
  Walker<ItemImpl*> cw = walker (children);
  if (last_child && (fdir == FOCUS_NEXT || fdir == FOCUS_PREV))
    while (cw.has_next())
      if (last_child == *cw++)
        break;
  /* let remaining descendants handle movement */
  while (cw.has_next())
    {
      ItemImpl *child = *cw;
      if (child->move_focus (fdir))
        return true;
      cw++;
    }
  return false;
}

void
ContainerImpl::expose_enclosure ()
{
  /* expose without children */
  Region region (allocation());
  for (ChildWalker cw = local_children(); cw.has_next(); cw++)
    if (cw->drawable())
      {
        ItemImpl &child = *cw;
        Region cregion (child.allocation());
        cregion.affine (child_affine (child).invert());
        region.subtract (cregion);
      }
  expose (region);
}

void
ContainerImpl::point_children (Point               p, /* window coordinates relative */
                               std::vector<ItemImpl*> &stack)
{
  for (ChildWalker cw = local_children(); cw.has_next(); cw++)
    {
      ItemImpl &child = *cw;
      Point cp = child_affine (child).point (p);
      if (child.point (cp))
        {
          child.ref();
          stack.push_back (&child);
          ContainerImpl *cc = dynamic_cast<ContainerImpl*> (&child);
          if (cc)
            cc->point_children (cp, stack);
        }
    }
}

void
ContainerImpl::screen_window_point_children (Point                   p, /* screen_window coordinates relative */
                                             std::vector<ItemImpl*> &stack)
{
  point_children (point_from_screen_window (p), stack);
}

void
ContainerImpl::render_item (RenderContext &rcontext)
{
  ItemImpl::render_item (rcontext);
  for (ChildWalker cw = local_children(); cw.has_next(); cw++)
    {
      ItemImpl &child = *cw;
      if (child.drawable() && rendering_region (rcontext).contains (child.allocation()) != Region::OUTSIDE)
        {
          if (child.test_flags (INVALID_REQUISITION))
            critical ("rendering item with invalid %s: %s (%p)", "requisition", cw->name().c_str(), &child);
          if (child.test_flags (INVALID_ALLOCATION))
            critical ("rendering item with invalid %s: %s (%p)", "allocation", cw->name().c_str(), &child);
          child.render_item (rcontext);
        }
    }
}

void
ContainerImpl::debug_tree (String indent)
{
  printerr ("%s%s(%p) (%fx%f%+f%+f)\n", indent.c_str(), this->name().c_str(), this,
            allocation().width, allocation().height, allocation().x, allocation().y);
  for (ChildWalker cw = local_children(); cw.has_next(); cw++)
    {
      ItemImpl &child = *cw;
      ContainerImpl *c = dynamic_cast<ContainerImpl*> (&child);
      if (c)
        c->debug_tree (indent + "  ");
      else
        printerr ("  %s%s(%p) (%fx%f%+f%+f)\n", indent.c_str(), child.name().c_str(), &child,
                  child.allocation().width, child.allocation().height, child.allocation().x, child.allocation().y);
    }
}

void
ContainerImpl::dump_test_data (TestStream &tstream)
{
  for (ChildWalker cw = local_children(); cw.has_next(); cw++)
    cw->make_test_dump (tstream);
}

ItemIface*
ContainerImpl::create_child (const std::string   &item_identifier,
                             const StringSeq &args)
{
  ItemImpl &item = Factory::create_ui_item (item_identifier, args);
  ref_sink (item);
  try {
    add (item);
  } catch (...) {
    unref (item);
    return NULL;
  }
  unref (item);
  return &item;
}

SingleContainerImpl::SingleContainerImpl () :
  child_item (NULL)
{}

ContainerImpl::ChildWalker
SingleContainerImpl::local_children () const
{
  ItemImpl **iter = const_cast<ItemImpl**> (&child_item), **iend = iter;
  if (child_item)
    iend++;
  return value_walker (PointerIterator<ItemImpl*> (iter), PointerIterator<ItemImpl*> (iend));
}

void
SingleContainerImpl::add_child (ItemImpl &item)
{
  if (child_item)
    throw Exception ("invalid attempt to add child \"", item.name(), "\" to single-child container \"", name(), "\" ",
                     "which already has a child \"", child_item->name(), "\"");
  item.ref_sink();
  ClassDoctor::item_set_parent (item, this);
  child_item = &item;
}

void
SingleContainerImpl::remove_child (ItemImpl &item)
{
  assert (child_item == &item); /* ensured by remove() */
  child_item = NULL;
  ClassDoctor::item_set_parent (item, NULL);
  item.unref();
}

void
SingleContainerImpl::size_request_child (Requisition &requisition, bool *hspread, bool *vspread)
{
  bool chspread = false, cvspread = false;
  if (has_allocatable_child())
    {
      ItemImpl &child = get_child();
      Requisition cr = child.requisition ();
      const PackInfo &pi = child.pack_info();
      requisition.width = pi.left_spacing + cr.width + pi.right_spacing;
      requisition.height = pi.bottom_spacing + cr.height + pi.top_spacing;
      chspread = child.hspread();
      cvspread = child.vspread();
    }
  if (hspread)
    *hspread = chspread;
  else
    set_flag (HSPREAD_CONTAINER, chspread);
  if (vspread)
    *vspread = cvspread;
  else
    set_flag (VSPREAD_CONTAINER, cvspread);
}

void
SingleContainerImpl::size_request (Requisition &requisition)
{
  size_request_child (requisition, NULL, NULL);
}

Allocation
ContainerImpl::layout_child (ItemImpl         &child,
                             const Allocation &carea)
{
  Requisition rq = child.requisition();
  const PackInfo &pi = child.pack_info();
  Allocation area = carea;
  /* pad allocation */
  area.x += pi.left_spacing;
  area.width -= pi.left_spacing + pi.right_spacing;
  area.y += pi.bottom_spacing;
  area.height -= pi.bottom_spacing + pi.top_spacing;
  /* expand/scale child */
  if (area.width > rq.width && !child.hexpand())
    {
      int width = iround (rq.width + pi.hscale * (area.width - rq.width));
      area.x += iround (pi.halign * (area.width - width));
      area.width = width;
    }
  if (area.height > rq.height && !child.vexpand())
    {
      int height = iround (rq.height + pi.vscale * (area.height - rq.height));
      area.y += iround (pi.valign * (area.height - height));
      area.height = height;
    }
  return area;
}

void
SingleContainerImpl::size_allocate (Allocation area, bool changed)
{
  if (has_allocatable_child())
    {
      ItemImpl &child = get_child();
      Allocation child_area = layout_child (child, area);
      child.set_allocation (child_area);
    }
}

void
SingleContainerImpl::pre_finalize()
{
  while (child_item)
    remove (child_item);
  ContainerImpl::pre_finalize();
}

SingleContainerImpl::~SingleContainerImpl()
{
  while (child_item)
    remove (child_item);
}

ResizeContainerImpl::ResizeContainerImpl() :
  m_tunable_requisition_counter (0), m_resizer (0)
{
  m_anchor_info.resize_container = this;
  update_anchor_info();
}

ResizeContainerImpl::~ResizeContainerImpl()
{
  clear_exec (&m_resizer);
  m_anchor_info.resize_container = NULL;
}

void
ResizeContainerImpl::hierarchy_changed (ItemImpl *old_toplevel)
{
  update_anchor_info();
  SingleContainerImpl::hierarchy_changed (old_toplevel);
}

void
ResizeContainerImpl::update_anchor_info ()
{
  ItemImpl *last, *item;
  m_anchor_info.viewport = NULL;
  // find first ViewportImpl
  for (last = item = this; item && !m_anchor_info.viewport; last = item, item = last->parent())
    m_anchor_info.viewport = dynamic_cast<ViewportImpl*> (item);
  // find topmost parent
  item = last;
  while (item)
    last = item, item = last->parent();
  item = last;
  // assign window iff one is found
  m_anchor_info.window = dynamic_cast<WindowImpl*> (item);
}

static inline String
impl_type (ItemImpl *item)
{
  String tag;
  if (item)
    tag = Factory::factory_context_impl_type (item->factory_context());
  const size_t cpos = tag.rfind (':');
  return cpos != String::npos ? tag.c_str() + cpos + 1 : tag;
}

void
ResizeContainerImpl::idle_sizing ()
{
  assert_return (m_resizer != 0);
  m_resizer = 0;
  if (anchored() && drawable())
    {
      ContainerImpl *pc = parent();
      if (pc && pc->test_any_flag (INVALID_REQUISITION | INVALID_ALLOCATION))
        DEBUG_RESIZE ("%12s 0x%016zx, %s", impl_type (this).c_str(), size_t (this), "pass upwards...");
      else
        {
          Allocation area = allocation();
          negotiate_size (&area);
        }
    }
}

void
ResizeContainerImpl::negotiate_size (const Allocation *carea)
{
  assert_return (requisitions_tunable() == false); // prevent recursion
  const bool have_allocation = carea != NULL;
  Allocation area;
  if (have_allocation)
    {
      area = *carea;
      change_flags_silently (INVALID_ALLOCATION, true);
    }
  const bool need_debugging = debug_enabled() && test_flags (INVALID_REQUISITION | INVALID_ALLOCATION);
  if (need_debugging)
    DEBUG_RESIZE ("%12s 0x%016zx, %s", impl_type (this).c_str(), size_t (this),
                  !carea ? "probe..." : String ("assign: " + carea->string()).c_str());
  /* this is the core of the resizing loop. via Item.tune_requisition(), we
   * allow items to adjust the requisition from within size_allocate().
   * whether the tuned requisition is honored at all, depends on
   * m_tunable_requisition_counter.
   * currently, we simply freeze the allocation after 3 iterations. for the
   * future it's possible to honor the tuned requisition only partially or
   * proportionally as m_tunable_requisition_counter decreases, so to mimick
   * a simulated annealing process yielding the final layout.
   */
  m_tunable_requisition_counter = 3;
  while (test_flags (INVALID_REQUISITION | INVALID_ALLOCATION))
    {
      const Requisition creq = requisition(); // unsets INVALID_REQUISITION
      if (!have_allocation)
        {
          // seed allocation from requisition
          area.width = creq.width;
          area.height = creq.height;
        }
      set_allocation (area); // unsets INVALID_ALLOCATION, may re-::invalidate_size()
      if (m_tunable_requisition_counter)
        m_tunable_requisition_counter--;
    }
  m_tunable_requisition_counter = 0;
  if (need_debugging && !carea)
    DEBUG_RESIZE ("%12s 0x%016zx, %s", impl_type (this).c_str(), size_t (this), String ("result: " + area.string()).c_str());
}

void
ResizeContainerImpl::invalidate_parent ()
{
  if (anchored() && drawable())
    {
      if (!m_resizer)
        {
          WindowImpl *w = get_window();
          EventLoop *loop = w ? w->get_loop() : NULL;
          if (loop)
            m_resizer = loop->exec_timer (0, slot (*this, &ResizeContainerImpl::idle_sizing), WindowImpl::PRIORITY_RESIZE);
        }
      return;
    }
  SingleContainerImpl::invalidate_parent();
}

MultiContainerImpl::MultiContainerImpl ()
{}

void
MultiContainerImpl::add_child (ItemImpl &item)
{
  item.ref_sink();
  ClassDoctor::item_set_parent (item, this);
  items.push_back (&item);
}

void
MultiContainerImpl::remove_child (ItemImpl &item)
{
  vector<ItemImpl*>::iterator it;
  for (it = items.begin(); it != items.end(); it++)
    if (*it == &item)
      {
        items.erase (it);
        ClassDoctor::item_set_parent (item, NULL);
        item.unref();
        return;
      }
  assert_unreached();
}

void
MultiContainerImpl::raise_child (ItemImpl &item)
{
  for (uint i = 0; i < items.size(); i++)
    if (items[i] == &item)
      {
        if (i + 1 != items.size())
          {
            items.erase (items.begin() + i);
            items.push_back (&item);
            invalidate();
          }
        break;
      }
}

void
MultiContainerImpl::lower_child (ItemImpl &item)
{
  for (uint i = 0; i < items.size(); i++)
    if (items[i] == &item)
      {
        if (i != 0)
          {
            items.erase (items.begin() + i);
            items.insert (items.begin(), &item);
            invalidate();
          }
        break;
      }
}

void
MultiContainerImpl::remove_all_children ()
{
  while (items.size())
    remove (*items[items.size() - 1]);
}

void
MultiContainerImpl::pre_finalize()
{
  remove_all_children();
  ContainerImpl::pre_finalize();
}

MultiContainerImpl::~MultiContainerImpl()
{
  while (items.size())
    remove (*items[items.size() - 1]);
}

} // Rapicorn
