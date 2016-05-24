// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#include "slider.hh"
#include "painter.hh"
#include "factory.hh"
#include "window.hh"

namespace Rapicorn {

// == SliderAreaImpl ==
SliderAreaImpl::SliderAreaImpl() :
  adjustment_ (NULL), avc_id_ (0), arc_id_ (0),
  adjustment_source_ (AdjustmentSourceType::NONE),
  flip_ (false),
  sig_slider_changed (Aida::slot (*this, &SliderAreaImpl::slider_changed))
{
  adjustment (*Adjustment::create());
}

SliderAreaImpl::~SliderAreaImpl()
{
  unset_adjustment();
}

void
SliderAreaImpl::slider_changed()
{}

bool
SliderAreaImpl::move (MoveType movement)
{
  Adjustment *adj = adjustment();
  return flipped() ? adj->move_flipped (movement) : adj->move (movement);
}

const CommandList&
SliderAreaImpl::list_commands ()
{
  static CommandP commands[] = {
    MakeNamedCommand (SliderAreaImpl, "increment", _("Increment slider"), move, MOVE_STEP_FORWARD),
    MakeNamedCommand (SliderAreaImpl, "decrement", _("Decrement slider"), move, MOVE_STEP_BACKWARD),
    MakeNamedCommand (SliderAreaImpl, "page-increment", _("Large slider increment"), move, MOVE_PAGE_FORWARD),
    MakeNamedCommand (SliderAreaImpl, "page-decrement", _("Large slider decrement"), move, MOVE_PAGE_BACKWARD),
  };
  static const CommandList command_list (commands, ContainerImpl::list_commands());
  return command_list;
}

void
SliderAreaImpl::unset_adjustment()
{
  if (adjustment_ && avc_id_)
    adjustment_->sig_value_changed() -= avc_id_;
  avc_id_ = 0;
  if (adjustment_ && arc_id_)
    adjustment_->sig_range_changed() -= arc_id_;
  arc_id_ = 0;
  adjustment_ = NULL;
}

AdjustmentSourceType
SliderAreaImpl::adjustment_source () const
{
  return adjustment_source_;
}

void
SliderAreaImpl::adjustment_source (AdjustmentSourceType adj_source)
{
  if (adjustment_source_ != adj_source)
    {
      adjustment_source_ = adj_source;
      changed ("adjustment_source");
    }
}

void
SliderAreaImpl::hierarchy_changed (WidgetImpl *old_toplevel)
{
  this->TableLayoutImpl::hierarchy_changed (old_toplevel);
  if (anchored() && adjustment_source_ != AdjustmentSourceType::NONE)
    {
      Adjustment *adj = NULL;
      find_adjustments (adjustment_source_, &adj);
      if (!adj)
        {
          String evalue = Aida::enum_info<AdjustmentSourceType>().value_to_string (adjustment_source_);
          throw Exception ("SliderArea failed to get Adjustment (", evalue, ") from ancestors: ", id());
        }
      adjustment (*adj);
    }
}

bool
SliderAreaImpl::flipped () const
{
  return flip_;
}

void
SliderAreaImpl::flipped (bool flip)
{
  if (flip_ != flip)
    {
      flip_ = flip;
      changed ("flipped");
    }
}

void
SliderAreaImpl::adjustment (Adjustment &adjustment)
{
  AdjustmentP newadj = shared_ptr_cast<Adjustment> (&adjustment);
  unset_adjustment();
  adjustment_ = newadj;
  avc_id_ = adjustment_->sig_value_changed() += [this] () { sig_slider_changed.emit(); };
  arc_id_ = adjustment_->sig_range_changed() += [this] () { sig_slider_changed.emit(); };
  changed ("adjustment");
}

Adjustment*
SliderAreaImpl::adjustment () const
{
  return &*adjustment_;
}

static const WidgetFactory<SliderAreaImpl> slider_area_factory ("Rapicorn::SliderArea");

// == SliderTroughImpl ==
SliderTroughImpl::SliderTroughImpl() :
  slider_area_ (NULL), conid_slider_changed_ (0)
{}

SliderTroughImpl::~SliderTroughImpl()
{}

bool
SliderTroughImpl::flipped() const
{
  return slider_area_ ? slider_area_->flipped() : false;
}

void
SliderTroughImpl::hierarchy_changed (WidgetImpl *old_toplevel)
{
  if (slider_area_ && conid_slider_changed_)
    slider_area_->sig_slider_changed() -= conid_slider_changed_;
  conid_slider_changed_ = 0;
  slider_area_ = NULL;
  this->SingleContainerImpl::hierarchy_changed (old_toplevel);
  if (anchored())
    {
      slider_area_ = parent_interface<SliderAreaImpl*>();
      conid_slider_changed_ = slider_area_->sig_slider_changed() += Aida::slot (*this, &SliderTroughImpl::reallocate_child);
    }
}

Adjustment*
SliderTroughImpl::adjustment () const
{
  return slider_area_ ? slider_area_->adjustment() : NULL;
}

double
SliderTroughImpl::nvalue()
{
  if (!slider_area_)
    return 0;
  Adjustment &adj = *adjustment();
  return flipped() ? adj.flipped_nvalue() : adj.nvalue();
}

void
SliderTroughImpl::size_request (Requisition &requisition)
{
  if (has_visible_child())
    {
      WidgetImpl &child = get_child();
      requisition = child.requisition();
      /* we confine spreading to within the trough, so don't propagate hspread/vspread here */
    }
}

void
SliderTroughImpl::size_allocate (Allocation area, bool changed)
{
  reallocate_child();
}

void
SliderTroughImpl::reallocate_child ()
{
  Allocation area = allocation();
  if (!has_visible_child())
    return;
  WidgetImpl &child = get_child();
  Requisition rq = child.requisition();
  /* expand/scale child */
  if (area.width > rq.width && !child.hspread())
    {
      if (child.hexpand())
        {
          Adjustment *adj = adjustment();
          double cwidth = adj ? round (adj->abs_length() * area.width) : 0;
          rq.width = MAX (cwidth, rq.width);
        }
      area.x += round (nvalue() * (area.width - rq.width));
      area.width = round (rq.width);
    }
  if (area.height > rq.height && !child.vspread())
    {
      if (child.vexpand())
        {
          Adjustment *adj = adjustment();
          double cheight = adj ? round (adj->abs_length() * area.height) : 0;
          rq.height = MAX (cheight, rq.height);
        }
      area.y += round (nvalue() * (area.height - rq.height));
      area.height = round (rq.height);
    }
  child.set_child_allocation (area);
}

void
SliderTroughImpl::reset (ResetMode mode)
{}

bool
SliderTroughImpl::handle_event (const Event &event)
{
  bool handled = false;
  switch (event.type)
    {
    case BUTTON_PRESS:
    case BUTTON_2PRESS:
    case BUTTON_3PRESS:
    case BUTTON_RELEASE:
    case BUTTON_2RELEASE:
    case BUTTON_3RELEASE:
    case BUTTON_CANCELED:
      /* forward button events to allow slider warps */
      if (has_visible_child())
        {
          EventHandler *ehandler = dynamic_cast<EventHandler*> (&get_child());
          if (ehandler)
            handled = ehandler->handle_event (event);
        }
      break;
    case SCROLL_UP:
    case SCROLL_LEFT:
      exec_command ("decrement");
      break;
    case SCROLL_DOWN:
    case SCROLL_RIGHT:
      exec_command ("increment");
      break;
    default: break;
    }
  return handled;
}

static const WidgetFactory<SliderTroughImpl> slider_trough_factory ("Rapicorn::SliderTrough");

// == SliderSkidImpl ==
SliderSkidImpl::SliderSkidImpl() :
  button_ (0),
  coffset_ (0),
  vertical_skid_ (false)
{}

SliderSkidImpl::~SliderSkidImpl()
{}

bool
SliderSkidImpl::flipped() const
{
  SliderTroughImpl &trough = parent_interface<SliderTroughImpl>();
  return trough.flipped();
}

bool
SliderSkidImpl::vertical_skid () const
{
  return vertical_skid_;
}

void
SliderSkidImpl::vertical_skid (bool vs)
{
  if (vertical_skid_ != vs)
    {
      vertical_skid_ = vs;
      changed ("vertical_skid");
    }
}

void
SliderSkidImpl::size_request (Requisition &requisition)
{
  bool chspread = false, cvspread = false;
  if (has_children())
    {
      WidgetImpl &child = get_child();
      if (child.visible())
        {
          requisition = child.requisition();
          chspread = child.hspread();
          cvspread = child.vspread();
        }
    }
  set_flag (HSPREAD_CONTAINER, chspread);
  set_flag (VSPREAD_CONTAINER, cvspread);
}

#if 0
double
SliderSkidImpl::nvalue()
{
  SliderTroughImpl &trough = parent_interface<SliderTroughImpl>();
  Adjustment &adj = *trough.adjustment();
  return flipped() ? adj.flipped_nvalue() : adj.nvalue();
}
#endif

void
SliderSkidImpl::reset (ResetMode mode)
{
  button_ = 0;
  coffset_ = 0;
}

bool
SliderSkidImpl::handle_event (const Event &event)
{
  const Allocation slider_allocation = rect_to_viewport (allocation());
  bool handled = false, proper_release = false;
  SliderTroughImpl &trough = parent_interface<SliderTroughImpl>();
  const Allocation trough_allocation = trough.rect_to_viewport (trough.allocation());
  Adjustment &adj = *trough.adjustment();
  switch (event.type)
    {
      const EventButton *bevent;
    case MOUSE_ENTER:
      this->hover (true);
      break;
    case MOUSE_LEAVE:
      this->hover (false);
      break;
    case BUTTON_PRESS:
    case BUTTON_2PRESS:
    case BUTTON_3PRESS:
      bevent = dynamic_cast<const EventButton*> (&event);
      if (!button_ and (bevent->button == 1 or bevent->button == 2))
        {
          button_ = bevent->button;
          get_window()->add_grab (this, true);
          handled = true;
          coffset_ = 0;
          const Point viewport_point = Point (event.x, event.y);
          double ep = vertical_skid() ? viewport_point.y : viewport_point.x;
          double cp = vertical_skid() ? ep - slider_allocation.y : ep - slider_allocation.x;
          double clength = vertical_skid() ? slider_allocation.height : slider_allocation.width;
          if (cp >= 0 && cp < clength)
            coffset_ = cp / clength;
          else
            {
              coffset_ = 0.5;
              // confine offset to not slip the skid off trough boundaries
              cp = ep - clength * coffset_;
              const Allocation &ta = trough_allocation;
              double start_slip = (vertical_skid() ? ta.y : ta.x) - cp;
              double tlength = vertical_skid() ? ta.y + ta.height : ta.x + ta.width;
              double end_slip = cp + clength - tlength;
              // adjust skid position
              cp += MAX (0, start_slip);
              cp -= MAX (0, end_slip);
              // recalculate offset
              coffset_ = (ep - cp) / clength;
            }
        }
      break;
    case MOUSE_MOVE:
      if (button_)
        {
          const Point viewport_point = Point (event.x, event.y);
          double ep = vertical_skid() ? viewport_point.y : viewport_point.x;
          const Allocation &ta = trough_allocation;
          double tp = vertical_skid() ? ta.y : ta.x;
          double pos = ep - tp;
          double tlength = vertical_skid() ? ta.height : ta.width;
          double clength = vertical_skid() ? slider_allocation.height : slider_allocation.width;
          tlength -= clength;
          pos -= coffset_ * clength;
          if (tlength > 0)
            pos /= tlength;
          pos = CLAMP (pos, 0, 1);
          if (flipped())
            adj.flipped_nvalue (pos);
          else
            adj.nvalue (pos);
        }
      break;
    case BUTTON_RELEASE:
    case BUTTON_2RELEASE:
    case BUTTON_3RELEASE:
      proper_release = true;
    case BUTTON_CANCELED:
      bevent = dynamic_cast<const EventButton*> (&event);
      if (button_ == bevent->button)
        {
          get_window()->remove_grab (this);
          button_ = 0;
          coffset_ = 0;
          handled = true;
        }
      (void) proper_release; // silence compiler
      break;
    default: break;
    }
  return handled;
}

static const WidgetFactory<SliderSkidImpl> slider_skid_factory ("Rapicorn::SliderSkid");

} // Rapicorn
