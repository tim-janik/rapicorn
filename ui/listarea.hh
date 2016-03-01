// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#ifndef __RAPICORN_LIST_AREA_HH__
#define __RAPICORN_LIST_AREA_HH__

#include <ui/adjustment.hh>
#include <ui/container.hh>
#include <ui/sizegroup.hh>
#include <ui/layoutcontainers.hh>
#include <ui/paintcontainers.hh>
#include <deque>

namespace Rapicorn {

class WidgetListImpl;

class WidgetListRowImpl : public virtual SingleContainerImpl,
                          public virtual WidgetListRowIface,
                          public virtual EventHandler {
  int             index_;
  WidgetListImpl* widget_list          () const;
protected:
  virtual void  construct              () override;
  virtual void  dump_private_data      (TestStream &tstream) override;
  virtual bool  handle_event           (const Event &event) override;
public:
  explicit      WidgetListRowImpl      ();
  virtual int   row_index              () const override;
  virtual void  row_index              (int i) override;
  virtual bool  selected               () const override;
  virtual void  selected               (bool s) override;
  virtual void  reset                  (ResetMode mode = RESET_ALL) override;
};
typedef std::shared_ptr<WidgetListRowImpl> WidgetListRowImplP;

class WidgetListImpl : public virtual MultiContainerImpl,
                       public virtual WidgetListIface,
                       public virtual AdjustmentSource,
                       public virtual EventHandler
{
  friend class WidgetListRowImpl;
  typedef vector<WidgetListRowImplP> WidgetRowVector;
  ListModelIfaceP        model_;
  String                 row_identifier_;
  vector<WidgetImplP>    widget_rows_; // FIXME: rename and merge w MultiContainer
  size_t                 conid_updated_;
  mutable AdjustmentP    hadjustment_, vadjustment_;
  vector<bool>           selection_;
  vector<WidgetGroupP>   size_groups_;
  SelectionMode          selection_mode_;
  bool                   block_invalidate_;
  int                   first_row_, last_row_, multi_sel_range_start_;
  uint                  insertion_cursor_;
  void                  model_updated           (const UpdateRequest &ur);
  void                  selection_changed       (int first, int length);
  virtual void          invalidate_parent       () override;
  WidgetListRowImpl*    get_widget_row          (uint64 idx)     { return idx < widget_rows_.size() // FIXME
                                                                                ? dynamic_cast<WidgetListRowImpl*> (widget_rows_[idx].get())
                                                                                : NULL; }
  void                  destroy_row             (uint64 index);
protected:
  void                  change_selection        (int current, int previous, bool toggle, bool range, bool preserve);
  bool                  key_press_event         (const EventKey &event);
  bool                  button_event            (const EventButton &event, WidgetListRowImpl *lrow, int index);
  virtual bool          handle_event            (const Event &event) override;
  virtual bool          row_event               (const Event &event, WidgetListRowImpl *lrow, int index) ;
  virtual void          reset                   (ResetMode mode) override;
  virtual bool          move_focus              (FocusDir fdir) override;
  virtual void          focus_lost              () override;
  virtual void          add_child               (WidgetImpl &widget) override;
  virtual void          remove_child            (WidgetImpl &widget) override;
  ssize_t               child_index             (WidgetImpl &widget) const;
  bool                  selected                (int row) { return size_t (row) < selection_.size() && selection_[row]; }
  void                  toggle_selected         (int row);
  void                  deselect_all            ();
public:
  // == WidgetListIface ==
  virtual SelectionMode                 selection_mode  () const override;
  virtual void                          selection_mode  (SelectionMode smode) override;
  virtual void                          set_selection   (const BoolSeq &selection) override;
  virtual BoolSeq                       get_selection   () override;
  virtual void                          select_range    (int first, int length) override;
  virtual void                          unselect_range  (int first, int length) override;
  virtual void                          bind_model      (ListModelIface &model, const String &row_identifier) override;
  virtual std::string                   row_identifier  () const override;
  virtual void                          row_identifier  (const std::string &row_identifier) override;
  virtual WidgetIfaceP                  create_row      (int64 index, const String &widget_identifier, const StringSeq &arguments) override;
  // == WidgetListImpl ==
  explicit              WidgetListImpl          ();
  virtual              ~WidgetListImpl          () override;
  bool                  validate_selection      (int fallback = 0);
  bool                  has_selection           () const;
  virtual void          hierarchy_changed       (WidgetImpl *old_toplevel) override;
  Adjustment&           hadjustment             () const;
  Adjustment&           vadjustment             () const;
  Adjustment*           get_adjustment          (AdjustmentSourceType adj_source,
                                                 const String        &name);
  void                  invalidate_model        (bool invalidate_heights,
                                                 bool invalidate_widgets);
  virtual void          size_request            (Requisition &requisition) override;
  virtual void          size_allocate           (Allocation area, bool changed) override;
  int                   focus_row               ();
  bool                  grab_row_focus          (int next_focus, int old_focus = -1);
  // == Layout implementations ==
  void                  layout_rows             ();
  void                  update_row              (uint64 index);
};

} // Rapicorn

#endif  /* __RAPICORN_LIST_AREA_HH__ */
