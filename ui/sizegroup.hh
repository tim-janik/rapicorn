/* Rapicorn
 * Copyright (C) 2008 Tim Janik
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
#ifndef __RAPICORN_SIZE_GROUP_HH__
#define __RAPICORN_SIZE_GROUP_HH__

#include <ui/item.hh>

namespace Rapicorn {

/* --- SizeGroup --- */
class SizeGroup : public virtual ReferenceCountImpl {
  friend            class ClassDoctor;
  friend            class Item;
protected:
  void                size_request_items (const vector<Item*> items,
                                          Requisition        &max_requisition);
  virtual Requisition group_requisition  () = 0;
  static void         delete_item        (Item &item);
  static void         invalidate_item    (Item &item);
  static Requisition  item_requisition   (Item &item);
public:
  static SizeGroup*   create_hgroup      ();
  static SizeGroup*   create_vgroup      ();
  virtual bool        active             () const = 0;
  virtual void        active             (bool isactive) = 0;
  virtual void        add_item           (Item &item) = 0;
  virtual void        remove_item        (Item &item) = 0;
};

} // Rapicorn

#endif  /* __RAPICORN_SIZE_GROUP_HH__ */
