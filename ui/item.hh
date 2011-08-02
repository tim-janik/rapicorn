// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#include <ui/serverapi.hh> // includes <ui/item.hh> after ItemIface declaration

#ifndef __RAPICORN_ITEM_HH_
#define __RAPICORN_ITEM_HH_

#include <ui/events.hh>
#include <ui/region.hh>
#include <ui/commands.hh>
#include <ui/properties.hh>
#include <ui/heritage.hh>

namespace Rapicorn {

/* --- Item structures and forward decls --- */
typedef Rect Allocation;
class ItemImpl;
class SizeGroup;
class Adjustment;
class ContainerImpl;
class WindowImpl;

/* --- event handler --- */
class EventHandler : public virtual BaseObject {
  typedef Signal<EventHandler, bool (const Event&), CollectorWhile0<bool> > EventSignal;
protected:
  virtual bool  handle_event    (const Event    &event);
public:
  explicit      EventHandler    ();
  EventSignal   sig_event;
  typedef enum {
    RESET_ALL
  } ResetMode;
  virtual void  reset           (ResetMode       mode = RESET_ALL) = 0;
};

/* --- ItemImpl --- */
typedef Signals::Slot1<void, ItemImpl&> ItemSlot;
class ItemImpl : public virtual ItemIface {
  friend                      class ClassDoctor;
  friend                      class ContainerImpl;
  friend                      class SizeGroup;
  uint32                      m_flags;          /* interface-inlined for fast read-out */
  ContainerImpl              *m_parent;         /* interface-inlined for fast read-out */
  Heritage                   *m_heritage;
  Requisition                 m_requisition;
  Allocation                  m_allocation;
  FactoryContext             *m_factory_context;
  Requisition                 inner_size_request (); // ungrouped size requisition
  void                        propagate_state    (bool notify_changed);
  ContainerImpl**             _parent_loc        () { return &m_parent; }
  void                        propagate_heritage ();
  void                        heritage           (Heritage  *heritage);
protected:
  virtual void                constructed             ();
  /* flag handling */
  bool                        change_flags_silently   (uint32 mask, bool on);
  enum {
    ANCHORED                  = 1 <<  0,
    VISIBLE                   = 1 <<  1,
    PARENT_VISIBLE            = 1 <<  2,
    HIDDEN_CHILD              = 1 <<  3,
    SENSITIVE                 = 1 <<  4,
    PARENT_SENSITIVE          = 1 <<  5,
    PRELIGHT                  = 1 <<  6,
    IMPRESSED                 = 1 <<  7,
    FOCUS_CHAIN               = 1 <<  8,
    HAS_DEFAULT               = 1 <<  9,
    INVALID_REQUISITION       = 1 << 10,
    INVALID_ALLOCATION        = 1 << 11,
    INVALID_CONTENT           = 1 << 12,
    HSPREAD_CONTAINER         = 1 << 13,
    VSPREAD_CONTAINER         = 1 << 14,
    HSPREAD                   = 1 << 15,
    VSPREAD                   = 1 << 16,
    HEXPAND                   = 1 << 17,
    VEXPAND                   = 1 << 18,
    HSHRINK                   = 1 << 19,
    VSHRINK                   = 1 << 20,
    ALLOCATABLE               = 1 << 21,
    POSITIVE_ALLOCATION       = 1 << 22,
    DEBUG                     = 1 << 23,
    LAST_FLAG                 = 1 << 24
  };
  void                        set_flag          (uint32 flag, bool on = true);
  void                        unset_flag        (uint32 flag) { set_flag (flag, false); }
  bool                        test_flags        (uint32 mask) const { return (m_flags & mask) != 0; }
  bool                        test_all_flags    (uint32 mask) const { return (m_flags & mask) == mask; }
  virtual bool                self_visible      () const;
  /* size requisition and allocation */
  Requisition                 cache_requisition (Requisition *requisition = NULL);
  virtual void                size_request      (Requisition &requisition) = 0;
  virtual void                size_allocate     (Allocation   area) = 0;
  virtual void                invalidate_parent ();
  bool                        tune_requisition  (Requisition  requisition);
  bool                        tune_requisition  (double       new_width,
                                                 double       new_height);
  /* signal methods */
  virtual void                do_invalidate     ();
  virtual void                do_changed        ();
  /* idlers & timers */
  uint                        exec_fast_repeater   (const BoolSlot &sl);
  uint                        exec_slow_repeater   (const BoolSlot &sl);
  uint                        exec_key_repeater    (const BoolSlot &sl);
  bool                        remove_exec          (uint            exec_id);
  virtual void                visual_update        ();
  /* misc */
  virtual                     ~ItemImpl             ();
  virtual void                finalize          ();
  virtual void                set_parent        (ContainerImpl *parent);
  virtual void                hierarchy_changed (ItemImpl *old_toplevel);
  virtual bool                move_focus        (FocusDirType fdir);
  virtual bool                custom_command    (const String       &command_name,
                                                 const StringList   &command_args);
  void                        anchored          (bool b) { set_flag (ANCHORED, b); }
  void                        notify_key_error  ();
public:
  explicit                    ItemImpl              ();
  bool                        anchored          () const { return test_flags (ANCHORED); }
  bool                        visible           () const { return test_flags (VISIBLE) && !test_flags (HIDDEN_CHILD); }
  void                        visible           (bool b) { set_flag (VISIBLE, b); }
  bool                        allocatable       () const { return visible() && test_all_flags (ALLOCATABLE | PARENT_VISIBLE); }
  bool                        drawable          () const { return visible() && test_flags (POSITIVE_ALLOCATION); }
  virtual bool                viewable          () const; // drawable() && parent->viewable();
  bool                        sensitive         () const { return test_all_flags (SENSITIVE | PARENT_SENSITIVE); }
  virtual void                sensitive         (bool b);
  bool                        insensitive       () const { return !sensitive(); }
  void                        insensitive       (bool b) { sensitive (!b); }
  bool                        prelight          () const { return test_flags (PRELIGHT); }
  virtual void                prelight          (bool b);
  bool                        branch_prelight   () const;
  bool                        impressed         () const { return test_flags (IMPRESSED); }
  virtual void                impressed         (bool b);
  bool                        branch_impressed  () const;
  bool                        has_default       () const { return test_flags (HAS_DEFAULT); }
  bool                        grab_default      () const;
  virtual bool                can_focus         () const;
  bool                        has_focus         () const;
  bool                        grab_focus        ();
  bool                        hexpand           () const { return test_flags (HEXPAND | HSPREAD | HSPREAD_CONTAINER); }
  void                        hexpand           (bool b) { set_flag (HEXPAND, b); }
  bool                        vexpand           () const { return test_flags (VEXPAND | VSPREAD | VSPREAD_CONTAINER); }
  void                        vexpand           (bool b) { set_flag (VEXPAND, b); }
  bool                        hspread           () const { return test_flags (HSPREAD | HSPREAD_CONTAINER); }
  void                        hspread           (bool b) { set_flag (HSPREAD, b); }
  bool                        vspread           () const { return test_flags (VSPREAD | VSPREAD_CONTAINER); }
  void                        vspread           (bool b) { set_flag (VSPREAD, b); }
  bool                        hshrink           () const { return test_flags (HSHRINK); }
  void                        hshrink           (bool b) { set_flag (HSHRINK, b); }
  bool                        vshrink           () const { return test_flags (VSHRINK); }
  void                        vshrink           (bool b) { set_flag (VSHRINK, b); }
  bool                        debug             () const { return test_flags (DEBUG); }
  void                        debug             (bool f) { set_flag (DEBUG, f); }
  virtual String              name              () const;
  virtual void                name              (const String &str);
  FactoryContext*             factory_context   () const;
  void                        factory_context   (FactoryContext *fc);
  ColorSchemeType             color_scheme      () const;
  void                        color_scheme      (ColorSchemeType cst);
  /* override requisition */
  double                      width             () const;
  void                        width             (double w);
  double                      height            () const;
  void                        height            (double h);
  /* properties */
  void                        set_property      (const String    &property_name,
                                                 const String    &value,
                                                 const nothrow_t &nt = dothrow);
  bool                        try_set_property  (const String    &property_name,
                                                 const String    &value,
                                                 const nothrow_t &nt = dothrow);
  String                      get_property      (const String    &property_name);
  Property*                   lookup_property   (const String    &property_name);
  virtual const PropertyList& list_properties   ();
  /* commands */
  bool                        exec_command      (const String    &command_call_string);
  Command*                    lookup_command    (const String    &command_name);
  virtual const CommandList&  list_commands     ();
  /* parents */
  ContainerImpl*              parent            () const { return m_parent; }
  bool                        has_ancestor      (const ItemImpl &ancestor) const;
  ItemImpl*                   common_ancestor   (const ItemImpl &other) const;
  ItemImpl*                   common_ancestor   (const ItemImpl *other) const { return common_ancestor (*other); }
  WindowImpl*                 get_window        () const;
  /* cross links */
  void                        cross_link        (ItemImpl       &link,
                                                 const ItemSlot &uncross);
  void                        cross_unlink      (ItemImpl       &link,
                                                 const ItemSlot &uncross);
  void                        uncross_links     (ItemImpl       &link);
  /* invalidation / changes */
  void                        invalidate        ();
  void                        invalidate_size   ();
  void                        changed           ();
  void                        expose            ();                             /* item allocation */
  void                        expose            (const Rect       &rect);       /* item coordinates relative */
  void                        expose            (const Region     &region);     /* item coordinates relative */
  void                        copy_area         (const Rect &rect, const Point &dest);
  void                        queue_visual_update  ();
  void                        force_visual_update  ();
  /* public signals */
  SignalFinalize<ItemImpl>                sig_finalize;
  Signal<ItemImpl, void ()>               sig_changed;
  Signal<ItemImpl, void ()>               sig_invalidate;
  Signal<ItemImpl, void (ItemImpl *oldt)> sig_hierarchy_changed;
  /* event handling */
  bool                       process_event          (const Event &event);       /* item coordinates relative */
  bool                       process_viewp0rt_event (const Event &event);       /* viewp0rt coordinates relative */
  /* coordinate handling */
protected:
  Affine                     affine_to_viewp0rt     ();                         /* item => viewp0rt affine */
  Affine                     affine_from_viewp0rt   ();                         /* viewp0rt => item affine */
public:
  virtual bool               point                  (Point        p);           /* item coordinates relative */
  Point                      point_to_viewp0rt      (Point        item_point);  /* item coordinates relative */
  Point                      point_from_viewp0rt    (Point        window_point);/* viewp0rt coordinates relative */
  virtual bool               translate_from         (const ItemImpl   &src_item,
                                                     const uint    n_points,
                                                     Point        *points) const;
  bool                       translate_to           (const uint    n_points,
                                                     Point        *points,
                                                     const ItemImpl   &target_item) const;
  bool                       translate_from         (const ItemImpl   &src_item,
                                                     const uint    n_rects,
                                                     Rect         *rects) const;
  bool                       translate_to           (const uint    n_rects,
                                                     Rect         *rects,
                                                     const ItemImpl   &target_item) const;
  bool                       viewp0rt_point         (Point        p);           /* viewp0rt coordinates relative */
  /* public size accessors */
  Requisition                requisition        ();                             // effective size requisition
  Requisition                size_request       () { return requisition(); }    // FIXME: remove
  void                       set_allocation     (const Allocation &area);       // assign new allocation
  const Allocation&          allocation         () { return m_allocation; }     // current allocation
  /* display */
  virtual void               render             (Display        &display) = 0;
  /* heritage / appearance */
  StateType             state                   () const;
  Heritage*             heritage                () const { return m_heritage; }
  Color                 foreground              () { return heritage()->foreground (state()); }
  Color                 background              () { return heritage()->background (state()); }
  Color                 dark_color              () { return heritage()->dark_color (state()); }
  Color                 dark_shadow             () { return heritage()->dark_shadow (state()); }
  Color                 dark_glint              () { return heritage()->dark_glint (state()); }
  Color                 light_color             () { return heritage()->light_color (state()); }
  Color                 light_shadow            () { return heritage()->light_shadow (state()); }
  Color                 light_glint             () { return heritage()->light_glint (state()); }
  Color                 focus_color             () { return heritage()->focus_color (state()); }
  /* debugging/testing */
  virtual String        test_dump               ();
protected:
  void                  make_test_dump          (TestStream   &tstream);
  virtual void          dump_test_data          (TestStream   &tstream);
  virtual void          dump_private_data       (TestStream   &tstream);
  /* convenience */
public:
  void                  find_adjustments        (AdjustmentSourceType adjsrc1,
                                                 Adjustment         **adj1,
                                                 AdjustmentSourceType adjsrc2 = ADJUSTMENT_SOURCE_NONE,
                                                 Adjustment         **adj2 = NULL,
                                                 AdjustmentSourceType adjsrc3 = ADJUSTMENT_SOURCE_NONE,
                                                 Adjustment         **adj3 = NULL,
                                                 AdjustmentSourceType adjsrc4 = ADJUSTMENT_SOURCE_NONE,
                                                 Adjustment         **adj4 = NULL);
public: /* packing */
  struct PackInfo {
    double hposition, hspan, vposition, vspan;
    uint left_spacing, right_spacing, bottom_spacing, top_spacing;
    double halign, hscale, valign, vscale;
  };
  const PackInfo&    pack_info       () const   { return const_cast<ItemImpl*> (this)->pack_info (false); }
  double             hposition       () const   { return pack_info ().hposition; }
  void               hposition       (double d);
  double             hspan           () const   { return pack_info ().hspan; }
  void               hspan           (double d);
  double             vposition       () const   { return pack_info ().vposition; }
  void               vposition       (double d);
  double             vspan           () const   { return pack_info ().vspan; }
  void               vspan           (double d);
  uint               left_spacing    () const   { return pack_info ().left_spacing; }
  void               left_spacing    (uint s);
  uint               right_spacing   () const   { return pack_info ().right_spacing; }
  void               right_spacing   (uint s);
  uint               bottom_spacing  () const   { return pack_info ().bottom_spacing; }
  void               bottom_spacing  (uint s);
  uint               top_spacing     () const   { return pack_info ().top_spacing; }
  void               top_spacing     (uint s);
  double             halign          () const   { return pack_info ().halign; }
  void               halign          (double f);
  double             hscale          () const   { return pack_info ().hscale; }
  void               hscale          (double f);
  double             valign          () const   { return pack_info ().valign; }
  void               valign          (double f);
  double             vscale          () const   { return pack_info ().vscale; }
  void               vscale          (double f);
  Point              position        () const   { const PackInfo &pi = pack_info(); return Point (pi.hposition, pi.vposition); }
  void               position        (Point p); // mirrors (hposition,vposition)
  double             hanchor         () const   { return halign(); } // mirrors halign
  void               hanchor         (double a) { halign (a); }      // mirrors halign
  double             vanchor         () const   { return valign(); } // mirrors valign
  void               vanchor         (double a) { valign (a); }      // mirrors valign
private:
  void               repack          (const PackInfo &orig, const PackInfo &pnew);
  PackInfo&          pack_info       (bool create);
public:
  virtual ItemIface* unique_component   (const String &path);
  virtual ItemSeq    collect_components (const String &path);
  template<class C> typename
  InterfaceMatch<C>::Result interface        (const String &ident = String(),
                                              const std::nothrow_t &nt = dothrow) const;
  template<class C> typename
  InterfaceMatch<C>::Result parent_interface (const String &ident = String(),
                                              const std::nothrow_t &nt = dothrow) const;
  virtual OwnedMutex&       owned_mutex        ();
protected:
  void                  allocation      (const Allocation &area);
  virtual bool          do_event        (const Event &event);
private:
  void                  type_cast_error (const char *dest_type) RAPICORN_NORETURN;
  bool                  match_interface (bool wself, bool wparent, bool children, InterfaceMatcher &imatcher) const;
};
inline bool operator== (const ItemImpl &item1, const ItemImpl &item2) { return &item1 == &item2; }
inline bool operator!= (const ItemImpl &item1, const ItemImpl &item2) { return &item1 != &item2; }

template<class C> typename ItemImpl::InterfaceMatch<C>::Result
ItemImpl::interface (const String         &ident,
                     const std::nothrow_t &nt) const
{
  InterfaceMatch<C> interface_match (ident);
  match_interface (1, 0, 1, interface_match);
  return interface_match.result (&nt == &dothrow);
}

template<class C> typename ItemImpl::InterfaceMatch<C>::Result
ItemImpl::parent_interface (const String         &ident,
                            const std::nothrow_t &nt) const
{
  InterfaceMatch<C> interface_match (ident);
  match_interface (0, 1, 0, interface_match);
  return interface_match.result (&nt == &dothrow);
}

} // Rapicorn

#endif  /* __RAPICORN_ITEM_HH_ */
