// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#ifndef __RAPICORN_WIDGET_HH_
#define __RAPICORN_WIDGET_HH_

#include <ui/object.hh>
#include <ui/events.hh>
#include <ui/region.hh>
#include <ui/commands.hh>
#include <ui/style.hh>

namespace Rapicorn {

/* --- Widget structures and forward decls --- */
typedef Rect Allocation;
class SizeGroup;
class WidgetGroup;
class Adjustment;
class ResizeContainerImpl;
class WindowImpl;
class ViewportImpl;
namespace Selector { class Selob; }
enum WidgetGroupType { WIDGET_GROUP_HSIZE = 1, WIDGET_GROUP_VSIZE, WIDGET_GROUP_RADIO };

class ContainerImpl;
typedef std::shared_ptr<ContainerImpl> ContainerImplP;
typedef std::weak_ptr<ContainerImpl>   ContainerImplW;

/* --- event handler --- */
class EventHandler : public virtual Aida::ImplicitBase {
  typedef Aida::Signal<bool (const Event&), Aida::CollectorWhile0<bool>> EventSignal;
public:
  explicit      EventHandler    ();
  EventSignal   sig_event;
  typedef enum {
    RESET_ALL
  } ResetMode;
  virtual void  reset           (ResetMode       mode = RESET_ALL) = 0;
  virtual bool  capture_event   (const Event    &event);
  virtual bool  handle_event    (const Event    &event);
};

class WidgetImpl;
typedef std::shared_ptr<WidgetImpl> WidgetImplP;
typedef std::weak_ptr<WidgetImpl>   WidgetImplW;

/// WidgetImpl is the base type for all UI element implementations and implements the Widget interface.
/// More details about widgets are covered in @ref Widget.
class WidgetImpl : public virtual WidgetIface, public virtual ObjectImpl {
  friend                      class WidgetGroup;
  friend                      class SizeGroup;
  friend                      class ContainerImpl;
  friend                      class WindowImpl;
  friend                      class RapicornInternal::ImplementationHelper;
public:
  struct AncestryCache;
private:
  uint32                      widget_flags_; // WidgetFlag
  uint16                      widget_state_; // WidgetState
  uint16                      inherited_state_; // WidgetState
  ContainerImpl              *parent_; // inlined for fast access
  const AncestryCache        *acache_; // cache poninter may change for const this
  FactoryContext             &factory_context_;
  Requisition                 requisition_;
  Allocation                  allocation_, clip_area_;
  Requisition                 inner_size_request (); // ungrouped size requisition
  void                        acache_check       () const;
  void                        expose_internal    (const Region &region); // expose region on ancestry Viewport
  WidgetGroup*                find_widget_group  (const String &group_name, WidgetGroupType group, bool force_create = false);
  void                        data_context_changed  ();
  bool                        match_interface       (bool wself, bool wparent, bool children, InterfaceMatcher &imatcher) const;
  bool                        process_event         (const Event &event, bool capture = false);  // widget coordinates relative
  virtual bool                may_toggle            () const;
protected:
  virtual void                fabricated            (); ///< Method called on all widgets after creation via Factory.
  virtual bool                do_event              (const Event &event);
  virtual void                foreach_recursive     (const std::function<void (WidgetImpl&)> &f);
  virtual const AncestryCache* fetch_ancestry_cache ();
  // flag handling
  enum WidgetFlag {
    FINALIZING                = 1 <<  0, ///< Flag used internally to short-cut destructor phase.
    CONSTRUCTED               = 1 <<  1, ///< Flag used internally to seal widget construction.
    ANCHORED                  = 1 <<  2, ///< Flag set on widgets while its ancestry contains a Window, see hierarchy_changed()
    VISIBLE                   = 1 <<  3, ///< Flag set on widgets to be visible on screen, see visible()
    ALLOW_FOCUS               = 1 <<  4, ///< Flag set by the widget user to indicate if a widget may or may not receive focus.
    FOCUS_CHAIN               = 1 <<  5, ///< Flag indicates if widget is (in the ancestry of) the focus widget.
    NEEDS_FOCUS_INDICATOR     = 1 <<  6, ///< Flag used for containers that need a focus-indicator to receive input focus.
    HAS_FOCUS_INDICATOR       = 1 <<  7, ///< Flag set on #NEEDS_FOCUS_INDICATOR containers if a descendant provides a focus-indicator.
    HAS_DEFAULT               = 1 <<  8, ///< Flag indicating widget to receive default activation, see has_default()
    HAS_CLIP_AREA             = 1 <<  9, ///< Flag indicating wether a clip_area() has been assigned or not.
    INVALID_REQUISITION       = 1 << 10, ///< Flag indicates the need update widget's size requisition, see requisition()
    INVALID_ALLOCATION        = 1 << 11, ///< Flag indicates the need update widget's allocation, see set_allocation()
    INVALID_CONTENT           = 1 << 12, ///< Flag indicates that the widget's entire contents need to be repainted, see expose()
    HSHRINK                   = 1 << 13, ///< Flag set on widgets that handle horizontal shrinking well, see hshrink()
    VSHRINK                   = 1 << 14, ///< Flag set on widgets that handle vertical shrinking well, see vshrink()
    HEXPAND                   = 1 << 15, ///< Flag set on widgets that are useful to expand horizontally, see hexpand()
    VEXPAND                   = 1 << 16, ///< Flag set on widgets that are useful to expand vertically, see vexpand()
    HSPREAD                   = 1 << 17, ///< Flag set on widgets that should expand/shrink horizontally with window growth, see hspread()
    VSPREAD                   = 1 << 18, ///< Flag set on widgets that should expand/shrink vertically with window growth, see vspread()
    HSPREAD_CONTAINER         = 1 << 19, ///< Flag set on containers that contain hspread() widgets
    VSPREAD_CONTAINER         = 1 << 20, ///< Flag set on containers that contain vspread() widgets
  };
  friend WidgetFlag           operator^         (WidgetFlag a, WidgetFlag b) { return WidgetFlag (uint64 (a) ^ uint64 (b)); }
  friend WidgetFlag           operator|         (WidgetFlag a, WidgetFlag b) { return WidgetFlag (uint64 (a) | uint64 (b)); }
  friend WidgetFlag           operator&         (WidgetFlag a, WidgetFlag b) { return WidgetFlag (uint64 (a) & uint64 (b)); }
  bool                        change_flags      (WidgetFlag mask, bool on);
  void                        set_flag          (WidgetFlag flag, bool on);
  void                        adjust_state      (WidgetState state, bool on);
  void                        propagate_state   (WidgetState prev_state);
  // resizing, requisition and allocation
  virtual void                size_request      (Requisition &requisition) = 0; ///< Type specific size requisition implementation, see requisition().
  virtual void                size_allocate     (Allocation area, bool changed) = 0; ///< Type specific size allocation implementation, see set_allocation().
  virtual void                invalidate_parent ();
  void                        clip_area         (const Allocation *clip);
  bool                        tune_requisition  (Requisition  requisition);
  bool                        tune_requisition  (double       new_width,
                                                 double       new_height);
  /* signal methods */
  virtual void                do_invalidate     ();
  virtual void                do_changed        (const String &name) override;
  /* idlers & timers */
  uint                        exec_fast_repeater   (const EventLoop::BoolSlot &sl);
  uint                        exec_slow_repeater   (const EventLoop::BoolSlot &sl);
  uint                        exec_key_repeater    (const EventLoop::BoolSlot &sl);
  uint                        exec_now             (const EventLoop::VoidSlot &sl); ///< Run @a sl as next thing from the window (or global) event loop.
  bool                        remove_exec          (uint            exec_id);
  bool                        clear_exec           (uint           *exec_id);
  virtual void                visual_update        ();
  /* misc */
  virtual                    ~WidgetImpl        ();
  virtual void                construct         () override;
  virtual void                set_parent        (ContainerImpl *parent);
  virtual void                hierarchy_changed (WidgetImpl *old_toplevel);
  virtual bool                can_focus         () const; ///< Widget specific sentinel on wether it can currently receive input focus.
  virtual bool                activate_widget   ();
  virtual bool                custom_command    (const String &command_name, const StringSeq &command_args);
  virtual void                set_user_data     (const String &name, const Any &any);
  virtual Any                 get_user_data     (const String &name);
  void                        sync_widget_groups (const String &group_list, WidgetGroupType group_type);
  void                        enter_widget_group (const String &group_name, WidgetGroupType group_type);
  void                        leave_widget_group (const String &group_name, WidgetGroupType group_type);
  StringVector                list_widget_groups (WidgetGroupType group_type) const;
  void                        notify_key_error  ();
  /// @{ @name Content Retrival and Offering
  bool                        request_content   (ContentSourceType csource, uint64 nonce, const String &data_type);
  bool                        own_content       (ContentSourceType csource, uint64 nonce, const StringVector &data_types);
  bool                        disown_content    (ContentSourceType csource, uint64 nonce);
  bool                        provide_content   (const String &data_type, const String &data, uint64 request_id);
  /// @}
public:
  explicit                    WidgetImpl        ();
  virtual WindowImpl*         as_window_impl    ()              { return NULL; }
  virtual ContainerImpl*      as_container_impl ()              { return NULL; }
  virtual Selector::Selob*    pseudo_selector   (Selector::Selob &selob, const String &ident, const String &arg, String &error) { return NULL; }
  bool                        test_flag         (WidgetFlag mask) const;
  bool                        test_all          (WidgetFlag mask) const { return (widget_flags_ & mask) == mask; }
  bool                        test_any          (WidgetFlag mask) const { return (widget_flags_ & mask) != 0; }
  bool                        isconstructed     () const { return test_flag (CONSTRUCTED); } ///< Check if widget is properly constructed.
  bool                        finalizing        () const { return test_flag (FINALIZING); }  ///< Check if the last widget reference is lost.
  bool                        anchored          () const { return test_flag (ANCHORED); }    ///< Get widget anchored state, see #ANCHORED
  virtual bool                visible           () const { return test_flag (VISIBLE); }     ///< Get widget visibility, see #VISIBLE
  virtual void                visible           (bool b) { set_flag (VISIBLE, b); }     ///< Toggle widget visibility
  bool                        ancestry_visible  () const; ///< Check if ancestry is fully visible.
  bool                        stashed           () const { return test_state (WidgetState::STASHED); }
  bool                        viewable          () const; // visible() && !STASHED
  bool                        drawable          () const; // viewable() && clipped_allocation > 0
  virtual bool                sensitive         () const override;
  virtual void                sensitive         (bool b) override;
  virtual bool                toggled           () const override;
  virtual void                toggled           (bool b) override;
  virtual bool                retained          () const override;
  virtual void                retained          (bool b) override;
  bool                        insensitive       () const { return !sensitive(); }               ///< Negation of sensitive()
  void                        insensitive       (bool b) { sensitive (!b); }                    ///< Negation of sensitive(bool)
  bool                        key_sensitive     () const;
  bool                        pointer_sensitive () const;
  bool                        hover             () const { return test_state (WidgetState::HOVER); } ///< Get widget "hover" state, see WidgetState::WidgetState::HOVER
  virtual void                hover             (bool b) { adjust_state (WidgetState::HOVER, b); }   ///< Toggled with "hover" state of a widget
  bool                        ancestry_hover    () const; ///< Check if ancestry contains hover().
  bool                        active            () const;
  virtual void                active            (bool b);
  bool                        ancestry_active   () const; ///< Check if ancestry contains active().
  bool                        has_default       () const { return test_flag (HAS_DEFAULT); }
  bool                        grab_default      () const;
  virtual bool                allow_focus       () const override; ///< Indicates if widget may receive input foucs.
  virtual void                allow_focus       (bool b) override; ///< Toggle if widget may receive input focus.
  bool                        focusable         () const; ///< Returns true if @a this widget participates in input focus selection.
  bool                        has_focus         () const; ///< Returns true if @a this widget has focus to receive keyboard events.
  bool                        grab_focus        ();
  void                        unset_focus       ();
  virtual bool                move_focus        (FocusDir fdir);
  virtual bool                activate          ();
  virtual bool                hexpand           () const { return test_any (HEXPAND | HSPREAD | HSPREAD_CONTAINER); } ///< Get horizontal expansion
  virtual void                hexpand           (bool b) { set_flag (HEXPAND, b); } ///< Allow horizontal expansion, see #VEXPAND
  virtual bool                vexpand           () const { return test_any (VEXPAND | VSPREAD | VSPREAD_CONTAINER); } ///< Get vertical expansion
  virtual void                vexpand           (bool b) { set_flag (VEXPAND, b); } ///< Allow vertical expansion, see #VEXPAND
  virtual bool                hspread           () const { return test_any (HSPREAD | HSPREAD_CONTAINER); } ///< Get horizontal spreading
  virtual void                hspread           (bool b) { set_flag (HSPREAD, b); } ///< Allow horizontal spreading, see #HSPREAD
  virtual bool                vspread           () const { return test_any (VSPREAD | VSPREAD_CONTAINER); } ///< Get vertical spreading
  virtual void                vspread           (bool b) { set_flag (VSPREAD, b); } ///< Allow vertical spreading, see #VSPREAD
  virtual bool                hshrink           () const { return test_any (HSHRINK | HSPREAD | HSPREAD_CONTAINER); } ///< Get horizontal shrinking flag
  virtual void                hshrink           (bool b) { set_flag (HSHRINK, b); } ///< Allow horizontal shrinking, see #HSHRINK
  virtual bool                vshrink           () const { return test_any (VSHRINK | VSPREAD | VSPREAD_CONTAINER); } ///< Get vertical shrinking flag
  virtual void                vshrink           (bool b) { set_flag (VSHRINK, b); } ///< Allow vertical shrinking, see #VSHRINK
  virtual String              hsize_group       () const;
  virtual void                hsize_group       (const String &group_list);
  virtual String              vsize_group       () const;
  virtual void                vsize_group       (const String &group_list);
  virtual String              id                () const;            ///< Get Widget id
  virtual void                id                (const String &str); ///< Set Widget id
  FactoryContext&             factory_context   () const        { return factory_context_; }
  UserSource                  user_source       () const;
  /* override requisition */
  double                      width             () const;
  void                        width             (double w);
  double                      height            () const;
  void                        height            (double h);
  /* properties */
  Property*                   lookup_property   (const String    &property_name);
  String                      get_property      (const String    &property_name);
  void                        set_property      (const String    &property_name,
                                                 const String    &value);
  bool                        try_set_property  (const String    &property_name,
                                                 const String    &value);
  // bindings
  void                        add_binding       (const String &property, const String &binding_path);
  void                        remove_binding    (const String &property);
  virtual void                data_context      (ObjectIface     &context);
  ObjectIfaceP                data_context      () const;
  /* commands */
  bool                        exec_command      (const String    &command_call_string);
  Command*                    lookup_command    (const String    &command_name);
  virtual const CommandList&  list_commands     ();
  WidgetImplP                 widgetp           () { return shared_ptr_cast<WidgetImpl> (this); }
  ContainerImpl*              parent            () const { return parent_; }
  ContainerImplP              parentp           () const;
  ContainerImpl*              root              () const;
  bool                        has_ancestor      (const WidgetImpl &ancestor) const;
  WidgetImpl*                 common_ancestor   (const WidgetImpl &other) const;
  WidgetImpl*                 common_ancestor   (const WidgetImpl *other) const { return common_ancestor (*other); }
  const AncestryCache*        ancestry_cache       () const { if (RAPICORN_UNLIKELY (!acache_)) acache_check(); return acache_; }
  WindowImpl*                 get_window           () const;
  ViewportImpl*               get_viewport         () const;
  ResizeContainerImpl*        get_resize_container () const;
  /* cross links */
  typedef std::function<void (WidgetImpl&)> WidgetSlot;
  size_t                      cross_link        (WidgetImpl &link, const WidgetSlot &uncross);
  void                        cross_unlink      (WidgetImpl &link, size_t link_id);
  void                        uncross_links     (WidgetImpl &link);
  /* invalidation / changes */
  virtual void                changed           (const String &name) override;
  virtual void                invalidate        (WidgetFlag mask = INVALID_REQUISITION | INVALID_ALLOCATION | INVALID_CONTENT);
  void                        invalidate_size   ()              { invalidate (INVALID_REQUISITION | INVALID_ALLOCATION); }
  void                        expose            () { expose (allocation()); } ///< Expose entire widget, see expose(const Region&)
  void                        expose            (const Rect &rect) { expose (Region (rect)); } ///< Rectangle constrained expose()
  void                        expose            (const Region &region);
  void                        queue_visual_update  ();
  void                        force_visual_update  ();
  /* public signals */
  Signal<void ()>                   sig_invalidate;
  Signal<void (WidgetImpl *old)>    sig_hierarchy_changed;
  /* coordinate handling */
protected:
  struct WidgetChain { WidgetImpl *widget; WidgetChain *next; WidgetChain() : widget (NULL), next (NULL) {} };
  // rendering
  class RenderContext;
  virtual void               render_widget             (RenderContext    &rcontext);
  virtual void               render_recursive          (RenderContext    &rcontext);
  virtual void               render                    (RenderContext    &rcontext, const Rect &rect) = 0;
  const Region&              rendering_region          (RenderContext    &rcontext) const;
  virtual cairo_t*           cairo_context             (RenderContext    &rcontext,
                                                        const Allocation &area = Allocation (-1, -1, 0, 0));
public:
  void                       render_into               (cairo_t *cr, const Region &region);
  virtual bool               point                     (Point        p);            // widget coordinates relative
  /* public size accessors */
  virtual Requisition        requisition        ();                              // effective size requisition
  void                       set_allocation     (const Allocation &area,
                                                 const Allocation *clip = NULL); // assign new allocation
  const Allocation&          allocation         () const { return allocation_; } ///< Return widget layout area, see also clipped_allocation().
  Allocation                 clipped_allocation () const;
  const Allocation*          clip_area          () const;
  // theming & appearance
  Color                 current_color           (StyleColor color_type, const String &detail = "");
  Color                 state_color             (WidgetState state, StyleColor color_type, const String &detail = "");
  Color                 normal_bg               () { return state_color (WidgetState::NORMAL, StyleColor::BACKGROUND); }
  Color                 normal_fg               () { return state_color (WidgetState::NORMAL, StyleColor::FOREGROUND); }
  Color                 active_bg               () { return state_color (WidgetState::ACTIVE, StyleColor::BACKGROUND); }
  Color                 active_fg               () { return state_color (WidgetState::ACTIVE, StyleColor::FOREGROUND); }
  Color                 selected_bg             () { return state_color (WidgetState::SELECTED, StyleColor::BACKGROUND); }
  Color                 selected_fg             () { return state_color (WidgetState::SELECTED, StyleColor::FOREGROUND); }
  Color                 state_color             (WidgetState state, bool foreground, const String &detail = "");
  Color                 theme_color             (double hue360, double saturation100, double brightness100, const String &detail = "");
  WidgetState           state                   () const { return WidgetState (widget_state_ | inherited_state_); } ///< Bit mask of WidgetState state flags.
  bool                  test_state              (WidgetState bits) const { return (state() & bits) == bits; } ///< Test presence of all WidgetState @a bits.
  StyleIfaceP           style                   () const;
  Color                 foreground              ();
  Color                 background              ();
  Color                 dark_color              ();
  Color                 dark_shadow             ();
  Color                 dark_glint              ();
  Color                 light_color             ();
  Color                 light_shadow            ();
  Color                 light_glint             ();
  Color                 focus_color             ();
  /* debugging/testing */
  virtual String        test_dump               ();
  String                debug_dump              (const String &flags = String());
  String                debug_name              (const String &format = "") const;
protected:
  void                  make_test_dump          (TestStream   &tstream);
  virtual void          dump_test_data          (TestStream   &tstream);
  virtual void          dump_private_data       (TestStream   &tstream);
  /* convenience */
public:
  void                  find_adjustments        (AdjustmentSourceType adjsrc1,
                                                 Adjustment         **adj1,
                                                 AdjustmentSourceType adjsrc2 = AdjustmentSourceType::NONE,
                                                 Adjustment         **adj2 = NULL,
                                                 AdjustmentSourceType adjsrc3 = AdjustmentSourceType::NONE,
                                                 Adjustment         **adj3 = NULL,
                                                 AdjustmentSourceType adjsrc4 = AdjustmentSourceType::NONE,
                                                 Adjustment         **adj4 = NULL);
  // packing
  struct PackInfo {
    double hposition, hspan, vposition, vspan;
    uint left_spacing, right_spacing, bottom_spacing, top_spacing;
    double halign, hscale, valign, vscale;
  };
  const PackInfo&    pack_info       () const   { return const_cast<WidgetImpl*> (this)->pack_info (false); }
  double             hposition       () const   { return pack_info ().hposition; }
  void               hposition       (double d);
  double             hspan           () const   { return pack_info ().hspan; }
  void               hspan           (double d);
  double             vposition       () const   { return pack_info ().vposition; }
  void               vposition       (double d);
  double             vspan           () const   { return pack_info ().vspan; }
  void               vspan           (double d);
  int                left_spacing    () const   { return pack_info ().left_spacing; }
  void               left_spacing    (int s);
  int                right_spacing   () const   { return pack_info ().right_spacing; }
  void               right_spacing   (int s);
  int                bottom_spacing  () const   { return pack_info ().bottom_spacing; }
  void               bottom_spacing  (int s);
  int                top_spacing     () const   { return pack_info ().top_spacing; }
  void               top_spacing     (int s);
  double             halign          () const   { return pack_info ().halign; }
  void               halign          (double f);
  double             hscale          () const   { return pack_info ().hscale; }
  void               hscale          (double f);
  double             valign          () const   { return pack_info ().valign; }
  void               valign          (double f);
  double             vscale          () const   { return pack_info ().vscale; }
  void               vscale          (double f);
  double             hanchor         () const   { return halign(); } // mirrors halign
  void               hanchor         (double a) { halign (a); }      // mirrors halign
  double             vanchor         () const   { return valign(); } // mirrors valign
  void               vanchor         (double a) { valign (a); }      // mirrors valign
private:
  void               repack          (const PackInfo &orig, const PackInfo &pnew);
  PackInfo&          pack_info       (bool create);
public:
  virtual bool         match_selector        (const String &selector);
  virtual WidgetIfaceP query_selector        (const String &selector);
  virtual WidgetSeq    query_selector_all    (const String &selector);
  virtual WidgetIfaceP query_selector_unique (const String &selector);
  template<class C> typename
  InterfaceMatch<C>::Result interface        (const String &ident = String(),
                                              const std::nothrow_t &nt = dothrow) const;
  template<class C> typename
  InterfaceMatch<C>::Result parent_interface (const String &ident = String(),
                                              const std::nothrow_t &nt = dothrow) const;
};

// == Implementations ==
template<class C> typename WidgetImpl::InterfaceMatch<C>::Result
WidgetImpl::interface (const String &ident, const std::nothrow_t &nt) const
{
  InterfaceMatch<C> interface_match (ident);
  match_interface (1, 0, 1, interface_match);
  return interface_match.result (&nt == &dothrow);
}

template<class C> typename WidgetImpl::InterfaceMatch<C>::Result
WidgetImpl::parent_interface (const String &ident, const std::nothrow_t &nt) const
{
  InterfaceMatch<C> interface_match (ident);
  match_interface (0, 1, 0, interface_match);
  return interface_match.result (&nt == &dothrow);
}

} // Rapicorn

#endif  /* __RAPICORN_WIDGET_HH_ */
