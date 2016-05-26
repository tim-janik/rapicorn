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
typedef IRect Allocation;
class SizeGroup;
class WidgetGroup;
typedef std::shared_ptr<WidgetGroup> WidgetGroupP;
typedef std::vector<WidgetGroupP> WidgetGroupVector;
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

struct PackInfo {
  float hposition, hspan, vposition, vspan;
  float halign, hscale, valign, vscale;
  int32 left_spacing : 16, right_spacing : 16, bottom_spacing : 16, top_spacing : 16;
  int32 ovr_width : 16, ovr_height : 16;
  WidgetGroupVector widget_groups;
};

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
  static const PackInfo       default_pack_info;
  uint32                      widget_flags_; // WidgetFlag
  uint16                      widget_state_; // WidgetState
  uint16                      inherited_state_; // WidgetState
  ContainerImpl              *parent_; // inlined for fast access
  const AncestryCache        *acache_; // cache poninter may change for const this
  FactoryContext             &factory_context_;
  Requisition                 requisition_;
  Allocation                  allocation_;
  PackInfo                   *pack_info_;
  cairo_surface_t            *cached_surface_;
  Requisition                 inner_size_request (); // ungrouped size requisition
  void                        acache_check       () const;
  void                        widget_adjust_state       (WidgetState state, bool on);
  void                        expose_unclipped   (const Region &region); // expose region on ancestry Viewport
  WidgetGroup*                find_widget_group  (const String &group_name, WidgetGroupType group, bool force_create = false);
  void                        data_context_changed  ();
  bool                        match_interface       (bool wself, bool wparent, bool children, InterfaceMatcher &imatcher) const;
  bool                        process_event         (const Event &event, bool capture = false);  // widget coordinates relative
  void                        repack                 (const PackInfo &orig, const PackInfo &pnew);
  PackInfo&                   widget_pack_info       ();
  void                        widget_propagate_state (WidgetState prev_state);
  virtual bool                widget_maybe_toggled   () const;
  virtual bool                widget_maybe_selected  () const;
  void                        widget_render_recursive (const IRect &ancestry_clip);
  void                        invalidate_all         ()              { widget_invalidate (INVALID_REQUISITION | INVALID_ALLOCATION | INVALID_CONTENT); }
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
    /* unused = 1 <<  9, */
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
  void                        set_flag          (WidgetFlag flag, bool on);
  bool                        change_flags_silently  (WidgetFlag mask, bool on);
  // resizing, requisition and allocation
  virtual void                widget_invalidate      (WidgetFlag mask); // FIXME: make private
  virtual void                size_request      (Requisition &requisition) = 0; ///< Type specific size requisition implementation, see requisition().
  virtual void                size_allocate     (Allocation area, bool changed) = 0; ///< Type specific size allocation implementation, see set_allocation().
  bool                        tune_requisition  (Requisition  requisition);
  bool                        tune_requisition  (int new_width, int new_height);
  /* signal methods */
  virtual void                do_changed        (const String &name) override;
  enum class WidgetChange { NONE, VIEWABLE, STATE, FLAGS, CHILD_ADDED, CHILD_REMOVED };
  virtual WidgetFlag          change_invalidation   (WidgetChange type, uint64 bits);
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
  virtual bool                acceleratable     () const override;      ///< See Widget::acceleratable and WidgetState::ACCELERATABLE
  virtual void                acceleratable     (bool b) override;      ///< See Widget::acceleratable and WidgetState::ACCELERATABLE
  virtual bool                hover             () const override;
  virtual void                hover             (bool b) override;
  virtual bool                panel             () const override;
  virtual void                panel             (bool b) override;
  virtual bool                receives_default  () const override;
  virtual void                receives_default  (bool b) override;
  virtual bool                selected          () const override;
  virtual void                selected          (bool b) override;
  virtual bool                focused           () const override;
  virtual void                focused           (bool b) override;
  virtual bool                insensitive       () const override;
  virtual void                insensitive       (bool b) override;
  bool                        sensitive         () const;
  void                        sensitive         (bool b);
  virtual bool                active            () const override;
  virtual void                active            (bool b) override;
  virtual bool                toggled           () const override;
  virtual void                toggled           (bool b) override;
  virtual bool                retained          () const override;
  virtual void                retained          (bool b) override;
  virtual bool                stashed           () const override;
  virtual void                stashed           (bool b) override;
  virtual bool                visible           () const override { return test_flag (VISIBLE); }     ///< Get widget visibility, see #VISIBLE
  virtual void                visible           (bool b) override { set_flag (VISIBLE, b); }     ///< Toggle widget visibility
  virtual bool                allow_focus       () const override; ///< Indicates if widget may receive input foucs.
  virtual void                allow_focus       (bool b) override; ///< Toggle if widget may receive input focus.
  bool                        ancestry_visible  () const; ///< Check if ancestry is fully visible.
  bool                        viewable          () const; // visible() && !STASHED
  bool                        drawable          () const; // viewable() && allocation > 0
  bool                        key_sensitive     () const;
  bool                        pointer_sensitive () const;
  bool                        ancestry_hover    () const; ///< Check if ancestry contains hover().
  bool                        ancestry_active   () const; ///< Check if ancestry contains active().
  bool                        has_default       () const { return test_flag (HAS_DEFAULT); }
  bool                        grab_default      () const;
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
  // properties
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
  void                  expose                  () { expose (allocation()); } ///< Expose entire widget, see expose(const Region&)
  void                  expose                  (const IRect &rect) { expose (Region (rect)); } ///< Rectangle constrained expose()
  void                  expose                  (const Region &region);
  virtual void          changed                 (const String &name) override;
  void                  invalidate_content      ()              { widget_invalidate (INVALID_CONTENT); }
  void                  invalidate_requisition  ()              { widget_invalidate (INVALID_REQUISITION); }
  void                  invalidate_allocation   ()              { widget_invalidate (INVALID_ALLOCATION); }
  void                  invalidate_size         ()              { widget_invalidate (INVALID_REQUISITION | INVALID_ALLOCATION); }
  void                  queue_visual_update     ();
  void                  force_visual_update     ();
  /* public signals */
  Signal<void (WidgetImpl *old)>    sig_hierarchy_changed;
  /* coordinate handling */
protected:
  struct WidgetChain { WidgetImpl *widget; WidgetChain *next; WidgetChain() : widget (NULL), next (NULL) {} };
  // rendering
  class RenderContext;
  void                       render_widget             ();
  virtual void               render                    (RenderContext &rcontext) = 0;
  Region                     rendering_region          (RenderContext &rcontext) const;
  virtual cairo_t*           cairo_context             (RenderContext &rcontext);
public:
  void                       compose_into              (cairo_t *cr, const vector<IRect> &irects);
  virtual bool               point                     (Point        p);            // widget coordinates relative
  /* public size accessors */
  virtual Requisition        requisition        ();                              // effective size requisition
  void                       set_allocation     (const Allocation &area); // assign new allocation
  const Allocation&          allocation         () const { return allocation_; } ///< Return widget layout area, see also clipped_allocation().
  Allocation                 clipped_allocation () const;
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
  const PackInfo&    pack_info       () const           { return pack_info_ ? *pack_info_ : default_pack_info; }
  virtual int        width           () const override  { return pack_info ().ovr_width; }
  virtual void       width           (int w) override;
  virtual int        height          () const override  { return pack_info ().ovr_height; }
  virtual void       height          (int h) override;
  virtual double     hposition       () const override  { return pack_info ().hposition; }
  virtual void       hposition       (double d) override;
  virtual double     hspan           () const override  { return pack_info ().hspan; }
  virtual void       hspan           (double d) override;
  virtual double     vposition       () const override  { return pack_info ().vposition; }
  virtual void       vposition       (double d) override;
  virtual double     vspan           () const override  { return pack_info ().vspan; }
  virtual void       vspan           (double d) override;
  virtual int        left_spacing    () const override  { return pack_info ().left_spacing; }
  virtual void       left_spacing    (int s) override;
  virtual int        right_spacing   () const override  { return pack_info ().right_spacing; }
  virtual void       right_spacing   (int s) override;
  virtual int        bottom_spacing  () const override  { return pack_info ().bottom_spacing; }
  virtual void       bottom_spacing  (int s) override;
  virtual int        top_spacing     () const override  { return pack_info ().top_spacing; }
  virtual void       top_spacing     (int s) override;
  virtual double     halign          () const override  { return pack_info ().halign; }
  virtual void       halign          (double f) override;
  virtual double     hscale          () const override  { return pack_info ().hscale; }
  virtual void       hscale          (double f) override;
  virtual double     valign          () const override  { return pack_info ().valign; }
  virtual void       valign          (double f) override;
  virtual double     vscale          () const override  { return pack_info ().vscale; }
  virtual void       vscale          (double f) override;
  virtual double     hanchor         () const override  { return halign(); } // mirrors halign
  virtual void       hanchor         (double a) override { halign (a); }     // mirrors halign
  virtual double     vanchor         () const override  { return valign(); } // mirrors valign
  virtual void       vanchor         (double a) override { valign (a); }     // mirrors valign
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
