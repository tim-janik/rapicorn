/* Rapicorn
 * Copyright (C) 2005 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#include "item.hh"
#include "itemimpl.hh"
#include "container.hh"

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
  m_parent (NULL),
  m_flags (VISIBLE | SENSITIVE),
  m_style (NULL),
  sig_finalize (*this),
  sig_changed (*this, &Item::do_changed),
  sig_invalidate (*this, &Item::do_invalidate),
  sig_hierarchy_changed (*this, &Item::hierarchy_changed)
{}

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
Item::propagate_flags()
{
  change_flags_silently (PARENT_SENSITIVE, !parent() || parent()->sensitive());
  expose();
  Container *container = dynamic_cast<Container*> (this);
  if (container)
    for (Container::ChildWalker it = container->local_children(); it.has_next(); it++)
      it->propagate_flags();
  if (!finalizing())
    sig_changed.emit(); /* notify changed() without invalidate() */
}

void
Item::set_flag (uint32 flag,
                bool   on)
{
  assert ((flag & (flag - 1)) == 0); /* single bit check */
  const uint propagate_flag_mask  = SENSITIVE | PARENT_SENSITIVE | PRELIGHT | IMPRESSED | HAS_FOCUS | HAS_DEFAULT;
  const uint invalidate_flag_mask = HEXPAND | VEXPAND | HSPREAD | VSPREAD | HSPREAD_CONTAINER | VSPREAD_CONTAINER | VISIBLE;
  bool fchanged = change_flags_silently (flag, on);
  if (fchanged)
    {
      if (flag & propagate_flag_mask)
        propagate_flags ();
      if (flag & invalidate_flag_mask)
        invalidate();
      changed();
    }
}

bool
Item::grab_focus () const
{
  return false;
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
Item::match_interface (InterfaceMatch &imatch,
                       const String   &ident)
{
  return imatch.done() || sig_find_interface.emit (imatch, ident) || ((!ident[0] || ident == name()) && imatch.match (this));
}

void
Item::finalize()
{
  sig_finalize.emit();
}

Item::~Item()
{
  if (parent_container())
    parent_container()->remove (this);
  if (m_style)
    {
      m_style->unref();
      m_style = NULL;
    }
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
  const char *cname = command_call_string.c_str();
  while ((*cname >= 'a' and *cname <= 'z') or
         (*cname >= 'A' and *cname <= 'Z') or
         (*cname >= '0' and *cname <= '9') or
         *cname == '-')
    cname++;
  String name = command_call_string.substr (0, cname - command_call_string.c_str());
  Item *item;

  String arg;
  while (*cname and (*cname == ' ' or *cname == '\t'))
    cname++;
  if (*cname == '(')
    {
      cname++;
      while (*cname and (*cname == ' ' or *cname == '\t'))
        cname++;
      const char *carg = cname;
      while (*cname and *cname != ')')
        cname++;
      if (*cname != ')')
        goto invalid_command;
      arg = String (carg, cname - carg);
    }
  else if (*cname)
    goto invalid_command;

  item = this;
  while (item)
    {
      Command *cmd = item->lookup_command (name);
      if (cmd)
        return cmd->exec (item, arg);
      item = item->parent();
    }
  return false;

 invalid_command:
  throw Exception ("Invalid command call: ", command_call_string.c_str());
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
    throw Exception ("no such property: ", property_name);
}

String
Item::get_property (const String   &property_name)
{
  Property *prop = lookup_property (property_name);
  if (!prop)
    throw Exception ("no such property: ", property_name);
  return prop->get_value (this);
}

const PropertyList&
Item::list_properties ()
{
  static Property *properties[] = {
    MakeProperty (Item, name,      _("Name"), _("Identification name of the item"), "", "rw"),
    MakeProperty (Item, visible,   _("Visible"), _("Whether this item is visible"), true, "rw"),
    MakeProperty (Item, sensitive, _("Sensitive"), _("Whether this item is sensitive (receives events)"), true, "rw"),
    MakeProperty (Item, hexpand,   _("Horizontal Expand"), _("Whether to expand this item horizontally"), false, "rw"),
    MakeProperty (Item, vexpand,   _("Vertical Expand"), _("Whether to expand this item vertically"), false, "rw"),
    MakeProperty (Item, hspread,   _("Horizontal Spread"), _("Whether to expand this item and all its parents horizontally"), false, "rw"),
    MakeProperty (Item, vspread,   _("Vertical Spread"), _("Whether to expand this item and all its parents vertically"), false, "rw"),
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

bool
Item::process_event (Event &event)
{
  EventHandler *controller = dynamic_cast<EventHandler*> (this);
  bool handled = false;
  if (controller)
    handled = controller->sig_event.emit (event);
  return handled;
}

void
Item::hierarchy_changed (Item *old_toplevel)
{
  anchored (old_toplevel == NULL);
}

void
Item::set_parent (Item *pitem)
{
  EventHandler *controller = dynamic_cast<EventHandler*> (this);
  if (controller)
    controller->reset();
  if (parent())
    {
      Root *rtoplevel = root();
      invalidate();
      style (NULL);
      m_parent = NULL;
      if (anchored() and rtoplevel)
        sig_hierarchy_changed.emit (rtoplevel);
    }
  if (pitem)
    {
      /* ensure parent items are always containers (see parent_container()) */
      if (!dynamic_cast<Container*> (pitem))
        throw Exception ("not setting non-Container item as parent: ", pitem->name());
      m_parent = pitem;
      style (m_parent->style());
      propagate_flags();
      invalidate();
      if (m_parent->anchored() and !anchored())
        sig_hierarchy_changed.emit (NULL);
    }
}

Container*
Item::parent_container() const
{
  return dynamic_cast<Container*> (m_parent); /* see set_parent() */
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

Root*
Item::root ()
{
  Item *parent = this;
  while (parent->parent())
    parent = parent->parent();
  return dynamic_cast<Root*> (parent); // NULL if parent is not of type Root*
}

void
Item::changed()
{
  if (test_flags (INVALIDATE_ON_CHANGE))
    invalidate();
  if (test_flags (EXPOSE_ON_CHANGE))
    expose();
  if (!finalizing())
    sig_changed.emit();
}

void
Item::invalidate()
{
  if (!test_flags (INVALID_REQUISITION | INVALID_ALLOCATION))
    {
      change_flags_silently (INVALID_REQUISITION | INVALID_ALLOCATION, true); /* skip notification */
      if (!finalizing())
        sig_invalidate.emit();
      if (parent())
        parent()->invalidate();
    }
}

void
ItemImpl::allocation (const Allocation &area)
{
  m_allocation = area;
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

void
ItemImpl::expose (const Allocation &area)
{
  if (parent() && !test_flags (INVALID_REQUISITION | INVALID_ALLOCATION))
    parent()->expose (area);
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
ItemImpl::point (double     x,
                 double     y,
                 Affine     affine)
{
  Allocation a = allocation();
  Point p = affine.ipoint (x, y);
  if (drawable() &&
      p.x >= a.x && p.x < a.x + a.width &&
      p.y >= a.y && p.y < a.y + a.height)
    return true;
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
ItemImpl::set_allocation (const Allocation &area)
{
  Allocation sarea = area;
  sarea.width = MAX (area.width, 0);
  sarea.height = MAX (area.height, 0);
  /* always reallocate to re-layout children */
  expose();
  change_flags_silently (INVALID_ALLOCATION, false); /* skip notification */
  size_allocate (sarea);
  Allocation a = allocation();
  set_flag (POSITIVE_ALLOCATION, a.width > 0 && a.height > 0);
  expose();
}

} // Rapicorn
