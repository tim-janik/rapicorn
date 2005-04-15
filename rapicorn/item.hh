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
#include <rapicorn/properties.hh>
#include <rapicorn/appearance.hh>

namespace Rapicorn {

/* --- Item structures and forward decls --- */
struct Requisition {
  double width, height;
  Requisition () : width (0), height (0) {}
};
struct Allocation {
  int x, y, width, height;
  Allocation() : x (0), y (0), width (0), height (0) {}
  Allocation (int cx, int cy, int cwidth, int cheight) : x (cx), y (cy), width (cwidth), height (cheight) {}
  bool operator== (const Allocation &other) { return other.x == x && other.y == y && other.width == width && other.height == height; }
  bool operator!= (const Allocation &other) { return !operator== (other); }
};
class Root;
class Container;

/* --- Item --- */
class Item : public virtual Convertible, public virtual DataListContainer, public virtual ReferenceCountImpl {
  /*Copy*/                      Item      (const Item&);
  Item&                         operator= (const Item&);
  Item                         *m_parent;       /* interface-inlined for fast read-out */
  uint32                        m_flags;        /* interface-inlined for fast read-out */
  Style                        *m_style;
  friend class Container;
  void                          propagate_flags ();
  void                          propagate_style ();
protected:
  /* flag handling */
  bool                          change_flags_silently (uint32 mask, bool on);
  enum {
    VISIBLE                     = 1 <<  0,
    HIDDEN_CHILD                = 1 <<  1,
    SENSITIVE                   = 1 <<  2,
    PARENT_SENSITIVE            = 1 <<  3,
    PRELIGHT                    = 1 <<  4,
    IMPRESSED                   = 1 <<  5,
    HAS_FOCUS                   = 1 <<  6,
    HAS_DEFAULT                 = 1 <<  7,
    /* REQUEST_DEFAULT          = 1 <<  8, */
    INVALID_REQUISITION         = 1 << 10,
    INVALID_ALLOCATION          = 1 << 11,
    EXPOSE_ON_CHANGE            = 1 << 12,
    INVALIDATE_ON_CHANGE        = 1 << 13,
    HEXPAND                     = 1 << 14,
    VEXPAND                     = 1 << 15,
    HSPREAD                     = 1 << 16,
    VSPREAD                     = 1 << 17,
    HSPREAD_CONTAINER           = 1 << 18,
    VSPREAD_CONTAINER           = 1 << 19,
    POSITIVE_ALLOCATION         = 1 << 20,
    DEBUG                       = 1 << 21,
    LAST_FLAG                   = 1 << 22
  };
  virtual void                  set_flag        (uint32 flag, bool on = true);
  void                          unset_flag      (uint32 flag) { set_flag (flag, false); }
  bool                          test_flags      (uint32 mask) const { return (m_flags & mask) == mask; }
  bool                          test_any_flag   (uint32 mask) const { return (m_flags & mask) != 0; }
  /* size requisition and allocation */
  virtual void                  size_request    (Requisition &requisition) = 0;
  virtual void                  size_allocate   (Allocation area) = 0;
  /* signal methods */
  virtual bool                  match_interface (InterfaceMatch &imatch, const String &ident);
  virtual void                  do_invalidate   () = 0;
  virtual void                  do_changed      () = 0;
  virtual bool                  do_event        (const Event &event) = 0;
  /* misc */
  virtual void                  style           (Style  *st);
  virtual void                  finalize        ();
  virtual                       ~Item           ();
public:
  explicit                      Item            ();
  bool                          visible         () const { return test_flags (VISIBLE) && !test_flags (HIDDEN_CHILD); }
  void                          visible         (bool b) { set_flag (VISIBLE, b); }
  bool                          sensitive       () const { return test_flags (SENSITIVE | PARENT_SENSITIVE); }
  virtual void                  sensitive       (bool b);
  bool                          insensitive     () const { return !sensitive(); }
  void                          insensitive     (bool b) { sensitive (!b); }
  bool                          prelight        () const { return test_flags (PRELIGHT); }
  virtual void                  prelight        (bool b);
  bool                          branch_prelight () const;
  bool                          impressed       () const { return test_flags (IMPRESSED); }
  virtual void                  impressed       (bool b);
  bool                          branch_impressed() const;
  bool                          has_focus       () const { return test_flags (HAS_FOCUS); }
  bool                          grab_focus      () const;
  bool                          has_default     () const { return test_flags (HAS_DEFAULT); }
  bool                          grab_default    () const;
  bool                          hexpand         () const { return test_any_flag (HEXPAND | HSPREAD | HSPREAD_CONTAINER); }
  void                          hexpand         (bool b) { set_flag (HEXPAND, b); }
  bool                          vexpand         () const { return test_any_flag (VEXPAND | VSPREAD | VSPREAD_CONTAINER); }
  void                          vexpand         (bool b) { set_flag (VEXPAND, b); }
  bool                          hspread         () const { return test_any_flag (HSPREAD | HSPREAD_CONTAINER); }
  void                          hspread         (bool b) { set_flag (HSPREAD, b); }
  bool                          vspread         () const { return test_any_flag (VSPREAD | VSPREAD_CONTAINER); }
  void                          vspread         (bool b) { set_flag (VSPREAD, b); }
  bool                          drawable        () const { return visible() && test_flags (POSITIVE_ALLOCATION); }
  bool                          debug           () const { return test_flags (DEBUG); }
  void                          debug           (bool f) { set_flag (DEBUG, f); }
  virtual String                name            () const = 0;
  virtual void                  name            (const String &str) = 0;
  /* properties */
  void                          set_property    (const String    &property_name,
                                                 const String    &value,
                                                 const nothrow_t &nt = dothrow);
  String                        get_property    (const String    &property_name);
  Property*                     lookup_property (const String    &property_name);
  virtual const PropertyList&   list_properties ();
  /* parents */
  virtual void                  set_parent      (Item *parent);
  Item*                         parent          () const { return m_parent; }
  Container*                    parent_container() const;
  bool                          has_ancestor    (const Item &ancestor);
  Root*                         root            ();
  /* invalidation / changes */
  void                          invalidate      ();
  void                          changed         ();
  virtual void                  expose          (const Allocation &area) = 0;
  void                          expose          ()       { expose (allocation()); }
  /* public signals */
  Signal<Item,
         bool (const Event&),
         CollectorWhile0<bool> >  sig_event;
  Signal<Item, void ()>           sig_finalize;
  Signal<Item, void ()>           sig_changed;
  Signal<Item, void ()>           sig_invalidate;
  /* event handling */
  bool                          handle_event    (Event&);
  virtual bool                  point           (double     x,  /* global coordinate system */
                                                 double     y,
                                                 Affine     affine) = 0;
  /* public size accessors */
  virtual const Requisition&    size_request    () = 0;                       /* re-request size */
  const Requisition&            requisition     () { return size_request(); } /* cached requisition */
  virtual void                  set_allocation  (const Allocation &area) = 0; /* assign new allocation */
  virtual const Allocation&     allocation      () = 0;                       /* current allocation */
  /* display */
  virtual void                  render          (Plane        &plane,
                                                 Affine        affine) = 0;
  /* styles / appearance */
  StateType             state                   () const;
  Style*                style                   () { return m_style; }
  Color                 foreground              () { return style()->color (state(), COLOR_FOREGROUND); }
  Color                 background              () { return style()->color (state(), COLOR_BACKGROUND); }
  Color                 selected_foreground     () { return style()->color (state(), COLOR_SELECTED_FOREGROUND); }
  Color                 selected_background     () { return style()->color (state(), COLOR_SELECTED_BACKGROUND); }
  Color                 focus_color             () { return style()->color (state(), COLOR_FOCUS); }
  Color                 default_color           () { return style()->color (state(), COLOR_DEFAULT); }
  Color                 light_glint             () { return style()->color (state(), COLOR_LIGHT_GLINT); }
  Color                 light_shadow            () { return style()->color (state(), COLOR_LIGHT_SHADOW); }
  Color                 dark_glint              () { return style()->color (state(), COLOR_DARK_GLINT); }
  Color                 dark_shadow             () { return style()->color (state(), COLOR_DARK_SHADOW); }
  Color                 white                   () { return style()->color (state(), COLOR_WHITE); }
  Color                 black                   () { return style()->color (state(), COLOR_BLACK); }
  Color                 red                     () { return style()->color (state(), COLOR_RED); }
  Color                 yellow                  () { return style()->color (state(), COLOR_YELLOW); }
  Color                 green                   () { return style()->color (state(), COLOR_GREEN); }
  Color                 cyan                    () { return style()->color (state(), COLOR_CYAN); }
  Color                 blue                    () { return style()->color (state(), COLOR_BLUE); }
  Color                 magenta                 () { return style()->color (state(), COLOR_MAGENTA); }
};

} // Rapicorn

#endif  /* __RAPICORN_ITEM_HH_ */
