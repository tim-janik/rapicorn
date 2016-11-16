// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#ifndef __RAPICORN_LIST_AREA_HH__
#define __RAPICORN_LIST_AREA_HH__

#include <ui/container.hh>
#include <ui/sizegroup.hh>
#include <ui/layoutcontainers.hh>
#include <ui/paintcontainers.hh>
#include <deque>

namespace Rapicorn {

class SelectableItemImpl : public virtual SingleContainerImpl,
                           public virtual SelectableItemIface,
                           public virtual EventHandler {
  int32 button_;
  virtual bool  widget_maybe_selected  () const override;
protected:
  virtual void  construct              () override;
  virtual bool  capture_event          (const Event &event) override;
  virtual bool  handle_event           (const Event &event) override;
public:
  explicit      SelectableItemImpl     ();
  virtual int32 button                 () const override;
  virtual void  button                 (int32 b) override;
  virtual void  reset                  (ResetMode mode = RESET_ALL) override;
};
typedef std::shared_ptr<SelectableItemImpl> SelectableItemImplP;


class WidgetListImpl : public virtual MultiContainerImpl,
                       public virtual WidgetListIface,
                       public virtual EventHandler
{
  ListModelIfaceP        model_;
  String                 row_identifier_;
  vector<WidgetImplP>    widget_rows_; // FIXME: rename and merge w MultiContainer
  size_t                 conid_updated_;
  vector<WidgetGroupP>   size_groups_;
  uint                  selection_changed_freeze_ : 20;
  uint                  selection_mode_ : 8;
  uint                  selection_changed_pending_ : 1;
  int                   first_row_, last_row_, multi_sel_range_start_;
  uint                  insertion_cursor_;
  void                  model_updated           (const UpdateRequest &ur);
  WidgetImpl*           get_row_widget          (uint64 idx) const { return idx < widget_rows_.size() ? widget_rows_[idx].get() : NULL; }
  int64                 row_widget_index        (WidgetImpl &widget);
  void                  destroy_row             (uint64 index);
  void                  row_select_range        (size_t first, size_t length, bool selected);
  void                  row_select              (uint64 idx, bool selected) { row_select_range (idx, 1, selected); }
  bool                  row_selected            (uint64 idx) const;
  bool                  validate_selection      (int fallback = 0);
protected:
  class SelectionChangedGuard {
    WidgetListImpl &wlist_;
  public:
    SelectionChangedGuard  (WidgetListImpl&);
    ~SelectionChangedGuard ();
  };
  void                  notify_selection_changed();
  void                  deselect_rows           ();
  void                  change_selection        (int current, int previous, bool toggle, bool range, bool preserve);
  bool                  key_press_event         (const EventKey &event);
  bool                  button_event            (const EventButton &event, WidgetImpl *row, int index);
  virtual void          hierarchy_changed       (WindowImpl *old_toplevel) override;
  virtual void          size_request            (Requisition &requisition) override;
  virtual void          size_allocate           (Allocation area) override;
  virtual bool          handle_event            (const Event &event) override;
  virtual bool          row_event               (const Event &event, WidgetImpl *row);
  virtual void          reset                   (ResetMode mode) override;
  virtual bool          move_focus              (FocusDir fdir) override;
  virtual void          focus_lost              () override;
  virtual void          add_child               (WidgetImpl &widget) override;
  virtual void          remove_child            (WidgetImpl &widget) override;
  virtual void          selectable_child_changed (WidgetChain &chain) override;
  ssize_t               child_index             (WidgetImpl &widget) const;
  void                  update_row              (uint64 index);
  void                  invalidate_model        (bool invalidate_heights, bool invalidate_widgets);
public:
  // == WidgetListIface ==
  virtual SelectionMode                 selection_mode  () const override;
  virtual void                          selection_mode  (SelectionMode smode) override;
  virtual void                          set_selection   (const BoolSeq &selection) override;
  virtual BoolSeq                       get_selection   () override;
  virtual void                          select_range    (int first, int length) override { row_select_range (first, length, true); }
  virtual void                          unselect_range  (int first, int length) override { row_select_range (first, length, false); }
  virtual void                          bind_model      (ListModelIface &model, const String &row_identifier) override;
  virtual std::string                   row_identifier  () const override;
  virtual void                          row_identifier  (const std::string &row_identifier) override;
  virtual WidgetIfaceP                  create_row      (int64 index, const String &widget_identifier, const StringSeq &arguments) override;
  // == WidgetListImpl ==
  explicit              WidgetListImpl          ();
  virtual              ~WidgetListImpl          () override;
  int                   focus_row               ();
  bool                  grab_row_focus          (int next_focus, int old_focus = -1);
};

} // Rapicorn

#endif  /* __RAPICORN_LIST_AREA_HH__ */
