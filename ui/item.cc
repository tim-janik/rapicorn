// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#include "item.hh"
#include "container.hh"
#include "adjustment.hh"
#include "window.hh"
#include "cmdlib.hh"
#include "sizegroup.hh"
#include "factory.hh"
#include "selector.hh"
#include "selob.hh"

#define SZDEBUG(...)    RAPICORN_KEY_DEBUG ("Sizing", __VA_ARGS__)

namespace Rapicorn {

struct ClassDoctor {
  static void update_widget_heritage (WidgetImpl &widget) { widget.heritage (widget.heritage()); }
};

EventHandler::EventHandler() :
  sig_event (Aida::slot (*this, &EventHandler::handle_event))
{}

bool
EventHandler::handle_event (const Event &event)
{
  return false;
}

WidgetImpl&
WidgetIface::impl ()
{
  WidgetImpl *iimpl = dynamic_cast<WidgetImpl*> (this);
  if (!iimpl)
    throw std::bad_cast();
  return *iimpl;
}

const WidgetImpl&
WidgetIface::impl () const
{
  const WidgetImpl *iimpl = dynamic_cast<const WidgetImpl*> (this);
  if (!iimpl)
    throw std::bad_cast();
  return *iimpl;
}

WidgetImpl::WidgetImpl () :
  flags_ (VISIBLE | SENSITIVE | ALLOCATABLE),
  parent_ (NULL),
  heritage_ (NULL),
  factory_context_ (NULL), // removing this breaks g++ pre-4.2.0 20060530
  ainfo_ (NULL),
  sig_changed (Aida::slot (*this, &WidgetImpl::do_changed)),
  sig_invalidate (Aida::slot (*this, &WidgetImpl::do_invalidate)),
  sig_hierarchy_changed (Aida::slot (*this, &WidgetImpl::hierarchy_changed))
{}

void
WidgetImpl::constructed()
{}

bool
WidgetImpl::viewable() const
{
  return drawable() && (!parent_ || parent_->viewable());
}

bool
WidgetImpl::self_visible () const
{
  return false;
}

bool
WidgetImpl::change_flags_silently (uint32 mask,
                                 bool   on)
{
  uint32 old_flags = flags_;
  if (on)
    flags_ |= mask;
  else
    flags_ &= ~mask;
  /* omit change notification */
  return old_flags != flags_;
}

void
WidgetImpl::propagate_state (bool notify_changed)
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
WidgetImpl::set_flag (uint32 flag, bool on)
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
WidgetImpl::grab_default () const
{
  return false;
}

void
WidgetImpl::sensitive (bool b)
{
  set_flag (SENSITIVE, b);
}

void
WidgetImpl::prelight (bool b)
{
  set_flag (PRELIGHT, b);
}

bool
WidgetImpl::branch_prelight () const
{
  const WidgetImpl *widget = this;
  while (widget)
    {
      if (widget->prelight())
        return true;
      widget = widget->parent();
    }
  return false;
}

void
WidgetImpl::impressed (bool b)
{
  set_flag (IMPRESSED, b);
}

bool
WidgetImpl::branch_impressed () const
{
  const WidgetImpl *widget = this;
  while (widget)
    {
      if (widget->impressed())
        return true;
      widget = widget->parent();
    }
  return false;
}

StateType
WidgetImpl::state () const
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
WidgetImpl::has_focus () const
{
  if (test_flags (FOCUS_CHAIN))
    {
      WindowImpl *rwidget = get_window();
      if (rwidget && rwidget->get_focus() == this)
        return true;
    }
  return false;
}

bool
WidgetImpl::can_focus () const
{
  return false;
}

bool
WidgetImpl::grab_focus ()
{
  if (has_focus())
    return true;
  if (!can_focus() || !sensitive() || !viewable())
    return false;
  /* unset old focus */
  WindowImpl *rwidget = get_window();
  if (rwidget)
    rwidget->set_focus (NULL);
  /* set new focus */
  rwidget = get_window();
  if (rwidget && rwidget->get_focus() == NULL)
    rwidget->set_focus (this);
  return rwidget->get_focus() == this;
}

bool
WidgetImpl::move_focus (FocusDirType fdir)
{
  if (!has_focus() && can_focus())
    return grab_focus();
  return false;
}

bool
WidgetImpl::activate_widget ()
{
  return false;
}

bool
WidgetImpl::activate ()
{
  if (!sensitive())
    return false;
  ContainerImpl *pcontainer = parent();
  while (pcontainer)
    {
      pcontainer->scroll_to_child (*this);
      pcontainer = pcontainer->parent();
    }
  return activate_widget();
}

void
WidgetImpl::notify_key_error ()
{
  WindowImpl *rwidget = get_window();
  if (rwidget)
    rwidget->beep();
}

size_t
WidgetImpl::cross_link (WidgetImpl &link, const WidgetSlot &uncross)
{
  assert_return (this != &link, 0);
  ContainerImpl *common_container = dynamic_cast<ContainerImpl*> (common_ancestor (link));
  assert_return (common_container != NULL, 0);
  return common_container->widget_cross_link (*this, link, uncross);
}

void
WidgetImpl::cross_unlink (WidgetImpl &link, size_t link_id)
{
  assert_return (this != &link);
  ContainerImpl *common_container = dynamic_cast<ContainerImpl*> (common_ancestor (link));
  assert_return (common_container != NULL);
  common_container->widget_cross_unlink (*this, link, link_id);
}

void
WidgetImpl::uncross_links (WidgetImpl &link)
{
  assert (this != &link);
  ContainerImpl *common_container = dynamic_cast<ContainerImpl*> (common_ancestor (link));
  assert (common_container != NULL);
  common_container->widget_uncross_links (*this, link);
}

bool
WidgetImpl::match_interface (bool wself, bool wparent, bool children, InterfaceMatcher &imatcher) const
{
  WidgetImpl *self = const_cast<WidgetImpl*> (this);
  if (wself && imatcher.match (self, name()))
    return true;
  if (wparent)
    {
      WidgetImpl *pwidget = parent();
      while (pwidget)
        {
          if (imatcher.match (pwidget, pwidget->name()))
            return true;
          pwidget = pwidget->parent();
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
WidgetImpl::match_selector (const String &selector)
{
  Selector::SelobAllocator sallocator;
  return Selector::Matcher::query_selector_bool (selector, *sallocator.widget_selob (*this));
}

WidgetIface*
WidgetImpl::query_selector (const String &selector)
{
  Selector::SelobAllocator sallocator;
  Selector::Selob *selob = Selector::Matcher::query_selector_first (selector, *sallocator.widget_selob (*this));
  return selob ? sallocator.selob_widget (*selob) : NULL;
}

WidgetSeq
WidgetImpl::query_selector_all (const String &selector)
{
  Selector::SelobAllocator sallocator;
  vector<Selector::Selob*> result = Selector::Matcher::query_selector_all (selector, *sallocator.widget_selob (*this));
  WidgetSeq widgets;
  for (vector<Selector::Selob*>::const_iterator it = result.begin(); it != result.end(); it++)
    {
      WidgetImpl *widget = sallocator.selob_widget (**it);
      if (widget)
        widgets.push_back (widget->*Aida::_handle);
    }
  return widgets;
}

WidgetIface*
WidgetImpl::query_selector_unique (const String &selector)
{
  Selector::SelobAllocator sallocator;
  Selector::Selob *selob = Selector::Matcher::query_selector_unique (selector, *sallocator.widget_selob (*this));
  return selob ? sallocator.selob_widget (*selob) : NULL;
}

uint
WidgetImpl::exec_slow_repeater (const EventLoop::BoolSlot &sl)
{
  WindowImpl *rwidget = get_window();
  if (rwidget)
    {
      EventLoop *loop = rwidget->get_loop();
      if (loop)
        return loop->exec_timer (250, 50, sl, EventLoop::PRIORITY_NOW);
    }
  return 0;
}

uint
WidgetImpl::exec_fast_repeater (const EventLoop::BoolSlot &sl)
{
  WindowImpl *rwidget = get_window();
  if (rwidget)
    {
      EventLoop *loop = rwidget->get_loop();
      if (loop)
        return loop->exec_timer (200, 20, sl, EventLoop::PRIORITY_NOW);
    }
  return 0;
}

uint
WidgetImpl::exec_key_repeater (const EventLoop::BoolSlot &sl)
{
  WindowImpl *rwidget = get_window();
  if (rwidget)
    {
      EventLoop *loop = rwidget->get_loop();
      if (loop)
        return loop->exec_timer (250, 33, sl, EventLoop::PRIORITY_NOW);
    }
  return 0;
}

bool
WidgetImpl::remove_exec (uint exec_id)
{
  WindowImpl *rwidget = get_window();
  if (rwidget)
    {
      EventLoop *loop = rwidget->get_loop();
      if (loop)
        return loop->try_remove (exec_id);
    }
  return false;
}

bool
WidgetImpl::clear_exec (uint *exec_id)
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
WidgetImpl::queue_visual_update ()
{
  uint timer_id = get_data (&visual_update_key);
  if (!timer_id)
    {
      WindowImpl *rwidget = get_window();
      if (rwidget)
        {
          EventLoop *loop = rwidget->get_loop();
          if (loop)
            {
              timer_id = loop->exec_timer (20, Aida::slot (*this, &WidgetImpl::force_visual_update));
              set_data (&visual_update_key, timer_id);
            }
        }
    }
}

void
WidgetImpl::force_visual_update ()
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
WidgetImpl::visual_update ()
{}

void
WidgetImpl::finalize()
{
  sig_finalize.emit();
}

WidgetImpl::~WidgetImpl()
{
  SizeGroup::delete_widget (*this);
  if (parent())
    parent()->remove (this);
  if (heritage_)
    {
      heritage_->unref();
      heritage_ = NULL;
    }
  uint timer_id = get_data (&visual_update_key);
  if (timer_id)
    {
      remove_exec (timer_id);
      set_data (&visual_update_key, uint (0));
    }
}

Command*
WidgetImpl::lookup_command (const String &command_name)
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
WidgetImpl::exec_command (const String &command_call_string)
{
  String cmd_name;
  StringSeq args;
  if (!command_scan (command_call_string, &cmd_name, &args))
    {
      critical ("Invalid command syntax: %s", command_call_string.c_str());
      return false;
    }
  cmd_name = string_strip (cmd_name);
  if (cmd_name == "")
    return true;

  WidgetImpl *widget = this;
  while (widget)
    {
      Command *cmd = widget->lookup_command (cmd_name);
      if (cmd)
        return cmd->exec (widget, args);
      widget = widget->parent();
    }

  if (command_lib_exec (*this, cmd_name, args))
    return true;

  widget = this;
  while (widget)
    {
      if (widget->custom_command (cmd_name, args))
        return true;
      widget = widget->parent();
    }
  critical ("toplevel failed to handle command"); // ultimately Window *always* handles commands
  return false;
}

bool
WidgetImpl::custom_command (const String    &command_name,
                          const StringSeq &command_args)
{
  return false;
}

const CommandList&
WidgetImpl::list_commands ()
{
  static Command *commands[] = {
  };
  static const CommandList command_list (commands);
  return command_list;
}

Property*
WidgetImpl::lookup_property (const String &property_name)
{
  return _property_lookup (property_name);
}

String
WidgetImpl::get_property (const String &property_name)
{
  return _property_get (property_name);
}

void
WidgetImpl::set_property (const String &property_name, const String &value)
{
  if (!_property_set (property_name, value))
    throw Exception ("no such property: " + name() + "::" + property_name);
}

const PropertyList&
WidgetImpl::list_properties ()
{
  return _property_list();
}

bool
WidgetImpl::try_set_property (const String &property_name, const String &value)
{
  return _property_set (property_name, value);
}

static class OvrKey : public DataKey<Requisition> {
  Requisition
  fallback()
  {
    return Requisition (-1, -1);
  }
} override_requisition;

double
WidgetImpl::width () const
{
  Requisition ovr = get_data (&override_requisition);
  return ovr.width >= 0 ? ovr.width : -1;
}

void
WidgetImpl::width (double w)
{
  Requisition ovr = get_data (&override_requisition);
  ovr.width = w >= 0 ? w : -1;
  set_data (&override_requisition, ovr);
  invalidate_size();
}

double
WidgetImpl::height () const
{
  Requisition ovr = get_data (&override_requisition);
  return ovr.height >= 0 ? ovr.height : -1;
}

void
WidgetImpl::height (double h)
{
  Requisition ovr = get_data (&override_requisition);
  ovr.height = h >= 0 ? h : -1;
  set_data (&override_requisition, ovr);
  invalidate_size();
}

void
WidgetImpl::propagate_heritage ()
{
  ContainerImpl *container = dynamic_cast<ContainerImpl*> (this);
  if (container)
    for (ContainerImpl::ChildWalker it = container->local_children(); it.has_next(); it++)
      it->heritage (heritage_);
}

void
WidgetImpl::heritage (Heritage *heritage)
{
  Heritage *old_heritage = heritage_;
  heritage_ = NULL;
  if (heritage)
    {
      heritage_ = heritage->adapt_heritage (*this, color_scheme());
      ref_sink (heritage_);
    }
  if (old_heritage)
    old_heritage->unref();
  if (heritage_ != old_heritage)
    {
      invalidate();
      propagate_heritage ();
    }
}

static bool
translate_from_ancestor (WidgetImpl         *ancestor,
                         const WidgetImpl   *child,
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
WidgetImpl::translate_from (const WidgetImpl   &src_widget,
                      const uint    n_points,
                      Point        *points) const
{
  WidgetImpl *ca = common_ancestor (src_widget);
  if (!ca)
    return false;
  WidgetImpl *widget = const_cast<WidgetImpl*> (&src_widget);
  while (widget != ca)
    {
      ContainerImpl *pc = widget->parent();
      Affine affine = pc->child_affine (*widget);
      affine.invert();
      for (uint i = 0; i < n_points; i++)
        points[i] = affine.point (points[i]);
      widget = pc;
    }
  bool can_translate = translate_from_ancestor (ca, this, n_points, points);
  if (!can_translate)
    return false;
  return true;
}

bool
WidgetImpl::translate_to (const uint    n_points,
                    Point        *points,
                    const WidgetImpl   &target_widget) const
{
  return target_widget.translate_from (*this, n_points, points);
}

bool
WidgetImpl::translate_from (const WidgetImpl   &src_widget,
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
  if (!translate_from (src_widget, points.size(), &points[0]))
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
WidgetImpl::translate_to (const uint    n_rects,
                    Rect         *rects,
                    const WidgetImpl   &target_widget) const
{
  return target_widget.translate_from (*this, n_rects, rects);
}

Point
WidgetImpl::point_from_screen_window (Point window_point) /* window coordinates relative */
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
WidgetImpl::point_to_screen_window (Point widget_point) /* widget coordinates relative */
{
  Point p = widget_point;
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
WidgetImpl::affine_from_screen_window () /* screen_window => widget affine */
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
WidgetImpl::affine_to_screen_window () /* widget => screen_window affine */
{
  Affine iaffine = affine_from_screen_window();
  if (!iaffine.is_identity())
    iaffine = iaffine.invert();
  return iaffine;
}

bool
WidgetImpl::process_event (const Event &event) /* widget coordinates relative */
{
  bool handled = false;
  EventHandler *controller = dynamic_cast<EventHandler*> (this);
  if (controller)
    handled = controller->sig_event.emit (event);
  return handled;
}

bool
WidgetImpl::process_screen_window_event (const Event &event) /* screen_window coordinates relative */
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
WidgetImpl::screen_window_point (Point p) /* window coordinates relative */
{
  return point (point_from_screen_window (p));
}

bool
WidgetImpl::point (Point p) /* widget coordinates relative */
{
  Allocation a = allocation();
  return (drawable() &&
          p.x >= a.x && p.x < a.x + a.width &&
          p.y >= a.y && p.y < a.y + a.height);
}

void
WidgetImpl::hierarchy_changed (WidgetImpl *old_toplevel)
{
  anchored (old_toplevel == NULL);
}

void
WidgetImpl::set_parent (ContainerImpl *pcontainer)
{
  EventHandler *controller = dynamic_cast<EventHandler*> (this);
  if (controller)
    controller->reset();
  ContainerImpl *pc = parent();
  if (pc)
    {
      ref (pc);
      WindowImpl *rtoplevel = get_window();
      invalidate();
      if (heritage())
        heritage (NULL);
      pc->unparent_child (*this);
      parent_ = NULL;
      ainfo_ = NULL;
      propagate_state (false); // propagate PARENT_VISIBLE, PARENT_SENSITIVE
      if (anchored() and rtoplevel)
        sig_hierarchy_changed.emit (rtoplevel);
      unref (pc);
    }
  if (pcontainer)
    {
      /* ensure parent widgets are always containers (see parent()) */
      if (!dynamic_cast<ContainerImpl*> (pcontainer))
        throw Exception ("not setting non-Container widget as parent: ", pcontainer->name());
      parent_ = pcontainer;
      ainfo_ = NULL;
      if (parent_->heritage())
        heritage (parent_->heritage());
      invalidate();
      propagate_state (true);
      if (parent_->anchored() and !anchored())
        sig_hierarchy_changed.emit (NULL);
    }
}

bool
WidgetImpl::has_ancestor (const WidgetImpl &ancestor) const
{
  const WidgetImpl *widget = this;
  while (widget)
    {
      if (widget == &ancestor)
        return true;
      widget = widget->parent();
    }
  return false;
}

WidgetImpl*
WidgetImpl::common_ancestor (const WidgetImpl &other) const
{
  WidgetImpl *widget1 = const_cast<WidgetImpl*> (this);
  do
    {
      WidgetImpl *widget2 = const_cast<WidgetImpl*> (&other);
      do
        {
          if (widget1 == widget2)
            return widget1;
          widget2 = widget2->parent();
        }
      while (widget2);
      widget1 = widget1->parent();
    }
  while (widget1);
  return NULL;
}

WindowImpl*
WidgetImpl::get_window () const
{
  const AnchorInfo *ainfo = anchor_info();
  if (ainfo)
    return ainfo->window;
  WidgetImpl *widget = const_cast<WidgetImpl*> (this);
  while (widget->parent_)
    widget = widget->parent_;
  return dynamic_cast<WindowImpl*> (widget); // NULL iff this is not type WindowImpl*
}

ViewportImpl*
WidgetImpl::get_viewport () const
{
  const AnchorInfo *ainfo = anchor_info();
  if (ainfo)
    return ainfo->viewport;
  for (WidgetImpl *widget = const_cast<WidgetImpl*> (this); widget; widget = widget->parent_)
    {
      ViewportImpl *v = dynamic_cast<ViewportImpl*> (widget);
      if (v)
        return v;
    }
  return NULL;
}

ResizeContainerImpl*
WidgetImpl::get_resize_container () const
{
  const AnchorInfo *ainfo = anchor_info();
  if (ainfo)
    return ainfo->resize_container;
  for (WidgetImpl *widget = const_cast<WidgetImpl*> (this); widget; widget = widget->parent_)
    {
      ResizeContainerImpl *c = dynamic_cast<ResizeContainerImpl*> (widget);
      if (c)
        return c;
    }
  return NULL;
}

const AnchorInfo*
WidgetImpl::force_anchor_info () const
{
  if (ainfo_)
    return ainfo_;
  // find resize container
  WidgetImpl *parent = const_cast<WidgetImpl*> (this);
  ResizeContainerImpl *rc = NULL;
  while (parent)
    {
      rc = dynamic_cast<ResizeContainerImpl*> (parent);
      if (rc)
        break;
      parent = parent->parent();
    }
  static const AnchorInfo orphan_anchor_info;
  const AnchorInfo *ainfo = rc ? rc->container_anchor_info() : &orphan_anchor_info;
  const_cast<WidgetImpl*> (this)->ainfo_ = ainfo;
  if (anchored())
    assert (get_window() != NULL); // FIXME
  return ainfo;
}

void
WidgetImpl::changed()
{
  if (!finalizing())
    sig_changed.emit();
}

void
WidgetImpl::invalidate_parent ()
{
  /* propagate (size) invalidation from children to parents */
  WidgetImpl *p = parent();
  if (p)
    p->invalidate_size();
}

void
WidgetImpl::invalidate()
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
      SizeGroup::invalidate_widget (*this);
    }
}

void
WidgetImpl::invalidate_size()
{
  if (!test_all_flags (INVALID_REQUISITION | INVALID_ALLOCATION))
    {
      change_flags_silently (INVALID_REQUISITION | INVALID_ALLOCATION, true); /* skip notification */
      if (!finalizing())
        sig_invalidate.emit();
      invalidate_parent(); /* need new size-request on parent */
      SizeGroup::invalidate_widget (*this);
    }
}

Requisition
WidgetImpl::inner_size_request()
{
  /* loop until we gather a valid requisition, this explicitely allows for
   * requisition invalidation during the size_request phase, widget implementations
   * have to ensure we're not looping endlessly
   */
  while (test_flags (WidgetImpl::INVALID_REQUISITION))
    {
      change_flags_silently (WidgetImpl::INVALID_REQUISITION, false); // skip notification
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
      requisition_ = inner;
    }
  return allocatable() ? requisition_ : Requisition();
}

Requisition
WidgetImpl::requisition ()
{
  return SizeGroup::widget_requisition (*this);
}

bool
WidgetImpl::tune_requisition (double new_width,
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
WidgetImpl::expose_internal (const Region &region)
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
WidgetImpl::expose (const Region &region) // widget relative
{
  if (drawable() && !test_flags (INVALID_CONTENT))
    {
      Region r (allocation());
      r.intersect (region);
      expose_internal (r);
    }
}

void
WidgetImpl::type_cast_error (const char *dest_type)
{
  fatal ("failed to dynamic_cast<%s> widget: %s", VirtualTypeid::cxx_demangle (dest_type).c_str(), name().c_str());
}

String
WidgetImpl::test_dump ()
{
  TestStream *tstream = TestStream::create_test_stream();
  make_test_dump (*tstream);
  String s = tstream->string();
  delete tstream;
  return s;
}

void
WidgetImpl::make_test_dump (TestStream &tstream)
{
  tstream.push_node (name());
  const PropertyList &plist = list_properties();
  size_t n_properties = 0;
  Aida::Property **properties = plist.list_properties (&n_properties);
  for (uint i = 0; i < n_properties; i++)
    {
      Property *property = dynamic_cast<Property*> (properties[i]);
      if (property && property->readable())
        {
          String value = get_property (property->ident);
          tstream.dump (property->ident, value);
        }
    }
  tstream.push_indent();
  dump_private_data (tstream);
  dump_test_data (tstream);
  tstream.pop_indent();
  tstream.pop_node ();
}

void
WidgetImpl::dump_test_data (TestStream &tstream)
{}

void
WidgetImpl::dump_private_data (TestStream &tstream)
{}

void
WidgetImpl::find_adjustments (AdjustmentSourceType adjsrc1,
                        Adjustment         **adj1,
                        AdjustmentSourceType adjsrc2,
                        Adjustment         **adj2,
                        AdjustmentSourceType adjsrc3,
                        Adjustment         **adj3,
                        AdjustmentSourceType adjsrc4,
                        Adjustment         **adj4)
{
  for (WidgetImpl *pwidget = this->parent(); pwidget; pwidget = pwidget->parent())
    {
      AdjustmentSource *adjustment_source = pwidget->interface<AdjustmentSource*>();
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
WidgetImpl::repack (const PackInfo &orig,
              const PackInfo &pnew)
{
  if (parent())
    parent()->repack_child (*this, orig, pnew);
  invalidate();
}

WidgetImpl::PackInfo&
WidgetImpl::pack_info (bool create)
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
WidgetImpl::hposition (double d)
{
  PackInfo &pa = pack_info (true), op = pa;
  pa.hposition = d;
  repack (op, pa);
}

void
WidgetImpl::hspan (double d)
{
  PackInfo &pa = pack_info (true), op = pa;
  pa.hspan = MAX (1, d);
  repack (op, pa);
}

void
WidgetImpl::vposition (double d)
{
  PackInfo &pa = pack_info (true), op = pa;
  pa.vposition = d;
  repack (op, pa);
}

void
WidgetImpl::vspan (double d)
{
  PackInfo &pa = pack_info (true), op = pa;
  pa.vspan = MAX (1, d);
  repack (op, pa);
}

void
WidgetImpl::left_spacing (int s)
{
  PackInfo &pa = pack_info (true), op = pa;
  pa.left_spacing = MAX (0, s);
  repack (op, pa);
}

void
WidgetImpl::right_spacing (int s)
{
  PackInfo &pa = pack_info (true), op = pa;
  pa.right_spacing = MAX (0, s);
  repack (op, pa);
}

void
WidgetImpl::bottom_spacing (int s)
{
  PackInfo &pa = pack_info (true), op = pa;
  pa.bottom_spacing = MAX (0, s);
  repack (op, pa);
}

void
WidgetImpl::top_spacing (int s)
{
  PackInfo &pa = pack_info (true), op = pa;
  pa.top_spacing = MAX (0, s);
  repack (op, pa);
}

void
WidgetImpl::halign (double f)
{
  PackInfo &pa = pack_info (true), op = pa;
  pa.halign = CLAMP (f, 0, 1);
  repack (op, pa);
}

void
WidgetImpl::hscale (double f)
{
  PackInfo &pa = pack_info (true), op = pa;
  pa.hscale = f;
  repack (op, pa);
}

void
WidgetImpl::valign (double f)
{
  PackInfo &pa = pack_info (true), op = pa;
  pa.valign = CLAMP (f, 0, 1);
  repack (op, pa);
}

void
WidgetImpl::vscale (double f)
{
  PackInfo &pa = pack_info (true), op = pa;
  pa.vscale = f;
  repack (op, pa);
}

void
WidgetImpl::do_changed()
{
}

void
WidgetImpl::do_invalidate()
{
}

bool
WidgetImpl::do_event (const Event &event)
{
  return false;
}

static DataKey<String> widget_name_key;

String
WidgetImpl::name () const
{
  String s = get_data (&widget_name_key);
  if (s.empty())
    return Factory::factory_context_name (factory_context());
  else
    return s;
}

void
WidgetImpl::name (const String &str)
{
  if (str.empty())
    delete_data (&widget_name_key);
  else
    set_data (&widget_name_key, str);
}

FactoryContext*
WidgetImpl::factory_context () const
{
  return factory_context_;
}

void
WidgetImpl::factory_context (FactoryContext *fc)
{
  if (fc)
    assert_return (factory_context_ == NULL);
  factory_context_ = fc;
}

UserSource
WidgetImpl::user_source () const
{
  return Factory::factory_context_source (factory_context());
}

static DataKey<ColorSchemeType> widget_color_scheme_key;

ColorSchemeType
WidgetImpl::color_scheme () const
{
  return get_data (&widget_color_scheme_key);
}

void
WidgetImpl::color_scheme (ColorSchemeType cst)
{
  if (cst != get_data (&widget_color_scheme_key))
    {
      if (!cst)
        delete_data (&widget_color_scheme_key);
      else
        set_data (&widget_color_scheme_key, cst);
      ClassDoctor::update_widget_heritage (*this);
    }
}

bool
WidgetImpl::tune_requisition (Requisition requisition)
{
  WidgetImpl *p = parent();
  if (p && !test_flags (INVALID_REQUISITION))
    {
      ResizeContainerImpl *rc = p->get_resize_container();
      if (rc && rc->requisitions_tunable())
        {
          Requisition ovr (width(), height());
          requisition.width = ovr.width >= 0 ? ovr.width : MAX (requisition.width, 0);
          requisition.height = ovr.height >= 0 ? ovr.height : MAX (requisition.height, 0);
          if (requisition.width != requisition_.width || requisition.height != requisition_.height)
            {
              requisition_ = requisition;
              invalidate_parent(); // need new size-request on parent
              return true;
            }
        }
    }
  return false;
}

void
WidgetImpl::set_allocation (const Allocation &area)
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
  const bool changed = allocation_ != sarea;
  allocation_ = sarea;
  size_allocate (allocation_, changed);
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
class WidgetImpl::RenderContext {
  friend class WidgetImpl;
  vector<cairo_surface_t*> surfaces;
  Region                   render_area;
  vector<cairo_t*>         cairos;
public:
  /*dtor*/     ~RenderContext();
  const Region& region() const { return render_area; }
};

void
WidgetImpl::render_into (cairo_t *cr, const Region &region)
{
  RenderContext rcontext;
  rcontext.render_area = allocation();
  rcontext.render_area.intersect (region);
  if (!rcontext.render_area.empty())
    {
      render_widget (rcontext);
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
WidgetImpl::render_widget (RenderContext &rcontext)
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
WidgetImpl::render (RenderContext &rcontext, const Rect &rect)
{}

const Region&
WidgetImpl::rendering_region (RenderContext &rcontext) const
{
  return rcontext.render_area;
}

cairo_t*
WidgetImpl::cairo_context (RenderContext  &rcontext,
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

WidgetImpl::RenderContext::~RenderContext()
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

// == WidgetIfaceVector ==
WidgetIfaceVector::WidgetIfaceVector (const WidgetSeq &widgetseq)
{
  for (auto it : widgetseq)
    push_back (it->*Aida::_servant);
}

WidgetSeq
WidgetIfaceVector::to_widget_seq () const
{
  WidgetSeq itseq;
  for (auto it : *this)
    itseq.push_back (it->*Aida::_handle);
  return itseq;
}

} // Rapicorn
