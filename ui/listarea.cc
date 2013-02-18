// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#include "listareaimpl.hh"
#include "sizegroup.hh"
#include "paintcontainers.hh"
#include "factory.hh"

//#define IFDEBUG(...)      do { /*__VA_ARGS__*/ } while (0)
#define IFDEBUG(...)      __VA_ARGS__

namespace Rapicorn {

const PropertyList&
WidgetList::_property_list()
{
  static Property *properties[] = {
    MakeProperty (WidgetList, browse, _("Browse Mode"), _("Browse selection mode"), "rw"),
    MakeProperty (WidgetList, model,  _("Model URL"), _("Resource locator for 1D list model"), "rw:M1"),
  };
  static const PropertyList property_list (properties, ContainerImpl::_property_list());
  return property_list;
}

WidgetListImpl::WidgetListImpl() :
  model_ (NULL),
  hadjustment_ (NULL), vadjustment_ (NULL),
  browse_ (true),
  need_resize_scroll_ (false), block_invalidate_ (false),
  current_row_ (18446744073709551615ULL)
{}

WidgetListImpl::~WidgetListImpl()
{
  /* remove model */
  ListModelIface *oldmodel = model_;
  model_ = NULL;
  if (oldmodel)
    unref (oldmodel);
  /* purge row map */
  RowMap rc; // empty
  row_map_.swap (rc);
  for (RowMap::iterator ri = rc.begin(); ri != rc.end(); ri++)
    cache_row (ri->second);
  /* destroy row cache */
  while (row_cache_.size())
    {
      ListRow *lr = row_cache_.back();
      row_cache_.pop_back();
      for (uint i = 0; i < lr->cols.size(); i++)
        unref (lr->cols[i]);
      unref (lr->rowbox);
      delete lr;
    }
  /* release size groups */
  while (size_groups_.size())
    {
      SizeGroup *sg = size_groups_.back();
      size_groups_.pop_back();
      unref (sg);
    }
}

void
WidgetListImpl::constructed ()
{
  if (!model_)
    {
      MemoryListStore *store = new MemoryListStore (5);
      for (uint i = 0; i < 20; i++)
        {
          Any row;
          String s;
          if (i && (i % 10) == 0)
            s = string_printf ("* %u SMALL ROW (watch scroll direction)", i);
          else
            s = string_printf ("|<br/>| <br/>| %u<br/>|<br/>|", i);
          row <<= s;
          store->insert (-1, row);
        }
      {
        model_ = store;
        ref_sink (model_);
        // model_->sig_inserted() += Aida::slot (*this, &WidgetListImpl::model_inserted);
        // model_->sig_changed() += Aida::slot (*this, &WidgetListImpl::model_changed);
        // model_->sig_removed() += Aida::slot (*this, &WidgetListImpl::model_removed);
      }
      unref (ref_sink (store));
      invalidate_model (true, true);
    }
}

void
WidgetListImpl::hierarchy_changed (WidgetImpl *old_toplevel)
{
  MultiContainerImpl::hierarchy_changed (old_toplevel);
  if (anchored())
    queue_visual_update();
}

Adjustment&
WidgetListImpl::hadjustment () const
{
  if (!hadjustment_)
    hadjustment_ = Adjustment::create (0, 0, 1, 0.01, 0.2);
  return *hadjustment_;
}

Adjustment&
WidgetListImpl::vadjustment () const
{
  if (!vadjustment_)
    {
      vadjustment_ = Adjustment::create (0, 0, 1, 0.01, 0.2);
      WidgetListImpl &self = const_cast<WidgetListImpl&> (*this);
      vadjustment_->sig_value_changed() += Aida::slot (self, &WidgetImpl::queue_visual_update);
    }
  return *vadjustment_;
}

Adjustment*
WidgetListImpl::get_adjustment (AdjustmentSourceType adj_source,
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
WidgetListImpl::model (const String &modelurl)
{
  BaseObject *obj = NULL; // FIXME: plor_get (modelurl);
  ListModelIface *model = dynamic_cast<ListModelIface*> (obj);
  ListModelIface *oldmodel = model_;
  model_ = model;
  if (model_)
    {
      ref_sink (model_);
      // model_->sig_inserted() += Aida::slot (*this, &WidgetListImpl::model_inserted);
      // model_->sig_changed() += Aida::slot (*this, &WidgetListImpl::model_changed);
      // model_->sig_removed() += Aida::slot (*this, &WidgetListImpl::model_removed);
      // #warning FIXME: missing: model_->sig_selection_changed() += slot (*this, &WidgetListImpl::selection_changed);
    }
  if (oldmodel)
    {
      unref (oldmodel);
    }
  invalidate_model (true, true);
}

String
WidgetListImpl::model () const
{
  return ""; // FIME: model_ ? model_->plor_name() : "";
}

void
WidgetListImpl::model_changed (int first, int last)
{
#warning FIXME: intersect changed rows with visible rows
  for (uint64 i = first; i <= uint64 (last); i++)
    {
      ListRow *lr = lookup_row (i);
      if (lr)
        lr->rowbox->invalidate();
    }
}

void
WidgetListImpl::model_inserted (int first, int last)
{
  invalidate_model (true, true);
}

void
WidgetListImpl::model_removed (int first, int last)
{
  invalidate_model (true, true);
}

void
WidgetListImpl::toggle_selected (int row)
{
  if (selection_.size() <= size_t (row))
    selection_.resize (row + 1);
  selection_[row] = !selection_[row];
  selection_changed (row, row);
}

void
WidgetListImpl::selection_changed (int first, int last)
{
  // FIXME: intersect with visible rows
  for (int i = first; i <= last; i++)
    {
      ListRow *lr = lookup_row (i);
      if (lr)
        lr->rowbox->color_scheme (selected (i) ? COLOR_SELECTED : COLOR_NORMAL);
    }
}

void
WidgetListImpl::invalidate_model (bool invalidate_heights,
                                bool invalidate_widgets)
{
  need_resize_scroll_ = true;
  model_sizes_.clear();
  invalidate();
}

void
WidgetListImpl::visual_update ()
{
  need_resize_scroll_ = true; // FIXME
  if (need_resize_scroll_)
    {
      measure_rows (allocation().height);
      resize_scroll_preserving();
    }
  for (RowMap::iterator it = row_map_.begin(); it != row_map_.end(); it++)
    {
      ListRow *lr = it->second;
      lr->rowbox->set_allocation (lr->area);
    }
}

void
WidgetListImpl::invalidate_parent ()
{
  if (!block_invalidate_)
    MultiContainerImpl::invalidate_parent();
}

void
WidgetListImpl::size_request (Requisition &requisition)
{
  bool chspread = false, cvspread = false;
  measure_rows (4096); // FIXME: use monitor size
  // FIXME: this should only measure current row
  requisition.width = 0;
  requisition.height = -1;
  for (ChildWalker cw = local_children(); cw.has_next(); cw++)
    {
      /* size request all children */
      if (!cw->visible())
        continue;
      Requisition crq = cw->requisition();
      requisition.width = MAX (requisition.width, crq.width);
      chspread = cvspread = false;
    }
  if (model_ && model_->count())
    requisition.height = -1;  // FIXME: requisition.height = lookup_row_size (ifloor (model_->count() * vadjustment_->nvalue())); // FIXME
  else
    requisition.height = -1;
  if (requisition.height < 0)
    requisition.height = 12 * 5; // FIXME: request single row height
  set_flag (HSPREAD_CONTAINER, chspread);
  set_flag (VSPREAD_CONTAINER, cvspread);
}

void
WidgetListImpl::size_allocate (Allocation area, bool changed)
{
  need_resize_scroll_ = need_resize_scroll_ || allocation() != area || changed;
  if (need_resize_scroll_)
    {
      measure_rows (area.height);
      resize_scroll();
    }
  for (RowMap::iterator it = row_map_.begin(); it != row_map_.end(); it++)
    {
      ListRow *lr = it->second;
      lr->rowbox->set_allocation (lr->area);
    }
}

bool
WidgetListImpl::pixel_positioning (const int64       mcount,
                                 const ModelSizes &ms) const
{
  return 0;
  return mcount == int64 (ms.row_sizes.size());
}

void
WidgetListImpl::measure_rows (int64 maxpixels)
{
  ModelSizes &ms = model_sizes_;
  /* create measuring context */
  if (!ms.measurement_row)
    {
      ms.measurement_row = create_row (0, false);
      Allocation area;
      area.x = area.y = -1;
      area.width = area.height = 0;
      ms.measurement_row->rowbox->set_allocation (area);
    }
  /* catch cases where measuring is useless */
  if (!model_ || model_->count() > maxpixels)
    {
      ms.clear();
      return;
    }
  /* check if measuring is needed */
  const int64 mcount = model_->count();
  if (ms.total_height > 0 && mcount == int64 (ms.row_sizes.size()))
    return;

  return; // FIXME

  /* measure all rows */
  ms.clear();
  ms.row_sizes.resize (mcount);
  ListRow *lr = ms.measurement_row;
  for (int64 row = 0; row < mcount; row++)
    {
      printerr ("measure: %llu\n", row);
      fill_row (lr, row);
      int rowheight = measure_row (lr);
      ms.row_sizes[row] = rowheight;
      ms.total_height += rowheight;
    }
}

int
WidgetListImpl::row_height (ModelSizes &ms,
                          int64       list_row)
{
  map<int64,int>::iterator it = ms.size_cache.find (list_row);
  if (it != ms.size_cache.end())
    return it->second;
  fill_row (ms.measurement_row, list_row);
  Requisition requisition = ms.measurement_row->rowbox->requisition();
  ms.size_cache[list_row] = requisition.height;
  // printerr ("CACHE: last=%lld size=%d\n", list_row, ms.size_cache.size());
  return requisition.height;
}

int64 /* list bottom relative y for list_row */
WidgetListImpl::row_layout (double      vscrollpos,
                          int64       mcount,
                          ModelSizes &ms,
                          int64       list_row)
{
  /* allocate scroll position row (list bottom relative) */
  const double list_alignment = vscrollpos / mcount;
  const int listpos = allocation().height * (1.0 - list_alignment);
  const int64 scrollrow = ifloor (vscrollpos);
  const double scrollrow_alignment = vscrollpos - scrollrow;
  const int scrollrow_lower = row_height (ms, scrollrow) * (1.0 - scrollrow_alignment);
  const int scrollrow_y = listpos - scrollrow_lower;
  int64 y = scrollrow_y;
  if (scrollrow > list_row)
    for (int64 row = list_row + 1; row <= scrollrow; row++)
      y += row_height (ms, row);
  else if (scrollrow < list_row)
    for (int64 row = scrollrow + 1; row <= list_row; row++)
      y -= row_height (ms, row);
  return y;
}

int64
WidgetListImpl::position2row (double   list_fraction,
                            double  *row_fraction)
{
  /* scroll position interpretation depends on the available vertical scroll
   * position resolution, which is derived from the number of pixels that
   * are (possibly) vertically allocated for the list view:
   * 1) Pixel size interpretation.
   *    All list rows are measured to determine their height. The fractional
   *    scroll position is then interpreted as a pointer into the total pixel
   *    height of the list.
   * 2) Positional interpretation (applicable for large models).
   *    The scroll position is interpreted as a fractional pointer into the
   *    interval [0,rowcount[. So the integer part of the scroll position will
   *    always point at one particular row and the fractional part is
   *    interpreted as an offset into the widget's row, as [row_index.row_fraction].
   *    From this, the actual scroll position is interpolated so that the top
   *    of the first row and the bottom of the last row are aligned with top and
   *    bottom of the list view respectively.
   * Note that list rows increase downwards, pixel coordinates increase upwards.
   */
  const int64 mcount = model_->count();
  const ModelSizes &ms = model_sizes_;
  if (pixel_positioning (mcount, ms))
    {
      /* pixel positioning */
      const int64 lpixel = list_fraction * ms.total_height;
      int64 row, accu = 0;
      for (row = 0; row < mcount; row++)
        {
          accu += ms.row_sizes[row];
          if (lpixel < accu)
            break;
        }
      if (row < mcount)
        {
          if (row_fraction)
            *row_fraction = 1.0 - (accu - lpixel) / ms.row_sizes[row];
          return row;
        }
      return -1; // invalid row
    }
  else
    {
      /* fractional positioning */
      const double scroll_value = list_fraction * mcount;
      const int64 row = min (mcount - 1, ifloor (scroll_value));
      if (row_fraction)
        *row_fraction = min (1.0, scroll_value - row);
      return row;
    }
}

double
WidgetListImpl::row2position (const int64  list_row,
                            const double list_alignment)
{
  const int64 mcount = model_->count();
  if (list_row < 0 || list_row >= mcount)
    return -1; // invalid row
  ModelSizes &ms = model_sizes_;
  const int listheight = allocation().height;
  /* see position2row() for fractional positioning vs. pixel positioning */
  if (pixel_positioning (mcount, ms))
    {
      /* pixel positioning */
      int64 row, accu = 0;
      for (row = 0; row < list_row; row++)
        accu += ms.row_sizes[row];
      /* here: accu == real_scroll_position for list_alignment == 0 */
      accu += ms.row_sizes[row] * list_alignment;       // fractional row to align
      accu -= listheight * list_alignment;              // fractional list to align
      accu = max (0, accu);                             // clamp row0 to list start
      return accu;
    }
  else
    {
      /* fractional positioning */
      const int listpos = listheight * (1.0 - list_alignment);
      const int rowheight = row_height (ms, list_row);
      const int maxstep = 16;                           // upper bound on O(row_layout())
      int64 last_rowpos = listpos;
      double lower = 0, upper = mcount;
      double value = list_row + 0.5;
      for (int i = 0; i <= max (56, listheight); i++)   // maximum usually unreached
        {
          int64 rowy = row_layout (value, mcount, ms, list_row); // rowy measure from list bottom
          int64 rowpos = rowy + rowheight * (1.0 - list_alignment);
          if (rowpos < listpos)
            lower = value;      /* value must grow */
          else if (rowpos > listpos)
            upper = value;      /* value must shrink */
          else
            break;              /* value is ideal */
          if (rowpos == last_rowpos)
            break;              /* value cannot be refined */
          last_rowpos = rowpos;
          value = CLAMP (0.5 * (lower + upper), value - maxstep, value + maxstep);
        }
      return value;
    }
}

int64
WidgetListImpl::scroll_row_layout (ListRow *lr_current,
                                 int64 *scrollrowy,
                                 int64 *scrollrowupper,
                                 int64 *scrollrowlower,
                                 int64 *listupperp,
                                 int64 *listheightp)
{
  /* scroll position interpretation:
   * the current slider position is interpreted as a fractional pointer into the
   * interval [0,count[. so the integer part of the scroll position will always
   * point at one particular widget and the fractional part is interpreted as an
   * offset into the widget's row.
   * Scrolling for large models works by interpreting the scroll adjustment
   * values as [row_index.row_fraction]. From this, a scroll position is
   * interpolated so that the top of the first row and the bottom of the last
   * row are aligned with top and bottom of the list view respectively.
   * Note that list rows increase downwards, pixel coordinates increase upwards.
   */
  const double norm_value = vadjustment_->nvalue();            // 0..1 scroll position
  const double scroll_value = norm_value * model_->count();    // fraction into count()
  const int64 scroll_widget = MIN (model_->count() - 1, ifloor (scroll_value));
  const double scroll_fraction = MIN (1.0, scroll_value - scroll_widget); // fraction into scroll_widget row

  int64 rowheight; // FIXME: make const: const int64 rowheight = lookup_row_size (scroll_widget);
  {
    Requisition requisition = lr_current->rowbox->requisition();
    rowheight = requisition.height;
  }

  assert_return (rowheight > 0, scroll_widget);
  const int64 rowlower = rowheight * (1 - scroll_fraction);       // fractional lower row pixels
  const int64 listlower = allocation().height * (1 - norm_value); // fractional lower list pixels
  *scrollrowy = listlower - rowlower;
  *scrollrowupper = rowheight - rowlower;
  *scrollrowlower = rowlower;
  *listupperp = allocation().height - listlower;
  *listheightp = allocation().height;
  return scroll_widget;
}

void
WidgetListImpl::resize_scroll () // model_->count() >= 1
{
  const int64 mcount = model_->count();

  /* flag old rows */
  for (RowMap::iterator it = row_map_.begin(); it != row_map_.end(); it++)
    it->second->allocated = 0; // FIXME

  int64 current = min (mcount - 1, ifloor (vadjustment_->nvalue() * mcount)); // FIXME
  ListRow *lr_current = fetch_row (current);

  int64 current_y, currentupper, currentlower, listupper, listheight;
  int64 current_FIXME = scroll_row_layout (lr_current, &current_y, &currentupper, &currentlower, &listupper, &listheight);
  (void) current_FIXME;
  RowMap rmap;

  cache_row (lr_current); // FIXME

  /* deactivate size-groups to avoid excessive resizes upon measure_row() */
  for (uint i = 0; i < size_groups_.size(); i++)
    size_groups_[i]->active (false);

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
  for (int64 i = current - 1; i >= 0 && accu < listupper; i--)
    {
      ListRow *lr = fetch_row (i);
      int64 rowheight = measure_row (lr);
      firstrow = i;
      rmap[firstrow] = lr;
      lr->allocated = true;
      accu += rowheight;
    }
  int64 firstrowoffset = MIN (0, listupper - accu);
  /* allocate rows below current */
  int64 lastrow = current;
  accu += firstrowoffset + currentlower;
  for (int64 i = current + 1; i < mcount && accu < listheight; i++)
    {
      ListRow *lr = fetch_row (i);
      int64 rowheight = measure_row (lr);
      lastrow = i;
      rmap[lastrow] = lr;
      lr->allocated = true;
      accu += rowheight;
    }
  /* clean up remaining old rows */
  for (RowMap::iterator it = row_map_.begin(); it != row_map_.end(); it++)
    cache_row (it->second);
  row_map_.swap (rmap);
  /* layout new rows */
  accu = firstrowoffset;
  for (int64 ix = firstrow; ix <= lastrow; ix++)
    {
      ListRow *lr = row_map_[ix];
      Allocation area = allocation();
      area.height = measure_row (lr);
      area.y = allocation().y + allocation().height - area.height - accu;
      lr->area = area;
      accu += area.height;
    }
  /* reactivate size-groups for proper size allocation */
  for (uint i = 0; i < size_groups_.size(); i++)
    size_groups_[i]->active (true);
  /* remember state */
  need_resize_scroll_ = 0;
}

void
WidgetListImpl::resize_scroll_preserving () // model_->count() >= 1
{
  if (!block_invalidate_ && drawable() &&
      !test_any_flag (INVALID_REQUISITION | INVALID_ALLOCATION | INVALID_CONTENT))
    {
      block_invalidate_ = true;
      resize_scroll();
      requisition();
      set_allocation (allocation());
      block_invalidate_ = false;
    }
  else
    resize_scroll();
}

void
WidgetListImpl::cache_row (ListRow *lr)
{
  row_cache_.push_back (lr);
  lr->rowbox->visible (false);
  lr->allocated = 0;
}

static uint dbg_cached = 0, dbg_refilled = 0, dbg_created = 0;

ListRow*
WidgetListImpl::create_row (uint64 nthrow, bool with_size_groups)
{
  Any row = model_->row (nthrow);
  ListRow *lr = new ListRow();
  WidgetImpl *widget = ref_sink (&Factory::create_ui_widget ("Label"));
  lr->cols.push_back (widget);
  IFDEBUG (dbg_created++);
  lr->rowbox = &ref_sink (&Factory::create_ui_widget ("ListRow"))->interface<ContainerImpl>();
  lr->rowbox->interface<HBox>().spacing (5); // FIXME

  while (size_groups_.size() < lr->cols.size())
    size_groups_.push_back (ref_sink (SizeGroup::create_hgroup()));
  if (with_size_groups)
    for (uint i = 0; i < lr->cols.size(); i++)
      size_groups_[i]->add_widget (*lr->cols[i]);

  for (uint i = 0; i < lr->cols.size(); i++)
    lr->rowbox->add (lr->cols[i]);
  add (lr->rowbox);
  return lr;
}

void
WidgetListImpl::fill_row (ListRow *lr, uint64 nthrow)
{
  Any row = model_->row (nthrow);
  for (uint i = 0; i < lr->cols.size(); i++)
    lr->cols[i]->set_property ("markup_text", row.as_string());
  Ambience *ambience = lr->rowbox->interface<Ambience*>();
  if (ambience)
    ambience->background (nthrow & 1 ? "background-odd" : "background-even");
  Frame *frame = lr->rowbox->interface<Frame*>();
  if (frame)
    frame->frame_type (nthrow == current_row_ ? FRAME_FOCUS : FRAME_NONE);
  lr->rowbox->color_scheme (selected (nthrow) ? COLOR_SELECTED : COLOR_NORMAL);
}

ListRow*
WidgetListImpl::lookup_row (uint64 row)
{
  RowMap::iterator ri = row_map_.find (row);
  if (ri != row_map_.end())
    return ri->second;
  else
    return NULL;
}

ListRow*
WidgetListImpl::fetch_row (uint64 row)
{
  bool filled = false;
  ListRow *lr;
  RowMap::iterator ri = row_map_.find (row);
  if (ri != row_map_.end())
    {
      lr = ri->second;
      row_map_.erase (ri);
      filled = true;
      IFDEBUG (dbg_cached++);
    }
  else if (row_cache_.size())
    {
      lr = row_cache_.back();
      row_cache_.pop_back();
      IFDEBUG (dbg_refilled++);
    }
  else /* create row */
    lr = create_row (row);
  if (!filled)
    fill_row (lr, row);
  lr->rowbox->visible (true);
  return lr;
}

uint64 // FIXME: signed
WidgetListImpl::measure_row (ListRow *lr,
                           uint64  *allocation_offset)
{
  if (allocation_offset)
    {
      Allocation carea = lr->rowbox->allocation();
      *allocation_offset = carea.y;
      if (carea.height < 0)
        assert_unreached(); // FIXME
      return carea.height;
    }
  else
    {
      Requisition requisition = lr->rowbox->requisition();
      if (requisition.height < 0)
        assert_unreached(); // FIXME
      return requisition.height;
    }
}

void
WidgetListImpl::reset (ResetMode mode)
{
  // current_row_ = 18446744073709551615ULL;
}

bool
WidgetListImpl::handle_event (const Event &event)
{
  bool handled = false;
  if (!model_)
    return handled;
  uint64 saved_current_row = current_row_;
  switch (event.type)
    {
      const EventKey *kevent;
    case KEY_PRESS:
      kevent = dynamic_cast<const EventKey*> (&event);
      switch (kevent->key)
        {
        case KEY_Down:
          if (current_row_ < uint64 (model_->count()))
            current_row_ = MIN (int64 (current_row_) + 1, model_->count() - 1);
          else
            current_row_ = 0;
          handled = true;
          break;
        case KEY_Up:
          if (current_row_ < uint64 (model_->count()))
            current_row_ = MAX (current_row_, 1) - 1;
          else if (model_->count())
            current_row_ = model_->count() - 1;
          handled = true;
          break;
        case KEY_space:
          if (current_row_ >= 0 && current_row_ < uint64 (model_->count()))
            toggle_selected (current_row_);
          handled = true;
          break;
        }
    case KEY_RELEASE:
    default:
      break;
    }
  if (saved_current_row != current_row_)
    {
      ListRow *lr;
      lr = lookup_row (saved_current_row);
      if (lr)
        {
          Frame *frame = lr->rowbox->interface<Frame*>();
          if (frame)
            frame->frame_type (FRAME_NONE);
        }
      lr = lookup_row (current_row_);
      if (lr)
        {
          Frame *frame = lr->rowbox->interface<Frame*>();
          if (frame)
            frame->frame_type (FRAME_FOCUS);
        }
      double vscrollupper = row2position (current_row_, 0.0) / model_->count();
      double vscrolllower = row2position (current_row_, 1.0) / model_->count();
      double nvalue = CLAMP (vadjustment_->nvalue(), vscrolllower, vscrollupper);
      if (nvalue != vadjustment_->nvalue())
        vadjustment_->nvalue (nvalue);
      if ((selection_mode() == SELECTION_SINGLE ||
           selection_mode() == SELECTION_BROWSE) &&
          selected (saved_current_row))
        toggle_selected (current_row_);
    }
  return handled;
}

static const WidgetFactory<WidgetListImpl> widget_list_factory ("Rapicorn::Factory::WidgetList");

} // Rapicorn
