// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#include "item.hh"
#include "container.hh"
#include "adjustment.hh"
#include "window.hh"
#include "cmdlib.hh"
#include "sizegroup.hh"
#include "factory.hh"
#include "selector.hh"

#define SZDEBUG(...)    RAPICORN_KEY_DEBUG ("Sizing", __VA_ARGS__)

namespace Rapicorn {

struct ClassDoctor {
  static void update_item_heritage (ItemImpl &item) { item.heritage (item.heritage()); }
};

EventHandler::EventHandler() :
  sig_event (*this, &EventHandler::handle_event)
{}

bool
EventHandler::handle_event (const Event &event)
{
  return false;
}

ItemImpl&
ItemIface::impl ()
{
  ItemImpl *iimpl = dynamic_cast<ItemImpl*> (this);
  if (!iimpl)
    throw std::bad_cast();
  return *iimpl;
}

const ItemImpl&
ItemIface::impl () const
{
  const ItemImpl *iimpl = dynamic_cast<const ItemImpl*> (this);
  if (!iimpl)
    throw std::bad_cast();
  return *iimpl;
}

ItemImpl::ItemImpl () :
  m_flags (VISIBLE | SENSITIVE | ALLOCATABLE),
  m_parent (NULL),
  m_heritage (NULL),
  m_factory_context (NULL), // removing this breaks g++ pre-4.2.0 20060530
  sig_finalize (*this),
  sig_changed (*this, &ItemImpl::do_changed),
  sig_invalidate (*this, &ItemImpl::do_invalidate),
  sig_hierarchy_changed (*this, &ItemImpl::hierarchy_changed)
{}

void
ItemImpl::constructed()
{}

bool
ItemImpl::viewable() const
{
  return drawable() && (!m_parent || m_parent->viewable());
}

bool
ItemImpl::self_visible () const
{
  return false;
}

bool
ItemImpl::change_flags_silently (uint32 mask,
                                 bool   on)
{
  uint32 old_flags = m_flags;
  if (on)
    m_flags |= mask;
  else
    m_flags &= ~mask;
  /* omit change notification */
  return old_flags != m_flags;
}

void
ItemImpl::propagate_state (bool notify_changed)
{
  change_flags_silently (PARENT_SENSITIVE, !parent() || parent()->sensitive());
  const bool wasallocatable = allocatable();
  change_flags_silently (PARENT_VISIBLE, self_visible() ||
                         (parent() && parent()->test_all_flags (VISIBLE | PARENT_VISIBLE)));
  if (wasallocatable != allocatable())
    invalidate();
  ContainerImpl *container = dynamic_cast<ContainerImpl*> (this);
  if (container)
    for (ContainerImpl::ChildWalker it = container->local_children(); it.has_next(); it++)
      it->propagate_state (notify_changed);
  if (notify_changed && !finalizing())
    sig_changed.emit(); /* notify changed() without invalidate() */
}

void
ItemImpl::set_flag (uint32 flag, bool on)
{
  assert ((flag & (flag - 1)) == 0); /* single bit check */
  const uint propagate_flag_mask = (SENSITIVE | PARENT_SENSITIVE | PRELIGHT | IMPRESSED | HAS_DEFAULT |
                                    VISIBLE | PARENT_VISIBLE);
  const uint repack_flag_mask = HEXPAND | VEXPAND | HSPREAD | VSPREAD |
                                HSPREAD_CONTAINER | VSPREAD_CONTAINER |
                                HSHRINK | VSHRINK | VISIBLE;
  bool fchanged = change_flags_silently (flag, on);
  if (fchanged)
    {
      if (flag & propagate_flag_mask)
        {
          expose();
          propagate_state (false);
        }
      if (flag & repack_flag_mask)
        {
          const PackInfo &pa = pack_info();
          repack (pa, pa); // includes invalidate();
          invalidate_parent(); // request resize even if flagged as invalid already
        }
      changed();
    }
}

bool
ItemImpl::grab_default () const
{
  return false;
}

void
ItemImpl::sensitive (bool b)
{
  set_flag (SENSITIVE, b);
}

void
ItemImpl::prelight (bool b)
{
  set_flag (PRELIGHT, b);
}

bool
ItemImpl::branch_prelight () const
{
  const ItemImpl *item = this;
  while (item)
    {
      if (item->prelight())
        return true;
      item = item->parent();
    }
  return false;
}

void
ItemImpl::impressed (bool b)
{
  set_flag (IMPRESSED, b);
}

bool
ItemImpl::branch_impressed () const
{
  const ItemImpl *item = this;
  while (item)
    {
      if (item->impressed())
        return true;
      item = item->parent();
    }
  return false;
}

StateType
ItemImpl::state () const
{
  StateType st = StateType (0);
  st |= insensitive() ? STATE_INSENSITIVE : StateType (0);
  st |= prelight()    ? STATE_PRELIGHT : StateType (0);
  st |= impressed()   ? STATE_IMPRESSED : StateType (0);
  st |= has_focus()   ? STATE_FOCUS : StateType (0);
  st |= has_default() ? STATE_DEFAULT : StateType (0);
  return st;
}

bool
ItemImpl::has_focus () const
{
  if (test_flags (FOCUS_CHAIN))
    {
      WindowImpl *ritem = get_window();
      if (ritem && ritem->get_focus() == this)
        return true;
    }
  return false;
}

bool
ItemImpl::can_focus () const
{
  return false;
}

bool
ItemImpl::grab_focus ()
{
  if (has_focus())
    return true;
  if (!can_focus() || !sensitive() || !viewable())
    return false;
  /* unset old focus */
  WindowImpl *ritem = get_window();
  if (ritem)
    ritem->set_focus (NULL);
  /* set new focus */
  ritem = get_window();
  if (ritem && ritem->get_focus() == NULL)
    ritem->set_focus (this);
  return ritem->get_focus() == this;
}

bool
ItemImpl::move_focus (FocusDirType fdir)
{
  if (!has_focus() && can_focus())
    return grab_focus();
  return false;
}

void
ItemImpl::notify_key_error ()
{
  WindowImpl *ritem = get_window();
  if (ritem)
    ritem->beep();
}

void
ItemImpl::cross_link (ItemImpl           &link,
                      const ItemSlot &uncross)
{
  assert (this != &link);
  ContainerImpl *common_container = dynamic_cast<ContainerImpl*> (common_ancestor (link));
  assert (common_container != NULL);
  common_container->item_cross_link (*this, link, uncross);
}

void
ItemImpl::cross_unlink (ItemImpl           &link,
                        const ItemSlot &uncross)
{
  assert (this != &link);
  ContainerImpl *common_container = dynamic_cast<ContainerImpl*> (common_ancestor (link));
  assert (common_container != NULL);
  common_container->item_cross_unlink (*this, link, uncross);
}

void
ItemImpl::uncross_links (ItemImpl &link)
{
  assert (this != &link);
  ContainerImpl *common_container = dynamic_cast<ContainerImpl*> (common_ancestor (link));
  assert (common_container != NULL);
  common_container->item_uncross_links (*this, link);
}

bool
ItemImpl::match_interface (bool wself, bool wparent, bool children, InterfaceMatcher &imatcher) const
{
  ItemImpl *self = const_cast<ItemImpl*> (this);
  if (wself && imatcher.match (self, name()))
    return true;
  if (wparent)
    {
      ItemImpl *pitem = parent();
      while (pitem)
        {
          if (imatcher.match (pitem, pitem->name()))
            return true;
          pitem = pitem->parent();
        }
    }
  if (children)
    {
      ContainerImpl *container = dynamic_cast<ContainerImpl*> (self);
      if (container)
        for (ContainerImpl::ChildWalker cw = container->local_children(); cw.has_next(); cw++)
          if (cw->match_interface (1, 0, 1, imatcher))
            return true;
    }
  return false;
}

bool
ItemImpl::match_selector (const String &selector)
{
  return Selector::Matcher::match_selector (selector, *this);
}

ItemIface*
ItemImpl::query_selector (const String &selector)
{
  return Selector::Matcher::query_selector_first (selector, *this);
}

ItemSeq
ItemImpl::query_selector_all (const String &selector)
{
  vector<ItemImpl*> result = Selector::Matcher::query_selector_all (selector, *this);
  ItemSeq items;
  for (vector<ItemImpl*>::const_iterator it = result.begin(); it != result.end(); it++)
    items.push_back (*it);
  return items;
}

ItemIface*
ItemImpl::query_selector_unique (const String &selector)
{
  return Selector::Matcher::query_selector_unique (selector, *this);
}

uint
ItemImpl::exec_slow_repeater (const BoolSlot &sl)
{
  WindowImpl *ritem = get_window();
  if (ritem)
    {
      EventLoop *loop = ritem->get_loop();
      if (loop)
        return loop->exec_timer (250, 50, sl);
    }
  return 0;
}

uint
ItemImpl::exec_fast_repeater (const BoolSlot &sl)
{
  WindowImpl *ritem = get_window();
  if (ritem)
    {
      EventLoop *loop = ritem->get_loop();
      if (loop)
        return loop->exec_timer (200, 20, sl);
    }
  return 0;
}

uint
ItemImpl::exec_key_repeater (const BoolSlot &sl)
{
  WindowImpl *ritem = get_window();
  if (ritem)
    {
      EventLoop *loop = ritem->get_loop();
      if (loop)
        return loop->exec_timer (250, 33, sl);
    }
  return 0;
}

bool
ItemImpl::remove_exec (uint exec_id)
{
  WindowImpl *ritem = get_window();
  if (ritem)
    {
      EventLoop *loop = ritem->get_loop();
      if (loop)
        return loop->try_remove (exec_id);
    }
  return false;
}

bool
ItemImpl::clear_exec (uint *exec_id)
{
  assert_return (exec_id != NULL, false);
  bool removed = false;
  if (*exec_id)
    removed = remove_exec (*exec_id);
  *exec_id = 0;
  return removed;
}

static DataKey<uint> visual_update_key;

void
ItemImpl::queue_visual_update ()
{
  uint timer_id = get_data (&visual_update_key);
  if (!timer_id)
    {
      WindowImpl *ritem = get_window();
      if (ritem)
        {
          EventLoop *loop = ritem->get_loop();
          if (loop)
            {
              timer_id = loop->exec_timer (20, slot (*this, &ItemImpl::force_visual_update));
              set_data (&visual_update_key, timer_id);
            }
        }
    }
}

void
ItemImpl::force_visual_update ()
{
  uint timer_id = get_data (&visual_update_key);
  if (timer_id)
    {
      remove_exec (timer_id);
      set_data (&visual_update_key, uint (0));
    }
  visual_update();
}

void
ItemImpl::visual_update ()
{}

void
ItemImpl::finalize()
{
  sig_finalize.emit();
}

ItemImpl::~ItemImpl()
{
  SizeGroup::delete_item (*this);
  if (parent())
    parent()->remove (this);
  if (m_heritage)
    {
      m_heritage->unref();
      m_heritage = NULL;
    }
  uint timer_id = get_data (&visual_update_key);
  if (timer_id)
    {
      remove_exec (timer_id);
      set_data (&visual_update_key, uint (0));
    }
}

OwnedMutex&
ItemImpl::owned_mutex ()
{
  if (m_parent)
    return m_parent->owned_mutex();
  else
    return Thread::Self::owned_mutex();
}

Command*
ItemImpl::lookup_command (const String &command_name)
{
  typedef std::map<const String, Command*> CommandMap;
  static std::map<const CommandList*,CommandMap*> clist_map;
  /* find/construct command map */
  const CommandList &clist = list_commands();
  CommandMap *cmap = clist_map[&clist];
  if (!cmap)
    {
      cmap = new CommandMap;
      for (uint i = 0; i < clist.n_commands; i++)
        (*cmap)[clist.commands[i]->ident] = clist.commands[i];
      clist_map[&clist] = cmap;
    }
  CommandMap::iterator it = cmap->find (command_name);
  if (it != cmap->end())
    return it->second;
  else
    return NULL;
}

bool
ItemImpl::exec_command (const String &command_call_string)
{
  String cmd_name;
  StringList args;
  if (!command_scan (command_call_string, &cmd_name, &args))
    {
      critical ("Invalid command syntax: %s", command_call_string.c_str());
      return false;
    }
  cmd_name = string_strip (cmd_name);
  if (cmd_name == "")
    return true;

  ItemImpl *item = this;
  while (item)
    {
      Command *cmd = item->lookup_command (cmd_name);
      if (cmd)
        return cmd->exec (item, args);
      item = item->parent();
    }

  if (command_lib_exec (*this, cmd_name, args))
    return true;

  item = this;
  while (item)
    {
      if (item->custom_command (cmd_name, args))
        return true;
      item = item->parent();
    }

  critical ("Command unimplemented: %s", command_call_string.c_str());
  return false;
}

bool
ItemImpl::custom_command (const String       &command_name,
                      const StringList   &command_args)
{
  return false;
}

const CommandList&
ItemImpl::list_commands ()
{
  static Command *commands[] = {
  };
  static const CommandList command_list (commands);
  return command_list;
}

Property*
ItemImpl::lookup_property (const String &property_name)
{
  typedef std::map<const String, Property*> PropertyMap;
  static std::map<const PropertyList*,PropertyMap*> plist_map;
  /* find/construct property map */
  const PropertyList &plist = list_properties();
  PropertyMap *pmap = plist_map[&plist];
  if (!pmap)
    {
      pmap = new PropertyMap;
      for (uint i = 0; i < plist.n_properties; i++)
        (*pmap)[plist.properties[i]->ident] = plist.properties[i];
      plist_map[&plist] = pmap;
    }
  PropertyMap::iterator it = pmap->find (property_name);
  if (it == pmap->end())        // try canonicalized
    it = pmap->find (string_substitute_char (property_name, '-', '_'));
  if (it != pmap->end())
    return it->second;
  else
    return NULL;
}

void
ItemImpl::set_property (const String    &property_name,
                    const String    &value,
                    const nothrow_t &nt)
{
  Property *prop = lookup_property (property_name);
  if (prop)
    prop->set_value (this, value);
  else if (&nt == &dothrow)
    throw Exception ("no such property: " + name() + "::" + property_name);
}

bool
ItemImpl::try_set_property (const String    &property_name,
                        const String    &value,
                        const nothrow_t &nt)
{
  Property *prop = lookup_property (property_name);
  if (prop)
    {
      prop->set_value (this, value);
      return true;
    }
  else
    return false;
}

String
ItemImpl::get_property (const String   &property_name)
{
  Property *prop = lookup_property (property_name);
  if (!prop)
    throw Exception ("no such property: " + name() + "::" + property_name);
  return prop->get_value (this);
}

static class OvrKey : public DataKey<Requisition> {
  Requisition
  fallback()
  {
    return Requisition (-1, -1);
  }
} override_requisition;

double
ItemImpl::width () const
{
  Requisition ovr = get_data (&override_requisition);
  return ovr.width >= 0 ? ovr.width : -1;
}

void
ItemImpl::width (double w)
{
  Requisition ovr = get_data (&override_requisition);
  ovr.width = w >= 0 ? w : -1;
  set_data (&override_requisition, ovr);
  invalidate_size();
}

double
ItemImpl::height () const
{
  Requisition ovr = get_data (&override_requisition);
  return ovr.height >= 0 ? ovr.height : -1;
}

void
ItemImpl::height (double h)
{
  Requisition ovr = get_data (&override_requisition);
  ovr.height = h >= 0 ? h : -1;
  set_data (&override_requisition, ovr);
  invalidate_size();
}

const PropertyList&
ItemImpl::list_properties ()
{
  static Property *properties[] = {
    MakeProperty (ItemImpl, name,      _("Name"), _("Identification name of the item"), "rw"),
    MakeProperty (ItemImpl, width,     _("Requested Width"), _("The width to request from its container for this item, -1=automatic"), -1, MAXINT, 5, "rw"),
    MakeProperty (ItemImpl, height,    _("Requested Height"), _("The height to request from its container for this item, -1=automatic"), -1, MAXINT, 5, "rw"),
    MakeProperty (ItemImpl, visible,   _("Visible"), _("Whether this item is visible"), "rw"),
    MakeProperty (ItemImpl, sensitive, _("Sensitive"), _("Whether this item is sensitive (receives events)"), "rw"),
    MakeProperty (ItemImpl, color_scheme, _("Color Scheme"), _("Color scheme to render this item"), "rw"),
    /* packing */
    MakeProperty (ItemImpl, hexpand,   _("Horizontal Expand"), _("Whether to expand this item horizontally"), "rw"),
    MakeProperty (ItemImpl, vexpand,   _("Vertical Expand"), _("Whether to expand this item vertically"), "rw"),
    MakeProperty (ItemImpl, hspread,   _("Horizontal Spread"), _("Whether to expand this item and all its parents horizontally"), "rw"),
    MakeProperty (ItemImpl, vspread,   _("Vertical Spread"), _("Whether to expand this item and all its parents vertically"), "rw"),
    MakeProperty (ItemImpl, hshrink,   _("Horizontal Shrink"), _("Whether the item may be shrunken horizontally"), "rw"),
    MakeProperty (ItemImpl, vshrink,   _("Vertical Shrink"),   _("Whether the item may be shrunken vertically"), "rw"),
    MakeProperty (ItemImpl, hposition, _("Horizontal Position"), _("Horizontal layout position for the item"), 0u, 99999u, 5u, "Prw"),
    MakeProperty (ItemImpl, hspan,     _("Horizontal Span"),     _("Horizontal span for item layout"), 1u, 100000u, 5u, "Prw"),
    MakeProperty (ItemImpl, vposition, _("Vertical Position"),   _("Vertical layout position for the item"), 0u, 99999u, 5u, "Prw"),
    MakeProperty (ItemImpl, vspan,     _("Vertical Span"),       _("Vertical span for item layout"), 1u, 100000u, 5u, "Prw"),
    MakeProperty (ItemImpl, left_spacing,   _("Left Spacing"),   _("Amount of spacing to add at the item's left side"), 0u, 65535u, 3u, "Prw"),
    MakeProperty (ItemImpl, right_spacing,  _("Right Spacing"),  _("Amount of spacing to add at the item's right side"), 0u, 65535u, 3u, "Prw"),
    MakeProperty (ItemImpl, bottom_spacing, _("Bottom Spacing"), _("Amount of spacing to add at the item's bottom side"), 0u, 65535u, 3u, "Prw"),
    MakeProperty (ItemImpl, top_spacing,    _("Top Spacing"),    _("Amount of spacing to add at the item's top side"), 0u, 65535u, 3u, "Prw"),
    MakeProperty (ItemImpl, halign, _("Horizontal Alignment"), _("Horizontal position within extra space when unexpanded, 0=left, 1=right"), 0, 1, 0.5, "Prw"),
    MakeProperty (ItemImpl, hscale, _("Horizontal Scale"),     _("Fractional horizontal expansion within extra space, 0=unexpanded, 1=expanded"), 0, 1, 0.5, "Prw"),
    MakeProperty (ItemImpl, valign, _("Vertical Alignment"),   _("Vertical position within extra space when unexpanded, 0=bottom, 1=top"), 0, 1, 0.5, "Prw"),
    MakeProperty (ItemImpl, vscale, _("Vertical Scale"),       _("Fractional vertical expansion within extra space, 0=unexpanded, 1=expanded"), 0, 1, 0.5, "Prw"),
    MakeProperty (ItemImpl, position, _("Position"),          _("Horizontal/vertical position of the item as point coordinate"), Point (-MAXDOUBLE, -MAXDOUBLE), Point (+MAXDOUBLE, +MAXDOUBLE), "Prw"),
    MakeProperty (ItemImpl, hanchor,  _("Horizontal Anchor"), _("Horizontal position of child anchor, 0=left, 1=right"), 0, 1, 0.5, "Prw"),
    MakeProperty (ItemImpl, vanchor,  _("Vertical Anchor"),   _("Vertical position of child anchor, 0=bottom, 1=top"), 0, 1, 0.5, "Prw"),
  };
  static const PropertyList property_list (properties);
  return property_list;
}

void
ItemImpl::propagate_heritage ()
{
  ContainerImpl *container = dynamic_cast<ContainerImpl*> (this);
  if (container)
    for (ContainerImpl::ChildWalker it = container->local_children(); it.has_next(); it++)
      it->heritage (m_heritage);
}

void
ItemImpl::heritage (Heritage *heritage)
{
  Heritage *old_heritage = m_heritage;
  m_heritage = NULL;
  if (heritage)
    {
      m_heritage = heritage->adapt_heritage (*this, color_scheme());
      ref_sink (m_heritage);
    }
  if (old_heritage)
    old_heritage->unref();
  if (m_heritage != old_heritage)
    {
      invalidate();
      propagate_heritage ();
    }
}

static bool
translate_from_ancestor (ItemImpl         *ancestor,
                         const ItemImpl   *child,
                         const uint    n_points,
                         Point        *points)
{
  if (child == ancestor)
    return true;
  ContainerImpl *pc = child->parent();
  translate_from_ancestor (ancestor, pc, n_points, points);
  Affine caffine = pc->child_affine (*child);
  for (uint i = 0; i < n_points; i++)
    points[i] = caffine.point (points[i]);
  return true;
}

bool
ItemImpl::translate_from (const ItemImpl   &src_item,
                      const uint    n_points,
                      Point        *points) const
{
  ItemImpl *ca = common_ancestor (src_item);
  if (!ca)
    return false;
  ItemImpl *item = const_cast<ItemImpl*> (&src_item);
  while (item != ca)
    {
      ContainerImpl *pc = item->parent();
      Affine affine = pc->child_affine (*item);
      affine.invert();
      for (uint i = 0; i < n_points; i++)
        points[i] = affine.point (points[i]);
      item = pc;
    }
  bool can_translate = translate_from_ancestor (ca, this, n_points, points);
  if (!can_translate)
    return false;
  return true;
}

bool
ItemImpl::translate_to (const uint    n_points,
                    Point        *points,
                    const ItemImpl   &target_item) const
{
  return target_item.translate_from (*this, n_points, points);
}

bool
ItemImpl::translate_from (const ItemImpl   &src_item,
                      const uint    n_rects,
                      Rect         *rects) const
{
  vector<Point> points;
  for (uint i = 0; i < n_rects; i++)
    {
      points.push_back (rects[i].lower_left());
      points.push_back (rects[i].lower_right());
      points.push_back (rects[i].upper_left());
      points.push_back (rects[i].upper_right());
    }
  if (!translate_from (src_item, points.size(), &points[0]))
    return false;
  for (uint i = 0; i < n_rects; i++)
    {
      const Point &p1 = points[i * 4 + 0], &p2 = points[i * 4 + 1];
      const Point &p3 = points[i * 4 + 2], &p4 = points[i * 4 + 3];
      const Point ll = min (min (p1, p2), min (p3, p4));
      const Point ur = max (max (p1, p2), max (p3, p4));
      rects[i].x = ll.x;
      rects[i].y = ll.y;
      rects[i].width = ur.x - ll.x;
      rects[i].height = ur.y - ll.y;
    }
  return true;
}

bool
ItemImpl::translate_to (const uint    n_rects,
                    Rect         *rects,
                    const ItemImpl   &target_item) const
{
  return target_item.translate_from (*this, n_rects, rects);
}

Point
ItemImpl::point_from_screen_window (Point window_point) /* window coordinates relative */
{
  Point p = window_point;
  ContainerImpl *pc = parent();
  if (pc)
    {
      const Affine &caffine = pc->child_affine (*this);
      p = pc->point_from_screen_window (p);  // to screen_window coords
      p = caffine * p;
    }
  return p;
}

Point
ItemImpl::point_to_screen_window (Point item_point) /* item coordinates relative */
{
  Point p = item_point;
  ContainerImpl *pc = parent();
  if (pc)
    {
      const Affine &caffine = pc->child_affine (*this);
      p = caffine.ipoint (p);           // to parent coords
      p = pc->point_to_screen_window (p);
    }
  return p;
}

Affine
ItemImpl::affine_from_screen_window () /* screen_window => item affine */
{
  Affine iaffine;
  ContainerImpl *pc = parent();
  if (pc)
    {
      const Affine &paffine = pc->affine_from_screen_window();
      const Affine &caffine = pc->child_affine (*this);
      if (paffine.is_identity())
        iaffine = caffine;
      else if (caffine.is_identity())
        iaffine = paffine;
      else
        iaffine = caffine * paffine;
    }
  return iaffine;
}

Affine
ItemImpl::affine_to_screen_window () /* item => screen_window affine */
{
  Affine iaffine = affine_from_screen_window();
  if (!iaffine.is_identity())
    iaffine = iaffine.invert();
  return iaffine;
}

bool
ItemImpl::process_event (const Event &event) /* item coordinates relative */
{
  bool handled = false;
  EventHandler *controller = dynamic_cast<EventHandler*> (this);
  if (controller)
    handled = controller->sig_event.emit (event);
  return handled;
}

bool
ItemImpl::process_screen_window_event (const Event &event) /* screen_window coordinates relative */
{
  bool handled = false;
  EventHandler *controller = dynamic_cast<EventHandler*> (this);
  if (controller)
    {
      const Affine &affine = affine_from_screen_window();
      if (affine.is_identity ())
        handled = controller->sig_event.emit (event);
      else
        {
          Event *ecopy = create_event_transformed (event, affine);
          handled = controller->sig_event.emit (*ecopy);
          delete ecopy;
        }
    }
  return handled;
}

bool
ItemImpl::screen_window_point (Point p) /* window coordinates relative */
{
  return point (point_from_screen_window (p));
}

bool
ItemImpl::point (Point p) /* item coordinates relative */
{
  Allocation a = allocation();
  return (drawable() &&
          p.x >= a.x && p.x < a.x + a.width &&
          p.y >= a.y && p.y < a.y + a.height);
}

void
ItemImpl::hierarchy_changed (ItemImpl *old_toplevel)
{
  anchored (old_toplevel == NULL);
}

void
ItemImpl::set_parent (ContainerImpl *pcontainer)
{
  EventHandler *controller = dynamic_cast<EventHandler*> (this);
  if (controller)
    controller->reset();
  ContainerImpl *pc = parent();
  if (pc)
    {
      ref (pc);
      WindowImpl *rtoplevel = get_window();
      pc->unparent_child (*this);
      invalidate();
      if (heritage())
        heritage (NULL);
      m_parent = NULL;
      propagate_state (false); // propagate PARENT_VISIBLE, PARENT_SENSITIVE
      if (anchored() and rtoplevel)
        sig_hierarchy_changed.emit (rtoplevel);
      unref (pc);
    }
  if (pcontainer)
    {
      /* ensure parent items are always containers (see parent()) */
      if (!dynamic_cast<ContainerImpl*> (pcontainer))
        throw Exception ("not setting non-Container item as parent: ", pcontainer->name());
      m_parent = pcontainer;
      if (m_parent->heritage())
        heritage (m_parent->heritage());
      invalidate();
      propagate_state (true);
      if (m_parent->anchored() and !anchored())
        sig_hierarchy_changed.emit (NULL);
    }
}

bool
ItemImpl::has_ancestor (const ItemImpl &ancestor) const
{
  const ItemImpl *item = this;
  while (item)
    {
      if (item == &ancestor)
        return true;
      item = item->parent();
    }
  return false;
}

ItemImpl*
ItemImpl::common_ancestor (const ItemImpl &other) const
{
  ItemImpl *item1 = const_cast<ItemImpl*> (this);
  do
    {
      ItemImpl *item2 = const_cast<ItemImpl*> (&other);
      do
        {
          if (item1 == item2)
            return item1;
          item2 = item2->parent();
        }
      while (item2);
      item1 = item1->parent();
    }
  while (item1);
  return NULL;
}

WindowImpl*
ItemImpl::get_window () const
{
  ItemImpl *parent = const_cast<ItemImpl*> (this);
  while (parent->parent())
    parent = parent->parent();
  return dynamic_cast<WindowImpl*> (parent); // NULL if parent is not of type WindowImpl*
}

ViewportImpl*
ItemImpl::get_viewport () const
{
  ItemImpl *parent = const_cast<ItemImpl*> (this);
  while (parent)
    {
      ViewportImpl *vp = dynamic_cast<ViewportImpl*> (parent);
      if (vp)
        return vp;
      parent = parent->parent();
    }
  return NULL;
}

ResizeContainerImpl*
ItemImpl::get_resize_container () const
{
  ItemImpl *parent = const_cast<ItemImpl*> (this);
  while (parent)
    {
      ResizeContainerImpl *rc = dynamic_cast<ResizeContainerImpl*> (parent);
      if (rc)
        return rc;
      parent = parent->parent();
    }
  return NULL;
}

void
ItemImpl::changed()
{
  if (!finalizing())
    sig_changed.emit();
}

void
ItemImpl::invalidate_parent ()
{
  /* propagate (size) invalidation from children to parents */
  ItemImpl *p = parent();
  if (p)
    p->invalidate_size();
}

void
ItemImpl::invalidate()
{
  const bool widget_state_invalidation = !test_all_flags (INVALID_REQUISITION | INVALID_ALLOCATION | INVALID_CONTENT);
  if (widget_state_invalidation)
    {
      expose();
      change_flags_silently (INVALID_REQUISITION | INVALID_ALLOCATION | INVALID_CONTENT, true); /* skip notification */
    }
  if (!finalizing())
    sig_invalidate.emit();
  if (widget_state_invalidation)
    {
      invalidate_parent(); /* need new size-request on parent */
      SizeGroup::invalidate_item (*this);
    }
}

void
ItemImpl::invalidate_size()
{
  if (!test_all_flags (INVALID_REQUISITION | INVALID_ALLOCATION))
    {
      change_flags_silently (INVALID_REQUISITION | INVALID_ALLOCATION, true); /* skip notification */
      if (!finalizing())
        sig_invalidate.emit();
      invalidate_parent(); /* need new size-request on parent */
      SizeGroup::invalidate_item (*this);
    }
}

Requisition
ItemImpl::inner_size_request()
{
  /* loop until we gather a valid requisition, this explicitely allows for
   * requisition invalidation during the size_request phase, item implementations
   * have to ensure we're not looping endlessly
   */
  while (test_flags (ItemImpl::INVALID_REQUISITION))
    {
      change_flags_silently (ItemImpl::INVALID_REQUISITION, false); // skip notification
      Requisition inner; // 0,0
      if (allocatable())
        {
          size_request (inner);
          inner.width = MAX (inner.width, 0);
          inner.height = MAX (inner.height, 0);
          Requisition ovr (width(), height());
          if (ovr.width >= 0)
            inner.width = ovr.width;
          if (ovr.height >= 0)
            inner.height = ovr.height;
          SZDEBUG ("size requesting: 0x%016zx:%s: %s => %.17gx%.17g", size_t (this),
                   Factory::factory_context_type (factory_context()).c_str(), name().c_str(),
                   inner.width, inner.height);
        }
      m_requisition = inner;
    }
  return allocatable() ? m_requisition : Requisition();
}

Requisition
ItemImpl::requisition ()
{
  return SizeGroup::item_requisition (*this);
}

bool
ItemImpl::tune_requisition (double new_width,
                            double new_height)
{
  Requisition req = requisition();
  if (new_width >= 0)
    req.width = new_width;
  if (new_height >= 0)
    req.height = new_height;
  return tune_requisition (req);
}

void
ItemImpl::expose_internal (const Region &region)
{
  if (!region.empty())
    {
      // queue expose region on nextmost viewport
      ViewportImpl *vp = parent() ? parent()->get_viewport() : get_viewport();
      if (vp)
        vp->expose_child_region (region);
    }
}

void
ItemImpl::expose (const Region &region) // item relative
{
  if (drawable() && !test_flags (INVALID_CONTENT))
    {
      Region r (allocation());
      r.intersect (region);
      expose_internal (r);
    }
}

void
ItemImpl::type_cast_error (const char *dest_type)
{
  fatal ("failed to dynamic_cast<%s> item: %s", VirtualTypeid::cxx_demangle (dest_type).c_str(), name().c_str());
}

String
ItemImpl::test_dump ()
{
  TestStream *tstream = TestStream::create_test_stream();
  make_test_dump (*tstream);
  String s = tstream->string();
  delete tstream;
  return s;
}

void
ItemImpl::make_test_dump (TestStream &tstream)
{
  tstream.push_node (name());
  const PropertyList &plist = list_properties();
  for (uint i = 0; i < plist.n_properties; i++)
    {
      Property *property = plist.properties[i];
      if (!property->readable())
        continue;
      String value = get_property (property->ident);
      tstream.dump (property->ident, value);
    }
  tstream.push_indent();
  dump_private_data (tstream);
  dump_test_data (tstream);
  tstream.pop_indent();
  tstream.pop_node ();
}

void
ItemImpl::dump_test_data (TestStream &tstream)
{}

void
ItemImpl::dump_private_data (TestStream &tstream)
{}

void
ItemImpl::find_adjustments (AdjustmentSourceType adjsrc1,
                        Adjustment         **adj1,
                        AdjustmentSourceType adjsrc2,
                        Adjustment         **adj2,
                        AdjustmentSourceType adjsrc3,
                        Adjustment         **adj3,
                        AdjustmentSourceType adjsrc4,
                        Adjustment         **adj4)
{
  for (ItemImpl *pitem = this->parent(); pitem; pitem = pitem->parent())
    {
      AdjustmentSource *adjustment_source = pitem->interface<AdjustmentSource*>();
      if (!adjustment_source)
        continue;
      if (adj1 && !*adj1)
        *adj1 = adjustment_source->get_adjustment (adjsrc1);
      if (adj2 && !*adj2)
        *adj2 = adjustment_source->get_adjustment (adjsrc2);
      if (adj3 && !*adj3)
        *adj3 = adjustment_source->get_adjustment (adjsrc3);
      if (adj4 && !*adj4)
        *adj4 = adjustment_source->get_adjustment (adjsrc4);
      if ((!adj1 || *adj1) &&
          (!adj2 || *adj2) &&
          (!adj3 || *adj3) &&
          (!adj4 || *adj4))
        break;
    }
}

void
ItemImpl::repack (const PackInfo &orig,
              const PackInfo &pnew)
{
  if (parent())
    parent()->repack_child (*this, orig, pnew);
  invalidate();
}

ItemImpl::PackInfo&
ItemImpl::pack_info (bool create)
{
  static const PackInfo pack_info_defaults = {
    0,   1,   0, 1,     /* hposition, hspan, vposition, vspan */
    0,   0,   0, 0,     /* left_spacing, right_spacing, bottom_spacing, top_spacing */
    0.5, 1, 0.5, 1,     /* halign, hscale, valign, vscale */
  };
  static DataKey<PackInfo*> pack_info_key;
  PackInfo *pi = get_data (&pack_info_key);
  if (!pi)
    {
      if (create)
        {
          pi = new PackInfo (pack_info_defaults);
          set_data (&pack_info_key, pi);
        }
      else /* read-only access */
        pi = const_cast<PackInfo*> (&pack_info_defaults);
    }
  return *pi;
}

void
ItemImpl::hposition (double d)
{
  PackInfo &pa = pack_info (true), op = pa;
  pa.hposition = d;
  repack (op, pa);
}

void
ItemImpl::hspan (double d)
{
  PackInfo &pa = pack_info (true), op = pa;
  pa.hspan = MAX (1, d);
  repack (op, pa);
}

void
ItemImpl::vposition (double d)
{
  PackInfo &pa = pack_info (true), op = pa;
  pa.vposition = d;
  repack (op, pa);
}

void
ItemImpl::vspan (double d)
{
  PackInfo &pa = pack_info (true), op = pa;
  pa.vspan = MAX (1, d);
  repack (op, pa);
}

void
ItemImpl::left_spacing (uint s)
{
  PackInfo &pa = pack_info (true), op = pa;
  pa.left_spacing = s;
  repack (op, pa);
}

void
ItemImpl::right_spacing (uint s)
{
  PackInfo &pa = pack_info (true), op = pa;
  pa.right_spacing = s;
  repack (op, pa);
}

void
ItemImpl::bottom_spacing (uint s)
{
  PackInfo &pa = pack_info (true), op = pa;
  pa.bottom_spacing = s;
  repack (op, pa);
}

void
ItemImpl::top_spacing (uint s)
{
  PackInfo &pa = pack_info (true), op = pa;
  pa.top_spacing = s;
  repack (op, pa);
}

void
ItemImpl::halign (double f)
{
  PackInfo &pa = pack_info (true), op = pa;
  pa.halign = CLAMP (f, 0, 1);
  repack (op, pa);
}

void
ItemImpl::hscale (double f)
{
  PackInfo &pa = pack_info (true), op = pa;
  pa.hscale = f;
  repack (op, pa);
}

void
ItemImpl::valign (double f)
{
  PackInfo &pa = pack_info (true), op = pa;
  pa.valign = CLAMP (f, 0, 1);
  repack (op, pa);
}

void
ItemImpl::vscale (double f)
{
  PackInfo &pa = pack_info (true), op = pa;
  pa.vscale = f;
  repack (op, pa);
}

void
ItemImpl::position (Point point) // mirrors (hposition,vposition)
{
  PackInfo &pa = pack_info (true), op = pa;
  pa.hposition = point.x;
  pa.vposition = point.y;
  repack (op, pa);
}

void
ItemImpl::do_changed()
{
}

void
ItemImpl::do_invalidate()
{
}

bool
ItemImpl::do_event (const Event &event)
{
  return false;
}

static DataKey<String> item_name_key;

String
ItemImpl::name () const
{
  String s = get_data (&item_name_key);
  if (s.empty())
    return Factory::factory_context_name (factory_context());
  else
    return s;
}

void
ItemImpl::name (const String &str)
{
  if (str.empty())
    delete_data (&item_name_key);
  else
    set_data (&item_name_key, str);
}

FactoryContext*
ItemImpl::factory_context () const
{
  return m_factory_context;
}

void
ItemImpl::factory_context (FactoryContext *fc)
{
  if (fc)
    assert_return (m_factory_context == NULL);
  m_factory_context = fc;
}

static DataKey<ColorSchemeType> item_color_scheme_key;

ColorSchemeType
ItemImpl::color_scheme () const
{
  return get_data (&item_color_scheme_key);
}

void
ItemImpl::color_scheme (ColorSchemeType cst)
{
  if (cst != get_data (&item_color_scheme_key))
    {
      if (!cst)
        delete_data (&item_color_scheme_key);
      else
        set_data (&item_color_scheme_key, cst);
      ClassDoctor::update_item_heritage (*this);
    }
}

bool
ItemImpl::tune_requisition (Requisition requisition)
{
  ItemImpl *p = parent();
  if (p && !test_flags (INVALID_REQUISITION))
    {
      ResizeContainerImpl *rc = p->get_resize_container();
      if (rc && rc->requisitions_tunable())
        {
          Requisition ovr (width(), height());
          requisition.width = ovr.width >= 0 ? ovr.width : MAX (requisition.width, 0);
          requisition.height = ovr.height >= 0 ? ovr.height : MAX (requisition.height, 0);
          if (requisition.width != m_requisition.width || requisition.height != m_requisition.height)
            {
              m_requisition = requisition;
              invalidate_parent(); // need new size-request on parent
              return true;
            }
        }
    }
  return false;
}

void
ItemImpl::set_allocation (const Allocation &area)
{
  Allocation sarea (iround (area.x), iround (area.y), iround (area.width), iround (area.height));
  const double smax = 4503599627370496.; // 52bit precision is maximum for doubles
  sarea.x      = CLAMP (sarea.x, -smax, smax);
  sarea.y      = CLAMP (sarea.y, -smax, smax);
  sarea.width  = CLAMP (sarea.width,  0, smax);
  sarea.height = CLAMP (sarea.height, 0, smax);
  /* remember old area */
  const Allocation oa = allocation();
  /* always reallocate to re-layout children */
  change_flags_silently (INVALID_ALLOCATION, false); /* skip notification */
  if (!allocatable())
    sarea = Allocation (0, 0, 0, 0);
  const bool changed = m_allocation != sarea;
  m_allocation = sarea;
  size_allocate (m_allocation, changed);
  Allocation a = allocation();
  bool need_expose = oa != a || test_flags (INVALID_CONTENT);
  change_flags_silently (INVALID_CONTENT, false); // skip notification
  // expose old area
  if (need_expose)
    {
      // expose unclipped
      Region region (oa);
      expose_internal (region);
    }
  /* expose new area */
  if (need_expose)
    expose();
  SZDEBUG ("size allocation: 0x%016zx:%s: %s => %s", size_t (this),
           Factory::factory_context_type (factory_context()).c_str(), name().c_str(),
           a.string().c_str());
}

// == rendering ==
class ItemImpl::RenderContext {
  friend class ItemImpl;
  vector<cairo_surface_t*> surfaces;
  Region                   render_area;
  vector<cairo_t*>         cairos;
public:
  /*dtor*/     ~RenderContext();
  const Region& region() const { return render_area; }
};

void
ItemImpl::render_into (cairo_t *cr, const Region &region)
{
  RenderContext rcontext;
  rcontext.render_area = allocation();
  rcontext.render_area.intersect (region);
  if (!rcontext.render_area.empty())
    {
      render_item (rcontext);
      cairo_save (cr);
      vector<Rect> rects;
      rcontext.render_area.list_rects (rects);
      for (size_t i = 0; i < rects.size(); i++)
        cairo_rectangle (cr, rects[i].x, rects[i].y, rects[i].width, rects[i].height);
      cairo_clip (cr);
      for (size_t i = 0; i < rcontext.surfaces.size(); i++)
        {
          cairo_set_source_surface (cr, rcontext.surfaces[i], 0, 0);
          cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
          cairo_paint_with_alpha (cr, 1.0);
        }
      cairo_restore (cr);
    }
}

void
ItemImpl::render_item (RenderContext &rcontext)
{
  size_t n_cairos = rcontext.cairos.size();
  Rect area = allocation();
  area.intersect (rendering_region (rcontext).extents());
  render (rcontext, area);
  while (rcontext.cairos.size() > n_cairos)
    {
      cairo_destroy (rcontext.cairos.back());
      rcontext.cairos.pop_back();
    }
}

void
ItemImpl::render (RenderContext &rcontext, const Rect &rect)
{}

const Region&
ItemImpl::rendering_region (RenderContext &rcontext) const
{
  return rcontext.render_area;
}

cairo_t*
ItemImpl::cairo_context (RenderContext  &rcontext,
                         const Allocation &area)
{
  Rect rect = area;
  if (area == Allocation (-1, -1, 0, 0))
    rect = allocation();
  const bool empty_dummy = rect.width < 1 || rect.height < 1; // we allow cairo_contexts with 0x0 pixels
  if (empty_dummy)
    rect.width = rect.height = 1;
  assert_return (rect.width > 0 && rect.height > 0, NULL);
  cairo_surface_t *surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, iceil (rect.width), iceil (rect.height));
  if (cairo_surface_status (surface) != CAIRO_STATUS_SUCCESS)
    critical ("%s: failed to create ARGB32 cairo surface with %lldx%lld pixels: %s\n", __func__, iceil (rect.width), iceil (rect.height),
              cairo_status_to_string (cairo_surface_status (surface)));
  cairo_surface_set_device_offset (surface, -rect.x, -rect.y);
  cairo_t *cr = cairo_create (surface);
  rcontext.cairos.push_back (cr);
  if (empty_dummy)
    cairo_surface_destroy (surface);
  else
    rcontext.surfaces.push_back (surface);
  return cr;
}

ItemImpl::RenderContext::~RenderContext()
{
  critical_unless (cairos.size() == 0);
  while (!cairos.empty())
    {
      cairo_destroy (cairos.back());
      cairos.pop_back();
    }
  while (!surfaces.empty())
    {
      cairo_surface_destroy (surfaces.back());
      surfaces.pop_back();
    }
}

} // Rapicorn
