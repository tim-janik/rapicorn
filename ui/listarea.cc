// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#include "listareaimpl.hh"
#include "sizegroup.hh"
#include "paintcontainers.hh"
#include "factory.hh"
#include "application.hh"

//#define IFDEBUG(...)      do { /*__VA_ARGS__*/ } while (0)
#define IFDEBUG(...)      __VA_ARGS__

namespace Rapicorn {

// == WidgetList ==
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

// == WidgetListRowImpl ==
class WidgetListRowImpl : public virtual SingleContainerImpl, public virtual FocusFrame::Client {
  FocusFrame     *focus_frame_;
  int             index_;
  WidgetListImpl* widget_list () const  { return dynamic_cast<WidgetListImpl*> (parent()); }
protected:
  virtual const PropertyList&
  _property_list ()
  {
    static Property *properties[] = {
      MakeProperty (WidgetListRowImpl, selected, _("Selected"), _("Indicates wether this row is selected"), "rw"),
    };
    static const PropertyList property_list (properties, SingleContainerImpl::_property_list());
    return property_list;
  }
  virtual bool
  can_focus () const
  {
    return focus_frame_ != NULL;
  }
  virtual bool
  register_focus_frame (FocusFrame &frame)
  {
    if (!focus_frame_)
      focus_frame_ = &frame;
    return focus_frame_ == &frame;
  }
  virtual void
  unregister_focus_frame (FocusFrame &frame)
  {
    if (focus_frame_ == &frame)
      focus_frame_ = NULL;
  }
public:
  WidgetListRowImpl() :
    focus_frame_ (NULL), index_ (INT_MIN)
  {
    color_scheme (COLOR_BASE);
  }
  int           row_index() const       { return index_; }
  void
  row_index (int i)
  {
    index_ = i >= 0 ? i : INT_MIN;
    WidgetListImpl *list = widget_list();
    if (list && index_ >= 0)
      color_scheme (list->selected (index_) ? COLOR_SELECTED : COLOR_BASE);
    visible (index_ >= 0);
  }
  virtual bool
  selected () const
  {
    WidgetListImpl *list = widget_list();
    return list && index_ >= 0 ? list->selected (index_) : false;
  }
  virtual void
  selected (bool s)
  {
    WidgetListImpl *list = widget_list();
    if (list && index_ >= 0)
      {
        if (list->selected (index_) != s)
          list->toggle_selected (index_);
        color_scheme (list->selected (index_) ? COLOR_SELECTED : COLOR_BASE);
      }
  }
};

static const WidgetFactory<WidgetListRowImpl> widget_list_row_factory ("Rapicorn::Factory::WidgetListRow");

// == WidgetListImpl ==
static const WidgetFactory<WidgetListImpl> widget_list_factory ("Rapicorn::Factory::WidgetList");

WidgetListImpl::WidgetListImpl() :
  model_ (NULL), conid_updated_ (0),
  hadjustment_ (NULL), vadjustment_ (NULL), cached_focus_ (NULL),
  virtualized_pixel_scrolling_ (true),
  need_scroll_layout_ (false), block_invalidate_ (false)
{}

WidgetListImpl::~WidgetListImpl()
{
  // remove model
  model ("");
  assert_return (model_ == NULL);
  // purge row map
  RowMap rc; // empty
  row_map_.swap (rc);
  for (RowMap::iterator ri = rc.begin(); ri != rc.end(); ri++)
    cache_row (ri->second);
  if (cached_focus_)
    {
      ListRow *lr = cached_focus_;
      cached_focus_ = NULL;
      lr->lrow->unset_focus();
      cache_row (lr);
    }
  // destroy row cache
  while (row_cache_.size())
    {
      ListRow *lr = row_cache_.back();
      row_cache_.pop_back();
      for (uint i = 0; i < lr->cols.size(); i++)
        unref (lr->cols[i]);
      unref (lr->lrow);
      delete lr;
    }
  // release size groups
  while (size_groups_.size())
    {
      SizeGroup *sg = size_groups_.back();
      size_groups_.pop_back();
      unref (sg);
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
WidgetListImpl::get_adjustment (AdjustmentSourceType adj_source, const String &name)
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
  ListModelIface *lmi = ApplicationImpl::the().xurl_find (modelurl);
  ListModelIface *oldmodel = model_;
  model_ = lmi;
  row_heights_.clear();
  if (oldmodel)
    {
      oldmodel->sig_updated() -= conid_updated_;
      conid_updated_ = 0;
      row_heights_.clear();
    }
  if (model_)
    {
      ref_sink (model_);
      row_heights_.resize (model_->count(), -1);
      conid_updated_ = model_->sig_updated() += Aida::slot (*this, &WidgetListImpl::model_updated);
    }
  if (oldmodel)
    unref (oldmodel);
  invalidate_model (true, true);
}

String
WidgetListImpl::model () const
{
  return ""; // FIME: use xurl_path (not model_->plor_name)
}

void
WidgetListImpl::model_updated (const UpdateRequest &urequest)
{
  switch (urequest.kind)
    {
    case UPDATE_READ:
      break;
    case UPDATE_INSERTION:
      cache_range (urequest.rowspan.start, ~size_t (0));
      row_heights_.resize (model_->count(), -1);
      break;
    case UPDATE_CHANGE:
      cache_range (urequest.rowspan.start, urequest.rowspan.start + urequest.rowspan.length);
      break;
    case UPDATE_DELETION:
      cache_range (urequest.rowspan.start, ~size_t (0));
      row_heights_.resize (model_->count(), -1);
      break;
    }
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
  // FIXME: intersect with visible rows?
  for (int i = first; i <= last; i++)
    {
      ListRow *lr = lookup_row (i);
      if (lr)
        lr->lrow->selected (selected (i));
    }
}

void
WidgetListImpl::invalidate_model (bool invalidate_heights, bool invalidate_widgets)
{
  need_scroll_layout_ = true;
  // FIXME: reset all cached row_heights_ here?
  invalidate();
}

void
WidgetListImpl::visual_update ()
{
  need_scroll_layout_ = true; // FIXME
  if (need_scroll_layout_)
    scroll_layout_preserving();
  for (RowMap::iterator it = row_map_.begin(); it != row_map_.end(); it++)
    {
      ListRow *lr = it->second;
      lr->lrow->set_allocation (lr->area, &allocation());
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
  // FIXME: allow property to specify how many rows should be visible?
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
  need_scroll_layout_ = need_scroll_layout_ || allocation() != area || changed;
  if (need_scroll_layout_)
    vscroll_layout();
  for (RowMap::iterator it = row_map_.begin(); it != row_map_.end(); it++)
    {
      ListRow *lr = it->second;
      lr->lrow->set_allocation (lr->area, &allocation());
    }
}

int
WidgetListImpl::focus_row()
{
  WidgetImpl *fchild = get_focus_child();
  WidgetListRowImpl *lrow = dynamic_cast<WidgetListRowImpl*> (fchild);
  return lrow ? lrow->row_index() : -1;
}

bool
WidgetListImpl::handle_event (const Event &event)
{
  bool handled = false;
  if (!model_)
    return handled;
  const int64 mcount = model_->count();
  return_unless (mcount > 0, handled);
  int current_focus = focus_row();
  const int saved_current_row = current_focus;
  switch (event.type)
    {
      const EventKey *kevent;
    case KEY_PRESS:
      kevent = dynamic_cast<const EventKey*> (&event);
      switch (kevent->key)
        {
        case KEY_Down:
          if (current_focus >= 0 && current_focus < model_->count())
            current_focus = MIN (current_focus + 1, model_->count() - 1);
          else
            current_focus = 0;
          handled = true;
          break;
        case KEY_Up:
          if (current_focus >= 0 && current_focus < model_->count())
            current_focus = MAX (current_focus, 1) - 1;
          else if (model_->count())
            current_focus = model_->count() - 1;
          handled = true;
          break;
        case KEY_space:
          if (current_focus >= 0 && current_focus < model_->count())
            toggle_selected (current_focus);
          handled = true;
          break;
        case KEY_Page_Up:
          if (first_row_ >= 0 && last_row_ >= first_row_ && last_row_ < model_->count())
            {
              // See KEY_Page_Down comment.
              const Allocation list_area = allocation();
              const int delta = list_area.height; // - row_height (current_focus) - 1;
              const int jumprow = vscroll_relative_row (current_focus, -MAX (0, delta)) + 1;
              current_focus = CLAMP (MIN (jumprow, current_focus - 1), 0, model_->count() - 1);
            }
          break;
        case KEY_Page_Down:
          if (first_row_ >= 0 && last_row_ >= first_row_ && last_row_ < model_->count())
            {
              /* Ideally, jump to a new row by a single screenful of pixels, but: never skip rows by
               * jumping more than a screenful of pixels, keep in mind that the view will be aligned
               * to fit the target row fully. Also make sure to jump by at least single row so we keep
               * moving regardless of view height (which might be less than current_row height).
               */
              const Allocation list_area = allocation();
              const int delta = list_area.height; //  - row_height (current_focus) - 1;
              const int jumprow = vscroll_relative_row (current_focus, +MAX (0, delta)) - 1;
              current_focus = CLAMP (MAX (jumprow, current_focus + 1), 0, model_->count() - 1);
            }
          break;
        }
    case KEY_RELEASE:
    default:
      break;
    }
  if (saved_current_row != current_focus)
    {
      ListRow *lr = lookup_row (current_focus);
      if (lr)
        lr->lrow->grab_focus();                 // new focus row is onscreen
      else
        {
          lr = fetch_row (current_focus);
          if (lr)                               // new focus row is offscreen
            {
              lr->lrow->grab_focus();
              cache_row (lr);
            }
          else                                  // no row gets focus
            {
              lr = lookup_row (saved_current_row);
              if (lr)
                lr->lrow->unset_focus();
            }
        }
      double vscrolllower = vscroll_row_position (current_focus, 1.0); // lower scrollpos for current at visible bottom
      double vscrollupper = vscroll_row_position (current_focus, 0.0); // upper scrollpos for current at visible top
      // fixup possible approximation error in first/last pixel via edge attraction
      if (vscrollupper <= 1)                    // edge attraction at top
        vscrolllower = vscrollupper = 0;
      else if (vscrolllower >= mcount - 1)      // edge attraction at bottom
        vscrolllower = vscrollupper = mcount;
      const double nvalue = CLAMP (vadjustment_->nvalue(), vscrolllower / model_->count(), vscrollupper / model_->count());
      if (nvalue != vadjustment_->nvalue())
        vadjustment_->nvalue (nvalue);
      if ((selection_mode() == SELECTION_SINGLE ||
           selection_mode() == SELECTION_BROWSE) &&
          selected (saved_current_row))
        toggle_selected (current_focus);
    }
  return handled;
}

int
WidgetListImpl::row_height (int nth_row)
{
  const int64 mcount = model_->count();
  assert_return (nth_row < mcount, -1);
  if (row_heights_.size() != (size_t) mcount)    // FIXME: hack around missing updates
    row_heights_.resize (model_->count(), -1);
  if (row_heights_[nth_row] < 0)
    {
      ListRow *lr = lookup_row (nth_row);
      bool keep_uncached = true;
      if (!lr)
        {
          lr = fetch_row (nth_row);
          keep_uncached = false;
        }
      row_heights_[nth_row] = lr->lrow->requisition().height;
      if (!keep_uncached)
        cache_row (lr);
    }
  return row_heights_[nth_row];
}

static uint dbg_cached = 0, dbg_refilled = 0, dbg_created = 0;

ListRow*
WidgetListImpl::create_row (uint64 nthrow, bool with_size_groups)
{
  Any row = model_->row (nthrow);
  ListRow *lr = new ListRow();
  IFDEBUG (dbg_created++);
  WidgetImpl *widget = &Factory::create_ui_child (*this, "RapicornWidgetListRow", Factory::ArgumentList(), false);
  lr->lrow = ref_sink (widget)->interface<WidgetListRowImpl*>();
  lr->lrow->interface<HBox>().spacing (5); // FIXME
  widget = ref_sink (&Factory::create_ui_child (*lr->lrow, "Label", Factory::ArgumentList()));
  lr->cols.push_back (widget);

  while (size_groups_.size() < lr->cols.size())
    size_groups_.push_back (ref_sink (SizeGroup::create_hgroup()));
  if (with_size_groups)
    for (uint i = 0; i < lr->cols.size(); i++)
      size_groups_[i]->add_widget (*lr->cols[i]);

  add (lr->lrow);
  return lr;
}

void
WidgetListImpl::fill_row (ListRow *lr, uint nthrow)
{
  Any row = model_->row (nthrow);
  for (uint i = 0; i < lr->cols.size(); i++)
    lr->cols[i]->set_property ("markup_text", row.as_string());
  Ambience *ambience = lr->lrow->interface<Ambience*>();
  if (ambience)
    ambience->background (nthrow & 1 ? "background-odd" : "background-even");
  lr->lrow->selected (selected (nthrow));
}

ListRow*
WidgetListImpl::fetch_row (int row)
{
  return_unless (row >= 0, NULL);
  bool need_focus = false, filled = false;
  ListRow *lr;
  RowMap::iterator ri = row_map_.find (row);
  if (ri != row_map_.end())
    {
      lr = ri->second;
      row_map_.erase (ri);
      filled = true;
      IFDEBUG (dbg_cached++);
    }
  else if (cached_focus_ && cached_focus_->lrow->row_index() == row)
    {
      lr = cached_focus_;
      cached_focus_ = NULL;
      filled = true;
      need_focus = true;
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
  lr->lrow->row_index (row); // visible=true
  if (need_focus)
    lr->lrow->grab_focus();
  return lr;
}

ListRow*
WidgetListImpl::lookup_row (int row)
{
  return_unless (row >= 0, NULL);
  RowMap::iterator ri = row_map_.find (row);
  if (ri != row_map_.end())
    return ri->second;
  else if (cached_focus_ && cached_focus_->lrow->row_index() == row)
    return cached_focus_;
  else
    return NULL;
}

void
WidgetListImpl::cache_row (ListRow *lr)
{
  if (lr->lrow->has_focus())
    {
      assert_return (cached_focus_ == NULL);
      cached_focus_ = lr;
      lr->lrow->visible (false);
    }
  else
    {
      row_cache_.push_back (lr);
      lr->lrow->row_index (-1); // visible=false
      lr->allocated = 0;
    }
}

void
WidgetListImpl::cache_range (size_t first, size_t bound)
{
  RowMap rmap;
  for (auto it = row_map_.begin(); it != row_map_.end(); it++)
    if (it->first < ssize_t (first) || it->first >= ssize_t (bound))
      rmap[it->first] = it->second;                     // keep row
    else
      {
        ListRow *lr = it->second;                       // retire
        cache_row (lr);
      }
  for (size_t i = first; i < min (bound, row_heights_.size()); i++)
    row_heights_[i] = -1;
  row_map_.swap (rmap);
}

void
WidgetListImpl::reset (ResetMode mode)
{
  // first_row_ = last_row_ = current_row_ = MIN_INT;
}

void
WidgetListImpl::scroll_layout_preserving () // model_->count() >= 1
{
  if (!block_invalidate_ && drawable() && !test_any_flag (INVALID_REQUISITION | INVALID_ALLOCATION | INVALID_CONTENT))
    {
      block_invalidate_ = true;
      scroll_layout();
      requisition();
      set_allocation (allocation());
      block_invalidate_ = false;
    }
  else
    scroll_layout();
}

// == Virtual Position Scrolling ==
/* Scroll position interpretation:
 * The current slider position is interpreted as a fractional pointer into the
 * interval [0,count[. The resulting integer part of the scroll position will always
 * point at one particular row (<= count) and the fractional part is interpreted as a
 * vertical offset into this particular row.
 * From this, a scroll position is interpolated so that the top of the first row and
 * the bottom of the last row are aligned with top and bottom of the list view
 * respectively. This is achieved by using the vertical row offset as one alignment
 * point within the row (the row alignment point), and using the current slider position
 * via proportional indexing into the list view height as second (list) alignment point.
 * Note that list rows increase downwards and pixel coordinates increase downwards.
 */
void
WidgetListImpl::vscroll_layout ()
{
  const int64 mcount = model_->count();
  assert_return (mcount >= 1);
  RowMap rmap;
  // deactivate size-groups to avoid excessive resizes upon row measuring
  for (uint i = 0; i < size_groups_.size(); i++)
    size_groups_[i]->active (false);
  // flag old rows
  for (RowMap::iterator it = row_map_.begin(); it != row_map_.end(); it++)
    it->second->allocated = 0;
  // calculate alignment point for vertical scroll layout
  const Allocation list_area = allocation();
  const double scroll_norm_value = vadjustment_->nvalue();                      // 0..1 scroll position
  const double scroll_value = scroll_norm_value * mcount;                       // fraction into count()
  const int64 scroll_widget = min (mcount - 1, ifloor (scroll_value));          // index into mcount at scroll_norm_value
  const double scroll_fraction = min (1.0, scroll_value - scroll_widget);       // fraction into scroll_widget row
  const int64 list_apoint = list_area.y + list_area.height * scroll_norm_value; // list alignment coordinate
  assert_return (scroll_widget >= 0 && scroll_widget < mcount);         // FIXME: properly catch scroll_widget > mcount
  // allocate row at alignment point
  ListRow *lr_sw = fetch_row (scroll_widget);
  {
    const Requisition lr_requisition = lr_sw->lrow->requisition();
    const int64 rowheight = lr_requisition.height;
    critical_unless (rowheight > 0);
    const int64 row_apoint = rowheight * scroll_fraction;                       // row alignment point
    lr_sw->area.y = list_apoint - row_apoint;
    lr_sw->area.height = lr_requisition.height;
    lr_sw->area.x = list_area.x;
    lr_sw->area.width = list_area.width;
    lr_sw->allocated = true; // FIXME: remove field?
    rmap[scroll_widget] = lr_sw;
    first_row_ = last_row_ = scroll_widget;
  }
  // allocate rows above scroll_widget
  int64 accu = lr_sw->area.y;                                                   // upper pixel bound
  int64 current = scroll_widget - 1;
  while (current >= 0 && accu >= list_area.y)
    {
      ListRow *lr = fetch_row (current);
      const Requisition lr_requisition = lr->lrow->requisition();
      critical_unless (lr_requisition.height > 0);
      lr->area.height = lr_requisition.height;
      accu -= lr->area.height;
      lr->area.y = accu;
      lr->area.x = list_area.x;
      lr->area.width = list_area.width;
      lr->allocated = true;
      rmap[current] = lr;
      first_row_ = current--;
    }
  // allocate rows below scroll_widget
  accu = lr_sw->area.y + lr_sw->area.height;                                    // lower pixel bound
  current = scroll_widget + 1;
  while (current < mcount && accu < list_area.y + list_area.height)
    {
      ListRow *lr = fetch_row (current);
      const Requisition lr_requisition = lr->lrow->requisition();
      critical_unless (lr_requisition.height > 0);
      lr->area.height = lr_requisition.height;
      lr->area.y = accu;
      accu += lr->area.height;
      lr->area.x = list_area.x;
      lr->area.width = list_area.width;
      lr->allocated = true;
      rmap[current] = lr;
      last_row_ = current++;
    }
  // clean up remaining old rows and put new row map into place
  for (RowMap::iterator it = row_map_.begin(); it != row_map_.end(); it++)
    cache_row (it->second);
  row_map_.swap (rmap);
  // reactivate size-groups for proper size allocation
  for (uint i = 0; i < size_groups_.size(); i++)
    size_groups_[i]->active (true);
  // reset state
  need_scroll_layout_ = 0;
}

// determine y position for target_row, at vertical scroll position @a value
int
WidgetListImpl::vscroll_row_yoffset (const double value, const int target_row)
{
  const int mcount = model_->count();
  assert_return (target_row >= 0 && target_row < mcount, 0);
  const Allocation list_area = allocation();
  const double scroll_norm_value = value / mcount;                              // 0..1 scroll position
  const double scroll_value = scroll_norm_value * mcount;                       // fraction into count()
  const int scroll_widget = min (mcount - 1, ifloor (scroll_value));            // index into mcount at scroll_norm_value
  const double scroll_fraction = min (1.0, scroll_value - scroll_widget);       // fraction into scroll_widget row
  const int list_apoint = list_area.y + list_area.height * scroll_norm_value;   // list alignment point
  const int scroll_apoint_height = row_height (scroll_widget) * scroll_fraction; // inner row alignment point
  int current = scroll_widget;
  int current_y = list_apoint - scroll_apoint_height;
  // adjust for target_row > current
  while (target_row > current)
    {
      current_y += row_height (current);
      current++;
    }
  // adjust for target_row < current
  while (target_row < current)
    {
      current--;
      current_y -= row_height (current);
    }
  return current_y;
}

// determine target row when moving away from src_row by @a pixel_delta in either direction
int
WidgetListImpl::vscroll_relative_row (const int src_row, int pixel_delta)
{
  const int mcount = model_->count();
  int current = src_row;
  if (pixel_delta < 0)
    while (pixel_delta < 0 && current > 0)
      {
        current--;
        pixel_delta += row_height (current);
      }
  else // pixel_delta >= 0
    while (pixel_delta > 0 && current + 1 < mcount)
      {
        current++;
        pixel_delta -= row_height (current);
      }
  return current;
}

// find vertical value that aligns target_row most closely within the visible list area.
double
WidgetListImpl::vscroll_row_position (const int target_row, const double list_alignment)
{
  const int64 mcount = model_->count();
  assert_return (target_row < mcount, 0);
  const Allocation list_area = allocation();
  // determine scroll position bounds around target position
  double lower = vscroll_relative_row (target_row, -list_area.height);
  double upper = vscroll_relative_row (target_row, +list_area.height + row_height (target_row)) + 1.0;
  // calculate alignment points for vertical scroll layout
  const int list_apoint = list_area.y + list_area.height * list_alignment;      // list alignment point
  const int scroll_apoint_height = row_height (target_row) * list_alignment;    // inner target row alignment point
  // approximation start value, picked so it positions target_row in the visible list area
  double delta, value = target_row + 0.5;                                       // initial approximation
  // refine approximation via bisection
  do
    {
      const int target_y = vscroll_row_yoffset (value, target_row);
      const int target_apoint = target_y + scroll_apoint_height;                // target row alignment point
      if (target_apoint < list_apoint)
        upper = value;                          // scroll value must shrink so target_apoint grows
      else if (target_apoint > list_apoint)
        lower = value;                          // scroll value must grow so target_apoint shrinks
      else
        break;
      const double new_value = (lower + upper) / 2.0;
      delta = value - new_value;
      value = new_value;
    }
  while (fabs (delta) > 1 / (2.0 * row_height (min (mcount - 1, ifloor (value)))));
  return value;                                 // aproximation for positioning target_row alignment point at list_alignment
}

// == Pixel Accurate Scrolling ==
void
WidgetListImpl::pscroll_layout ()
{
  fatal ("UNIMPLEMENTED");
}

double
WidgetListImpl::pscroll_row_position (const int target_row, const double list_alignment)
{
  fatal ("UNIMPLEMENTED");
#if 0
  /* pixel positioning */
  int64 row, accu = 0;
  for (row = 0; row < list_row; row++)
    accu += ms.row_sizes[row];
  /* here: accu == real_scroll_position for list_alignment == 0 */
  accu += ms.row_sizes[row] * list_alignment;       // fractional row to align
  accu -= listheight * list_alignment;              // fractional list to align
  accu = max (0, accu);                             // clamp row0 to list start
  return accu;

  /* scroll position interpretation depends on the available vertical scroll
   * position resolution, which is derived from the number of pixels that
   * are (possibly) vertically allocated for the list view:
   * 1) Pixel size interpretation.
   *    All list rows are measured to determine their height. The fractional
   *    scroll position is then interpreted as a pointer into the total pixel
   *    height of the list.
   * Note that list rows increase downwards and pixel coordinates increase downwards.
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
#endif
}

} // Rapicorn
