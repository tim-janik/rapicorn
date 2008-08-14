/* Rapicorn
 * Copyright (C) 2005 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * A copy of the GNU Lesser General Public License should ship along
 * with this library; if not, see http://www.gnu.org/copyleft/.
 */
#include "container.hh"
#include "containerimpl.hh"
#include "root.hh"

namespace Rapicorn {

struct ClassDoctor {
  static void item_set_parent (Item &item, Container *parent) { item.set_parent (parent); }
};

/* --- CrossLinks --- */
struct CrossLink {
  Item                             *owner, *link;
  Signals::Trampoline1<void,Item&> *uncross;
  CrossLink                        *next;
  explicit      CrossLink (Item                             *o,
                           Item                             *l,
                           Signals::Trampoline1<void,Item&> &it) :
    owner (o), link (l),
    uncross (ref_sink (&it)), next (NULL)
  {}
  /*Des*/       ~CrossLink()
  {
    unref (uncross);
  }
  RAPICORN_PRIVATE_CLASS_COPY (CrossLink);
};
struct CrossLinks {
  Container *container;
  CrossLink *links;
};
static inline void      container_uncross_link_R        (Container *container,
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
  Container   *mutable_container;
  CrossLink   *clink;
  explicit      UncrossNode (Container *xcontainer,
                             CrossLink *xclink) :
    next (NULL), mutable_container (xcontainer), clink (xclink)
  {}
  RAPICORN_PRIVATE_CLASS_COPY (UncrossNode);
};
static UncrossNode *uncross_callback_stack = NULL;
static Mutex        uncross_callback_stack_mutex;

void
Container::item_cross_link (Item           &owner,
                            Item           &link,
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
Container::item_cross_unlink (Item           &owner,
                              Item           &link,
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
Container::item_uncross_links (Item &owner,
                               Item &link)
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
item_has_ancestor (const Item *item,
                   const Item *ancestor)
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
Container::uncross_descendant (Item &descendant)
{
  assert (descendant.has_ancestor (*this)); // could be disabled for performance
  Item *item = &descendant;
  ref (this);
  ref (item);
  Container *cc = dynamic_cast<Container*> (item);
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
      Container *saved_parent = *_parent_loc();
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
container_uncross_link_R (Container *container,
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

/* --- Container --- */
Container::~Container ()
{}

const PropertyList&
Container::list_properties()
{
  static Property *properties[] = {
  };
  static const PropertyList property_list (properties, Item::list_properties());
  return property_list;
}

const CommandList&
Container::list_commands()
{
  static Command *commands[] = {
  };
  static const CommandList command_list (commands, Item::list_commands());
  return command_list;
}

static DataKey<Container*> child_container_key;

void
Container::child_container (Container *child_container)
{
  if (child_container && !child_container->has_ancestor (*this))
    throw Exception ("child container is not descendant of container \"", name(), "\": ", child_container->name());
  set_data (&child_container_key, child_container);
}

Container&
Container::child_container ()
{
  Container *container = get_data (&child_container_key);
  if (!container)
    container = this;
  return *container;
}

void
Container::add (Item &item)
{
  if (item.parent())
    throw Exception ("not adding item with parent: ", item.name());
  Container &container = child_container();
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
Container::add (Item *item)
{
  if (!item)
    throw NullPointer();
  add (*item);
}

void
Container::remove (Item &item)
{
  Container *container = item.parent();
  if (!container)
    throw NullPointer();
  item.ref();
  if (item.visible())
    {
      item.invalidate();
      invalidate();
    }
  Container *dcontainer = container;
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
Container::child_affine (const Item &item)
{
  return Affine(); // Identity
}

void
Container::hierarchy_changed (Item *old_toplevel)
{
  Item::hierarchy_changed (old_toplevel);
  for (ChildWalker cw = local_children(); cw.has_next(); cw++)
    cw->sig_hierarchy_changed.emit (old_toplevel);
}

void
Container::dispose_item (Item &item)
{
  if (&item == get_data (&child_container_key))
    child_container (NULL);
}

void
Container::repack_child (Item           &item,
                         const PackInfo &orig,
                         const PackInfo &pnew)
{}

static DataKey<Item*> focus_child_key;

void
Container::unparent_child (Item &item)
{
  ref (this);
  if (&item == get_data (&focus_child_key))
    delete_data (&focus_child_key);
  Container *ancestor = this;
  do
    {
      ancestor->uncross_descendant (item);
      ancestor = ancestor->parent();
    }
  while (ancestor);
  unref (this);
}

void
Container::set_focus_child (Item *item)
{
  if (!item)
    delete_data (&focus_child_key);
  else
    {
      assert (item->parent() == this);
      set_data (&focus_child_key, item);
    }
}

Item*
Container::get_focus_child () const
{
  return get_data (&focus_child_key);
}

struct LesserItemByHBand {
  bool
  operator() (Item *const &i1,
              Item *const &i2) const
  {
    const Allocation &a1 = i1->allocation();
    const Allocation &a2 = i2->allocation();
    /* sort items by horizontal bands first */
    if (a1.y >= a2.y + a2.height)
      return true;
    if (a1.y + a1.height <= a2.y)
      return false;
    /* sort items with overlapping horizontal bands by vertical position */
    if (a1.x != a2.x)
      return a1.x < a2.x;
    /* resort to center */
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
  Point        middle;
  LesserItemByDirection (FocusDirType d,
                         const Point &p) :
    dir (d), middle (p)
  {}
  double
  directional_distance (const Allocation &a) const
  {
    switch (dir)
      {
      case FOCUS_RIGHT:
        return a.x - middle.x;
      case FOCUS_UP:
        return a.y - middle.y;
      case FOCUS_LEFT:
        return middle.x - (a.x + a.width);
      case FOCUS_DOWN:
        return middle.y - (a.y + a.height);
      default:
        return -1;      /* unused */
      }
  }
  bool
  operator() (Item *const &i1,
              Item *const &i2) const
  {
    /* calculate item distances along dir, dist >= 0 lies ahead */
    const Allocation &a1 = i1->allocation();
    const Allocation &a2 = i2->allocation();
    double dd1 = directional_distance (a1);
    double dd2 = directional_distance (a2);
    /* sort items along dir */
    if (dd1 != dd2)
      return dd1 < dd2;
    /* same horizontal/vertical band distance, sort by closest edge distance */
    dd1 = a1.dist (middle);
    dd2 = a2.dist (middle);
    if (dd1 != dd2)
      return dd1 < dd2;
    /* same edge distance, resort to center distance */
    dd1 = middle.dist (Point (a1.x + a1.width * 0.5, a1.y + a1.height * 0.5));
    dd2 = middle.dist (Point (a2.x + a2.width * 0.5, a2.y + a2.height * 0.5));
    return dd1 < dd2;
  }
};

static inline Point
rect_center (const Allocation &a)
{
  return Point (a.x + a.width * 0.5, a.y + a.height * 0.5);
}

bool
Container::move_focus (FocusDirType fdir)
{
  /* check focus ability */
  if (!visible() || !sensitive())
    return false;
  /* focus self */
  if (!has_focus() && can_focus())
    return grab_focus();
  Item *last_child = get_data (&focus_child_key);
  /* let last focus descendant handle movement */
  if (last_child && last_child->move_focus (fdir))
    return true;
  /* copy children */
  vector<Item*> children;
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
      Item *current;
    case FOCUS_NEXT:
      stable_sort (children.begin(), children.end(), LesserItemByHBand());
      break;
    case FOCUS_PREV:
      stable_sort (children.begin(), children.end(), LesserItemByHBand());
      reverse (children.begin(), children.end());
      break;
    case FOCUS_UP:
    case FOCUS_LEFT:
      current = get_root()->get_focus();
      refpoint = current ? rect_center (current->allocation()) : lower_right;
      { /* filter items with negative distance (not ahead in focus direction) */
        LesserItemByDirection lesseribd = LesserItemByDirection (fdir, refpoint);
        vector<Item*> children2;
        for (vector<Item*>::const_iterator it = children.begin(); it != children.end(); it++)
          if (lesseribd.directional_distance ((*it)->allocation()) >= 0)
            children2.push_back (*it);
        children.swap (children2);
        stable_sort (children.begin(), children.end(), lesseribd);
      }
      break;
    case FOCUS_RIGHT:
    case FOCUS_DOWN:
      current = get_root()->get_focus();
      refpoint = current ? rect_center (current->allocation()) : upper_left;
      { /* filter items with negative distance (not ahead in focus direction) */
        LesserItemByDirection lesseribd = LesserItemByDirection (fdir, refpoint);
        vector<Item*> children2;
        for (vector<Item*>::const_iterator it = children.begin(); it != children.end(); it++)
          if (lesseribd.directional_distance ((*it)->allocation()) >= 0)
            children2.push_back (*it);
        children.swap (children2);
        stable_sort (children.begin(), children.end(), lesseribd);
      }
      break;
    }
  /* skip children beyond last focus descendant */
  Walker<Item*> cw = walker (children);
  if (last_child && (fdir == FOCUS_NEXT || fdir == FOCUS_PREV))
    while (cw.has_next())
      if (last_child == *cw++)
        break;
  /* let remaining descendants handle movement */
  while (cw.has_next())
    {
      Item *child = *cw;
      if (child->move_focus (fdir))
        return true;
      cw++;
    }
  return false;
}

void
Container::expose_enclosure ()
{
  /* expose without children */
  Region region (allocation());
  for (ChildWalker cw = local_children(); cw.has_next(); cw++)
    if (cw->drawable())
      {
        Item &child = *cw;
        Region cregion (child.allocation());
        cregion.affine (child_affine (child).invert());
        region.subtract (cregion);
      }
  expose (region);
}

void
Container::point_children (Point               p, /* root coordinates relative */
                           std::vector<Item*> &stack)
{
  for (ChildWalker cw = local_children(); cw.has_next(); cw++)
    {
      Item &child = *cw;
      Point cp = child_affine (child).point (p);
      if (child.point (cp))
        {
          child.ref();
          stack.push_back (&child);
          Container *cc = dynamic_cast<Container*> (&child);
          if (cc)
            cc->point_children (cp, stack);
        }
    }
}

void
Container::viewport_point_children (Point                   p, /* viewport coordinates relative */
                                    std::vector<Item*>     &stack)
{
  point_children (point_from_viewport (p), stack);
}

bool
Container::match_interface (InterfaceMatch &imatch) const
{
  if (imatch.done() || imatch.match (const_cast<Container*> (this), name()))
    return true;
  for (ChildWalker cw = local_children(); cw.has_next(); cw++)
    if (cw->match_interface (imatch))
      break;
  return imatch.done();
}

void
Container::render (Display &display)
{
  for (ChildWalker cw = local_children(); cw.has_next(); cw++)
    {
      if (!cw->drawable())
        continue;
      const IRect ia = cw->allocation();
      display.push_clip_rect (ia.x, ia.y, ia.width, ia.height);
      if (cw->test_flags (INVALID_REQUISITION))
        warning ("rendering item with invalid %s: %s (%p)", "requisition", cw->name().c_str(), &*cw);
      if (cw->test_flags (INVALID_ALLOCATION))
        warning ("rendering item with invalid %s: %s (%p)", "allocation", cw->name().c_str(), &*cw);
      if (!display.empty())
        cw->render (display);
      display.pop_clip_rect();
    }
}

void
Container::debug_tree (String indent)
{
  printf ("%s%s(%p) (%fx%f%+f%+f)\n", indent.c_str(), this->name().c_str(), this,
          allocation().width, allocation().height, allocation().x, allocation().y);
  for (ChildWalker cw = local_children(); cw.has_next(); cw++)
    {
      Item &child = *cw;
      Container *c = dynamic_cast<Container*> (&child);
      if (c)
        c->debug_tree (indent + "  ");
      else
        printf ("  %s%s(%p) (%fx%f%+f%+f)\n", indent.c_str(), child.name().c_str(), &child,
                child.allocation().width, child.allocation().height, child.allocation().x, child.allocation().y);
    }
}

SingleContainerImpl::SingleContainerImpl () :
  child_item (NULL)
{}

Container::ChildWalker
SingleContainerImpl::local_children () const
{
  Item **iter = const_cast<Item**> (&child_item), **iend = iter;
  if (child_item)
    iend++;
  return value_walker (PointerIterator<Item*> (iter), PointerIterator<Item*> (iend));
}

void
SingleContainerImpl::add_child (Item &item)
{
  if (child_item)
    throw Exception ("invalid attempt to add child \"", item.name(), "\" to single-child container \"", name(), "\" ",
                     "which already has a child \"", child_item->name(), "\"");
  item.ref_sink();
  ClassDoctor::item_set_parent (item, this);
  child_item = &item;
}

void
SingleContainerImpl::remove_child (Item &item)
{
  assert (child_item == &item); /* ensured by remove() */
  child_item = NULL;
  ClassDoctor::item_set_parent (item, NULL);
  item.unref();
}

void
SingleContainerImpl::size_request (Requisition &requisition)
{
  bool chspread = false, cvspread = false;
  if (has_allocatable_child())
    {
      Item &child = get_child();
      Requisition cr = child.size_request ();
      const PackInfo &pi = child.pack_info();
      requisition.width = pi.left_spacing + cr.width + pi.right_spacing;
      requisition.height = pi.bottom_spacing + cr.height + pi.top_spacing;
      chspread = child.hspread();
      cvspread = child.vspread();
    }
  set_flag (HSPREAD_CONTAINER, chspread);
  set_flag (VSPREAD_CONTAINER, cvspread);
}

void
SingleContainerImpl::size_allocate (Allocation area)
{
  allocation (area);
  if (has_allocatable_child())
    {
      Item &child = get_child();
      Requisition rq = child.size_request();
      const PackInfo &pi = child.pack_info();
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
      child.set_allocation (area);
    }
}

void
SingleContainerImpl::pre_finalize()
{
  while (child_item)
    remove (child_item);
  Container::pre_finalize();
}

SingleContainerImpl::~SingleContainerImpl()
{
  while (child_item)
    remove (child_item);
}

MultiContainerImpl::MultiContainerImpl ()
{}

void
MultiContainerImpl::add_child (Item &item)
{
  item.ref_sink();
  ClassDoctor::item_set_parent (item, this);
  items.push_back (&item);
}

void
MultiContainerImpl::remove_child (Item &item)
{
  vector<Item*>::iterator it;
  for (it = items.begin(); it != items.end(); it++)
    if (*it == &item)
      {
        items.erase (it);
        ClassDoctor::item_set_parent (item, NULL);
        item.unref();
        return;
      }
  assert_not_reached();
}

void
MultiContainerImpl::pre_finalize()
{
  while (items.size())
    remove (*items[items.size() - 1]);
  Container::pre_finalize();
}

MultiContainerImpl::~MultiContainerImpl()
{
  while (items.size())
    remove (*items[items.size() - 1]);
}

} // Rapicorn
