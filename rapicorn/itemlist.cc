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
#include "itemlist.hh"
#include "containerimpl.hh"
#include "table.hh"

namespace Rapicorn {

const PropertyList&
ItemList::list_properties()
{
  static Property *properties[] = {
    MakeProperty (ItemList, browse,     _("Browse Mode"), _("Browse selection mode"), "rw"),
  };
  static const PropertyList property_list (properties, Container::list_properties());
  return property_list;
}

struct Array {
  int val;
  Array (int i) : val (i) {}
  int aval() { return val; }
};

struct Model7 : public virtual ReferenceCountImpl {
  uint64        count   (void)       { return 35; }
  Array         get     (uint64 nth) { return Array (nth); }
};

class ItemListImpl : public virtual SingleContainerImpl, public virtual ItemList {
  Model7       *m_model;
  Table        *m_table;
  bool          need_heights, need_layout;
  vector<uint>  row_heights;
  vector<Item*> cached_items;
  bool          m_browse;
  uint64        first_item_row;
  vector<Item*> row_items;
  uint64        first_height_row;
public:
  ItemListImpl() :
    m_model (NULL), m_table (NULL),
    need_heights (true), need_layout (true),
    m_browse (true),
    first_item_row (0), first_height_row (0)
  {}
  ~ItemListImpl()
  {
    Model7 *oldmodel = m_model;
    m_model = NULL;
    if (oldmodel)
      unref (oldmodel);
    if (m_table->parent())
      {
        this->remove (*m_table);
        unref (m_table);
        m_table = NULL;
      }
  }
  virtual void
  constructed ()
  {
    if (!m_table)
      {
        m_table = Factory::create_item ("Table").interface<Table*>();
        ref_sink (m_table);
        this->add (m_table);
      }
    if (!m_model)
      model (new Model7());
  }
  virtual Model7* model         () const        { return m_model; }
  virtual void
  model (Model7 *model)
  {
    Model7 *oldmodel = m_model;
    m_model = model;
    if (m_model)
      ref_sink (m_model);
    if (oldmodel)
      unref (oldmodel);
    invalidate_model (true, true);
  }
  void
  invalidate_model (bool invalidate_heights,
                    bool invalidate_widgets)
  {
    if (invalidate_heights)
      {
        need_heights = true;
        row_heights.resize (0);
      }
    if (invalidate_widgets)
      {
        ChildWalker cw = m_table->local_children();
        while (cw.has_next())
          {
            Item &child = *(cw++);
            if (child.visible())
              {
                child.visible (false);
                cached_items.push_back (&child);
              }
          }
      }
    invalidate();
  }
  void
  set_child (Item        &child,
             uint64       row,
             const Array &value)
  {
    child.set_property ("markup_text", string_printf ("%llu", row));
  }
  Item*
  create_child (uint64       row,
                const Array &value)
  {
    Item *child = NULL;
    if (cached_items.size())
      {
        child = cached_items.back();
        cached_items.pop_back();
      }
    if (!child)
      {
        child = &Factory::create_item ("Label");
        m_table->add (*child);
      }
    Packer packer = m_table->child_packer (*child);
    packer.set_property ("bottom_attach", string_printf ("%llu", row));
    child->visible (true);
    return child;
  }
  void
  update()
  {
    if (!m_model)
      return;
    if (need_heights)
      {
        need_heights = false;
        while (row_items.size())
          {
            Item *child = row_items.back();
            row_items.pop_back();
            child->visible (false);
            cached_items.push_back (child);
          }
        first_height_row = 0;
        row_heights.resize (m_model->count());
        for (uint64 r = 0; r < row_heights.size(); r++)
          {
            Item *child = create_child (0, m_model->get (r));
            set_child (*child, r, m_model->get (r));
            Requisition req = child->requisition();
            printout ("childreq: %f x %f\n", req.width, req.height);
            row_heights[r] = req.height;
            child->visible (false);
            cached_items.push_back (child);
          }
      }
    if (need_layout)
      {
        need_layout = false;
        for (uint64 r = 0; r < row_heights.size(); r++)
          {
            Item *child = create_child (r, m_model->get (r));
            set_child (*child, r, m_model->get (r));
          }
      }
  }
  virtual Container::Packer
  create_packer (Item &item)
  {
    return void_packer(); /* no child properties */
  }
  virtual void
  size_request (Requisition &requisition)
  {
    bool chspread = false, cvspread = false;
    update();
    if (has_allocatable_child())
      {
        Item &child = get_child();
        requisition = child.size_request ();
        if (0)
          {
            chspread = child.hspread();
            cvspread = child.vspread();
          }
      }
    printout ("havechild: %d %f\n", has_allocatable_child(), requisition.height);
    set_flag (HSPREAD_CONTAINER, chspread);
    set_flag (VSPREAD_CONTAINER, cvspread);
  }
  virtual void
  size_allocate (Allocation area)
  {
    allocation (area);
    if (has_allocatable_child())
      {
        Item &child = get_child();
        child.set_allocation (area);
        printout ("alloch: %f\n", area.height);
      }
  }
  virtual bool  browse         () const  { return m_browse; }
  virtual void  browse         (bool b)  { m_browse = b; invalidate(); }
};
static const ItemFactory<ItemListImpl> item_list_factory ("Rapicorn::Factory::ItemList");

} // Rapicorn
