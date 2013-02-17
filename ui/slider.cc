// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#include "slider.hh"
#include "tableimpl.hh"
#include "painter.hh"
#include "factory.hh"
#include "window.hh"

namespace Rapicorn {

SliderArea::SliderArea() :
  sig_slider_changed (Aida::slot (*this, &SliderArea::slider_changed))
{}

void
SliderArea::slider_changed()
{}

bool
SliderArea::move (MoveType movement)
{
  Adjustment *adj = adjustment();
  return flipped() ? adj->move_flipped (movement) : adj->move (movement);
}

const CommandList&
SliderArea::list_commands ()
{
  static Command *commands[] = {
    MakeNamedCommand (SliderArea, "increment", _("Increment slider"), move, MOVE_STEP_FORWARD),
    MakeNamedCommand (SliderArea, "decrement", _("Decrement slider"), move, MOVE_STEP_BACKWARD),
    MakeNamedCommand (SliderArea, "page-increment", _("Large slider increment"), move, MOVE_PAGE_FORWARD),
    MakeNamedCommand (SliderArea, "page-decrement", _("Large slider decrement"), move, MOVE_PAGE_BACKWARD),
  };
  static const CommandList command_list (commands, ContainerImpl::list_commands());
  return command_list;
}

const PropertyList&
SliderArea::_property_list()
{
  static Property *properties[] = {
    MakeProperty (SliderArea, flipped,           _("Flipped"),           _("Invert (flip) display of the adjustment value"), "rw"),
    MakeProperty (SliderArea, adjustment_source, _("Adjustment Source"), _("Type of source to retrive an adjustment from"), "rw"),
  };
  static const PropertyList property_list (properties, ContainerImpl::_property_list());
  return property_list;
}

class SliderAreaImpl : public virtual TableImpl, public virtual SliderArea {
  Adjustment          *adjustment_;
  size_t               avc_id_, arc_id_;
  AdjustmentSourceType adjustment_source_;
  bool                 flip_;
  void
  unset_adjustment()
  {
    if (avc_id_)
      adjustment_->sig_value_changed() -= avc_id_;
    avc_id_ = 0;
    if (arc_id_)
      adjustment_->sig_range_changed() -= arc_id_;
    arc_id_ = 0;
    adjustment_->unref();
    adjustment_ = NULL;
  }
  virtual const PropertyList& _property_list() { return SliderArea::_property_list(); }
protected:
  virtual AdjustmentSourceType
  adjustment_source () const
  {
    return adjustment_source_;
  }
  virtual void
  adjustment_source (AdjustmentSourceType adj_source)
  {
    adjustment_source_ = adj_source;
  }
  virtual void
  hierarchy_changed (ItemImpl *old_toplevel)
  {
    this->TableImpl::hierarchy_changed (old_toplevel);
    if (anchored() && adjustment_source_ != ADJUSTMENT_SOURCE_NONE)
      {
        Adjustment *adj = NULL;
        find_adjustments (adjustment_source_, &adj);
        if (!adj)
          {
            Aida::EnumInfo ast = Aida::enum_info<AdjustmentSourceType>();
            throw Exception ("SliderArea failed to get Adjustment (",
                             ast.string (adjustment_source_),
                             ") from ancestors: ", name());
          }
        adjustment (*adj);
      }
  }
  virtual bool
  flipped () const
  {
    return flip_;
  }
  virtual void
  flipped (bool flip)
  {
    if (flip_ != flip)
      {
        flip_ = flip;
        changed();
      }
  }
  virtual ~SliderAreaImpl()
  {
    unset_adjustment();
  }
public:
  SliderAreaImpl() :
    adjustment_ (NULL), avc_id_ (0), arc_id_ (0),
    adjustment_source_ (ADJUSTMENT_SOURCE_NONE),
    flip_ (false)
  {
    Adjustment *adj = Adjustment::create();
    adjustment (*adj);
    adj->unref();
  }
  virtual void
  adjustment (Adjustment &adjustment)
  {
    adjustment.ref();
    if (adjustment_)
      unset_adjustment();
    adjustment_ = &adjustment;
    avc_id_ = adjustment_->sig_value_changed() += [this] () { sig_slider_changed.emit(); };
    arc_id_ = adjustment_->sig_range_changed() += [this] () { sig_slider_changed.emit(); };
    changed();
  }
  virtual Adjustment*
  adjustment () const
  {
    return adjustment_;
  }
  virtual void
  control (const String &command_name,
           const String &arg)
  {
  }
};
static const ItemFactory<SliderAreaImpl> slider_area_factory ("Rapicorn::Factory::SliderArea");

class SliderSkidImpl;

class SliderTroughImpl : public virtual SingleContainerImpl, public virtual EventHandler {
  SliderArea *slider_area_;
  size_t conid_slider_changed_;
  bool
  flipped()
  {
    return slider_area_ ? slider_area_->flipped() : false;
  }
public:
  SliderTroughImpl() :
    slider_area_ (NULL), conid_slider_changed_ (0)
  {}
  ~SliderTroughImpl()
  {}
protected:
  virtual void
  hierarchy_changed (ItemImpl *old_toplevel)
  {
    if (slider_area_ && conid_slider_changed_)
      slider_area_->sig_slider_changed() -= conid_slider_changed_;
    conid_slider_changed_ = 0;
    slider_area_ = NULL;
    this->SingleContainerImpl::hierarchy_changed (old_toplevel);
    if (anchored())
      {
        slider_area_ = parent_interface<SliderArea*>();
        conid_slider_changed_ = slider_area_->sig_slider_changed() += Aida::slot (*this, &SliderTroughImpl::reallocate_child);
      }
  }
  Adjustment*
  adjustment () const
  {
    return slider_area_ ? slider_area_->adjustment() : NULL;
  }
  double
  nvalue()
  {
    if (!slider_area_)
      return 0;
    Adjustment &adj = *adjustment();
    return flipped() ? adj.flipped_nvalue() : adj.nvalue();
  }
  void
  size_request (Requisition &requisition)
  {
    if (has_allocatable_child())
      {
        ItemImpl &child = get_child();
        requisition = child.requisition();
        /* we confine spreading to within the trough, so don't propagate hspread/vspread here */
      }
  }
  virtual void
  size_allocate (Allocation area, bool changed)
  {
    reallocate_child();
  }
  void
  reallocate_child ()
  {
    Allocation area = allocation();
    if (!has_allocatable_child())
      return;
    ItemImpl &child = get_child();
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
    child.set_allocation (area);
  }
  friend class SliderSkidImpl;
  virtual void
  reset (ResetMode mode = RESET_ALL)
  {}
  virtual bool
  handle_event (const Event &event)
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
          handled = get_child().process_event (event);
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
};
static const ItemFactory<SliderTroughImpl> slider_trough_factory ("Rapicorn::Factory::SliderTrough");

class SliderSkidImpl : public virtual SingleContainerImpl, public virtual EventHandler {
  uint        button_;
  double      coffset_;
  bool        vertical_skid_;
  bool
  flipped()
  {
    SliderTroughImpl &trough = parent_interface<SliderTroughImpl>();
    return trough.flipped();
  }
  bool
  vertical_skid () const
  {
    return vertical_skid_;
  }
  void
  vertical_skid (bool vs)
  {
    if (vertical_skid_ != vs)
      {
        vertical_skid_ = vs;
        changed();
      }
  }
public:
  SliderSkidImpl() :
    button_ (0),
    coffset_ (0),
    vertical_skid_ (false)
  {}
  ~SliderSkidImpl()
  {}
protected:
  virtual void
  size_request (Requisition &requisition)
  {
    bool chspread = false, cvspread = false;
    if (has_children())
      {
        ItemImpl &child = get_child();
        if (child.allocatable())
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
  nvalue()
  {
    SliderTroughImpl &trough = parent_interface<SliderTroughImpl>();
    Adjustment &adj = *trough.adjustment();
    return flipped() ? adj.flipped_nvalue() : adj.nvalue();
  }
#endif
  virtual void
  reset (ResetMode mode = RESET_ALL)
  {
    button_ = 0;
    coffset_ = 0;
  }
  virtual bool
  handle_event (const Event &event)
  {
    bool handled = false, proper_release = false;
    SliderTroughImpl &trough = parent_interface<SliderTroughImpl>();
    Adjustment &adj = *trough.adjustment();
    switch (event.type)
      {
        const EventButton *bevent;
      case MOUSE_ENTER:
        this->prelight (true);
        break;
      case MOUSE_LEAVE:
        this->prelight (false);
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
            double ep = vertical_skid() ? event.y : event.x;
            double cp = vertical_skid() ? ep - allocation().y : ep - allocation().x;
            double clength = vertical_skid() ? allocation().height : allocation().width;
            if (cp >= 0 && cp < clength)
              coffset_ = cp / clength;
            else
              {
                coffset_ = 0.5;
                // confine offset to not slip the skid off trough boundaries
                cp = ep - clength * coffset_;
                const Allocation &ta = trough.allocation();
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
            double ep = vertical_skid() ? event.y : event.x;
            const Allocation &ta = trough.allocation();
            double tp = vertical_skid() ? ta.y : ta.x;
            double pos = ep - tp;
            double tlength = vertical_skid() ? ta.height : ta.width;
            double clength = vertical_skid() ? allocation().height : allocation().width;
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
private:
  virtual const PropertyList&
  _property_list() // escape check-_property_list ';'
  {
    static Property *properties[] = {
      MakeProperty (SliderSkidImpl, vertical_skid, _("Vertical Skid"), _("Adjust behaviour to vertical skid movement"), "rw"),
    };
    static const PropertyList property_list (properties, SingleContainerImpl::_property_list());
    return property_list;
  }
};
static const ItemFactory<SliderSkidImpl> slider_skid_factory ("Rapicorn::Factory::SliderSkid");

} // Rapicorn
