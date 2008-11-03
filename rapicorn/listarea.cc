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
#include "paintcontainers.hh"

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
  m_model (NULL),
  m_hadjustment (NULL), m_vadjustment (NULL),
  m_n_cols (0), m_row_size_offset (0),
  m_browse (true), m_need_resize_scroll (false),
  m_current_row (18446744073709551615ULL),
  m_measurement_row (NULL)
{}

ItemListImpl::~ItemListImpl()
{
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
      unref (lr->rowbox);
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
  m_need_resize_scroll = true;
  m_row_size_offset = 0;
  m_row_sizes.clear();
  invalidate();
}

void
ItemListImpl::visual_update ()
{
  m_need_resize_scroll = true; // FIXME
  if (m_need_resize_scroll)
    {
      measure_rows (allocation().height, m_vadjustment->nvalue());
      resize_scroll();
    }
  for (RowMap::iterator it = m_row_map.begin(); it != m_row_map.end(); it++)
    {
      ListRow *lr = it->second;
      lr->rowbox->set_allocation (lr->area);
    }
}

void
ItemListImpl::size_request (Requisition &requisition)
{
  bool chspread = false, cvspread = false;
  measure_rows (4096, m_vadjustment->nvalue()); // FIXME: use monitor size
  // FIXME: this should only measure current row
  requisition.width = 0;
  requisition.height = -1;
  for (ChildWalker cw = local_children(); cw.has_next(); cw++)
    {
      /* size request all children */
      if (!cw->allocatable())
        continue;
      Requisition crq = cw->size_request();
      requisition.width = MAX (requisition.width, crq.width);
      chspread = cvspread = false;
    }
  if (m_model && m_model->count())
    requisition.height = lookup_row_size (ifloor (m_model->count() * m_vadjustment->nvalue())); // FIXME
  else
    requisition.height = -1;
  if (requisition.height < 0)
    requisition.height = 12 * 5; // FIXME: request single row height
  set_flag (HSPREAD_CONTAINER, chspread);
  set_flag (VSPREAD_CONTAINER, cvspread);
}

void
ItemListImpl::size_allocate (Allocation area)
{
  m_need_resize_scroll |= allocation() != area;
  allocation (area);
  if (m_need_resize_scroll)
    {
      measure_rows (area.height, m_vadjustment->nvalue());
      resize_scroll();
    }
  for (RowMap::iterator it = m_row_map.begin(); it != m_row_map.end(); it++)
    {
      ListRow *lr = it->second;
      lr->rowbox->set_allocation (lr->area);
    }
}

int64
ItemListImpl::lookup_row_size (int64 row)
{
  row -= m_row_size_offset;
  if (row < 0 || !m_model || row >= m_model->count() || row >= m_row_sizes.size())
    return -1;
  return m_row_sizes[row];
}

int64
ItemListImpl::lookup_or_measure_row (int64    row,
                                     ListRow *cached_lr)
{
  int64 pixels = lookup_row_size (row);
  if (pixels < 0)
    {
      fill_row (cached_lr, row);
      pixels = measure_row (cached_lr);
    }
  return pixels;
}

void
ItemListImpl::measure_rows (int64  maxpixels,
                            double fraction)
{
  return; // FIXME
  if (!m_model || m_model->count() < 1)
    {
      /* assign 0 sizes */
      m_row_size_offset = 0;
      m_row_sizes.clear();
      return;
    }
  /* create list row for measuring */
  if (!m_measurement_row)
    {
      m_measurement_row = create_row (0, false);
      Allocation area;
      area.x = area.y = -1;
      area.width = area.height = 1;
      m_measurement_row->rowbox->set_allocation (area);
    }
  ListRow *lr = m_measurement_row;
  SizeQueue row_sizes;
  int64 current = min (m_model->count() - 1, ifloor (fraction * m_model->count()));
  int64 row = current;
  int64 row_size_offset = current;
  /* measure current row */
  row_sizes.push_back (lookup_or_measure_row (row, lr));
  /* measure below current row */
  int64 pixelaccu = 0;
  row = current + 1;
  while (pixelaccu <= maxpixels && row < m_model->count())
    {
      int64 pixels = lookup_or_measure_row (row, lr);
      pixelaccu += pixels;
      row_sizes.push_back (pixels);
      row += 1;
    }
  /* measure above current row */
  pixelaccu = 0;
  row = current - 1;
  while (pixelaccu <= maxpixels && row >= 0)
    {
      int64 pixels = lookup_or_measure_row (row, lr);
      pixelaccu += pixels;
      row_sizes.push_front (pixels);
      row_size_offset -= 1;
      row -= 1;
    }
  /* assign new sizes */
  m_row_size_offset = row_size_offset;
  m_row_sizes.swap (row_sizes);
  /* cleanup */
  // FIXME: cache_row (lr);
  // FIXME: optimize by using std::vector as SizeQueue (construct with reverse copies)
}

int64
ItemListImpl::scroll_row_layout (ListRow *lr_current,
                                 int64 *scrollrowy,
                                 int64 *scrollrowupper,
                                 int64 *scrollrowlower,
                                 int64 *listupperp,
                                 int64 *listheightp)
{
  /* scroll position interpretation:
   * the current slider position is interpreted as a fractional pointer into the
   * interval [0,count[. so the integer part of the scroll position will always
   * point at one particular item and the fractional part is interpreted as an
   * offset into the item's row.
   * Scrolling for large models works by interpreting the scroll adjustment
   * values as [row_index.row_fraction]. From this, a scroll position is
   * interpolated so that the top of the first row and the bottom of the last
   * row are aligned with top and bottom of the list view respectively.
   * Note that list rows increase downwards, pixel coordinates increase upwards.
   */
  const double norm_value = m_vadjustment->nvalue();            // 0..1 scroll position
  const double scroll_value = norm_value * m_model->count();    // fraction into count()
  const int64 scroll_item = MIN (m_model->count() - 1, ifloor (scroll_value));
  const double scroll_fraction = MIN (1.0, scroll_value - scroll_item); // fraction into scroll_item row

  int64 rowheight; // FIXME: make const: const int64 rowheight = lookup_row_size (scroll_item);
  {
    Requisition requisition = lr_current->rowbox->size_request();
    rowheight = requisition.height;
  }

  return_val_if_fail (rowheight > 0, scroll_item);
  const int64 rowlower = rowheight * (1 - scroll_fraction);       // fractional lower row pixels
  const int64 listlower = allocation().height * (1 - norm_value); // fractional lower list pixels
  *scrollrowy = listlower - rowlower;
  *scrollrowupper = rowheight - rowlower;
  *scrollrowlower = rowlower;
  *listupperp = allocation().height - listlower;
  *listheightp = allocation().height;
  return scroll_item;
}

void
ItemListImpl::resize_scroll () // m_model->count() >= 1
{
  /* flag old rows */
  for (RowMap::iterator it = m_row_map.begin(); it != m_row_map.end(); it++)
    it->second->allocated = 0; // FIXME

  int64 current = min (m_model->count() - 1, ifloor (m_vadjustment->nvalue() * m_model->count())); // FIXME
  ListRow *lr_current = fetch_row (current);

  Allocation area = allocation();
  int64 current_y, currentupper, currentlower, listupper, listheight;
  int64 current_FIXME = scroll_row_layout (lr_current, &current_y, &currentupper, &currentlower, &listupper, &listheight);
  RowMap rmap;

  cache_row (lr_current); // FIXME

  /* allocate current row */
  int64 accu = 0;
  {
    ListRow *lr = fetch_row (current);
    lr->rowbox->requisition(); accu = currentupper; // FIXME
    rmap[current] = lr;
    lr->allocated = true; // FIXME: remove field
  }
  int64 firstrow = current;
  /* allocate rows above current */
  for (int64 i = current - 1; accu < listupper; i--)
    {
      ListRow *lr = i >= 0 ? fetch_row (i) : NULL; // FIXME
      int64 rowheight = measure_row (lr);
      if (rowheight < 0)
        break; // no more rows for layouting
      firstrow = i;
      rmap[firstrow] = lr;
      lr->allocated = true;
      accu += rowheight;
    }
  int64 firstrowoffset = MIN (0, listupper - accu);
  /* allocate rows below current */
  int64 lastrow = current;
  accu += firstrowoffset + currentlower;
  for (int64 i = current + 1; accu < listheight; i++)
    {
      ListRow *lr = i < m_model->count() ? fetch_row (i) : NULL; // FIXME
      int64 rowheight = measure_row (lr);
      if (rowheight < 0)
        break; // no more rows for layouting
      lastrow = i;
      rmap[lastrow] = lr;
      lr->allocated = true;
      accu += rowheight;
    }
  /* clean up remaining old rows */
  for (RowMap::iterator it = m_row_map.begin(); it != m_row_map.end(); it++)
    cache_row (it->second);
  m_row_map.swap (rmap);
  /* layout new rows */
  accu = firstrowoffset;
  //int64 ixrow = firstrow;
  for (RowMap::iterator it = m_row_map.begin(); it != m_row_map.end(); it++) // , ixrow++)
    {
      Allocation area = allocation();
      ListRow *lr = it->second;
      area.height = measure_row (lr);
      area.y = allocation().y + allocation().height - accu;
      lr->area = area;
      accu += area.height;
    }
  /* remember state */
  m_need_resize_scroll = 0;
}

void
ItemListImpl::cache_row (ListRow *lr)
{
  m_row_cache.push_back (lr);
  lr->rowbox->visible (false);
  lr->allocated = 0;
}

static uint dbg_cached = 0, dbg_refilled = 0, dbg_created = 0;

ListRow*
ItemListImpl::create_row (uint64 nthrow,
                          bool   with_size_groups)
{
  Array row = m_model->get (nthrow);
  ListRow *lr = new ListRow();
  for (uint i = 0; i < row.count(); i++)
    {
      Item *item = ref_sink (&Factory::create_item ("Label"));
      lr->cols.push_back (item);
    }
  IFDEBUG (dbg_created++);
  lr->rowbox = &ref_sink (&Factory::create_item ("ListRow"))->interface<Container>();
  lr->rowbox->interface<HBox>().spacing (5); // FIXME

  while (m_size_groups.size() < lr->cols.size())
    m_size_groups.push_back (ref_sink (SizeGroup::create_hgroup()));
  if (with_size_groups)
    for (uint i = 0; i < lr->cols.size(); i++)
      m_size_groups[i]->add_item (*lr->cols[i]);

  for (uint i = 0; i < lr->cols.size(); i++)
    lr->rowbox->add (lr->cols[i]);
  add (lr->rowbox);
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
ItemListImpl::lookup_row (uint64 row)
{
  RowMap::iterator ri = m_row_map.find (row);
  if (ri != m_row_map.end())
    return ri->second;
  else
    return NULL;
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
  lr->rowbox->visible (true);
  return lr;
}

void
ItemListImpl::position_row (ListRow *lr,
                            uint64   row,
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
  lr->rowbox->vposition (tablerow);
  lr->rowbox->vspan (1);
  lr->rowbox->color_scheme (row == m_current_row ? COLOR_SELECTED : COLOR_NORMAL);
  Ambience *ambience = lr->rowbox->interface<Ambience*>();
  if (ambience)
    ambience->background (row & 1 ? "background-odd" : "background-even");
}

uint64 // FIXME: signed
ItemListImpl::measure_row (ListRow *lr,
                           uint64  *allocation_offset)
{
  if (allocation_offset)
    {
      Allocation carea = lr->rowbox->allocation();
      *allocation_offset = carea.y;
      return carea.height;
    }
  else
    {
      Requisition requisition = lr->rowbox->requisition();
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
ItemListImpl::reset (ResetMode mode)
{
  // m_current_row = 18446744073709551615ULL;
}

bool
ItemListImpl::handle_event (const Event &event)
{
  bool handled = false;
  if (!m_model)
    return handled;
  uint64 saved_current_row = m_current_row;
  switch (event.type)
    {
      const EventKey *kevent;
    case KEY_PRESS:
      kevent = dynamic_cast<const EventKey*> (&event);
      switch (kevent->key)
        {
        case KEY_Down:
          if (m_current_row < uint64 (m_model->count()))
            m_current_row = MIN (int64 (m_current_row) + 1, m_model->count() - 1);
          else
            m_current_row = 0;
          handled = true;
          break;
        case KEY_Up:
          if (m_current_row < uint64 (m_model->count()))
            m_current_row = MAX (m_current_row, 1) - 1;
          else if (m_model->count())
            m_current_row = m_model->count() - 1;
          handled = true;
          break;
        }
    case KEY_RELEASE:
    default:
      break;
    }
  if (saved_current_row != m_current_row)
    {
      ListRow *lr;
      lr = lookup_row (saved_current_row);
      if (lr)
        lr->rowbox->color_scheme (COLOR_NORMAL);
      lr = lookup_row (m_current_row);
      if (lr)
        lr->rowbox->color_scheme (COLOR_SELECTED);
    }
  printerr ("m_current_row=%llu\n", m_current_row);
  return handled;
}

static const ItemFactory<ItemListImpl> item_list_factory ("Rapicorn::Factory::ItemList");

} // Rapicorn
