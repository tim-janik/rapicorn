// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#include "listarea.hh"
#include "factory.hh"
#include "application.hh"

//#define IFDEBUG(...)      do { /*__VA_ARGS__*/ } while (0)
#define IFDEBUG(...)      __VA_ARGS__

namespace Rapicorn {

// == WidgetListRowImpl ==
WidgetListImpl*
WidgetListRowImpl::widget_list () const
{
  return dynamic_cast<WidgetListImpl*> (parent());
}

void
WidgetListRowImpl::dump_private_data (TestStream &tstream)
{
  WidgetListImpl *list = widget_list();
  int kind = 0;
  if (list && index_ >= 0)
    {
      ListRow *lr = list->row_map_[index_];
      if (lr && this == &*lr->lrow)
        kind = 1;
      else
        {
          lr = list->off_map_[index_];
          if (lr && this == &*lr->lrow)
            kind = 2;
          else
            kind = 3;
        }
    }
  const char *knames[] = { "nil", "mapped", "cached", "dangling" };
  tstream.dump ("row_kind", String (knames[kind]));
}

bool
WidgetListRowImpl::handle_event (const Event &event)
{
  WidgetListImpl *list = widget_list();
  return list && list->row_event (event, this, index_);
}

WidgetListRowImpl::WidgetListRowImpl() :
  index_ (INT_MIN)
{
  color_scheme (ColorScheme::BASE);
}

void
WidgetListRowImpl::construct ()
{
  set_flag (NEEDS_FOCUS_INDICATOR); // prerequisite for focusable
  set_flag (ALLOW_FOCUS);
  SingleContainerImpl::construct();
}

int
WidgetListRowImpl::row_index() const
{
  return index_;
}

void
WidgetListRowImpl::row_index (int i)
{
  index_ = i >= 0 ? i : INT_MIN;
  WidgetListImpl *list = widget_list();
  if (list && index_ >= 0)
    color_scheme (list->selected (index_) ? ColorScheme::SELECTED : ColorScheme::BASE);
  visible (index_ >= 0);
  changed ("row_index");
}

bool
WidgetListRowImpl::selected () const
{
  WidgetListImpl *list = widget_list();
  return list && index_ >= 0 ? list->selected (index_) : false;
}

void
WidgetListRowImpl::selected (bool s)
{
  WidgetListImpl *list = widget_list();
  if (list && index_ >= 0)
    {
      if (list->selected (index_) != s)
        {
          list->toggle_selected (index_);
          changed ("selected");
        }
      color_scheme (list->selected (index_) ? ColorScheme::SELECTED : ColorScheme::BASE);
    }
}

void
WidgetListRowImpl::reset (ResetMode mode)
{}

static const WidgetFactory<WidgetListRowImpl> widget_list_row_factory ("Rapicorn::WidgetListRow");


// == WidgetListImpl ==
static const WidgetFactory<WidgetListImpl> widget_list_factory ("Rapicorn::WidgetList");

WidgetListImpl::WidgetListImpl() :
  model_ (NULL), conid_updated_ (0),
  hadjustment_ (NULL), vadjustment_ (NULL),
  selection_mode_ (SelectionMode::SINGLE), virtualized_pixel_scrolling_ (true),
  need_scroll_layout_ (false), block_invalidate_ (false),
  first_row_ (-1), last_row_ (-1), multi_sel_range_start_ (-1)
{}

WidgetListImpl::~WidgetListImpl()
{
  // remove model
  model ("");
  assert_return (model_ == NULL);
  // purge row map
  RowMap rc;
  row_map_.swap (rc);
  for (auto ri : rc)
    destroy_row (ri.second);
  // purge row cache
  rc.clear();
  rc.swap (off_map_);
  for (auto ri : rc)
    destroy_row (ri.second);
  // release size groups
  while (size_groups_.size())
    {
      WidgetGroupP sg = size_groups_.back();
      size_groups_.pop_back();
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
    case AdjustmentSourceType::ANCESTRY_HORIZONTAL:
      return &hadjustment();
    case AdjustmentSourceType::ANCESTRY_VERTICAL:
      return &vadjustment();
    default:
      return NULL;
    }
}

void
WidgetListImpl::model (const String &modelurl)
{
  critical ("deprecated property does not work: WidgetListImpl::model");
}

String
WidgetListImpl::model () const
{
  return ""; // FIME: property deprectaed
}

void
WidgetListImpl::set_list_model (ListModelIface &model)
{
  ListModelIfaceP oldmodel = model_;
  model_ = shared_ptr_cast<ListModelIface> (&model);
  row_heights_.clear();
  if (oldmodel)
    {
      oldmodel->sig_updated() -= conid_updated_;
      conid_updated_ = 0;
      row_heights_.clear();
    }
  if (model_)
    {
      row_heights_.resize (model_->count(), -1);
      conid_updated_ = model_->sig_updated() += Aida::slot (*this, &WidgetListImpl::model_updated);
    }
  invalidate_model (true, true);
  changed ("model");
}

SelectionMode
WidgetListImpl::selection_mode () const
{
  return selection_mode_;
}

void
WidgetListImpl::selection_mode (SelectionMode smode)
{
  if (selection_mode_ != smode)
    {
      selection_mode_ = smode;
      validate_selection (0);
      selection_changed (0, selection_.size());
      changed ("selection_mode");
    }
}

void
WidgetListImpl::set_selection (const BoolSeq &bseq)
{
  size_t lastrow = 0, firstrow = ~size_t (0);
  const size_t max_size = min (bseq.size(), selection_.size());
  for (size_t i = 0; i < max_size; i++)
    if (bseq[i] != selection_[i])
      {
        selection_[i] = bseq[i];
        firstrow = min (firstrow, i);
        lastrow = max (lastrow, i);
      }
  if (!validate_selection (firstrow))
    {
      firstrow = 0;
      lastrow = selection_.size() - 1;
    }
  if (firstrow <= lastrow)
    selection_changed (firstrow, 1 + lastrow - firstrow);
}

BoolSeq
WidgetListImpl::get_selection ()
{
  BoolSeq bseq;
  bseq.resize (selection_.size());
  std::copy (selection_.begin(), selection_.end(), bseq.begin());
  return bseq;
}

void
WidgetListImpl::select_range (int first, int length)
{
  return_unless (first >= 0 && length >= 0);
  size_t lastrow = 0, firstrow = ~size_t (0);
  const size_t max_size = min (size_t (first + length), selection_.size());
  for (size_t i = first; i < max_size; i++)
    if (!selection_[i])
      {
        selection_[i] = true;
        firstrow = min (firstrow, i);
        lastrow = max (lastrow, i);
      }
  if (!validate_selection (firstrow))
    {
      firstrow = 0;
      lastrow = selection_.size() - 1;
    }
  if (firstrow <= lastrow)
    selection_changed (firstrow, 1 + lastrow - firstrow);
}

void
WidgetListImpl::unselect_range (int first, int length)
{
  return_unless (first >= 0 && length >= 0);
  size_t lastrow = 0, firstrow = ~size_t (0);
  const size_t max_size = min (size_t (first + length), selection_.size());
  for (size_t i = first; i < max_size; i++)
    if (selection_[i])
      {
        selection_[i] = false;
        firstrow = min (firstrow, i);
        lastrow = max (lastrow, i);
      }
  if (!validate_selection (firstrow))
    {
      firstrow = 0;
      lastrow = selection_.size() - 1;
    }
  if (firstrow <= lastrow)
    selection_changed (firstrow, 1 + lastrow - firstrow);
}

bool
WidgetListImpl::validate_selection (int fallback)
{
  // ensure a valid selection
  bool changed = false;
  int first = -1;
  switch (selection_mode())
    {
    case SelectionMode::NONE:                // nothing to select ever
      for (size_t i = 0; i < selection_.size(); i++)
        if (selection_[i])
          {
            selection_[i] = false;
            changed = true;
          }
      break;
    case SelectionMode::SINGLE:              // maintain a single selection at most
    case SelectionMode::BROWSE:              // always maintain a single selection
      for (size_t i = 0; i < selection_.size(); i++)
        if (selection_[i])
          {
            if (first < 0)
              first = i;
            else
              {
                selection_[i] = 0;
                changed = true;
              }
          }
      if (selection_mode() == SelectionMode::BROWSE && first < 0 && selection_.size() > 0)
        {
          selection_[CLAMP (fallback, 0, ssize_t (selection_.size() - 1))] = true;
          changed = true;
        }
      break;
    case SelectionMode::MULTIPLE:            // allow any combination of selected rows
      break;
    }
  return changed == false;
}

bool
WidgetListImpl::has_selection () const
{
  // OPTIMIZE: speed up by always caching first & last selected row
  for (size_t i = 0; i < selection_.size(); i++)
    if (selection_[i])
      return true;
  return false;
}

void
WidgetListImpl::model_updated (const UpdateRequest &urequest)
{
  switch (urequest.kind)
    {
    case UpdateKind::READ:
      break;
    case UpdateKind::INSERTION:
      destroy_range (urequest.rowspan.start, ~size_t (0));
      row_heights_.resize (model_->count(), -1);
      invalidate_model (true, true);
      break;
    case UpdateKind::CHANGE:
      for (int64 i = urequest.rowspan.start; i < urequest.rowspan.start + urequest.rowspan.length; i++)
        update_row (i);
      break;
    case UpdateKind::DELETION:
      destroy_range (urequest.rowspan.start, ~size_t (0));
      row_heights_.resize (model_->count(), -1);
      invalidate_model (true, true);
      break;
    }
}

void
WidgetListImpl::toggle_selected (int row)
{
  if (selection_.size() <= size_t (row))
    selection_.resize (row + 1);
  selection_[row] = !selection_[row];
  selection_changed (row, 1 + row);
}

void
WidgetListImpl::deselect_all ()
{
  selection_.assign (selection_.size(), 0);
  if (selection_.size())
    selection_changed (0, selection_.size());
}

void
WidgetListImpl::selection_changed (int first, int length)
{
  for (int i = first; i < first + length; i++)
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
  requisition.width = 0;
  requisition.height = -1;
  for (auto ri : row_map_)
    {
      ListRow *lr = ri.second;
      critical_unless (lr->lrow->visible());
      const Requisition crq = lr->lrow->requisition();
      requisition.width = MAX (requisition.width, crq.width);
      chspread = cvspread = false;
    }
  if (model_ && model_->count())
    requisition.height = -1;  // FIXME: allow property to specify how many rows should be visible
  else
    requisition.height = -1;
  if (requisition.height < 0)
    requisition.height = 12 * 5; // FIXME: request single label height
  set_flag (HSPREAD_CONTAINER, chspread);
  set_flag (VSPREAD_CONTAINER, cvspread);
}

void
WidgetListImpl::size_allocate (Allocation area, bool changed)
{
  need_scroll_layout_ = need_scroll_layout_ || allocation() != area || changed;
  if (need_scroll_layout_)
    vscroll_layout();
  for (auto ri : row_map_)
    {
      ListRow *lr = ri.second;
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
WidgetListImpl::grab_row_focus (int next_focus, int old_focus)
{
  const int64 mcount = model_ ? model_->count() : 0;
  ListRow *lr = lookup_row (next_focus, false);
  bool success;
  if (lr)
    {
      success = lr->lrow->grab_focus();         // focus onscreen row
    }
  else
    {
      lr = fetch_row (next_focus);
      if (lr)                                   // new focus row was offscreen
        {
          lr->lrow->visible (true);
          success = lr->lrow->grab_focus();
          cache_row (lr);
        }
      else                                      // no row gets focus
        {
          lr = lookup_row (old_focus);
          if (lr)
            lr->lrow->unset_focus();
          success = false;
        }
    }
  const int current_focus = success ? focus_row () : -1;
  if (success && current_focus >= 0)
    {                                           // scroll to focus row
      double vscrolllower = vscroll_row_position (current_focus, 1.0); // lower scrollpos for current at visible bottom
      double vscrollupper = vscroll_row_position (current_focus, 0.0); // upper scrollpos for current at visible top
      // fixup possible approximation error in first/last pixel via edge attraction
      if (vscrollupper <= 1)                    // edge attraction at top
        vscrolllower = vscrollupper = 0;
      else if (vscrolllower >= mcount - 1)      // edge attraction at bottom
        vscrolllower = vscrollupper = mcount;
      const double nvalue = CLAMP (vadjustment_->nvalue(), vscrolllower / mcount, vscrollupper / mcount);
      if (nvalue != vadjustment_->nvalue())
        vadjustment_->nvalue (nvalue);
    }
  return success && current_focus >= 0;
}

void
WidgetListImpl::focus_lost ()
{
  // not resetting focus_child so we memorize focus child on next focus in
}

bool
WidgetListImpl::move_focus (FocusDir fdir)
{
  // check focus ability
  if (!visible() || !sensitive())
    return false;
  // allow last focus descendant to handle movement
  WidgetImpl *last_child = get_focus_child();
  if (last_child && last_child->move_focus (fdir))      // refocus or row internal move_focus
    return true;
  // pick row for initial focus
  if (!test_any_flag (uint64 (WidgetState::FOCUSED)))
    {
      const int last_focus = focus_row();               // -1 initially
      return grab_row_focus (MAX (0, last_focus));      // list focus-in
    }
  // focus out for FocusDir::PREV and FocusDir::NEXT, cursor focus is handled via events
  return false;                                         // list focus-out
}

void
WidgetListImpl::change_selection (const int current, int previous, const bool toggle, const bool range, const bool preserve)
{
  const int64 mcount = model_->count();
  return_unless (mcount > 0);
  return_unless (previous < mcount);
  return_unless (current < mcount);
  switch (selection_mode())
    {
      int sel, old;
    case SelectionMode::NONE:                // nothing to select ever
      break;
    case SelectionMode::BROWSE:              // always maintain a single selection
      sel = MAX (0, current >= 0 ? current : previous);
      old = focus_row();
      if (sel != old)
        {
          if (old >= 0 && selected (old))
            toggle_selected (old);
          if (!selected (sel))
            toggle_selected (sel);
        }
      break;
    case SelectionMode::SINGLE:              // maintain a single selection at most
      if (!preserve || toggle)
        {
          sel = selected (current) ? current : -1;
          deselect_all();
          if (current >= 0 && (!toggle || sel < 0))
            toggle_selected (current);
        }
      break;
    case SelectionMode::MULTIPLE:            // allow any combination of selected rows
      if (!preserve)
        deselect_all();
      if (current < 0)
        ;               // nothing to select
      else if (toggle)
        toggle_selected (current);
      else if (range)
        {
          if (multi_sel_range_start_ < 0)
            multi_sel_range_start_ = previous >= 0 ? previous : current;
          for (int i = MAX (0, MIN (multi_sel_range_start_, current)); i <= MAX (multi_sel_range_start_, current); i++)
            if (!selected (i))
              toggle_selected (i);
        }
      else if (!preserve)
        toggle_selected (current);
      if (!range)
        multi_sel_range_start_ = -1;
      break;
    }
}

bool
WidgetListImpl::key_press_event (const EventKey &event)
{
  const int64 mcount = model_->count();
  bool handled = false;
  bool preserve_old_selection = event.key_state & MOD_CONTROL;
  bool toggle_selection = false, range_selection = event.key_state & MOD_SHIFT;
  int current_focus = focus_row();
  const int saved_current_row = current_focus;
  switch (event.key)
    {
    case KEY_space:
      if (current_focus >= 0 && current_focus < mcount)
        {
          toggle_selection = true;
          range_selection = false;
        }
      handled = true;
      break;
    case KEY_Down:
      if (!preserve_old_selection && current_focus >= 0 && !selected (current_focus))
        toggle_selection = true;        // treat like space
      else if (current_focus + 1 < mcount)
        current_focus = MAX (current_focus + 1, 0);
      handled = true;
      break;
    case KEY_Up:
      if (!preserve_old_selection && current_focus >= 0 && !selected (current_focus))
        toggle_selection = true;        // treat like space
      else if (current_focus > 0)
        current_focus = current_focus - 1;
      handled = true;
      break;
    case KEY_Page_Up:
      if (first_row_ >= 0 && last_row_ >= first_row_ && last_row_ < mcount)
        {
          // See KEY_Page_Down comment.
          const Allocation list_area = allocation();
          const int delta = list_area.height; // - row_height (current_focus) - 1;
          const int jumprow = vscroll_relative_row (current_focus, -MAX (0, delta)) + 1;
          current_focus = CLAMP (MIN (jumprow, current_focus - 1), 0, mcount - 1);
        }
      handled = true;
      break;
    case KEY_Page_Down:
      if (first_row_ >= 0 && last_row_ >= first_row_ && last_row_ < mcount)
        {
          /* Ideally, jump to a new row by a single screenful of pixels, but: never skip rows by
           * jumping more than a screenful of pixels, keep in mind that the view will be aligned
           * to fit the target row fully. Also make sure to jump by at least single row so we keep
           * moving regardless of view height (which might be less than current_row height).
           */
          const Allocation list_area = allocation();
          const int delta = list_area.height; //  - row_height (current_focus) - 1;
          const int jumprow = vscroll_relative_row (current_focus, +MAX (0, delta)) - 1;
          current_focus = CLAMP (MAX (jumprow, current_focus + 1), 0, mcount - 1);
        }
      handled = true;
      break;
    }
  if (handled)
    {
      grab_row_focus (current_focus, saved_current_row);
      change_selection (current_focus, saved_current_row, toggle_selection, range_selection, preserve_old_selection);
    }
  return handled;
}

bool
WidgetListImpl::button_event (const EventButton &event, WidgetListRowImpl *lrow, int index)
{
  bool handled = false;
  bool preserve_old_selection = event.key_state & MOD_CONTROL;
  bool toggle_selection = event.key_state & MOD_CONTROL, range_selection = event.key_state & MOD_SHIFT;
  int current_focus = index;
  const int saved_current_row = current_focus;
  if (event.type == BUTTON_PRESS && event.button == 1 && lrow)
    {
      grab_row_focus (current_focus, saved_current_row);
      change_selection (current_focus, saved_current_row, toggle_selection, range_selection, preserve_old_selection);
      handled = true;
    }
  return handled;
}

bool
WidgetListImpl::row_event (const Event &event, WidgetListRowImpl *lrow, int index)
{
  return_unless (model_ != NULL, false);
  const int64 mcount = model_->count();
  return_unless (mcount > 0, false);
  bool handled = false;
  switch (event.type)
    {
    case KEY_PRESS:
      handled = key_press_event (*dynamic_cast<const EventKey*> (&event));
      break;
    case BUTTON_PRESS:
      handled = button_event (*dynamic_cast<const EventButton*> (&event), lrow, index);
      break;
    default:
      break;
    }
  return handled;
}

bool
WidgetListImpl::handle_event (const Event &event)
{
  return_unless (model_ != NULL, false);
  bool handled = false;
  switch (event.type)
    {
    case KEY_PRESS:
    case KEY_RELEASE:
      handled = row_event (event, NULL, -1);
      break;
    default:
      break;
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
      const bool keep_invisible = !lr->lrow->visible();
      if (keep_invisible)                                       // FIXME: resetting visible is very expensive
        lr->lrow->visible (true); // proper requisition need visible row
      row_heights_[nth_row] = lr->lrow->requisition().height;
      if (keep_invisible)
        lr->lrow->visible (false); // restore state
      if (!keep_uncached)
        cache_row (lr);
    }
  return row_heights_[nth_row];
}

static uint dbg_cached = 0, dbg_created = 0;

void
WidgetListImpl::fill_row (ListRow *lr, int nthrow)
{
  assert_return (lr->lrow->row_index() == nthrow);
  Any row = model_->row (nthrow);
  for (size_t i = 0; i < lr->cols.size(); i++)
    lr->cols[i]->set_property ("markup_text", row.to_string());
  AmbienceIface *ambience = lr->lrow->interface<AmbienceIface*>();
  if (ambience)
    ambience->background (nthrow & 1 ? "background-odd" : "background-even");
  lr->lrow->selected (selected (nthrow));
  lr->allocated = 0;
}

ListRow*
WidgetListImpl::create_row (uint64 nthrow, bool with_size_groups)
{
  Any row = model_->row (nthrow);
  ListRow *lr = new ListRow();
  IFDEBUG (dbg_created++);
  WidgetImplP widget = Factory::create_ui_child (*this, "WidgetListRow", Factory::ArgumentList(), false);
  assert (widget != NULL);
  lr->lrow = shared_ptr_cast<WidgetListRowImpl> (widget);
  assert (lr->lrow != NULL);
  lr->lrow->interface<HBoxIface>().spacing (5); // FIXME
  widget = Factory::create_ui_child (*lr->lrow, "Label", Factory::ArgumentList());
  assert (widget != NULL);
  lr->cols.push_back (widget);

  while (size_groups_.size() < lr->cols.size())
    size_groups_.push_back (WidgetGroup::create (" internal WidgetListImpl SizeGroup HSIZE", WIDGET_GROUP_HSIZE));
  if (with_size_groups)
    for (uint i = 0; i < lr->cols.size(); i++)
      size_groups_[i]->add_widget (*lr->cols[i]);

  add (*lr->lrow);
  return lr;
}

ListRow*
WidgetListImpl::fetch_row (int row)
{
  return_unless (row >= 0, NULL);
  ListRow *lr;
  RowMap::iterator ri;
  if (row_map_.end() != (ri = row_map_.find (row)))             // fetch visible row
    {
      lr = ri->second;
      row_map_.erase (ri);
      IFDEBUG (dbg_cached++);
    }
  else if (off_map_.end() != (ri = off_map_.find (row)))    // fetch invisible row
    {
      lr = ri->second;
      off_map_.erase (ri);
      change_unviewable (*lr->lrow, false);
      IFDEBUG (dbg_cached++);
    }
  else                                                          // create row
    {
      lr = create_row (row);
      lr->lrow->row_index (row); // visible=true
      fill_row (lr, row);
    }
  return lr;
}

void
WidgetListImpl::destroy_row (ListRow *lr)
{
  assert_return (lr && lr->lrow);
  ContainerImpl *parent = lr->lrow->parent();
  if (parent)
    parent->remove (*lr->lrow);
  delete lr;
}

void
WidgetListImpl::cache_row (ListRow *lr)
{
  const int64 mcount = model_ ? model_->count() : 0;
  const int row_index = lr->lrow->row_index();
  assert_return (row_index >= 0);
  assert_return (off_map_.find (row_index) == off_map_.end());
  if (row_index >= mcount)
    destroy_row (lr);
  else
    {
      change_unviewable (*lr->lrow, true);      // take widget offscreen
      off_map_[row_index] = lr;
      lr->allocated = 0;
    }
  // prune if we have too many items
  if (off_map_.size() > MAX (20, 2 * row_map_.size()))
    {
      const int threshold = MAX (20, row_map_.size());
      const int first = first_row_ - threshold / 2, last = last_row_ + threshold / 2;
      RowMap newmap;
      for (auto ri : off_map_)
        if ((ri.first < first || ri.first > last) && !ri.second->lrow->has_focus())
          {
            destroy_row (ri.second);
            row_heights_[ri.first] = -1;
          }
        else
          newmap[ri.first] = ri.second;
      newmap.swap (off_map_);
    }
}

void
WidgetListImpl::destroy_range (size_t first, size_t bound)
{
  for (auto rmap : { &row_map_, &off_map_ })          // remove range from row_map_ *and* off_map_
    {
      RowMap newmap;
      for (auto ri : *rmap)
        if (ri.first < ssize_t (first) || ri.first >= ssize_t (bound))
          newmap[ri.first] = ri.second;                 // keep row
        else                                            // or get rid of it
          destroy_row (ri.second);
      newmap.swap (*rmap);                              // assign updated row map
    }
  for (auto i = first; i < min (bound, row_heights_.size()); i++)
    row_heights_[i] = -1;
}

ListRow*
WidgetListImpl::lookup_row (int row, bool maybe_cached)
{
  return_unless (row >= 0, NULL);
  RowMap::iterator ri;
  if (row_map_.end() != (ri = row_map_.find (row)))
    return ri->second;
  if (maybe_cached && off_map_.end() != (ri = off_map_.find (row)))
    return ri->second;
  return NULL;
}

void
WidgetListImpl::update_row (int row)
{
  return_unless (row >= 0);
  ListRow *lr = lookup_row (row);
  if (lr)
    fill_row (lr, row);
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
  const int64 mcount = model_ ? model_->count() : 0;
  if (mcount == 0)
    {
      destroy_range (0, ~size_t (0));
      return;
    }
  RowMap rmap;
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
  ListRow *lr_sw = NULL;
  if (1)
    {
      ListRow *lr = fetch_row (scroll_widget);
      const Requisition lr_requisition = lr->lrow->requisition();
      critical_unless (lr_requisition.height > 0);                              // or do max(1) ?
      const int64 rowheight = lr_requisition.height;
      const int64 row_apoint = rowheight * scroll_fraction;                     // row alignment point
      Allocation carea;
      carea.height = lr_requisition.height;
      carea.y = list_apoint - row_apoint;
      carea.x = list_area.x;
      carea.width = list_area.width;
      if (carea != lr->area)
        {
          lr->area = carea;
          lr->lrow->invalidate (INVALID_ALLOCATION);
        }
      rmap[scroll_widget] = lr;
      first_row_ = last_row_ = scroll_widget;
      lr_sw = lr;
    }
  // allocate rows above scroll_widget
  int64 accu = lr_sw->area.y;                                                   // upper pixel bound
  int64 current = scroll_widget - 1;
  while (current >= 0 && accu >= list_area.y)
    {
      ListRow *lr = fetch_row (current);
      const Requisition lr_requisition = lr->lrow->requisition();
      critical_unless (lr_requisition.height > 0);                              // or do max(1) ?
      Allocation carea;
      carea.height = lr_requisition.height;
      accu -= carea.height;
      carea.y = accu;
      carea.x = list_area.x;
      carea.width = list_area.width;
      if (carea != lr_sw->area)
        {
          lr->area = carea;
          lr->lrow->invalidate (INVALID_ALLOCATION);
        }
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
      critical_unless (lr_requisition.height > 0);                              // or do max(1) ?
      Allocation carea;
      carea.height = lr_requisition.height;
      carea.y = accu;
      accu += carea.height;
      carea.x = list_area.x;
      carea.width = list_area.width;
      if (carea != lr_sw->area)
        {
          lr->area = carea;
          lr->lrow->invalidate (INVALID_ALLOCATION);
        }
      rmap[current] = lr;
      last_row_ = current++;
    }
  // clean up remaining old rows and put new row map into place
  for (RowMap::iterator it = row_map_.begin(); it != row_map_.end(); it++)
    cache_row (it->second);
  row_map_.swap (rmap);
  // ensure proper allocation for all rows
  for (auto ri : row_map_)
    {
      ListRow *lr = ri.second;
      if (lr->lrow->test_any_flag (INVALID_REQUISITION))
        lr->lrow->requisition();                                // shouldn't happen, done above
      assert (lr->lrow->test_any_flag (INVALID_REQUISITION) == 0 || this->test_any_flag (INVALID_REQUISITION));
      if (lr->lrow->test_any_flag (INVALID_ALLOCATION))
        lr->lrow->set_allocation (lr->area, &list_area);
      assert (lr->lrow->test_any_flag (INVALID_ALLOCATION) == 0 || this->test_any_flag (INVALID_ALLOCATION));
      assert (lr->lrow->test_any_flag (INVALID_REQUISITION) == 0 || this->test_any_flag (INVALID_REQUISITION));
    }
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
