// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#include "scrollwidgets.hh"
#include "window.hh"
#include "factory.hh"

namespace Rapicorn {

// == ScrollAreaImpl ==
ScrollAreaImpl::ScrollAreaImpl() :
  hadjustment_ (NULL), vadjustment_ (NULL)
{}

Adjustment&
ScrollAreaImpl::hadjustment ()
{
  if (!hadjustment_)
    hadjustment_ = Adjustment::create (0, 0, 1, 0.01, 0.2);
  return *hadjustment_;
}

Adjustment&
ScrollAreaImpl::vadjustment ()
{
  if (!vadjustment_)
    vadjustment_ = Adjustment::create (0, 0, 1, 0.01, 0.2);
  return *vadjustment_;
}

Adjustment*
ScrollAreaImpl::get_adjustment (AdjustmentSourceType adj_source, const String &name)
{
  switch (adj_source)
    {
    case AdjustmentSourceType::ANCESTRY_HORIZONTAL:
      return &hadjustment();
    case AdjustmentSourceType::ANCESTRY_VERTICAL:
      return &vadjustment();
    default:
      return NULL;
    }
}

double
ScrollAreaImpl::x_offset ()
{
  return round (hadjustment().value());
}

double
ScrollAreaImpl::y_offset ()
{
  return round (vadjustment().value());
}

void
ScrollAreaImpl::scroll_to (double x, double y)
{
  hadjustment().freeze();
  vadjustment().freeze();
  hadjustment_->value (round (x));
  vadjustment_->value (round (y));
  hadjustment_->constrain();
  vadjustment_->constrain();
  hadjustment_->thaw();
  vadjustment_->thaw();
}

static const WidgetFactory<ScrollAreaImpl> scroll_area_factory ("Rapicorn::ScrollArea");

// == ScrollPortImpl ==
ScrollPortImpl::ScrollPortImpl() :
  hadjustment_ (NULL), vadjustment_ (NULL), conid_hadjustment_ (0), conid_vadjustment_ (0), fix_id_ (0),
  sig_scrolled (Aida::slot (*this, &ScrollPortImpl::do_scrolled))
{}

void
ScrollPortImpl::hierarchy_changed (WidgetImpl *old_toplevel)
{
  if (hadjustment_ && conid_hadjustment_)
    hadjustment_->sig_value_changed() -= conid_hadjustment_;
  hadjustment_ = NULL;
  conid_hadjustment_ = 0;
  if (vadjustment_ && conid_vadjustment_)
    vadjustment_->sig_value_changed() -= conid_vadjustment_;
  vadjustment_ = NULL;
  conid_vadjustment_ = 0;
  ResizeContainerImpl::hierarchy_changed (old_toplevel);
  if (anchored())
    {
      find_adjustments (AdjustmentSourceType::ANCESTRY_HORIZONTAL, &hadjustment_,
                        AdjustmentSourceType::ANCESTRY_VERTICAL, &vadjustment_);
      if (hadjustment_)
        conid_hadjustment_ = hadjustment_->sig_value_changed() += Aida::slot (*this, &ScrollPortImpl::adjustment_changed);
      if (vadjustment_)
        conid_vadjustment_ = vadjustment_->sig_value_changed() += Aida::slot (*this, &ScrollPortImpl::adjustment_changed);
    }
}

void
ScrollPortImpl::fix_adjustments ()
{
  assert_return (fix_id_ != 0);
  fix_id_ = 0;
  if (!has_visible_child() || test_any (INVALID_REQUISITION | INVALID_ALLOCATION))
    return;
  /* size allocation negotiation may re-allocate the widget several times with new tentative sizes.
   * since some combinations could lead to the adjustments contraining the current scroll values
   * undesirably and loosing the last scroll position, we fix up the adjustments once the dust settled.
   */
  WidgetImpl &child = get_child();
  const Allocation area = child.allocation();
  const double small_step = 10;
  if (hadjustment_)
    {
      hadjustment_->freeze();
      hadjustment_->lower (0);
      hadjustment_->upper (area.width);
      double d, w = allocation().width, l = min (w, 1);
      hadjustment_->page (w);
      hadjustment_->step_increment (min (small_step, round (hadjustment_->page() / 10)));
      hadjustment_->page_increment (max (small_step, round (hadjustment_->page() / 3)));
      w = MIN (hadjustment_->page(), area.width);
      d = hadjustment_->step_increment ();
      hadjustment_->step_increment (CLAMP (d, l, w));
      d = hadjustment_->page_increment ();
      hadjustment_->page_increment (CLAMP (d, l, w));
    }
  if (vadjustment_)
    {
      vadjustment_->freeze();
      vadjustment_->lower (0);
      vadjustment_->upper (area.height);
      double d, h = allocation().height, l = min (h, 1);
      vadjustment_->page (h);
      vadjustment_->step_increment (min (small_step, round (vadjustment_->page() / 10)));
      vadjustment_->page_increment (max (small_step, round (vadjustment_->page() / 3)));
      h = MIN (vadjustment_->page(), area.height);
      d = vadjustment_->step_increment ();
      vadjustment_->step_increment (CLAMP (d, l, h));
      d = vadjustment_->page_increment ();
      vadjustment_->page_increment (CLAMP (d, l, h));
    }
  if (hadjustment_)
    hadjustment_->constrain();
  if (vadjustment_)
    vadjustment_->constrain();
  if (hadjustment_)
    hadjustment_->thaw();
  if (vadjustment_)
    vadjustment_->thaw();
}

void
ScrollPortImpl::adjustment_changed()
{
  sig_scrolled.emit(); // FIXME: need to issue 0-distance move here
}

void
ScrollPortImpl::do_scrolled ()
{
  if (0)
    expose();
  else
    invalidate_size();
}

void
ScrollPortImpl::size_allocate (const Allocation area, bool changed)
{
  return_unless (has_visible_child());
  const int xoffset = hadjustment_ ? iround (hadjustment_->value()) : 0;
  const int yoffset = vadjustment_ ? iround (vadjustment_->value()) : 0;
  WidgetImpl &child = get_child();
  const Requisition rq = child.requisition();
  Allocation child_area;
  child_area.x = area.x - xoffset;
  child_area.y = area.y - yoffset;
  if (child.hexpand())
    child_area.width = max (rq.width, area.width);
  else
    child_area.width = rq.width;
  if (child.vexpand())
    child_area.height = max (rq.height, area.height);
  else
    child_area.height = rq.height;
  child_area = layout_child (child, child_area);
  child.set_allocation (child_area);
  if (!fix_id_)
    {
      WindowImpl *window = get_window();
      EventLoop *loop = window ? window->get_loop() : NULL;
      if (loop)
        fix_id_ = loop->exec_callback (Aida::slot (*this, &ScrollPortImpl::fix_adjustments), EventLoop::PRIORITY_NOW);
    }
}

void
ScrollPortImpl::set_focus_child (WidgetImpl *widget)
{
  ResizeContainerImpl::set_focus_child (widget);
  WidgetImpl *fchild = get_focus_child();
  if (!fchild)
    return;
  WindowImpl *rt = get_window();
  if (!rt)
    return;
  WidgetImpl *const rfwidget = rt->get_focus();
  WidgetImpl *fwidget = rfwidget;
  if (!fwidget)
    return;
  // list focus chain descendants between focus_widget and our immediate child
  std::list<WidgetImpl*> fwidgets;
  while (fwidget)
    {
      fwidgets.push_back (fwidget);
      if (fwidget == fchild)
        break;
      fwidget = fwidget->parent();
    }
  // find the first focus descendant that fits the scroll area
  fwidget = rfwidget; // fallback to innermost focus widget
  const DRect area = allocation();
  for (WidgetImpl *widget : fwidgets)
    {
      DRect a = widget->allocation();
      if (a.width <= area.width && a.height <= area.height)
        {
          fwidget = widget;
          break;
        }
    }
  if (!fwidget)
    return;
  scroll_to_child (*fwidget);
}

void
ScrollPortImpl::scroll_to_child (WidgetImpl &widget)
{
  const DRect area = allocation();
  // adjust scroll area to widget's area
  DRect farea = widget.allocation();
  if (0)
    printerr ("scroll-focus: area=%s farea=%s child=%p (%s)\n",
              area.string().c_str(), farea.string().c_str(), &widget,
              widget.allocation().string().c_str());
  // calc new scroll position, giving precedence to lower left
  double deltax = 0, deltay = 0;
  if (farea.upper_x() > area.upper_x() + deltax)
    deltax += farea.upper_x() - (area.upper_x() + deltax);
  if (farea.x < area.x + deltax)
    deltax += farea.x - (area.x + deltax);
  if (farea.upper_y() > area.upper_y() + deltay)
    deltay += farea.upper_y() - (area.upper_y() + deltay);
  if (farea.y < area.y + deltay)
    deltay += farea.y - (area.y + deltay);
  // scroll to new position
  hadjustment_->freeze();
  vadjustment_->freeze();
  if (deltax)
    hadjustment_->value (hadjustment_->value() + deltax);
  if (deltay)
    vadjustment_->value (vadjustment_->value() + deltay);
  hadjustment_->constrain();
  vadjustment_->constrain();
  hadjustment_->thaw();
  vadjustment_->thaw();
}

bool
ScrollPortImpl::scroll (EventType scroll_dir)
{
  switch (scroll_dir)
    {
    case SCROLL_UP:           return vadjustment_ ? vadjustment_->move (MOVE_STEP_BACKWARD) : false;
    case SCROLL_LEFT:         return hadjustment_ ? hadjustment_->move (MOVE_STEP_BACKWARD) : false;
    case SCROLL_DOWN:         return vadjustment_ ? vadjustment_->move (MOVE_STEP_FORWARD) : false;
    case SCROLL_RIGHT:        return hadjustment_ ? hadjustment_->move (MOVE_STEP_FORWARD) : false;
    default:                  ;
    }
  return false;
}

const CommandList&
ScrollPortImpl::list_commands ()
{
  static CommandP commands[] = {
    MakeNamedCommand (ScrollPortImpl, "scroll-up",    _("Scroll upwards"),    scroll, SCROLL_UP),
    MakeNamedCommand (ScrollPortImpl, "scroll-left",  _("Scroll leftwards"),  scroll, SCROLL_LEFT),
    MakeNamedCommand (ScrollPortImpl, "scroll-down",  _("Scroll downwards"),  scroll, SCROLL_DOWN),
    MakeNamedCommand (ScrollPortImpl, "scroll-right", _("Scroll rightwards"), scroll, SCROLL_RIGHT),
  };
  static const CommandList command_list (commands, ResizeContainerImpl::list_commands());
  return command_list;
}

bool
ScrollPortImpl::handle_event (const Event &event)
{
  bool handled = false;
  switch (event.type)
    {
    case SCROLL_UP:           handled = exec_command ("scroll-up");           break;
    case SCROLL_LEFT:         handled = exec_command ("scroll-left");         break;
    case SCROLL_DOWN:         handled = exec_command ("scroll-down");         break;
    case SCROLL_RIGHT:        handled = exec_command ("scroll-right");        break;
    default:                  break;
    }
  return handled;
}

void
ScrollPortImpl::reset (ResetMode mode)
{}

ScrollPortImpl::~ScrollPortImpl()
{
  clear_exec (&fix_id_);
}

static const WidgetFactory<ScrollPortImpl> scroll_port_factory ("Rapicorn::ScrollPort");

} // Rapicorn
