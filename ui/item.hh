/* Rapicorn
 * Copyright (C) 2005 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * A copy of the GNU Lesser General Public License should ship along
 * with this library; if not, see http://www.gnu.org/copyleft/.
 */
#include <ui/interface.hh> // includes <ui/item.hh> after It3m declaration

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
class Item;
class SizeGroup;
class Adjustment;
class Container;
class Root;

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

/* --- Item --- */
typedef Signals::Slot1<void, Item&> ItemSlot;
class Item : public virtual It3m, public virtual Convertible {
  friend                      class ClassDoctor;
  friend                      class Container;
  friend                      class SizeGroup;
  uint32                      m_flags;          /* interface-inlined for fast read-out */
  Container                  *m_parent;         /* interface-inlined for fast read-out */
  Heritage                   *m_heritage;
  Requisition                 inner_size_request (); // ungrouped size requisition
  void                        propagate_state    (bool notify_changed);
  Container**                 _parent_loc        () { return &m_parent; }
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
  virtual Requisition         cache_requisition (Requisition *requisition = NULL) = 0;
  virtual void                size_request      (Requisition &requisition) = 0;
  virtual void                size_allocate     (Allocation   area) = 0;
  virtual void                invalidate_parent ();
  virtual bool                tune_requisition  (Requisition  requisition);
  bool                        tune_requisition  (double       new_width,
                                                 double       new_height);
  /* signal methods */
  virtual bool                match_interface   (InterfaceMatch &imatch) const;
  virtual void                do_invalidate     () = 0;
  virtual void                do_changed        () = 0;
  /* idlers & timers */
  uint                        exec_fast_repeater   (const BoolSlot &sl);
  uint                        exec_slow_repeater   (const BoolSlot &sl);
  uint                        exec_key_repeater    (const BoolSlot &sl);
  bool                        remove_exec          (uint            exec_id);
  virtual void                visual_update        ();
  /* misc */
  virtual                     ~Item             ();
  virtual void                finalize          ();
  virtual void                set_parent        (Container *parent);
  virtual void                hierarchy_changed (Item *old_toplevel);
  virtual bool                move_focus        (FocusDirType fdir);
  virtual bool                custom_command    (const String       &command_name,
                                                 const StringList   &command_args);
  void                        anchored          (bool b) { set_flag (ANCHORED, b); }
  void                        notify_key_error  ();
public:
  explicit                    Item              ();
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
  virtual String              name              () const = 0;
  virtual void                name              (const String &str) = 0;
  virtual FactoryContext*     factory_context   () const = 0;
  virtual void                factory_context   (FactoryContext *fc) = 0;
  virtual ColorSchemeType     color_scheme      () const = 0;
  virtual void                color_scheme      (ColorSchemeType cst) = 0;
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
  Container*                  parent            () const { return m_parent; }
  bool                        has_ancestor      (const Item &ancestor) const;
  Item*                       common_ancestor   (const Item &other) const;
  Item*                       common_ancestor   (const Item *other) const { return common_ancestor (*other); }
  Root*                       get_root          () const;
  /* cross links */
  void                        cross_link        (Item           &link,
                                                 const ItemSlot &uncross);
  void                        cross_unlink      (Item           &link,
                                                 const ItemSlot &uncross);
  void                        uncross_links     (Item           &link);
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
  SignalFinalize<Item>            sig_finalize;
  Signal<Item, void ()>           sig_changed;
  Signal<Item, void ()>           sig_invalidate;
  Signal<Item, void (Item *oldt)> sig_hierarchy_changed;
  /* event handling */
  bool                       process_event          (const Event &event);       /* item coordinates relative */
  bool                       process_viewport_event (const Event &event);       /* viewport coordinates relative */
  /* coordinate handling */
protected:
  Affine                     affine_to_viewport     ();                         /* item => viewport affine */
  Affine                     affine_from_viewport   ();                         /* viewport => item affine */
public:
  virtual bool               point                  (Point        p);           /* item coordinates relative */
  Point                      point_to_viewport      (Point        item_point);  /* item coordinates relative */
  Point                      point_from_viewport    (Point        root_point);  /* viewport coordinates relative */
  virtual bool               translate_from         (const Item   &src_item,
                                                     const uint    n_points,
                                                     Point        *points) const;
  bool                       translate_to           (const uint    n_points,
                                                     Point        *points,
                                                     const Item   &target_item) const;
  bool                       translate_from         (const Item   &src_item,
                                                     const uint    n_rects,
                                                     Rect         *rects) const;
  bool                       translate_to           (const uint    n_rects,
                                                     Rect         *rects,
                                                     const Item   &target_item) const;
  bool                       viewport_point         (Point        p);           /* viewport coordinates relative */
  /* public size accessors */
  Requisition                requisition        ();                             // effective size requisition
  Requisition                size_request       () { return requisition(); }    // FIXME: remove
  virtual void               set_allocation     (const Allocation &area) = 0;   /* assign new allocation */
  virtual const Allocation&  allocation         () = 0;                         /* current allocation */
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
  void                  get_test_dump           (TestStream   &tstream);
protected:
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
  const PackInfo&    pack_info       () const   { return const_cast<Item*> (this)->pack_info (false); }
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
  virtual It3m*      unique_component   (const String &path);
  virtual ItemSeq    collect_components (const String &path);
  template<typename Type>
  typename
  InterfaceType<Type>::Result parent_interface  (const String &ident = String(), const std::nothrow_t &nt = dothrow) const
  {
    InterfaceType<Type> interface_type (ident);
    match_parent_interface (interface_type);
    return interface_type.result (&nt == &dothrow);
  }
  template<typename Type>
  typename
  InterfaceType<Type>::Result parent_interface  (const std::nothrow_t &nt) const { return parent_interface<Type> (String(), nt); }
  template<typename Type>
  typename
  InterfaceType<Type>::Result toplevel_interface  (const String &ident = String(), const std::nothrow_t &nt = dothrow) const
  {
    InterfaceType<Type> interface_type (ident);
    match_toplevel_interface (interface_type);
    return interface_type.result (&nt == &dothrow);
  }
  template<typename Type>
  typename
  InterfaceType<Type>::Result toplevel_interface  (const std::nothrow_t &nt) const { return toplevel_interface<Type> (String(), nt); }
  virtual OwnedMutex&         owned_mutex       ();
private:
  bool                 match_parent_interface   (InterfaceMatch &imatch) const;
  bool                 match_toplevel_interface (InterfaceMatch &imatch) const;
  void                 type_cast_error          (const char *dest_type) RAPICORN_NORETURN;
};
inline bool operator== (const Item &item1, const Item &item2) { return &item1 == &item2; }
inline bool operator!= (const Item &item1, const Item &item2) { return &item1 != &item2; }

} // Rapicorn

#endif  /* __RAPICORN_ITEM_HH_ */
