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
#ifndef __RAPICORN_LIST_AREA_IMPL_HH__
#define __RAPICORN_LIST_AREA_IMPL_HH__

#include <rapicorn/listarea.hh>
#include <rapicorn/containerimpl.hh>
#include <rapicorn/adjustment.hh>
#include <rapicorn/layoutcontainers.hh>
#include <rapicorn/table.hh>
#include <deque>

namespace Rapicorn {

struct ListRow {
  vector<Item*> cols;
  Container    *rowbox;
  Allocation    area;
  uint          allocated : 1;
  ListRow() : rowbox (NULL), allocated (0) {}
};

struct ModelSizes {
  vector<int>    row_sizes;
  map<int64,int> size_cache; // ca. 28bytes per node
  int64          total_height;
  ListRow       *measurement_row;
  explicit ModelSizes() : total_height (0), measurement_row (NULL) {}
  void     clear     () { row_sizes.clear(); size_cache.clear(); total_height = 0; }
};

class ItemListImpl : public virtual MultiContainerImpl,
                     public virtual ItemList,
                     public virtual AdjustmentSource,
                     public virtual EventHandler
{
  typedef map<int64,ListRow*>  RowMap;
  typedef std::deque<int>      SizeQueue;
  Model1                *m_model;
  mutable Adjustment    *m_hadjustment, *m_vadjustment;
  bool                   m_browse;
  uint                   m_n_cols;
  RowMap                 m_row_map;
  vector<ListRow*>       m_row_cache;
  vector<SizeGroup*>     m_size_groups;
  bool                   m_need_resize_scroll;
  bool                   m_block_invalidate;
  uint64                 m_current_row;
  ModelSizes             m_model_sizes;
  virtual void          invalidate_parent ();
protected:
  virtual bool          handle_event            (const Event    &event);
  virtual void          reset                   (ResetMode       mode);
  virtual bool          can_focus               () const { return true; }
public:
  explicit              ItemListImpl            ();
  virtual              ~ItemListImpl            ();
  virtual void          constructed             ();
  virtual bool          browse                  () const        { return m_browse; }
  virtual void          browse                  (bool b)        { m_browse = b; invalidate(); }
  virtual void          model                   (const String &modelurl);
  virtual String        model                   () const;
  virtual void          hierarchy_changed       (Item *old_toplevel);
  Adjustment&           hadjustment             () const;
  Adjustment&           vadjustment             () const;
  Adjustment*           get_adjustment          (AdjustmentSourceType adj_source,
                                                 const String        &name);
  void                  invalidate_model        (bool invalidate_heights,
                                                 bool invalidate_widgets);
  virtual void          visual_update           ();
  virtual void          size_request            (Requisition &requisition);
  virtual void          size_allocate           (Allocation area);
  /* sizing and positioning */
  bool                  pixel_positioning       (const int64       mcount,
                                                 const ModelSizes &ms) const;
  void                  measure_rows            (int64    maxpixels);
  int64                 position2row            (double   list_fraction,
                                                 double  *row_fraction);
  double                row2position            (int64    list_row,
                                                 double   list_alignment = 0.5);
  int                   row_height              (ModelSizes &ms,
                                                 int64       list_row);
  int64                 row_layout              (double      vscrollpos,
                                                 int64       mcount,
                                                 ModelSizes &ms,
                                                 int64       list_row);

  void                  measure_rows            (int64    maxpixels,
                                                 double   fraction);
  int64                 scroll_row_layout       (ListRow *lr_current,
                                                 int64 *scrollrowy,
                                                 int64 *scrollrowupper,
                                                 int64 *scrollrowlower,
                                                 int64 *listupperp,
                                                 int64 *listheightp);
  void                  resize_scroll           ();
  void                  resize_scroll_preserving();
  void                  cache_row               (ListRow *lr);
  void                  fill_row                (ListRow *lr,
                                                 uint64   row);
  ListRow*              create_row              (uint64 row,
                                                 bool   with_size_groups = true);
  ListRow*              lookup_row              (uint64 row);
  ListRow*              fetch_row               (uint64 row);
  uint64                measure_row             (ListRow *lr,
                                                 uint64  *allocation_offset = NULL);
};

} // Rapicorn

#endif  /* __RAPICORN_LIST_AREA_IMPL_HH__ */
