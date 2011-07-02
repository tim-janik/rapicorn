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
struct Container : public virtual ItemImpl {
  friend              class ItemImpl;
  friend              class Window;
  void                uncross_descendant(ItemImpl          &descendant);
  void                item_cross_link   (ItemImpl           &owner,
                                         ItemImpl           &link,
                                         const ItemSlot &uncross);
  void                item_cross_unlink (ItemImpl           &owner,
                                         ItemImpl           &link,
                                         const ItemSlot &uncross);
  void                item_uncross_links(ItemImpl           &owner,
                                         ItemImpl           &link);
protected:
  virtual            ~Container         ();
  virtual void        add_child         (ItemImpl           &item) = 0;
  virtual void        repack_child      (ItemImpl           &item,
                                         const PackInfo &orig,
                                         const PackInfo &pnew);
  virtual void        remove_child      (ItemImpl           &item) = 0;
  virtual void        unparent_child    (ItemImpl           &item);
  virtual void        dispose_item      (ItemImpl           &item);
  virtual void        hierarchy_changed (ItemImpl           *old_toplevel);
  virtual bool        move_focus        (FocusDirType    fdir);
  void                expose_enclosure  (); /* expose without children */
  virtual void        set_focus_child   (ItemImpl           *item);
  virtual void        dump_test_data    (TestStream     &tstream);
public:
  ItemImpl*               get_focus_child   () const;
  typedef Walker<ItemImpl>  ChildWalker;
  void                  child_container (Container      *child_container);
  Container&            child_container ();
  virtual ChildWalker   local_children  () const = 0;
  virtual bool          has_children    () = 0;
  void                  remove          (ItemImpl           &item);
  void                  remove          (ItemImpl           *item)  { RAPICORN_CHECK (item != NULL); remove (*item); }
  void                  add             (ItemImpl                   &item);
  void                  add             (ItemImpl                   *item);
  virtual Affine        child_affine    (const ItemImpl             &item); /* container => item affine */
  virtual
  const PropertyList&   list_properties (); /* essentially chaining to ItemImpl:: */
  const CommandList&    list_commands   (); /* essentially chaining to ItemImpl:: */
  virtual void          point_children  (Point                   p, /* item coordinates relative */
                                         std::vector<ItemImpl*>     &stack);
  void         viewp0rt_point_children  (Point                   p, /* viewp0rt coordinates relative */
                                         std::vector<ItemImpl*>     &stack);
  virtual void          render          (Display                &display);
  void                  debug_tree      (String indent = String());
};

} // Rapicorn

#endif  /* __RAPICORN_CONTAINER_HH__ */
