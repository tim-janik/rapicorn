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
      WidgetListRowImpl *row = list->get_widget_row (index_);
      if (this == row)
        kind = 1;
    }
  const char *knames[] = { "nil", "visible" };
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
  selection_mode_ (SelectionMode::SINGLE), block_invalidate_ (false),
  first_row_ (-1), last_row_ (-1), multi_sel_range_start_ (-1), insertion_cursor_ (0)
{}

WidgetListImpl::~WidgetListImpl()
{
  // remove model
  assert_return (model_ == NULL);
  // purge widget rows
  for (size_t j = 0; j < widget_rows_.size(); j++)
    destroy_row (widget_rows_.size() - 1 - j); // container.cc is faster destroying from end
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
WidgetListImpl::row_identifier (const String &row_identifier)
{
  row_identifier_ = row_identifier;
}

String
WidgetListImpl::row_identifier () const
{
  return row_identifier_;
}

void
WidgetListImpl::bind_model (ListModelIface &model, const String &row_identifier)
{
  ListModelIfaceP oldmodel = model_;
  model_ = shared_ptr_cast<ListModelIface> (&model);
  if (oldmodel)
    {
      oldmodel->sig_updated() -= conid_updated_;
      conid_updated_ = 0;
    }
  if (model_)
    {
      conid_updated_ = model_->sig_updated() += Aida::slot (*this, &WidgetListImpl::model_updated);
      row_identifier_ = row_identifier;
    }
  else
    row_identifier_ = "";
  invalidate_model (true, true);
  changed ("model");
}

WidgetIfaceP
WidgetListImpl::create_row (int64 index, const String &widget_identifier, const StringSeq &arguments)
{
  WidgetImplP widget = Factory::create_ui_child (*this, widget_identifier, arguments, false /*autoadd*/);
  if (widget)
    {
      if (index < 0)
        insertion_cursor_ = widget_rows_.size() + index; // e.g. -1 replaces last element
      else
        insertion_cursor_ = index;
      add (*widget);
    }
  return widget;
}

/// Find index of @a widget in this container or return -1.
ssize_t
WidgetListImpl::child_index (WidgetImpl &widget) const
{
  for (size_t i = 0; i < widget_rows_.size(); i++)
    if (widget_rows_[i].get() == &widget)
      return i;
  return -1;
}

void
WidgetListImpl::add_child (WidgetImpl &widget)
{
  critical_unless (widget.parent() == NULL); // unparented
  MultiContainerImpl::add_child (widget);
  if (widget.parent() == this) // parented to self
    {
      insertion_cursor_ = std::max (0U, insertion_cursor_);
      insertion_cursor_ = std::min (size_t (insertion_cursor_), widget_rows_.size()); // e.g. MAXINT appends
      if (insertion_cursor_ >= widget_rows_.size())
        widget_rows_.resize (widget_rows_.size() + 1);
      widget_rows_[insertion_cursor_] = shared_ptr_cast<WidgetImpl> (&widget);
      insertion_cursor_ += 1;
    }
}

void
WidgetListImpl::remove_child (WidgetImpl &widget)
{
  MultiContainerImpl::remove_child (widget);
  const ssize_t index = child_index (widget);
  if (index >= 0 && widget.parent() == this) // parented
    {
      widget_rows_[index] = NULL;
    }
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
      // destroy_range (urequest.rowspan.start, ~size_t (0));
      // row_heights_.resize (model_->count(), -1);
      invalidate_model (true, true);
      break;
    case UpdateKind::CHANGE:
      for (int64 i = urequest.rowspan.start; i < urequest.rowspan.start + urequest.rowspan.length; i++)
        update_row (i);
      break;
    case UpdateKind::DELETION:
      // destroy_range (urequest.rowspan.start, ~size_t (0));
      // row_heights_.resize (model_->count(), -1);
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
      WidgetListRowImpl *row = get_widget_row (i);
      if (row)
        row->selected (selected (i));
    }
}

void
WidgetListImpl::invalidate_model (bool invalidate_heights, bool invalidate_widgets)
{
  // FIXME: reset all cached row_heights_ here?
  invalidate();
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
  requisition.height = 0;
  for (auto child : widget_rows_)
    {
      if (!child || !child->visible())
        continue;
      const Requisition crq = measure_child (*child);
      requisition.width = MAX (requisition.width, crq.width);
      requisition.height += crq.height;
      chspread |= child->hspread();
      cvspread |= child->vspread();
    }
  set_flag (HSPREAD_CONTAINER, chspread);
  set_flag (VSPREAD_CONTAINER, cvspread);
}

void
WidgetListImpl::size_allocate (Allocation area, bool changed)
{
  const Allocation list_area = allocation();
  int64 list_y = list_area.y;
  for (auto child : widget_rows_)
    {
      if (!child || !child->visible())
        continue;
      const Requisition crq = measure_child (*child);
      Allocation carea;
      carea.x = list_area.x;
      carea.width = list_area.width;
      carea.y = list_y;
      carea.height = crq.height;
      list_y += carea.height;
      carea = layout_child (*child, carea); // handle spacing/alignment
      child->set_allocation (carea, &list_area);
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
  WidgetListRowImpl *row = get_widget_row (next_focus);
  bool success;
  if (row && row->grab_focus())                 // assign new focus
    success = true;
  else
    {
      row = get_widget_row (old_focus);
      if (row)
        row->unset_focus();                     // or no row gets focus
      success = false;
    }
  const int current_focus = success ? focus_row () : -1;
  if (success && row)
    {
      // scroll_to_child (row);
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
          const int jumprow = 0 * delta; // FIXME: vscroll_relative_row (current_focus, -MAX (0, delta)) + 1;
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
          const int jumprow = 0 * delta; // vscroll_relative_row (current_focus, +MAX (0, delta)) - 1;
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

void
WidgetListImpl::destroy_row (uint64 idx)
{
  return_unless (idx < widget_rows_.size());
  WidgetImplP row = widget_rows_[idx];
  if (row)
    {
      WidgetImplP widget = row; // keep a reference across remove()
      widget_rows_[idx] = NULL;
      ContainerImpl *parent = widget->parent();
      if (parent)
        parent->remove (*widget);
    }
}

void
WidgetListImpl::update_row (uint64 index)
{
  // bail out if there's no model to update from
  return_unless (model_ != NULL);
  const uint64 mcount = model_->count();
  return_unless (index < mcount);
  // ensure the row has a widget
  HBoxImpl *hbox;
  if (widget_rows_.size() < mcount)
    widget_rows_.resize (mcount);
  WidgetListRowImpl* row = get_widget_row (index);
  if (!row)
    {
      Any vany = model_->row (index);
      WidgetImplP widget = Factory::create_ui_child (*this, row_identifier_, Factory::ArgumentList(), false);
      if (widget)
        widget_rows_[index] = widget;
      row = get_widget_row (index);
      if (!row)
        {
          user_warning (UserSource ("Rapicorn", __FILE__, __LINE__), "%s: failed to create list row widget: %s", __func__, row_identifier_); // FIXME
          return; // FIXME: add UI error widget?
        }
      hbox = row->interface<HBoxImpl*>();
      if (hbox)
        {
          hbox->spacing (5); // FIXME
          while (size_groups_.size() < hbox->n_children())
            size_groups_.push_back (WidgetGroup::create (" Rapicorn.WidgetListImpl.SizeGroup-HORIZONTAL", WIDGET_GROUP_HSIZE));
          size_t i = 0;
          for (auto descendant : *hbox)
            size_groups_[i++]->add_widget (*descendant);
        }
      add (*row);
    }
  else
    hbox = row->interface<HBoxImpl*>();
  // update column contents
  if (row->row_index() != ssize_t (index))
    row->row_index (index);
  if (hbox)
    {
      Any dat = model_->row (index);
      for (auto descendant : *hbox)
        descendant->set_property ("markup_text", dat.to_string());
      AmbienceIface *ambience = row->interface<AmbienceIface*>();
      if (ambience)
        ambience->background (index & 1 ? "background-odd" : "background-even");
      row->selected (selected (index));
    }
}

void
WidgetListImpl::reset (ResetMode mode)
{
  // first_row_ = last_row_ = current_row_ = MIN_INT;
}

} // Rapicorn
