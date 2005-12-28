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
Container::add (Item &item, const PackPropertyList &pack_plist)
{
  if (item.parent())
    throw Exception ("not adding item with parent: ", item.name());
  item.ref();
  Container &container = child_container();
  if (this != &container)
    {
      container.add (item, pack_plist);
      return;
    }
  bool added = container.add_child (item, pack_plist);
  item.unref();
  if (added)
    item.invalidate();
  else
    throw Exception ("invalid attempt to add child: ", item.name());
}

void
Container::add (Item *item, const PackPropertyList &pack_plist)
{
  if (!item)
    throw NullPointer();
  add (*item, pack_plist);
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

void
Container::point_children (double                 x,
                           double                 y,
                           Affine                 affine,
                           std::vector<Item*>    &stack)
{
  Affine a = affine;
  for (ChildWalker cw = local_children(); cw.has_next(); cw++)
    if (cw->point (x, y, a))
      {
        cw->ref();
        stack.push_back (&*cw);
        Container *c = dynamic_cast<Container*> (&*cw);
        if (c)
          c->point_children (x, y, affine, stack);
      }
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
Container::Packer::apply_properties (const PackPropertyList &pack_plist)
{
  m_child_packer->update();
  for (PackPropertyList::const_iterator it = pack_plist.begin(); it != pack_plist.end(); it++)
    set_property (it->first, it->second, nothrow);
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

bool
SingleContainerImpl::add_child (Item &item, const PackPropertyList &pack_plist)
{
  if (!child_item)
    {
      item.ref_sink();
      item.set_parent (this);
      child_item = &item;
      return true;
    }
  else
    return false;
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

void
ControlArea::control (const String &command)
{
  uint pos = command.find ("::");
  String command_name, arg;
  if (pos != command.npos)
    {
      arg = command.substr (pos + 2);
      command_name = command.substr (0, pos);
    }
  else
    command_name = command;
  control (command_name, arg);
}

MultiContainerImpl::MultiContainerImpl ()
{}

bool
MultiContainerImpl::add_child (Item &item, const PackPropertyList &pack_plist)
{
  item.ref_sink();
  item.set_parent (this);
  items.push_back (&item);
  return true;
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

Root::Root() :
  sig_expose (*this)
{
  change_flags_silently (ANCHORED, true);       /* root is always anchored */
}

class RootImpl : public Root, public SingleContainerImpl {
public:
  RootImpl()
  {
    Appearance *appearance = Appearance::create_default();
    style (appearance->create_style ("normal"));
    unref (appearance);
    set_flag (PARENT_SENSITIVE, true);
  }
  ~RootImpl()
  {
    /* make sure all children are removed while this is still of type Root.
     * necessary because C++ alters the object type during constructors and destructors
     */
    if (has_children())
      remove (get_child());
  }
  virtual void
  size_request (Requisition &requisition)
  {
    if (has_visible_child())
      {
        Item &child = get_child();
        requisition = child.size_request();
      }
  }
  virtual void
  size_allocate (Allocation area)
  {
    allocation (area);
    if (!has_visible_child())
      return;
    Item &child = get_child();
    Requisition rq = child.size_request();
    child.set_allocation (area);
  }
  virtual void
  expose (const Allocation &area)
  {
    sig_expose.emit (area);
  }
protected:
  vector<Item*>
  item_difference (const vector<Item*> &clist, /* preserve order of clist */
                   const vector<Item*> &cminus)
  {
    map<Item*,bool> mminus;
    for (uint i = 0; i < cminus.size(); i++)
      mminus[cminus[i]] = true;
    vector<Item*> result;
    for (uint i = 0; i < clist.size(); i++)
      if (!mminus[clist[i]])
        result.push_back (clist[i]);
    return result;
  }
  EventContext last_event_context;
  vector<Item*> last_entered_children;
  bool
  dispatch_mouse_movement (const EventContext &econtext)
  {
    last_event_context = econtext;
    EventMouse &mevent = *create_event_mouse (MOUSE_MOVE, econtext);
    vector<Item*> pierced;
    /* figure all entered children */
    bool unconfined;
    Item *grab_item = get_grab (&unconfined);
    if (grab_item)
      {
        if (unconfined or grab_item->point (mevent.x, mevent.y, Affine()))
          pierced.push_back (ref (grab_item));
      }
    else if (drawable())
      {
        pierced.push_back (ref (this)); /* root receives all mouse events */
        point_children (mevent.x, mevent.y, Affine(), pierced);
      }
    /* send leave events */
    vector<Item*> left_children = item_difference (last_entered_children, pierced);
    mevent.type = MOUSE_LEAVE;
    for (vector<Item*>::reverse_iterator it = left_children.rbegin(); it != left_children.rend(); it++)
      (*it)->process_event (mevent);
    /* send enter events */
    vector<Item*> entered_children = item_difference (pierced, last_entered_children);
    mevent.type = MOUSE_ENTER;
    for (vector<Item*>::reverse_iterator it = entered_children.rbegin(); it != entered_children.rend(); it++)
      (*it)->process_event (mevent);
    /* send actual move event */
    bool handled = false;
    mevent.type = MOUSE_MOVE;
    for (vector<Item*>::reverse_iterator it = pierced.rbegin(); it != pierced.rend(); it++)
      if (!handled && (*it)->sensitive())
        handled = (*it)->process_event (mevent);
    /* cleanup */
    delete &mevent;
    for (vector<Item*>::reverse_iterator it = last_entered_children.rbegin(); it != last_entered_children.rend(); it++)
      (*it)->unref();
    last_entered_children = pierced;
    return handled;
  }
  bool
  dispatch_event_to_pierced_or_grab (Event &event)
  {
    vector<Item*> pierced;
    /* figure all entered children */
    Item *grab_item = get_grab();
    if (grab_item)
      pierced.push_back (ref (grab_item));
    else if (drawable())
      {
        pierced.push_back (ref (this)); /* root receives all events */
        point_children (event.x, event.y, Affine(), pierced);
      }
    /* send actual event */
    bool handled = false;
    for (vector<Item*>::reverse_iterator it = pierced.rbegin(); it != pierced.rend(); it++)
      {
        if (!handled && (*it)->sensitive())
          handled = (*it)->process_event (event);
        (*it)->unref();
      }
    return handled;
  }
private:
  struct ButtonState {
    Item *item;
    uint button;
    ButtonState (Item *i, uint b) : item (i), button (b) {}
    ButtonState () : item (NULL), button (0) {}
    bool operator< (const ButtonState &bs2) const
    {
      const ButtonState &bs1 = *this;
      return bs1.item < bs2.item || (bs1.item == bs2.item &&
                                     bs1.button < bs2.button);
    }
  };
  map<ButtonState,uint> button_state_map;
  bool
  dispatch_button_press (const EventContext &econtext,
                         uint                button,
                         uint                press_count)
  {
    assert (press_count >= 1 && press_count <= 3);
    EventButton &bevent = *create_event_button (press_count == 3 ? BUTTON_3PRESS : press_count == 2 ? BUTTON_2PRESS : BUTTON_PRESS, econtext, button);
    /* figure all entered children */
    const vector<Item*> &pierced = last_entered_children;
    /* send actual event */
    bool handled = false;
    for (vector<Item*>::const_reverse_iterator it = pierced.rbegin(); it != pierced.rend(); it++)
      if (!handled && (*it)->sensitive())
        {
          ButtonState bs (*it, bevent.button);
          if (button_state_map[bs] == 0)                /* no press delivered for <button> on <item> yet */
            {
              button_state_map[bs] = press_count;       /* record single press */
              handled = (*it)->process_event (bevent);
            }
        }
    delete &bevent;
    return handled;
  }
  bool
  dispatch_button_release (const EventContext &econtext,
                           uint                button)
  {
    EventButton &bevent = *create_event_button (BUTTON_RELEASE, econtext, button);
    bool handled = false;
    for (map<ButtonState,uint>::iterator it = button_state_map.begin(); it != button_state_map.end();)
      {
        const ButtonState &bs = it->first;
        uint press_count = it->second;
        map<ButtonState,uint>::iterator current = it++;
        if (bs.button == button)
          {
            if (press_count == 3)
              bevent.type = BUTTON_3RELEASE;
            else if (press_count == 2)
              bevent.type = BUTTON_2RELEASE;
            handled |= bs.item->process_event (bevent);
            button_state_map.erase (current);
          }
      }
    bevent.type = BUTTON_RELEASE;
    delete &bevent;
    return handled;
  }
  void
  cancel_item_events (Item *item)
  {
    /* cancel enter events */
    for (int i = last_entered_children.size(); i > 0;)
      {
        Item *current = last_entered_children[--i]; /* walk backwards */
        if (item == current || !item)
          {
            EventMouse *mevent = create_event_mouse (MOUSE_LEAVE, last_event_context);
            current->process_event (*mevent);
            delete mevent;
            current->unref();
            last_entered_children.erase (last_entered_children.begin() + i);
          }
      }
    /* cancel button press events */
    for (map<ButtonState,uint>::iterator it = button_state_map.begin(); it != button_state_map.end();)
      {
        const ButtonState &bs = it->first;
        map<ButtonState,uint>::iterator current = it++;
        if (bs.item == item || !item)
          {
            EventButton *bevent = create_event_button (BUTTON_CANCELED, last_event_context, bs.button);
            bs.item->process_event (*bevent);
            delete bevent;
            button_state_map.erase (current);
          }
      }
  }
  virtual void cancel_item_events (Item &item) { cancel_item_events (&item); }
public:
  virtual void
  dispatch_cancel_events ()
  {
    cancel_item_events (NULL);
  }
  virtual bool
  dispatch_button_event (const EventContext &econtext,
                         bool                is_press,
                         uint                button)
  {
    bool handled = false;
    dispatch_mouse_movement (econtext);
    if (is_press)
      handled = dispatch_button_press (econtext, button, 1);
    else
      handled = dispatch_button_release (econtext, button);
    dispatch_mouse_movement (econtext);
    return handled;
  }
  virtual bool
  dispatch_leave_event (const EventContext &econtext)
  {
    dispatch_mouse_movement (econtext);
    EventMouse &mevent = *create_event_mouse (MOUSE_LEAVE, econtext);
    /* send leave events */
    while (last_entered_children.size())
      {
        Item *item = last_entered_children.back();
        last_entered_children.pop_back();
        item->process_event (mevent);
        item->unref();
      }
    delete &mevent;
    return false;
  }
  virtual bool
  dispatch_move_event (const EventContext &econtext)
  {
    bool handled = dispatch_mouse_movement (econtext);
    return handled;
  }
  
  virtual bool
  dispatch_focus_event (const EventContext &econtext,
                        bool                is_in)
  {
    dispatch_mouse_movement (econtext);
    EventFocus *fevent = create_event_focus (is_in ? FOCUS_IN : FOCUS_OUT, econtext);
    bool handled = false; // dispatch_event_to_pierced_or_grab (*fevent);
    delete fevent;
    return handled;
  }
  virtual bool
  dispatch_key_event (const EventContext &econtext,
                      bool                is_press,
                      KeyValue            key,
                      const char         *key_name)
  {
    dispatch_mouse_movement (econtext);
    EventKey *kevent = create_event_key (is_press ? KEY_PRESS : KEY_RELEASE, econtext, key, key_name);
    Item *grab_item = get_grab();
    grab_item = grab_item ? grab_item : this;
    bool handled = grab_item->process_event (*kevent);
    delete kevent;
    return handled;
  }
  virtual bool
  dispatch_scroll_event (const EventContext &econtext,
                         EventType           scroll_type)
  {
    bool handled = false;
    if (scroll_type == SCROLL_UP || scroll_type == SCROLL_RIGHT || scroll_type == SCROLL_DOWN || scroll_type == SCROLL_LEFT)
      {
        dispatch_mouse_movement (econtext);
        EventScroll *sevent = create_event_scroll (scroll_type, econtext);
        handled = dispatch_event_to_pierced_or_grab (*sevent);
        delete sevent;
      }
    return handled;
  }
private:
  struct GrabEntry {
    Item *item;
    bool  unconfined;
    GrabEntry (Item *i, bool uc) :
      item (i),
      unconfined (uc)
    {}
  };
  vector<GrabEntry> grab_stack;
  virtual void
  remove_grab_item (Item &child)
  {
    bool stack_changed = false;
    for (int i = grab_stack.size() - 1; i >= 0; i--)
      if (grab_stack[i].item == &child)
        {
          grab_stack.erase (grab_stack.begin() + i);
          stack_changed = true;
        }
    if (stack_changed)
      grab_stack_changed();
  }
  void
  grab_stack_changed()
  {
    dispatch_move_event (last_event_context);
  }
public:
  virtual void
  add_grab (Item &child,
            bool  unconfined)
  {
    if (!child.has_ancestor (*this))
      throw Exception ("child is not descendant of container \"", name(), "\": ", child.name());
    grab_stack.push_back (GrabEntry (&child, unconfined));
  }
  virtual void
  remove_grab (Item &child)
  {
    for (int i = grab_stack.size() - 1; i >= 0; i--)
      if (grab_stack[i].item == &child)
        {
          grab_stack.erase (grab_stack.begin() + i);
          grab_stack_changed();
          return;
        }
    throw Exception ("no such child in grab stack: ", child.name());
  }
  virtual Item*
  get_grab (bool *unconfined = NULL)
  {
    for (int i = grab_stack.size() - 1; i >= 0; i--)
      if (grab_stack[i].item->visible())
        {
          if (unconfined)
            *unconfined = grab_stack[i].unconfined;
          return grab_stack[i].item;
        }
    return NULL;
  }
  virtual void
  dispose_item (Item &item)
  {
    remove_grab_item (item);
    cancel_item_events (item);
    SingleContainerImpl::dispose_item (item);
  }
  using Item::render;
  virtual void
  render (Plane &plane)
  {
    plane.fill (background());
    Display display;
    const Allocation &a = allocation();
    display.push_clip_rect (a.x, a.y, a.width, a.height);
    display.push_clip_rect (plane.rect());
    if (!display.empty())
      render (display);
    display.render_combined (plane);
    display.pop_clip_rect();
    display.pop_clip_rect();
  }
};

static const ItemFactory<RootImpl> root_factory ("Rapicorn::Root");

} // Rapicorn
