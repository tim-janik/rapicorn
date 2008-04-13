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
#ifndef __RAPICORN_ITEM_HH_
#define __RAPICORN_ITEM_HH_

#include <rapicorn/events.hh>
#include <rapicorn/region.hh>
#include <rapicorn/commands.hh>
#include <rapicorn/properties.hh>
#include <rapicorn/appearance.hh>

namespace Rapicorn {

/* --- Item structures and forward decls --- */
struct Requisition {
  double width, height;
  Requisition () : width (0), height (0) {}
  Requisition (double w, double h) : width (w), height (h) {}
};
typedef Rect Allocation;
class Item;
class Container;
class Root;

/* --- event handler --- */
class EventHandler : public virtual ReferenceCountImpl {
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
class Item;
typedef Signals::Slot1<void, Item&> ItemSlot;
class Item : public virtual Convertible, public virtual DataListContainer, public virtual ReferenceCountImpl {
  friend                      class ClassDoctor;
  friend                      class Container;
  uint32                      m_flags;          /* interface-inlined for fast read-out */
  Container                  *m_parent;         /* interface-inlined for fast read-out */
  Style                      *m_style;
  void                        propagate_flags (bool notify_changed = true);
  void                        propagate_style ();
  Container**                 _parent_loc     () { return &m_parent; }
  RAPICORN_PRIVATE_CLASS_COPY  (Item);
protected:
  virtual void                constructed             ();
  /* flag handling */
  bool                        change_flags_silently   (uint32 mask, bool on);
  enum {
    ANCHORED                  = 1 <<  0,
    VISIBLE                   = 1 <<  1,
    HIDDEN_CHILD              = 1 <<  2,
    SENSITIVE                 = 1 <<  3,
    PARENT_SENSITIVE          = 1 <<  4,
    PRELIGHT                  = 1 <<  5,
    IMPRESSED                 = 1 <<  6,
    FOCUS_CHAIN               = 1 <<  7,
    HAS_DEFAULT               = 1 <<  8,
    INVALID_REQUISITION       = 1 <<  9,
    INVALID_ALLOCATION        = 1 << 10,
    INVALID_CONTENT           = 1 << 11,
    HEXPAND                   = 1 << 14,
    VEXPAND                   = 1 << 15,
    HSPREAD                   = 1 << 16,
    VSPREAD                   = 1 << 17,
    HSPREAD_CONTAINER         = 1 << 18,
    VSPREAD_CONTAINER         = 1 << 19,
    ALLOCATABLE               = 1 << 20,
    POSITIVE_ALLOCATION       = 1 << 21,
    DEBUG                     = 1 << 22,
    LAST_FLAG                 = 1 << 23
  };
  void                        set_flag          (uint32 flag, bool on = true);
  void                        unset_flag        (uint32 flag) { set_flag (flag, false); }
  bool                        test_flags        (uint32 mask) const { return (m_flags & mask) != 0; }
  bool                        test_all_flags    (uint32 mask) const { return (m_flags & mask) == mask; }
  /* size requisition and allocation */
  virtual void                size_request      (Requisition &requisition) = 0;
  virtual void                size_allocate     (Allocation   area) = 0;
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
  /* misc */
  virtual                     ~Item             ();
  virtual void                style             (Style  *st);
  virtual void                finalize          ();
  virtual void                hierarchy_changed (Item *old_toplevel);
  virtual bool                move_focus        (FocusDirType fdir);
  virtual bool                custom_command    (const String       &command_name,
                                                 const StringVector &command_args);
  void                        anchored          (bool b) { set_flag (ANCHORED, b); }
  void                        notify_key_error  ();
public:
  explicit                    Item              ();
  bool                        anchored          () const { return test_flags (ANCHORED); }
  bool                        visible           () const { return test_flags (VISIBLE) && !test_flags (HIDDEN_CHILD); }
  void                        visible           (bool b) { set_flag (VISIBLE, b); }
  bool                        allocatable       () const { return visible() && test_flags (ALLOCATABLE); }
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
  bool                        debug             () const { return test_flags (DEBUG); }
  void                        debug             (bool f) { set_flag (DEBUG, f); }
  virtual String              name              () const = 0;
  virtual void                name              (const String &str) = 0;
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
  virtual void                set_parent        (Container *parent);
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
  virtual const Requisition& size_request       () = 0;                         /* re-request size */
  const Requisition&         requisition        () { return size_request(); }   /* cached requisition */
  virtual void               set_allocation     (const Allocation &area) = 0;   /* assign new allocation */
  virtual const Allocation&  allocation         () = 0;                         /* current allocation */
  /* display */
  virtual void               render             (Display        &display) = 0;
  /* styles / appearance */
  StateType             state                   () const;
  Style*                style                   () { return m_style; }
  Color                 foreground              () { return style()->standard_color (state(), COLOR_FOREGROUND); }
  Color                 background              () { return style()->standard_color (state(), COLOR_BACKGROUND); }
  Color                 selected_foreground     () { return style()->selected_color (state(), COLOR_FOREGROUND); }
  Color                 selected_background     () { return style()->selected_color (state(), COLOR_BACKGROUND); }
  Color                 focus_color             () { return style()->standard_color (state(), COLOR_FOCUS); }
  Color                 default_color           () { return style()->standard_color (state(), COLOR_DEFAULT); }
  Color                 light_glint             () { return style()->standard_color (state(), COLOR_LIGHT_GLINT); }
  Color                 light_shadow            () { return style()->standard_color (state(), COLOR_LIGHT_SHADOW); }
  Color                 dark_glint              () { return style()->standard_color (state(), COLOR_DARK_GLINT); }
  Color                 dark_shadow             () { return style()->standard_color (state(), COLOR_DARK_SHADOW); }
  Color                 white                   () { return style()->color_scheme (Style::STANDARD).generic_color (0xffffffff); }
  Color                 black                   () { return style()->color_scheme (Style::STANDARD).generic_color (0xff000000); }
public:
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
