// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#include "adjustment.hh"
#include "factory.hh"

namespace Rapicorn {

Adjustment::Adjustment() :
  sig_value_changed (Aida::slot (*this, &Adjustment::value_changed)),
  sig_range_changed (Aida::slot (*this, &Adjustment::range_changed))
{}

Adjustment::~Adjustment()
{}

double
Adjustment::flipped_value()
{
  double v = value() - lower();
  return upper() - page() - v;
}

void
Adjustment::flipped_value (double newval)
{
  double l = lower();
  double u = upper() - page();
  newval = CLAMP (newval, l, u);
  double v = newval - l;
  value (u - v);
}

double
Adjustment::nvalue ()
{
  double l = lower(), u = upper() - page(), ar = u - l;
  return ar > 0 ? (value() - l) / ar : 0.0;
}

void
Adjustment::nvalue (double newval)
{
  double l = lower(), u = upper() - page(), ar = u - l;
  value (l + newval * ar);
}

double
Adjustment::flipped_nvalue ()
{
  return 1.0 - nvalue();
}

void
Adjustment::flipped_nvalue (double newval)
{
  nvalue (1.0 - newval);
}

void
Adjustment::value_changed()
{}

void
Adjustment::range_changed()
{}

double
Adjustment::abs_range ()
{
  return fabs (upper() - page() - lower());
}

double
Adjustment::abs_length ()
{
  double p = page();
  double ar = fabs (upper() - lower());
  return ar > 0 ? p / ar : 0.0;
}

bool
Adjustment::move_flipped (MoveType movet)
{
  switch (movet)
    {
    case MOVE_PAGE_FORWARD:     return move (MOVE_PAGE_BACKWARD);
    case MOVE_STEP_FORWARD:     return move (MOVE_STEP_BACKWARD);
    case MOVE_STEP_BACKWARD:    return move (MOVE_STEP_FORWARD);
    case MOVE_PAGE_BACKWARD:    return move (MOVE_PAGE_FORWARD);
    case MOVE_NONE:             ;
    }
  return false;
}

bool
Adjustment::move (MoveType move)
{
  switch (move)
    {
    case MOVE_PAGE_FORWARD:
      value (value() + page_increment());
      return true;
    case MOVE_STEP_FORWARD:
      value (value() + step_increment());
      return true;
    case MOVE_STEP_BACKWARD:
      value (value() - step_increment());
      return true;
    case MOVE_PAGE_BACKWARD:
      value (value() - page_increment());
      return true;
    case MOVE_NONE: ;
    }
  return false;
}

String
Adjustment::string ()
{
  String s = string_format ("Adjustment(%g,[%f,%f],istep=%+f,pstep=%+f,page=%f%s)",
                            value(), lower(), upper(),
                            step_increment(), page_increment(), page(),
                            frozen() ? ",frozen" : "");
  return s;
}

struct AdjustmentMemorizedState {
  double value, lower, upper, step_increment, page_increment, page;
};
static DataKey<AdjustmentMemorizedState> memorized_state_key;

class AdjustmentSimpleImpl : public virtual Adjustment, public virtual DataListContainer {
  double value_, lower_, upper_, step_increment_, page_increment_, page_;
  uint   freeze_count_;
public:
  AdjustmentSimpleImpl() :
    value_ (0), lower_ (0), upper_ (100),
    step_increment_ (1), page_increment_ (10), page_ (0),
    freeze_count_ (0)
  {}
  /* value */
  virtual double        value	        ()                      { return value_; }
  virtual void
  value (double newval)
  {
    double old_value = value_;
    if (isnan (newval))
      {
        critical ("Adjustment::value(): invalid value: %g", newval);
        newval = 0;
      }
    value_ = CLAMP (newval, lower_, upper_ - page_);
    if (old_value != value_ && !freeze_count_)
      sig_value_changed.emit ();
  }
  /* range */
  virtual bool                  frozen          () const                { return freeze_count_ > 0; }
  virtual double                lower	        () const                { return lower_; }
  virtual void                  lower           (double newval)         { assert_return (freeze_count_); lower_ = newval; }
  virtual double                upper	        () const                { return upper_; }
  virtual void		        upper	        (double newval)         { assert_return (freeze_count_); upper_ = newval; }
  virtual double	        step_increment	() const                { return step_increment_; }
  virtual void		        step_increment	(double newval)         { assert_return (freeze_count_); step_increment_ = newval; }
  virtual double	        page_increment	() const                { return page_increment_; }
  virtual void		        page_increment	(double newval)         { assert_return (freeze_count_); page_increment_ = newval; }
  virtual double	        page	        () const                { return page_; }
  virtual void		        page	        (double newval)         { assert_return (freeze_count_); page_ = newval; }
  virtual void
  freeze ()
  {
    if (!freeze_count_)
      {
        AdjustmentMemorizedState m;
        m.value = value_;
        m.lower = lower_;
        m.upper = upper_;
        m.step_increment = step_increment_;
        m.page_increment = page_increment_;
        m.page = page_;
        set_data (&memorized_state_key, m);
      }
    freeze_count_++;
  }
  virtual void
  constrain ()
  {
    assert_return (freeze_count_);
    if (lower_ > upper_)
      lower_ = upper_ = (lower_ + upper_) / 2;
    page_ = CLAMP (page_, 0, upper_ - lower_);
    page_increment_ = MAX (page_increment_, 0);
    if (page_ > 0)
      page_increment_ = MIN (page_increment_, page_);
    step_increment_ = MAX (0, step_increment_);
    if (page_increment_ > 0)
      step_increment_ = MIN (step_increment_, page_increment_);
    else if (page_ > 0)
      step_increment_ = MIN (step_increment_, page_);
    value_ = CLAMP (value_, lower_, upper_ - page_);
  }
  virtual void
  thaw ()
  {
    assert_return (freeze_count_);
    if (freeze_count_ == 1)
      constrain();
    freeze_count_--;
    if (!freeze_count_)
      {
        AdjustmentMemorizedState m = swap_data (&memorized_state_key);
        if (m.lower != lower_ || m.upper != upper_ || m.page != page_ ||
            m.step_increment != step_increment_ || m.page_increment != page_increment_)
          sig_range_changed.emit();
        if (m.value != value_)
          sig_value_changed.emit ();
      }
  }
};

AdjustmentP
Adjustment::create (double value, double lower, double upper, double step_increment, double page_increment, double page_size)
{
  auto adj = std::make_shared<AdjustmentSimpleImpl>();
  adj->freeze();
  adj->lower (lower);
  adj->upper (upper);
  adj->step_increment (step_increment);
  adj->page_increment (page_increment);
  adj->page (page_size);
  adj->thaw();
  return adj;
}

} // Rapicorn
