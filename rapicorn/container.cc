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
#include "container.hh"
#include "containerimpl.hh"
using namespace std;

namespace Rapicorn {

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
Container::add (Item                   &item,
                const PackPropertyList &pack_plist,
                PackPropertyList       *unused_props)
{
  if (item.parent())
    throw Exception ("not adding item with parent: ", item.name());
  Container &container = child_container();
  if (this != &container)
    {
      container.add (item, pack_plist, unused_props);
      return;
    }
  item.ref();
  try {
    container.add_child (item);
  } catch (...) {
    item.unref();
    throw;
  }
  /* always run packer code */
  Packer packer = create_packer (item);
  packer.apply_properties (pack_plist, unused_props);
  /* can invalidate etc. the fully setup item now */
  item.invalidate();
  item.unref();
}

void
Container::add (Item                   *item,
                const PackPropertyList &pack_plist,
                PackPropertyList       *unused_props)
{
  if (!item)
    throw NullPointer();
  add (*item, pack_plist, unused_props);
}

void
Container::remove (Item &item)
{
  Container *container = item.parent_container();
  if (!container)
    throw NullPointer();
  item.ref();
  item.invalidate();
  Container *dcontainer = container;
  while (dcontainer)
    {
      dcontainer->dispose_item (item);
      dcontainer = dcontainer->parent_container();
    }
  container->remove_child (item);
  item.unref();
}

Affine
Container::child_affine (Item &item)
{
  return Affine(); // Identity
}

void
Container::dispose_item (Item &item)
{
  if (&item == get_data (&child_container_key))
    child_container (NULL);
}

void
Container::hierarchy_changed (Item *old_toplevel)
{
  Item::hierarchy_changed (old_toplevel);
  for (ChildWalker cw = local_children(); cw.has_next(); cw++)
    cw->sig_hierarchy_changed.emit (old_toplevel);
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
Container::root_point_children (Point                   p, /* root coordinates relative */
                                std::vector<Item*>     &stack)
{
  point_children (point_from_root (p), stack);
}

bool
Container::match_interface (InterfaceMatch &imatch,
                            const String   &ident)
{
  if (imatch.done() ||
      sig_find_interface.emit (imatch, ident) ||
      ((!ident[0] || ident == name()) && imatch.match (this)))
    return true;
  for (ChildWalker cw = local_children(); cw.has_next(); cw++)
    if (cw->match_interface (imatch, ident))
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
      const Allocation area = cw->allocation();
      display.push_clip_rect (area.x, area.y, area.width, area.height);
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
  printf ("%s%s(%p) (%dx%d%+d%+d)\n", indent.c_str(), this->name().c_str(), this,
          allocation().width, allocation().height, allocation().x, allocation().y);
  for (ChildWalker cw = local_children(); cw.has_next(); cw++)
    {
      Item &child = *cw;
      Container *c = dynamic_cast<Container*> (&child);
      if (c)
        c->debug_tree (indent + "  ");
      else
        printf ("  %s%s(%p) (%dx%d%+d%+d)\n", indent.c_str(), child.name().c_str(), &child,
                child.allocation().width, child.allocation().height, child.allocation().x, child.allocation().y);
    }
}

Container::ChildPacker::ChildPacker ()
{}

Container::Packer::Packer (ChildPacker *cp) :
  m_child_packer (ref_sink (cp))
{}

Container::Packer::Packer (const Packer &src) :
  m_child_packer (ref_sink (src.m_child_packer))
{}

Property*
Container::Packer::lookup_property (const String &property_name)
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

String
Container::Packer::get_property (const String   &property_name)
{
  Property *prop = lookup_property (property_name);
  if (!prop)
    throw Exception ("no such property: ", property_name);
  m_child_packer->update();
  return prop->get_value (m_child_packer);
}

void
Container::Packer::set_property (const String    &property_name,
                                 const String    &value,
                                 const nothrow_t &nt)
{
  Property *prop = lookup_property (property_name);
  if (prop)
    {
      m_child_packer->update();
      prop->set_value (m_child_packer, value);
      m_child_packer->commit();
    }
  else if (&nt == &dothrow)
    throw Exception ("no such property: ", property_name);
}

void
Container::Packer::apply_properties (const PackPropertyList &pack_plist,
                                     PackPropertyList       *unused_props)
{
  m_child_packer->update();
  for (PackPropertyList::const_iterator it = pack_plist.begin(); it != pack_plist.end(); it++)
    {
      Property *prop = lookup_property (it->first);
      if (prop)
        prop->set_value (m_child_packer, it->second);
      else if (unused_props)
        (*unused_props)[it->first] = it->second;
      else
        throw Exception ("no such pack property: " + it->first);
    }
  m_child_packer->commit();
}

const PropertyList&
Container::Packer::list_properties()
{
  return m_child_packer->list_properties();
}

Container::Packer::~Packer ()
{
  unref (m_child_packer);
}

Container::Packer
Container::child_packer (Item &item)
{
  Container *container = item.parent_container();
  if (!container)
    throw NullPointer();
  return container->create_packer (item);
}

Container::ChildPacker*
Container::void_packer ()
{
  class PackerSingleton : public ChildPacker {
    PackerSingleton() { ref_sink(); }
    PRIVATE_CLASS_COPY (PackerSingleton);
  public:
    static PackerSingleton*
    dummy_packer()
    {
      static PackerSingleton *singleton = new PackerSingleton;
      return singleton;
    }
    ~PackerSingleton() { assert_not_reached(); }
    virtual const PropertyList&
    list_properties ()
    {
      static Property *properties[] = { };
      static const PropertyList property_list (properties);
      return property_list;
    }
    virtual void update () {}
    virtual void commit () {}
  };
  return PackerSingleton::dummy_packer();
}

SingleContainerImpl::SingleContainerImpl () :
  child_item (NULL)
{}

Container::ChildWalker
SingleContainerImpl::local_children ()
{
  Item **iter = &child_item, **iend = iter;
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
  item.set_parent (this);
  child_item = &item;
}

void
SingleContainerImpl::remove_child (Item &item)
{
  assert (child_item == &item); /* ensured by remove() */
  child_item = NULL;
  item.set_parent (NULL);
  item.unref();
}

void
SingleContainerImpl::size_request (Requisition &requisition)
{
  bool chspread = false, cvspread = false;
  if (has_visible_child())
    {
      Item &child = get_child();
      requisition = child.size_request ();
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
  if (has_visible_child())
    {
      Item &child = get_child();
      child.set_allocation (area);
    }
}

Container::Packer
SingleContainerImpl::create_packer (Item &item)
{
  return void_packer(); /* no child properties */
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
  item.set_parent (this);
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
        item.set_parent (NULL);
        item.unref();
        return;
      }
  assert_not_reached();
}

MultiContainerImpl::~MultiContainerImpl()
{
  while (items.size())
    remove (*items[items.size() - 1]);
}

} // Rapicorn
