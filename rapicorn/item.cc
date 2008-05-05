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
#include "item.hh"
#include "itemimpl.hh"
#include "container.hh"
#include "adjustment.hh"
#include "root.hh"
#include "cmdlib.hh"

namespace Rapicorn {

EventHandler::EventHandler() :
  sig_event (*this, &EventHandler::handle_event)
{}

bool
EventHandler::handle_event (const Event &event)
{
  return false;
}

Item::Item () :
  m_flags (VISIBLE | SENSITIVE | ALLOCATABLE | HFILL | VFILL),
  m_parent (NULL),
  m_style (NULL),
  sig_finalize (*this),
  sig_changed (*this, &Item::do_changed),
  sig_invalidate (*this, &Item::do_invalidate),
  sig_hierarchy_changed (*this, &Item::hierarchy_changed)
{}

void
Item::constructed()
{}

bool
Item::viewable() const
{
  return drawable() && (!m_parent || m_parent->viewable());
}

bool
Item::change_flags_silently (uint32 mask,
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
Item::propagate_flags (bool notify_changed)
{
  change_flags_silently (PARENT_SENSITIVE, !parent() || parent()->sensitive());
  Container *container = dynamic_cast<Container*> (this);
  if (container)
    for (Container::ChildWalker it = container->local_children(); it.has_next(); it++)
      it->propagate_flags();
  if (notify_changed && !finalizing())
    sig_changed.emit(); /* notify changed() without invalidate() */
}

void
Item::set_flag (uint32 flag,
                bool   on)
{
  assert ((flag & (flag - 1)) == 0); /* single bit check */
  const uint propagate_flag_mask  = SENSITIVE | PARENT_SENSITIVE | PRELIGHT | IMPRESSED | HAS_DEFAULT;
  const uint repack_flag_mask = HEXPAND | VEXPAND | HSHRINK | VSHRINK | HFILL | VFILL |
                                HSPREAD | VSPREAD | HSPREAD_CONTAINER | VSPREAD_CONTAINER | VISIBLE;
  bool fchanged = change_flags_silently (flag, on);
  if (fchanged)
    {
      if (flag & propagate_flag_mask)
        {
          expose();
          propagate_flags (false);
        }
      if (flag & repack_flag_mask)
        repack(); // includes invalidate();
      changed();
    }
}

bool
Item::grab_default () const
{
  return false;
}

void
Item::sensitive (bool b)
{
  set_flag (SENSITIVE, b);
}

void
Item::prelight (bool b)
{
  set_flag (PRELIGHT, b);
}

bool
Item::branch_prelight () const
{
  const Item *item = this;
  while (item)
    {
      if (item->prelight())
        return true;
      item = item->parent();
    }
  return false;
}

void
Item::impressed (bool b)
{
  set_flag (IMPRESSED, b);
}

bool
Item::branch_impressed () const
{
  const Item *item = this;
  while (item)
    {
      if (item->impressed())
        return true;
      item = item->parent();
    }
  return false;
}

StateType
Item::state () const
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
Item::has_focus () const
{
  if (test_flags (FOCUS_CHAIN))
    {
      Root *ritem = get_root();
      if (ritem && ritem->get_focus() == this)
        return true;
    }
  return false;
}

bool
Item::can_focus () const
{
  return false;
}

bool
Item::grab_focus ()
{
  if (has_focus())
    return true;
  if (!can_focus() || !sensitive() || !viewable())
    return false;
  /* unset old focus */
  Root *ritem = get_root();
  if (ritem)
    ritem->set_focus (NULL);
  /* set new focus */
  ritem = get_root();
  if (ritem && ritem->get_focus() == NULL)
    ritem->set_focus (this);
  return ritem->get_focus() == this;
}

bool
Item::move_focus (FocusDirType fdir)
{
  if (!has_focus() && can_focus())
    return grab_focus();
  return false;
}

void
Item::notify_key_error ()
{
  Root *ritem = get_root();
  if (ritem)
    ritem->beep();
}

void
Item::cross_link (Item           &link,
                  const ItemSlot &uncross)
{
  assert (this != &link);
  Container *common_container = dynamic_cast<Container*> (common_ancestor (link));
  assert (common_container != NULL);
  common_container->item_cross_link (*this, link, uncross);
}

void
Item::cross_unlink (Item           &link,
                    const ItemSlot &uncross)
{
  assert (this != &link);
  Container *common_container = dynamic_cast<Container*> (common_ancestor (link));
  assert (common_container != NULL);
  common_container->item_cross_unlink (*this, link, uncross);
}

void
Item::uncross_links (Item &link)
{
  assert (this != &link);
  Container *common_container = dynamic_cast<Container*> (common_ancestor (link));
  assert (common_container != NULL);
  common_container->item_uncross_links (*this, link);
}

bool
Item::match_interface (InterfaceMatch &imatch) const
{
  return imatch.done() || imatch.match (const_cast<Item*> (this), name());
}

bool
Item::match_parent_interface (InterfaceMatch &imatch) const
{
  Item *pitem = parent();
  if (pitem && imatch.match (pitem, pitem->name()))
    return true;
  if (pitem)
    return pitem->match_parent_interface (imatch);
  else
    return false;
}

bool
Item::match_toplevel_interface (InterfaceMatch &imatch) const
{
  Item *pitem = parent();
  if (pitem && pitem->match_toplevel_interface (imatch))
    return true;
  if (imatch.match (const_cast<Item*> (this), name()))
    return true;
  return false;
}

uint
Item::exec_slow_repeater (const BoolSlot &sl)
{
  Root *ritem = get_root();
  if (ritem)
    {
      EventLoop *loop = ritem->get_loop();
      if (loop)
        return loop->exec_timer (250, 50, sl);
    }
  return 0;
}

uint
Item::exec_fast_repeater (const BoolSlot &sl)
{
  Root *ritem = get_root();
  if (ritem)
    {
      EventLoop *loop = ritem->get_loop();
      if (loop)
        return loop->exec_timer (200, 20, sl);
    }
  return 0;
}

uint
Item::exec_key_repeater (const BoolSlot &sl)
{
  Root *ritem = get_root();
  if (ritem)
    {
      EventLoop *loop = ritem->get_loop();
      if (loop)
        return loop->exec_timer (250, 33, sl);
    }
  return 0;
}

bool
Item::remove_exec (uint exec_id)
{
  Root *ritem = get_root();
  if (ritem)
    {
      EventLoop *loop = ritem->get_loop();
      if (loop)
        return loop->try_remove (exec_id);
    }
  return false;
}

static DataKey<uint> visual_update_key;

void
Item::queue_visual_update ()
{
  uint timer_id = get_data (&visual_update_key);
  if (!timer_id)
    {
      Root *ritem = get_root();
      if (ritem)
        {
          EventLoop *loop = ritem->get_loop();
          if (loop)
            {
              timer_id = loop->exec_timer (60, slot (*this, &Item::force_visual_update));
              set_data (&visual_update_key, timer_id);
            }
        }
    }
}

void
Item::force_visual_update ()
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
Item::visual_update ()
{}

void
Item::finalize()
{
  sig_finalize.emit();
}

Item::~Item()
{
  if (parent())
    parent()->remove (this);
  if (m_style)
    {
      m_style->unref();
      m_style = NULL;
    }
  uint timer_id = get_data (&visual_update_key);
  if (timer_id)
    {
      remove_exec (timer_id);
      set_data (&visual_update_key, uint (0));
    }
}

OwnedMutex&
Item::owned_mutex ()
{
  if (m_parent)
    return m_parent->owned_mutex();
  else
    return Thread::Self::owned_mutex();
}

Command*
Item::lookup_command (const String &command_name)
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
Item::exec_command (const String &command_call_string)
{
  String cmd_name;
  StringVector args;
  if (!command_scan (command_call_string, &cmd_name, &args))
    {
      warning ("Invalid command syntax: %s", command_call_string.c_str());
      return false;
    }
  cmd_name = string_strip (cmd_name);
  if (cmd_name == "")
    return true;

  Item *item = this;
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

  warning ("Command unimplemented: %s", command_call_string.c_str());
  return false;
}

bool
Item::custom_command (const String       &command_name,
                      const StringVector &command_args)
{
  return false;
}

const CommandList&
Item::list_commands ()
{
  static Command *commands[] = {
  };
  static const CommandList command_list (commands);
  return command_list;
}

Property*
Item::lookup_property (const String &property_name)
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
  if (it != pmap->end())
    return it->second;
  else
    return NULL;
}

void
Item::set_property (const String    &property_name,
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
Item::try_set_property (const String    &property_name,
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
Item::get_property (const String   &property_name)
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
Item::width () const
{
  Requisition ovr = get_data (&override_requisition);
  return ovr.width >= 0 ? ovr.width : -1;
}

void
Item::width (double w)
{
  Requisition ovr = get_data (&override_requisition);
  ovr.width = w >= 0 ? w : -1;
  set_data (&override_requisition, ovr);
  invalidate_size();
}

double
Item::height () const
{
  Requisition ovr = get_data (&override_requisition);
  return ovr.height >= 0 ? ovr.height : -1;
}

void
Item::height (double h)
{
  Requisition ovr = get_data (&override_requisition);
  ovr.height = h >= 0 ? h : -1;
  set_data (&override_requisition, ovr);
  invalidate_size();
}

const PropertyList&
Item::list_properties ()
{
  static Property *properties[] = {
    MakeProperty (Item, name,      _("Name"), _("Identification name of the item"), "rw"),
    MakeProperty (Item, width,     _("Requested Width"), _("The width to request from its container for this item, -1=automatic"), -1, MAXINT, 5, "rw"),
    MakeProperty (Item, height,    _("Requested Height"), _("The height to request from its container for this item, -1=automatic"), -1, MAXINT, 5, "rw"),
    MakeProperty (Item, visible,   _("Visible"), _("Whether this item is visible"), "rw"),
    MakeProperty (Item, sensitive, _("Sensitive"), _("Whether this item is sensitive (receives events)"), "rw"),
    MakeProperty (Item, hexpand,   _("Horizontal Expand"), _("Whether to expand this item horizontally"), "rw"),
    MakeProperty (Item, vexpand,   _("Vertical Expand"), _("Whether to expand this item vertically"), "rw"),
    MakeProperty (Item, hspread,   _("Horizontal Spread"), _("Whether to expand this item and all its parents horizontally"), "rw"),
    MakeProperty (Item, vspread,   _("Vertical Spread"), _("Whether to expand this item and all its parents vertically"), "rw"),
    MakeProperty (Item, hshrink,   _("Horizontal Shrink"), _("Whether the item may be shrunken horizontally"), "rw"),
    MakeProperty (Item, vshrink,   _("Vertical Shrink"),   _("Whether the item may be shrunken vertically"), "rw"),
    MakeProperty (Item, hfill,     _("Horizontal Fill"), _("Whether the item may fill all extra horizontal space"), "rw"),
    MakeProperty (Item, vfill,     _("Vertical Fill"), _("Whether the item may fill all extra vertical space"), "rw"),
    /* packing */
    MakeProperty (Item, left_attach,    _("Left Attach"),    _("Column index to attach the item's left side to"), 0u, 99999u, 5u, "Prw"),
    MakeProperty (Item, right_attach,   _("Right Attach"),   _("Column index to attach the item's right side to"), 1u, 100000u, 5u, "Prw"),
    MakeProperty (Item, bottom_attach,  _("Bottom Attach"),  _("Row index to attach the item's bottom side to"), 0u, 99999u, 5u, "Prw"),
    MakeProperty (Item, top_attach,     _("Top Attach"),     _("Row index to attach the item's top side to"), 1u, 100000u, 5u, "Prw"),
    MakeProperty (Item, left_spacing,   _("Left Spacing"),   _("Amount of spacing to add at the item's left side"), 0u, 65535u, 3u, "Prw"),
    MakeProperty (Item, right_spacing,  _("Right Spacing"),  _("Amount of spacing to add at the item's right side"), 0u, 65535u, 3u, "Prw"),
    MakeProperty (Item, bottom_spacing, _("Bottom Spacing"), _("Amount of spacing to add at the item's bottom side"), 0u, 65535u, 3u, "Prw"),
    MakeProperty (Item, top_spacing,    _("Top Spacing"),    _("Amount of spacing to add at the item's top side"), 0u, 65535u, 3u, "Prw"),
    MakeProperty (Item, halign, _("Horizontal Alignment"), _("Horizontal position within extra space when unexpanded, 0=left, 1=right"), 0, 1, 0.5, "Prw"),
    MakeProperty (Item, hscale, _("Horizontal Scale"),     _("Fractional horizontal expansion within extra space, 0=unexpanded, 1=expanded"), 0, 1, 0.5, "Prw"),
    MakeProperty (Item, valign, _("Vertical Alignment"),   _("Vertical position within extra space when unexpanded, 0=bottom, 1=top"), 0, 1, 0.5, "Prw"),
    MakeProperty (Item, vscale, _("Vertical Scale"),       _("Fractional vertical expansion within extra space, 0=unexpanded, 1=expanded"), 0, 1, 0.5, "Prw"),
  };
  static const PropertyList property_list (properties);
  return property_list;
}

void
Item::propagate_style ()
{
  Container *container = dynamic_cast<Container*> (this);
  if (container)
    for (Container::ChildWalker it = container->local_children(); it.has_next(); it++)
      it->style (m_style);
}

void
Item::style (Style *st)
{
  if (m_style)
    {
      m_style->unref();
      m_style = NULL;
    }
  if (st)
    {
      m_style = st->create_style (st->name());
      ref_sink (m_style);
      invalidate();
    }
  propagate_style ();
}

static bool
translate_from_ancestor (Item         *ancestor,
                         const Item   *child,
                         const uint    n_points,
                         Point        *points)
{
  if (child == ancestor)
    return true;
  Container *pc = child->parent();
  translate_from_ancestor (ancestor, pc, n_points, points);
  Affine caffine = pc->child_affine (*child);
  for (uint i = 0; i < n_points; i++)
    points[i] = caffine.point (points[i]);
  return true;
}

bool
Item::translate_from (const Item   &src_item,
                      const uint    n_points,
                      Point        *points) const
{
  Item *ca = common_ancestor (src_item);
  if (!ca)
    return false;
  Item *item = const_cast<Item*> (&src_item);
  while (item != ca)
    {
      Container *pc = item->parent();
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
Item::translate_to (const uint    n_points,
                    Point        *points,
                    const Item   &target_item) const
{
  return target_item.translate_from (*this, n_points, points);
}

bool
Item::translate_from (const Item   &src_item,
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
Item::translate_to (const uint    n_rects,
                    Rect         *rects,
                    const Item   &target_item) const
{
  return target_item.translate_from (*this, n_rects, rects);
}

Point
Item::point_from_viewport (Point root_point) /* root coordinates relative */
{
  Point p = root_point;
  Container *pc = parent();
  if (pc)
    {
      const Affine &caffine = pc->child_affine (*this);
      p = pc->point_from_viewport (p);  // to viewport coords
      p = caffine * p;
    }
  return p;
}

Point
Item::point_to_viewport (Point item_point) /* item coordinates relative */
{
  Point p = item_point;
  Container *pc = parent();
  if (pc)
    {
      const Affine &caffine = pc->child_affine (*this);
      p = caffine.ipoint (p);           // to parent coords
      p = pc->point_to_viewport (p);
    }
  return p;
}

Affine
Item::affine_from_viewport () /* viewport => item affine */
{
  Affine iaffine;
  Container *pc = parent();
  if (pc)
    {
      const Affine &paffine = pc->affine_from_viewport();
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
Item::affine_to_viewport () /* item => viewport affine */
{
  Affine iaffine = affine_from_viewport();
  if (!iaffine.is_identity())
    iaffine = iaffine.invert();
  return iaffine;
}

bool
Item::process_event (const Event &event) /* item coordinates relative */
{
  bool handled = false;
  EventHandler *controller = dynamic_cast<EventHandler*> (this);
  if (controller)
    handled = controller->sig_event.emit (event);
  return handled;
}

bool
Item::process_viewport_event (const Event &event) /* viewport coordinates relative */
{
  bool handled = false;
  EventHandler *controller = dynamic_cast<EventHandler*> (this);
  if (controller)
    {
      const Affine &affine = affine_from_viewport();
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
Item::viewport_point (Point p) /* root coordinates relative */
{
  return point (point_from_viewport (p));
}

bool
Item::point (Point p) /* item coordinates relative */
{
  Allocation a = allocation();
  return (drawable() &&
          p.x >= a.x && p.x < a.x + a.width &&
          p.y >= a.y && p.y < a.y + a.height);
}

void
Item::hierarchy_changed (Item *old_toplevel)
{
  anchored (old_toplevel == NULL);
}

void
Item::set_parent (Container *pcontainer)
{
  EventHandler *controller = dynamic_cast<EventHandler*> (this);
  if (controller)
    controller->reset();
  Container *pc = parent();
  if (pc)
    {
      ref (pc);
      Root *rtoplevel = get_root();
      pc->unparent_child (*this);
      invalidate();
      style (NULL);
      m_parent = NULL;
      if (anchored() and rtoplevel)
        sig_hierarchy_changed.emit (rtoplevel);
      unref (pc);
    }
  if (pcontainer)
    {
      /* ensure parent items are always containers (see parent()) */
      if (!dynamic_cast<Container*> (pcontainer))
        throw Exception ("not setting non-Container item as parent: ", pcontainer->name());
      m_parent = pcontainer;
      style (m_parent->style());
      propagate_flags();
      invalidate();
      if (m_parent->anchored() and !anchored())
        sig_hierarchy_changed.emit (NULL);
    }
}

bool
Item::has_ancestor (const Item &ancestor) const
{
  const Item *item = this;
  while (item)
    {
      if (item == &ancestor)
        return true;
      item = item->parent();
    }
  return false;
}

Item*
Item::common_ancestor (const Item &other) const
{
  Item *item1 = const_cast<Item*> (this);
  do
    {
      Item *item2 = const_cast<Item*> (&other);
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

Root*
Item::get_root () const
{
  Item *parent = const_cast<Item*> (this);
  while (parent->parent())
    parent = parent->parent();
  return dynamic_cast<Root*> (parent); // NULL if parent is not of type Root*
}

void
Item::changed()
{
  if (!finalizing())
    sig_changed.emit();
}

void
Item::invalidate()
{
  if (!test_all_flags (INVALID_REQUISITION | INVALID_ALLOCATION | INVALID_CONTENT))
    {
      change_flags_silently (INVALID_REQUISITION | INVALID_ALLOCATION | INVALID_CONTENT, true); /* skip notification */
      if (!finalizing())
        sig_invalidate.emit();
      if (parent())
        parent()->invalidate_size();
    }
}

void
Item::invalidate_size()
{
  if (!test_all_flags (INVALID_REQUISITION | INVALID_ALLOCATION))
    {
      change_flags_silently (INVALID_REQUISITION | INVALID_ALLOCATION, true); /* skip notification */
      if (!finalizing())
        sig_invalidate.emit();
      if (parent())
        parent()->invalidate_size();
    }
}

bool
Item::tune_requisition (Requisition requisition)
{
  return false; /* ItemImpl implements this */
}

bool
Item::tune_requisition (double new_width,
                        double new_height)
{
  Requisition req = size_request();
  if (new_width >= 0)
    req.width = new_width;
  if (new_height >= 0)
    req.height = new_height;
  return tune_requisition (req);
}

void
Item::copy_area (const Rect  &rect,
                 const Point &dest)
{
  Root *ritem = get_root();
  if (ritem)
    {
      Allocation a = allocation();
      Rect srect (Point (a.x, a.y), a.width, a.height);
      Rect drect (Point (a.x, a.y), a.width, a.height);
      /* clip src and dest rectangles to allocation */
      srect.intersect (rect);
      drect.intersect (Rect (dest, rect.width, rect.height));
      /* offset src rectangle by the amount the dest rectangle was clipped */
      Rect src (Point (srect.x + drect.x - dest.x, srect.y + drect.y - dest.y),
                min (srect.width, drect.width),
                min (srect.height, drect.height));
      /* the actual copy */
      if (!src.empty())
        ritem->copy_area (src, drect.lower_left());
    }
}

void
Item::expose ()
{
  expose (allocation());
}

void
Item::expose (const Rect &rect) /* item coordinates relative */
{
  expose (Region (rect));
}

void
Item::expose (const Region &region) /* item coordinates relative */
{
  Region r (allocation());
  r.intersect (region);
  Root *rt = get_root();
  if (!r.empty() && rt && !test_flags (INVALID_CONTENT))
    {
      const Affine &affine = affine_to_viewport();
      r.affine (affine);
      rt->expose_root_region (r);
    }
}

void
Item::type_cast_error (const char *dest_type)
{
  error ("failed to dynamic_cast<%s> item: %s", VirtualTypeid::cxx_demangle (dest_type).c_str(), name().c_str());
}

void
Item::find_adjustments (AdjustmentSourceType adjsrc1,
                        Adjustment         **adj1,
                        AdjustmentSourceType adjsrc2,
                        Adjustment         **adj2,
                        AdjustmentSourceType adjsrc3,
                        Adjustment         **adj3,
                        AdjustmentSourceType adjsrc4,
                        Adjustment         **adj4)
{
  for (Item *pitem = this->parent(); pitem; pitem = pitem->parent())
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
Item::repack()
{
  if (parent())
    parent()->repack_child (*this);
  invalidate();
}

Item::PackInfo&
Item::pack_info (bool create)
{
  static const PackInfo pack_info_defaults = {
    0,   1,   0, 1,     /* *_attach */
    0,   0,   0, 0,     /* *_spacing */
    0.5, 1, 0.5, 1,     /* align/scale */
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

String
ItemImpl::name () const
{
  return m_name;
}

void
ItemImpl::name (const String &str)
{
  m_name = str;
}

bool
ItemImpl::tune_requisition (Requisition requisition)
{
  Item *p = parent();
  if (p && !test_flags (INVALID_REQUISITION))
    {
      Root *r = p->get_root();
      if (r && r->tunable_requisitions())
        {
          Requisition ovr (width(), height());
          requisition.width = ovr.width >= 0 ? ovr.width : MAX (requisition.width, 0);
          requisition.height = ovr.height >= 0 ? ovr.height : MAX (requisition.height, 0);
          if (requisition.width != m_requisition.width || requisition.height != m_requisition.height)
            {
              m_requisition = requisition;
              p->invalidate_size(); /* need new size-request on parent */
              return true;
            }
        }
    }
  return false;
}

const Requisition&
ItemImpl::size_request ()
{
  while (test_flags (INVALID_REQUISITION))
    {
      change_flags_silently (INVALID_REQUISITION, false); /* skip notification */
      Requisition req;
      size_request (req);
      req.width = MAX (req.width, 0);
      req.height = MAX (req.height, 0);
      Requisition ovr (width(), height());
      if (ovr.width >= 0)
        req.width = ovr.width;
      if (ovr.height >= 0)
        req.height = ovr.height;
      if (!allocatable())
        req = Requisition (0, 0);
      m_requisition = req;
    }
  return m_requisition;
}

const Allocation&
ItemImpl::allocation()
{
  return m_allocation;
}

void
ItemImpl::allocation (const Allocation &area)
{
  m_allocation = area;
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
  Allocation oa = allocation();
  /* always reallocate to re-layout children */
  change_flags_silently (INVALID_ALLOCATION, false); /* skip notification */
  change_flags_silently (POSITIVE_ALLOCATION, false); /* !drawable() while size_allocate() is called */
  size_allocate (allocatable () ? sarea : Allocation (0, 0, 0, 0));
  Allocation a = allocation();
  change_flags_silently (POSITIVE_ALLOCATION, a.width > 0 && a.height > 0);
  bool need_expose = oa != a || test_flags (INVALID_CONTENT);
  change_flags_silently (INVALID_CONTENT, false); /* skip notification */
  /* expose old area */
  if (need_expose)
    {
      /* expose unclipped */
      Region r (oa);
      Root *rt = get_root();
      if (!r.empty() && rt)
        {
          const Affine &affine = affine_to_viewport();
          r.affine (affine);
          Rect rc = r.extents();
          rt->expose (rc);
        }
    }
  /* expose new area */
  if (need_expose)
    expose();
}

} // Rapicorn
