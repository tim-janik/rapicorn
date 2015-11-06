// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#ifndef __RAPICORN_ADJUSTMENT_HH__
#define __RAPICORN_ADJUSTMENT_HH__

#include <ui/widget.hh>

namespace Rapicorn {

enum MoveType {
  MOVE_NONE,
  MOVE_STEP_FORWARD,
  MOVE_STEP_BACKWARD,
  MOVE_PAGE_FORWARD,
  MOVE_PAGE_BACKWARD,
};

class Adjustment;
typedef std::shared_ptr<Adjustment> AdjustmentP;
typedef std::weak_ptr  <Adjustment> AdjustmentW;

class Adjustment : public virtual std::enable_shared_from_this<Adjustment> {
  typedef Aida::Signal<void ()> SignalValueChanged;
  typedef Aida::Signal<void ()> SignalRangeChanged;
protected:
  explicit              Adjustment      ();
  virtual               ~Adjustment     ();
  virtual void          value_changed   ();
  virtual void          range_changed   ();
public:
  /* value */
  virtual double        value	        () = 0;
  virtual void		value	        (double newval) = 0;
  double                flipped_value   ();
  void                  flipped_value   (double newval);
  SignalValueChanged	sig_value_changed;
  /* normalized (0..1) value */
  double                nvalue	        ();
  void		        nvalue	        (double newval);
  double                flipped_nvalue  ();
  void                  flipped_nvalue  (double newval);
  /* range */
  virtual bool          frozen          () const = 0;
  virtual void          freeze          () = 0;
  virtual double        lower	        () const = 0;
  virtual void		lower	        (double newval) = 0;
  virtual double        upper	        () const = 0;
  virtual void		upper	        (double newval) = 0;
  virtual double	step_increment	() const = 0;
  virtual void		step_increment	(double newval) = 0;
  virtual double	page_increment	() const = 0;
  virtual void		page_increment	(double newval) = 0;
  virtual double	page	        () const = 0;
  virtual void		page	        (double newval) = 0;
  virtual void          constrain       () = 0;
  virtual void          thaw            () = 0;
  SignalRangeChanged	sig_range_changed;
  double                abs_range       ();
  double                abs_length      ();
  bool                  move            (MoveType move);
  bool                  move_flipped    (MoveType move);
  String                string          ();
  /* factory */
  static AdjustmentP    create          (double  value = 0,
                                         double  lower = 0,
                                         double  upper = 100,
                                         double  step_increment = 1,
                                         double  page_increment = 10,
                                         double  page_size = 0);
};

class AdjustmentSource {
public:
  virtual Adjustment*   get_adjustment  (AdjustmentSourceType   adj_source,
                                         const String          &name = "") = 0;
};

} // Rapicorn

#endif  /* __RAPICORN_ADJUSTMENT_HH__ */
