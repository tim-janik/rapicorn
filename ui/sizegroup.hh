// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __RAPICORN_SIZE_GROUP_HH__
#define __RAPICORN_SIZE_GROUP_HH__

#include <ui/widget.hh>

namespace Rapicorn {

class WidgetGroup : public virtual ReferenceCountable {
  const String        name_;
  const WidgetGroupType type_;
protected:
  vector<WidgetImpl*> widgets_;
  explicit            WidgetGroup        (const String &name, WidgetGroupType type);
  virtual            ~WidgetGroup        ();
  virtual void        widget_transit     (WidgetImpl &widget) {}
  virtual void        widget_invalidated (WidgetImpl &widget) {}
public:
  typedef vector<WidgetGroup*> GroupVector;
  String              name              () const { return name_; }                  ///< Get WidgetGroup name
  WidgetGroupType     type              () const { return type_; }                  ///< Get WidgetGroup  type
  void                add_widget        (WidgetImpl &widget);                       ///< Add a widget to group
  void                remove_widget     (WidgetImpl &widget);                       ///< Remove a widget from group
  static void         delete_widget     (WidgetImpl &widget);
  static void         invalidate_widget (WidgetImpl &widget);
  static GroupVector  list_groups       (WidgetImpl &widget);                       ///< List all groups a widget has been added to
  static WidgetGroup* create            (const String &name, WidgetGroupType type); ///< Create WidgetGroup from @a name and @a type
};

/* --- SizeGroup --- */
class SizeGroup : public virtual WidgetGroup {
  friend              class WidgetImpl;
  friend              class WidgetGroup;
  Requisition         req_;
  uint                active_ : 1;
  uint                all_dirty_ : 1;        // need resize
  void                update            ();
  void                invalidate_sizes  ();
  explicit            SizeGroup                 (const String &name, WidgetGroupType type);
protected:
  void                size_request_widgets      (const vector<WidgetImpl*> widgets,
                                                 Requisition        &max_requisition);
  virtual Requisition group_requisition         ()                      { update(); return req_; }
  virtual void        widget_transit            (WidgetImpl &widget);
  virtual void        widget_invalidated        (WidgetImpl &widget);
  static Requisition  widget_requisition        (WidgetImpl &widget);
public:
  virtual bool        active                    () const        { return active_; }
  virtual void        active                    (bool isactive) { active_ = isactive; all_dirty_ = false; invalidate_sizes(); }
};

} // Rapicorn

#endif  /* __RAPICORN_SIZE_GROUP_HH__ */
