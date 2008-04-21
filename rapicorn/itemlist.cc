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
#include "adjustment.hh"
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
  uint64        count   (void)       { return 20; }
  Array         get     (uint64 nth) { return Array (nth); }
};

struct ListRow {
  Item *child;
};

class ItemListImpl : public virtual SingleContainerImpl,
                     public virtual ItemList,
                     public virtual AdjustmentSource
{
  typedef map<uint64,ListRow*> RowMap;
  Table                 *m_table;
  Model7                *m_model;
  mutable Adjustment    *m_hadjustment, *m_vadjustment;
  RowMap                 m_row_map;
  vector<ListRow*>       m_row_cache;
  bool                   m_browse;
public:
  ItemListImpl() :
    m_table (NULL), m_model (NULL),
    m_hadjustment (NULL), m_vadjustment (NULL),
    m_browse (true)
  {}
  ~ItemListImpl()
  {
    if (m_table->parent())
      {
        this->remove (*m_table);
        unref (m_table);
        m_table = NULL;
      }
    Model7 *oldmodel = m_model;
    m_model = NULL;
    if (oldmodel)
      unref (oldmodel);
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
  virtual bool  browse         () const  { return m_browse; }
  virtual void  browse         (bool b)  { m_browse = b; invalidate(); }
  virtual void
  hierarchy_changed (Item *old_toplevel)
  {
    Item::hierarchy_changed (old_toplevel);
    if (anchored())
      queue_visual_update();
  }
  Adjustment&
  hadjustment () const
  {
    if (!m_hadjustment)
      m_hadjustment = Adjustment::create (0, 0, 1, 0.01, 0.2);
    return *m_hadjustment;
  }
  Adjustment&
  vadjustment () const
  {
    if (!m_vadjustment)
      {
        m_vadjustment = Adjustment::create (0, 0, 1, 0.01, 0.2);
        m_vadjustment->sig_value_changed += slot ((Item&) *this, &Item::queue_visual_update);
      }
    return *m_vadjustment;
  }
  Adjustment*
  get_adjustment (AdjustmentSourceType adj_source,
                  const String        &name)
  {
    switch (adj_source)
      {
      case ADJUSTMENT_SOURCE_ANCESTRY_HORIZONTAL:
        return &hadjustment();
      case ADJUSTMENT_SOURCE_ANCESTRY_VERTICAL:
        return &vadjustment();
      default:
        return NULL;
      }
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
    invalidate();
  }
  virtual void
  visual_update ()
  {
    layout_list();
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
    if (has_allocatable_child())
      {
        Item &child = get_child();
        requisition = child.size_request ();
        requisition.height = 12 * 5; // FIXME: request single row height
        if (0)
          {
            chspread = child.hspread();
            cvspread = child.vspread();
          }
      }
    set_flag (HSPREAD_CONTAINER, chspread);
    set_flag (VSPREAD_CONTAINER, cvspread);
  }
  virtual void
  size_allocate (Allocation area)
  {
    bool need_resize = allocation() != area;
    allocation (area);
    if (has_allocatable_child() && need_resize)
      layout_list();
  }
  void
  cache_row (ListRow *lr)
  {
    m_row_cache.push_back (lr);
    lr->child->visible (false);
  }
  void
  fill_row (ListRow *lr,
            uint64   row)
  {
    if (row == 10)
      lr->child->set_property ("markup_text", string_printf ("* %llu SCROLL DIRECTION TEST", row));
    else
      lr->child->set_property ("markup_text", string_printf ("|<br/>| <br/>| %llu<br/>|<br/>|", row));
  }
  ListRow*
  fetch_row (uint64 row)
  {
    bool filled = false;
    ListRow *lr;
    RowMap::iterator ri = m_row_map.find (row);
    if (ri != m_row_map.end())
      {
        lr = ri->second;
        m_row_map.erase (ri);
        filled = true;
        printerr ("CACHED: %lld\n", row);
      }
    else if (m_row_cache.size())
      {
        lr = m_row_cache.back();
        m_row_cache.pop_back();
        printerr ("refill: %lld\n", row);
      }
    else /* create row */
      {
        lr = new ListRow();
        lr->child = ref_sink (&Factory::create_item ("Label"));
        m_table->add (*lr->child);
        printerr ("create: %lld\n", row);
      }
    if (!filled)
      fill_row (lr, row);
    lr->child->visible (true);
    return lr;
  }
  void
  position_row (ListRow *lr,
                uint64   visible_slot)
  {
    /* List rows increase downwards, table rows increase upwards.
     * Table layout (table-row:visible-slot:list-row):
     * *:*: dummy children
     * etc
     * 6:2: row[i]
     * 5:2: row[i]-border
     * 4:1: row[j]
     * 3:1: row[j]-border
     * 2:0: row[k]
     * 1:0: row[k]-border
     * 0:*: final row-border
     */
    Packer packer = m_table->child_packer (*lr->child);
    uint64 tablerow = (visible_slot + 1) * 2;
    packer.set_property ("top_attach", string_printf ("%llu", tablerow + 1));
    packer.set_property ("bottom_attach", string_printf ("%llu", tablerow));
  }
  uint64
  measure_row (ListRow *lr,
               uint64  *allocation_offset = NULL)
  {
    if (allocation_offset)
      {
        Allocation carea = lr->child->allocation();
        *allocation_offset = carea.y;
        return carea.height;
      }
    else
      {
        Requisition requisition = lr->child->size_request ();
        return requisition.height;
      }
  }
  uint64
  get_scroll_item (double *row_offsetp,
                   double *pixel_offsetp)
  {
    /* scroll position interpretation:
     * the current slider position is interpreted as a fractional pointer
     * into the interval [0,count[. so the integer part of the scroll
     * position will always point at one particular item and the fractional
     * part is interpreted as an offset into the row.
     */
    double norm_value = 1.0 - m_vadjustment->nvalue(); /* 0..1 scroll position */
    double scroll_value = norm_value * m_model->count();
    int64 scroll_item = MIN (m_model->count() - 1, ifloor (scroll_value));
    *row_offsetp = MIN (1.0, scroll_value - scroll_item);
    *pixel_offsetp = norm_value;
    return scroll_item;
  }
  void
  layout_list ()
  {
    double area_height = MAX (allocation().height, 1);
    if (!m_model || !m_table || m_model->count() < 1)
      return;
    double row_offset, pixel_offset; /* 0..1 */
    uint64 r, current_item = get_scroll_item (&row_offset, &pixel_offset);
    RowMap rc;
    uint64 rcmin = current_item, rcmax = current_item;
    double height_before = area_height, height_after = area_height; // FIXME: add 2 * border?
    /* fill rows from scroll_item towards list end */
    double accu_height = 0;
    printerr ("count=%lld current=%lld nvalue=%f hb=%f ha=%f cond=%d\n",
              m_model->count(), current_item, m_vadjustment->nvalue(),
              height_before, height_after,
              current_item < m_model->count() && accu_height <= height_before);
    for (r = current_item; r < m_model->count() && accu_height <= height_before; r++)
      {
        ListRow *lr = fetch_row (r);
        rc[r] = lr;
        accu_height += measure_row (lr);
        rcmax = MAX (rcmax, r);
      }
    double avheight = MAX (7, accu_height / MAX (r - current_item, 1));
    /* fill rows from list start towards scroll_item */
    uint64 rowsteps = CLAMP (iround (height_before / avheight), 1, 17);
    uint64 first = current_item;
    accu_height = 0;
    while (first > 0 && accu_height <= height_after)
      {
        uint64 last = first;
        first -= MIN (first, rowsteps);
        for (r = first; r < last; r++)
          {
            ListRow *lr = fetch_row (r);
            rc[r] = lr;
            accu_height += measure_row (lr);
            rcmin = MIN (rcmin, r);
          }
      }
    /* layout visible rows */
    if (rc.size())
      for (r = rcmin; r <= rcmax; r++)
        position_row (rc.at (r), rcmax - r);
    /* clear unused rows */
    m_row_map.swap (rc);
    for (RowMap::iterator ri = rc.begin(); ri != rc.end(); ri++)
      cache_row (ri->second);
    /* assign list row coordinates */
    if (has_allocatable_child())
      {
        Item &child = get_child();
        Requisition requisition = child.size_request();
        Allocation carea = allocation();
        carea.width = requisition.width;
        carea.height = requisition.height;
        child.set_allocation (carea);
      }
    /* align scroll item */
    if (current_item < m_model->count() && has_allocatable_child())
      {
        Allocation area = allocation();
        ListRow *lr = m_row_map[current_item];
        uint64 row_y, row_height = measure_row (lr, &row_y); // FIXME: add border
        int64 row_pixel = row_y - area.y + row_height * (1.0 - row_offset);
        int64 list_pixel = area.height * (1.0 - pixel_offset);
        int64 pixel_diff = list_pixel - row_pixel;
        /* shift rows */
        Item &child = get_child();
        Requisition requisition = child.size_request();
        Allocation carea = allocation();
        if (requisition.height <= carea.height)
          pixel_diff = carea.height - requisition.height;
        carea.width = requisition.width;
        carea.height = requisition.height;
        carea.y += pixel_diff;
        printerr ("list_pixel=%lld row_pixel=%lld pixel_diff=%lld pixel_offset=%f current=%lld row_offset=%f\n",
                  (int64) -1, (int64) -1, pixel_diff, pixel_offset, current_item, row_offset);
        child.set_allocation (carea);
      }
  }
};
static const ItemFactory<ItemListImpl> item_list_factory ("Rapicorn::Factory::ItemList");

} // Rapicorn
