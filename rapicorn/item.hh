/* Rapicorn
 * Copyright (C) 2005 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#ifndef __RAPICORN_ITEM_HH_
#define __RAPICORN_ITEM_HH_

#include <rapicorn/events.hh>
#include <rapicorn/primitives.hh>
#include <rapicorn/commands.hh>
#include <rapicorn/properties.hh>
#include <rapicorn/appearance.hh>

namespace Rapicorn {

/* --- Item structures and forward decls --- */
struct Requisition {
  float width, height;
  Requisition () : width (0), height (0) {}
  Requisition (float w, float h) : width (w), height (h) {}
};
struct Allocation {
  int x, y, width, height;
  Allocation() : x (0), y (0), width (0), height (0) {}
  Allocation (int cx, int cy, int cwidth, int cheight) : x (cx), y (cy), width (cwidth), height (cheight) {}
  bool operator== (const Allocation &other) { return other.x == x && other.y == y && other.width == width && other.height == height; }
  bool operator!= (const Allocation &other) { return !operator== (other); }
};
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
class Item : public virtual Convertible, public virtual DataListContainer, public virtual ReferenceCountImpl {
  /*Copy*/                    Item      (const Item&);
  Item&                       operator= (const Item&);
  Item                       *m_parent;         /* interface-inlined for fast read-out */
  uint32                      m_flags;          /* interface-inlined for fast read-out */
  Style                      *m_style;
  friend class Container;
  void                        propagate_flags   ();
  void                        propagate_style   ();
protected:
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
    HAS_FOCUS                 = 1 <<  7,
    HAS_DEFAULT               = 1 <<  8,
    /* REQUEST_DEFAULT        = 1 <<  8, */
    INVALID_REQUISITION       = 1 << 10,
    INVALID_ALLOCATION        = 1 << 11,
    EXPOSE_ON_CHANGE          = 1 << 12,
    INVALIDATE_ON_CHANGE      = 1 << 13,
    HEXPAND                   = 1 << 14,
    VEXPAND                   = 1 << 15,
    HSPREAD                   = 1 << 16,
    VSPREAD                   = 1 << 17,
    HSPREAD_CONTAINER         = 1 << 18,
    VSPREAD_CONTAINER         = 1 << 19,
    POSITIVE_ALLOCATION       = 1 << 20,
    DEBUG                     = 1 << 21,
    LAST_FLAG                 = 1 << 22
  };
  void                        set_flag          (uint32 flag, bool on = true);
  void                        unset_flag        (uint32 flag) { set_flag (flag, false); }
  bool                        test_flags        (uint32 mask) const { return (m_flags & mask) == mask; }
  bool                        test_any_flag     (uint32 mask) const { return (m_flags & mask) != 0; }
  /* size requisition and allocation */
  virtual void                size_request      (Requisition &requisition) = 0;
  virtual void                size_allocate     (Allocation area) = 0;
  /* signal methods */
  virtual bool                match_interface   (InterfaceMatch &imatch, const String &ident);
  virtual void                do_invalidate     () = 0;
  virtual void                do_changed        () = 0;
  /* misc */
  virtual                     ~Item             ();
  virtual void                style             (Style  *st);
  virtual void                finalize          ();
  virtual void                hierarchy_changed (Item *old_toplevel);
  void                        anchored          (bool b) { set_flag (ANCHORED, b); }
public:
  explicit                    Item              ();
  bool                        anchored          () const { return test_flags (ANCHORED); }
  bool                        visible           () const { return test_flags (VISIBLE) && !test_flags (HIDDEN_CHILD); }
  void                        visible           (bool b) { set_flag (VISIBLE, b); }
  bool                        sensitive         () const { return test_flags (SENSITIVE | PARENT_SENSITIVE); }
  virtual void                sensitive         (bool b);
  bool                        insensitive       () const { return !sensitive(); }
  void                        insensitive       (bool b) { sensitive (!b); }
  bool                        prelight          () const { return test_flags (PRELIGHT); }
  virtual void                prelight          (bool b);
  bool                        branch_prelight   () const;
  bool                        impressed         () const { return test_flags (IMPRESSED); }
  virtual void                impressed         (bool b);
  bool                        branch_impressed  () const;
  bool                        has_focus         () const { return test_flags (HAS_FOCUS); }
  bool                        grab_focus        () const;
  bool                        has_default       () const { return test_flags (HAS_DEFAULT); }
  bool                        grab_default      () const;
  bool                        hexpand           () const { return test_any_flag (HEXPAND | HSPREAD | HSPREAD_CONTAINER); }
  void                        hexpand           (bool b) { set_flag (HEXPAND, b); }
  bool                        vexpand           () const { return test_any_flag (VEXPAND | VSPREAD | VSPREAD_CONTAINER); }
  void                        vexpand           (bool b) { set_flag (VEXPAND, b); }
  bool                        hspread           () const { return test_any_flag (HSPREAD | HSPREAD_CONTAINER); }
  void                        hspread           (bool b) { set_flag (HSPREAD, b); }
  bool                        vspread           () const { return test_any_flag (VSPREAD | VSPREAD_CONTAINER); }
  void                        vspread           (bool b) { set_flag (VSPREAD, b); }
  bool                        drawable          () const { return visible() && test_flags (POSITIVE_ALLOCATION); }
  bool                        debug             () const { return test_flags (DEBUG); }
  void                        debug             (bool f) { set_flag (DEBUG, f); }
  virtual String              name              () const = 0;
  virtual void                name              (const String &str) = 0;
  /* override requisition */
  float                       width             () const;
  void                        width             (float w);
  float                       height            () const;
  void                        height            (float h);
  /* properties */
  void                        set_property      (const String    &property_name,
                                                 const String    &value,
                                                 const nothrow_t &nt = dothrow);
  String                      get_property      (const String    &property_name);
  Property*                   lookup_property   (const String    &property_name);
  virtual const PropertyList& list_properties   ();
  /* commands */
  bool                        exec_command      (const String    &command_call_string,
                                                 const nothrow_t &nt = dothrow);
  Command*                    lookup_command    (const String    &command_name);
  virtual const CommandList&  list_commands     ();
  /* parents */
  virtual void                set_parent        (Item *parent);
  Item*                       parent            () const { return m_parent; }
  Container*                  parent_container  () const;
  bool                        has_ancestor      (const Item &ancestor) const;
  Root*                       root              ();
  /* invalidation / changes */
  void                        invalidate        ();
  void                        changed           ();
  virtual void                expose            (const Allocation &area) = 0;
  void                        expose            ()       { expose (allocation()); }
  /* public signals */
  SignalFinalize<Item>            sig_finalize;
  Signal<Item, void ()>           sig_changed;
  Signal<Item, void ()>           sig_invalidate;
  Signal<Item, void (Item *oldt)> sig_hierarchy_changed;
public:
  /* event handling */
  bool                        process_event     (Event     &event);
  virtual bool                point             (double     x,  /* global coordinate system */
                                                 double     y,
                                                 Affine     affine) = 0;
  /* public size accessors */
  virtual const Requisition&  size_request      () = 0;                       /* re-request size */
  const Requisition&          requisition       () { return size_request(); } /* cached requisition */
  virtual void                set_allocation    (const Allocation &area) = 0; /* assign new allocation */
  virtual const Allocation&   allocation        () = 0;                       /* current allocation */
  /* display */
  virtual void                render            (Display        &display) = 0;
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
    InterfaceType<Type> interface_type;
    match_parent_interface (interface_type, ident);
    return interface_type.result (&nt == &dothrow);
  }
  template<typename Type>
  typename
  InterfaceType<Type>::Result parent_interface  (const std::nothrow_t &nt) const { return parent_interface<Type> (String(), nt); }
private:
  bool                  match_parent_interface  (InterfaceMatch &imatch, const String &ident) const;
};

} // Rapicorn

#endif  /* __RAPICORN_ITEM_HH_ */
