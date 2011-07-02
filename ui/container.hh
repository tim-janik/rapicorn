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
#ifndef __RAPICORN_CONTAINER_HH__
#define __RAPICORN_CONTAINER_HH__

#include <ui/item.hh>

namespace Rapicorn {

/* --- Container --- */
struct Container : public virtual Item {
  friend              class Item;
  friend              class Window;
  void                uncross_descendant(Item          &descendant);
  void                item_cross_link   (Item           &owner,
                                         Item           &link,
                                         const ItemSlot &uncross);
  void                item_cross_unlink (Item           &owner,
                                         Item           &link,
                                         const ItemSlot &uncross);
  void                item_uncross_links(Item           &owner,
                                         Item           &link);
protected:
  virtual            ~Container         ();
  virtual void        add_child         (Item           &item) = 0;
  virtual void        repack_child      (Item           &item,
                                         const PackInfo &orig,
                                         const PackInfo &pnew);
  virtual void        remove_child      (Item           &item) = 0;
  virtual void        unparent_child    (Item           &item);
  virtual void        dispose_item      (Item           &item);
  virtual void        hierarchy_changed (Item           *old_toplevel);
  virtual bool        move_focus        (FocusDirType    fdir);
  void                expose_enclosure  (); /* expose without children */
  virtual void        set_focus_child   (Item           *item);
  virtual void        dump_test_data    (TestStream     &tstream);
public:
  Item*               get_focus_child   () const;
  typedef Walker<Item>  ChildWalker;
  void                  child_container (Container      *child_container);
  Container&            child_container ();
  virtual ChildWalker   local_children  () const = 0;
  virtual bool          has_children    () = 0;
  void                  remove          (Item           &item);
  void                  remove          (Item           *item)  { RAPICORN_CHECK (item != NULL); remove (*item); }
  void                  add             (Item                   &item);
  void                  add             (Item                   *item);
  virtual Affine        child_affine    (const Item             &item); /* container => item affine */
  virtual
  const PropertyList&   list_properties (); /* essentially chaining to Item:: */
  const CommandList&    list_commands   (); /* essentially chaining to Item:: */
  virtual void          point_children  (Point                   p, /* item coordinates relative */
                                         std::vector<Item*>     &stack);
  void         viewp0rt_point_children  (Point                   p, /* viewp0rt coordinates relative */
                                         std::vector<Item*>     &stack);
  virtual void          render          (Display                &display);
  void                  debug_tree      (String indent = String());
};

} // Rapicorn

#endif  /* __RAPICORN_CONTAINER_HH__ */
