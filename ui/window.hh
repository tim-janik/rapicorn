// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __RAPICORN_WINDOW_HH__
#define __RAPICORN_WINDOW_HH__

#include <ui/container.hh>
#include <ui/loop.hh>

namespace Rapicorn {

/* --- Window --- */
class Window : public virtual Container, public virtual EventLoop::Source {
  friend class  Item;
  void          uncross_focus           (Item        &fitem);
protected:
  virtual void  beep                    (void) = 0;
  void          set_focus               (Item         *item);
  virtual void  expose_window_region    (const Region &region) = 0;     // window item coords
  virtual void  copy_area               (const Rect   &src,
                                         const Point  &dest) = 0;
  virtual void  cancel_item_events      (Item         *item) = 0;
  void          cancel_item_events      (Item         &item)            { cancel_item_events (&item); }
  virtual bool  dispatch_event          (const Event  &event) = 0;
  virtual void  set_parent              (Container    *parent);
  virtual bool  custom_command          (const String       &command_name,
                                         const StringList   &command_args) = 0;
  /* loop source (FIXME) */
  virtual bool  prepare                 (uint64 current_time_usecs,
                                         int64 *timeout_usecs_p) = 0;
  explicit      Window                  ();
public://FIXME: protected:
  virtual bool  check                   (uint64 current_time_usecs) = 0;
  virtual bool  dispatch                () = 0;
  virtual void  draw_now                () = 0;
public:
  Item*         get_focus               () const;
  virtual bool  tunable_requisitions    () = 0;
  cairo_surface_t* create_snapshot      (const Rect  &subarea);
  virtual void  add_grab                (Item  &child,
                                         bool   unconfined = false) = 0;
  void          add_grab                (Item  *child,
                                         bool   unconfined = false) { RAPICORN_CHECK (child != NULL); add_grab (*child, unconfined); }
  virtual void  remove_grab             (Item  &child) = 0;
  void          remove_grab             (Item  *child)              { RAPICORN_CHECK (child != NULL); remove_grab (*child); }
  virtual Item* get_grab                (bool  *unconfined = NULL) = 0;
  Item*         find_item               (const String &name);
  /* signals */
  typedef Signal<Window, bool (const String&, const StringVector&), CollectorWhile0<bool> >   CommandSignal;
  typedef Signal<Window, void ()> NotifySignal;
  NotifySignal          sig_displayed;
  /* viewp0rt ops */
  virtual void  create_viewp0rt         () = 0;
  virtual bool  has_viewp0rt            () = 0;
  virtual void  destroy_viewp0rt        () = 0;
  /* main loop functions */
  virtual EventLoop* get_loop           () = 0;
  virtual void       enable_auto_close  ();
  /* MT-safe */
  virtual Wind0w&    wind0w          () = 0;
};

} // Rapicorn

#endif  /* __RAPICORN_WINDOW_HH__ */
