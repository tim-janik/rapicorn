// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __RAPICORN_LIST_AREA_IMPL_HH__
#define __RAPICORN_LIST_AREA_IMPL_HH__

#include <ui/listarea.hh>
#include <ui/container.hh>
#include <ui/adjustment.hh>
#include <ui/layoutcontainers.hh>
#include <ui/table.hh>
#include <ui/models.hh>
#include <deque>

namespace Rapicorn {

class WidgetListRowImpl;

struct ListRow {
  vector<WidgetImpl*> cols; // FIXME
  WidgetListRowImpl *lrow;
  Allocation     area;
  uint           allocated : 1;
  ListRow() : lrow (NULL), allocated (0) {}
};

class WidgetListImpl : public virtual MultiContainerImpl,
                       public virtual WidgetList,
                       public virtual AdjustmentSource,
                       public virtual EventHandler
{
  friend class WidgetListRowImpl;
  typedef map<int64,ListRow*>  RowMap;
  typedef std::deque<int>      SizeQueue;
  ListModelIface        *model_;
  size_t                 conid_updated_;
  vector<int>            row_heights_;
  mutable Adjustment    *hadjustment_, *vadjustment_;
  RowMap                 row_map_, row_cache_;
  vector<bool>           selection_;
  vector<SizeGroup*>     size_groups_;
  bool                   virtualized_pixel_scrolling_;
  bool                   need_scroll_layout_;
  bool                   block_invalidate_;
  int                   first_row_, last_row_, multi_sel_range_start_;
  void                  model_updated           (const UpdateRequest &ur);
  void                  selection_changed       (int first, int last);
  virtual void          invalidate_parent ();
protected:
  void                  change_selection        (int previous, int current, bool toggle, bool range, bool preserve);
  virtual bool          key_press_event         (const EventKey &event);
  virtual bool          handle_event            (const Event    &event);
  virtual bool          row_event               (const Event    &event, WidgetListRowImpl *lrow, int index);
  virtual void          reset                   (ResetMode       mode);
  virtual bool          can_focus               () const { return true; }
  SelectionMode         selection_mode          () { return SELECTION_MULTIPLE; }
  bool                  selected                (int row) { return size_t (row) < selection_.size() && selection_[row]; }
  void                  toggle_selected         (int row);
  void                  deselect_all            ();
public:
  explicit              WidgetListImpl            ();
  virtual              ~WidgetListImpl            ();
  virtual bool          browse                  () const        { return 0; } // FIXME
  virtual void          browse                  (bool b)        { (void) b; invalidate(); } // FIXME
  virtual void          model                   (const String &modelurl);
  virtual String        model                   () const;
  virtual void          hierarchy_changed       (WidgetImpl *old_toplevel);
  Adjustment&           hadjustment             () const;
  Adjustment&           vadjustment             () const;
  Adjustment*           get_adjustment          (AdjustmentSourceType adj_source,
                                                 const String        &name);
  void                  invalidate_model        (bool invalidate_heights,
                                                 bool invalidate_widgets);
  virtual void          visual_update           ();
  virtual void          size_request            (Requisition &requisition);
  virtual void          size_allocate           (Allocation area, bool changed);
  int                   focus_row               ();
  /* sizing and positioning */
  int                   row_height              (int            nth_row);
  void                  scroll_layout_preserving();
  void                  cache_row               (ListRow *lr);
  void                  destroy_row             (ListRow *lr);
  void                  destroy_range           (size_t first, size_t bound);
  void                  fill_row                (ListRow *lr, int row);
  ListRow*              create_row              (uint64 row,
                                                 bool   with_size_groups = true);
  ListRow*              lookup_row              (int    row, bool maybe_cached = true);
  ListRow*              fetch_row               (int    row);
  void                  update_row              (int    row);
  // == Scrolling Implementation ==
  void          scroll_layout           ()                              { return virtualized_pixel_scrolling_ ? vscroll_layout() : pscroll_layout(); }
  double        scroll_row_position     (const int r, const double a)   { return virtualized_pixel_scrolling_ ? vscroll_row_position (r, a) : pscroll_row_position (r, a); }
  // == Virtualized Scrolling ==
  void          vscroll_layout          ();
  double        vscroll_row_position    (const int target_row, const double list_alignment);
  int           vscroll_row_yoffset     (const double value, const int target_row);
  int           vscroll_relative_row    (const int src_row, int pixel_delta);
  // == Pixel Accurate Scrolling ==
  void          pscroll_layout          ();
  double        pscroll_row_position    (const int target_row, const double list_alignment);
};

} // Rapicorn

#endif  /* __RAPICORN_LIST_AREA_IMPL_HH__ */
