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
#include "listareaimpl.hh"
#include "sizegroup.hh"

//#define IFDEBUG(...)      do { /*__VA_ARGS__*/ } while (0)
#define IFDEBUG(...)      __VA_ARGS__

namespace Rapicorn {

const PropertyList&
ItemList::list_properties()
{
  static Property *properties[] = {
    MakeProperty (ItemList, browse, _("Browse Mode"), _("Browse selection mode"), "rw"),
    MakeProperty (ItemList, model,  _("Model URL"), _("Resource locator for 1D list model"), "rw:M1"),
  };
  static const PropertyList property_list (properties, Container::list_properties());
  return property_list;
}

static uint
n_columns_from_type (const Type &type)
{
  switch (type.storage())
    {
    case NUM:
    case REAL:
    case STRING:
    case ARRAY:
    case OBJECT:
    case CHOICE:
    case TYPE_REFERENCE:
    case SEQUENCE:
      return 1;
    case RECORD:
      return 1; // FIXME: type.n_fields();
    }
  return 0;
}

ItemListImpl::ItemListImpl() :
  m_table (NULL), m_model (NULL),
  m_hadjustment (NULL), m_vadjustment (NULL),
  m_n_cols (0), m_browse (true)
{}

ItemListImpl::~ItemListImpl()
{
  /* destroy table */
  if (m_table->parent())
    {
      this->remove (*m_table);
      unref (m_table);
      m_table = NULL;
    }
  /* remove model */
  Model1 *oldmodel = m_model;
  m_model = NULL;
  if (oldmodel)
    unref (oldmodel);
  /* purge row map */
  RowMap rc; // empty
  m_row_map.swap (rc);
  for (RowMap::iterator ri = rc.begin(); ri != rc.end(); ri++)
    cache_row (ri->second);
  /* destroy row cache */
  while (m_row_cache.size())
    {
      ListRow *lr = m_row_cache.back();
      m_row_cache.pop_back();
      for (uint i = 0; i < lr->cols.size(); i++)
        unref (lr->cols[i]);
      unref (lr->hbox);
      delete lr;
    }
  /* release size groups */
  while (m_size_groups.size())
    {
      SizeGroup *sg = m_size_groups.back();
      m_size_groups.pop_back();
      unref (sg);
    }
}

void
ItemListImpl::constructed ()
{
  if (!m_table)
    {
      m_table = Factory::create_item ("Table").interface<Table*>();
      ref_sink (m_table);
      this->add (m_table);
    }
  if (!m_model)
    {
      Store1 &store = *Store1::create_memory_store (Type::lookup ("String"));
      for (uint i = 0; i < 20; i++)
        {
          Array row;
          String s;
          if (i == 10)
            s = string_printf ("* %u SMALL ITEM (watch scroll direction)", i);
          else
            s = string_printf ("|<br/>| <br/>| %u<br/>|<br/>|", i);
          row[0] = s;
          for (uint j = 1; j < 4; j++)
            row[j] = string_printf ("%u-%u", i + 1, j + 1);
          store.insert (-1, row);
        }
      model (store.object_url());
      unref (ref_sink (store));
    }
}

void
ItemListImpl::hierarchy_changed (Item *old_toplevel)
{
  Item::hierarchy_changed (old_toplevel);
  if (anchored())
    queue_visual_update();
}

Adjustment&
ItemListImpl::hadjustment () const
{
  if (!m_hadjustment)
    m_hadjustment = Adjustment::create (0, 0, 1, 0.01, 0.2);
  return *m_hadjustment;
}

Adjustment&
ItemListImpl::vadjustment () const
{
  if (!m_vadjustment)
    {
      m_vadjustment = Adjustment::create (0, 0, 1, 0.01, 0.2);
      m_vadjustment->sig_value_changed += slot ((Item&) *this, &Item::queue_visual_update);
    }
  return *m_vadjustment;
}

Adjustment*
ItemListImpl::get_adjustment (AdjustmentSourceType adj_source,
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

void
ItemListImpl::model (const String &modelurl)
{
  Deletable *obj = from_object_url (modelurl);
  Model1 *model = dynamic_cast<Model1*> (obj);
  Model1 *oldmodel = m_model;
  m_model = model;
  if (m_model)
    ref_sink (m_model);
  if (oldmodel)
    unref (oldmodel);
  if (!m_model)
    m_n_cols = 0;
  else
    m_n_cols = n_columns_from_type (m_model->type());
  invalidate_model (true, true);
}

String
ItemListImpl::model () const
{
  return m_model ? m_model->object_url() : "";
}

void
ItemListImpl::invalidate_model (bool invalidate_heights,
                                bool invalidate_widgets)
{
  invalidate();
}

void
ItemListImpl::visual_update ()
{
  layout_list();
}

void
ItemListImpl::size_request (Requisition &requisition)
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

void
ItemListImpl::size_allocate (Allocation area)
{
  bool need_resize = allocation() != area;
  allocation (area);
  if (has_allocatable_child() && need_resize)
    layout_list();
}

void
ItemListImpl::cache_row (ListRow *lr)
{
  m_row_cache.push_back (lr);
  lr->hbox->visible (false);
}

static uint dbg_cached = 0, dbg_refilled = 0, dbg_created = 0;

ListRow*
ItemListImpl::create_row (uint64 nthrow)
{
  Array row = m_model->get (nthrow);
  ListRow *lr = new ListRow();
  for (uint i = 0; i < row.count(); i++)
    {
      Item *item = ref_sink (&Factory::create_item ("Label"));
      lr->cols.push_back (item);
    }
  IFDEBUG (dbg_created++);
  lr->hbox = &ref_sink (&Factory::create_item ("HBox"))->interface<HBox>();
  lr->hbox->spacing (m_table->col_spacing());

  while (m_size_groups.size() < lr->cols.size())
    m_size_groups.push_back (ref_sink (SizeGroup::create_hgroup()));
  for (uint i = 0; i < lr->cols.size(); i++)
    m_size_groups[i]->add_item (*lr->cols[i]);

  for (uint i = 0; i < lr->cols.size(); i++)
    lr->hbox->add (lr->cols[i]);
  m_table->add (lr->hbox);
  return lr;
}

void
ItemListImpl::fill_row (ListRow *lr,
                        uint64   nthrow)
{
  Array row = m_model->get (nthrow);
  for (uint i = 0; i < lr->cols.size(); i++)
    lr->cols[i]->set_property ("markup_text", i < row.count() ? row[i].string() : "");
}

ListRow*
ItemListImpl::fetch_row (uint64 row)
{
  bool filled = false;
  ListRow *lr;
  RowMap::iterator ri = m_row_map.find (row);
  if (ri != m_row_map.end())
    {
      lr = ri->second;
      m_row_map.erase (ri);
      filled = true;
      IFDEBUG (dbg_cached++);
    }
  else if (m_row_cache.size())
    {
      lr = m_row_cache.back();
      m_row_cache.pop_back();
      IFDEBUG (dbg_refilled++);
    }
  else /* create row */
    lr = create_row (row);
  if (!filled)
    fill_row (lr, row);
  lr->hbox->visible (true);
  return lr;
}

void
ItemListImpl::position_row (ListRow *lr,
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
  uint64 tablerow = (visible_slot + 1) * 2;
  lr->hbox->vposition (tablerow);
  lr->hbox->vspan (1);
}

uint64
ItemListImpl::measure_row (ListRow *lr,
                           uint64  *allocation_offset)
{
  if (allocation_offset)
    {
      Allocation carea = lr->hbox->allocation();
      *allocation_offset = carea.y;
      return carea.height;
    }
  else
    {
      Requisition requisition = lr->hbox->requisition();
      return requisition.height;
    }
}

uint64
ItemListImpl::get_scroll_item (double *row_offsetp,
                               double *pixel_offsetp)
{
  /* scroll position interpretation:
   * the current slider position is interpreted as a fractional pointer
   * into the interval [0,count[. so the integer part of the scroll
   * position will always point at one particular item and the fractional
   * part is interpreted as an offset into the item's row.
   */
  const double norm_value = m_vadjustment->nvalue(); /* 0..1 scroll position */
  const double scroll_value = norm_value * m_model->count();
  const int64 scroll_item = MIN (m_model->count() - 1, ifloor (scroll_value));
  *row_offsetp = MIN (1.0, scroll_value - scroll_item);
  *pixel_offsetp = norm_value;
  return scroll_item;
}

bool
ItemListImpl::need_pixel_scrolling ()
{
  /* Scrolling for large models works by interpreting the scroll adjustment
   * values as [row_index.row_fraction]. From this, a scroll position is
   * interpolated so that the top of the first row and the bottom of the
   * last row are aligned with top and bottom of the list view
   * respectively.
   * However, for models small enough, so that more scrollbar pixels per
   * row are available than the size of the smallest row, a "backward"
   * scrolling effect can be observed, resulting from the list top/bottom
   * alignment interpolation values progressing faster than row_fraction
   * increments.
   * To counteract this effect and for an overall smoother scrolling
   * behavior, we use pixel scrolling for smaller models, which requires
   * all rows, visible and invisible rows to be layed out.
   * In this context, this method is used to draw the distinction between
   * "large" and "small" models.
   */
  double scroll_pixels_per_row = allocation().height / m_model->count();
  double minimum_pixels_per_row = 1 + 3 + 1; /* outer border + minimum font size + inner border */
  return scroll_pixels_per_row > minimum_pixels_per_row;
}

void
ItemListImpl::layout_list ()
{
  double area_height = MAX (allocation().height, 1);
  if (!m_model || !m_table || m_model->count() < 1)
    return;
  const bool pixel_scrolling = need_pixel_scrolling();
  double row_offset, pixel_offset; /* 0..1 */
  const uint64 current_item = get_scroll_item (&row_offset, &pixel_offset);
  RowMap rc;
  uint64 r, rcmin = current_item, rcmax = current_item;
  double height_before = area_height, height_after = area_height;
  IFDEBUG (dbg_cached = dbg_refilled = dbg_created = 0);
  /* deactivate size-groups to avoid excessive resizes upon measure_row() */
  for (uint i = 0; i < m_size_groups.size(); i++)
    m_size_groups[i]->active (false);
  /* fill rows from scroll_item towards list end */
  double accu_height = 0;
  for (r = current_item; r < uint64 (m_model->count()) && (accu_height <= height_before || pixel_scrolling); r++)
    {
      ListRow *lr = fetch_row (r);
      rc[r] = lr;
      if (r != current_item)    // current may be partially offscreen
        accu_height += measure_row (lr);
      rcmax = MAX (rcmax, r);
    }
  double avheight = MAX (7, accu_height / MAX (r - current_item, 1));
  /* fill rows from list start towards scroll_item */
  uint64 rowsteps = CLAMP (iround (height_before / avheight), 1, 17);
  uint64 first = current_item;
  accu_height = 0;
  while (first > 0 && (accu_height <= height_after || pixel_scrolling))
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
  /* activate size-groups to properly layout children */
  for (uint i = 0; i < m_size_groups.size(); i++)
    m_size_groups[i]->active (true);
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
  if (current_item < uint64 (m_model->count()) && has_allocatable_child())
    {
      const Allocation area = allocation();
      Item &child = get_child();
      Requisition requisition = child.size_request();
      int64 pixel_diff;
      if (requisition.height <= area.height)
        pixel_diff = area.height - requisition.height;
      else if (pixel_scrolling)
        pixel_diff = - (1.0 - pixel_offset) * (requisition.height - area.height);
      else /* fractional row scrolling */
        {
          ListRow *lr = m_row_map[current_item];
          uint64 row_y, row_height = measure_row (lr, &row_y); // FIXME: add border
          int64 row_pixel = row_y - area.y + row_height * (1.0 - row_offset);
          int64 list_pixel = area.height * (1.0 - pixel_offset);
          pixel_diff = list_pixel - row_pixel;
        }
      /* shift rows */
      Allocation carea = area;
      carea.width = requisition.width;
      carea.height = requisition.height;
      carea.y += pixel_diff;
      IFDEBUG (printout ("List: cached=%u refilled=%u created=%u current=%3lld row_offset=%f pixel_offset=%f diff=%lld ps=%d\n",
                         dbg_cached, dbg_refilled, dbg_created,
                         current_item, row_offset, pixel_offset,
                         pixel_diff, pixel_scrolling));
      child.set_allocation (carea);
    }
}

static const ItemFactory<ItemListImpl> item_list_factory ("Rapicorn::Factory::ItemList");

} // Rapicorn
