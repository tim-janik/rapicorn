// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#ifndef __RAPICORN_SIZE_GROUP_HH__
#define __RAPICORN_SIZE_GROUP_HH__

#include <ui/widget.hh>

namespace Rapicorn {

class WidgetGroup;
typedef std::shared_ptr<WidgetGroup> WidgetGroupP;
typedef std::weak_ptr  <WidgetGroup> WidgetGroupW;

class WidgetGroup : public virtual std::enable_shared_from_this<WidgetGroup> {
  friend class        FriendAllocator<WidgetGroup>;
  const String        name_;
  const WidgetGroupType type_;
protected:
  vector<WidgetImpl*> widgets_;
  explicit            WidgetGroup        (const String &name, WidgetGroupType type);
  virtual            ~WidgetGroup        ();
  virtual void        widget_transit     (WidgetImpl &widget) {}
  virtual void        widget_invalidated (WidgetImpl &widget) {}
public:
  typedef vector<WidgetGroupP> GroupVector;
  String              name              () const { return name_; }                  ///< Get WidgetGroup name
  WidgetGroupType     type              () const { return type_; }                  ///< Get WidgetGroup  type
  void                add_widget        (WidgetImpl &widget);                       ///< Add a widget to group
  void                remove_widget     (WidgetImpl &widget);                       ///< Remove a widget from group
  static void         delete_widget     (WidgetImpl &widget);
  static void         invalidate_widget (WidgetImpl &widget);
  static GroupVector  list_groups       (WidgetImpl &widget);                       ///< List all groups a widget has been added to
  static WidgetGroupP create            (const String &name, WidgetGroupType type); ///< Create WidgetGroup from @a name and @a type
};

/* --- SizeGroup --- */
class SizeGroup;
typedef std::shared_ptr<SizeGroup> SizeGroupP;
typedef std::weak_ptr  <SizeGroup> SizeGroupW;

class SizeGroup : public virtual WidgetGroup {
  friend              class WidgetImpl;
  friend class        FriendAllocator<SizeGroup>;
  Requisition         req_;
  uint                enabled_ : 1;
  uint                all_dirty_ : 1;        // need resize
  const Requisition&  group_requisition         ();
  void                invalidate_sizes          ();
  explicit            SizeGroup                 (const String &name, WidgetGroupType type);
  static Requisition  widget_requisition        (WidgetImpl &widget);   // called by WidgetImpl
protected:
  virtual void        widget_transit            (WidgetImpl &widget);
  virtual void        widget_invalidated        (WidgetImpl &widget);
public:
  virtual bool        enabled                   () const         { return enabled_; }
  virtual void        enabled                   (bool isenabled) { enabled_ = isenabled; all_dirty_ = false; invalidate_sizes(); }
};

} // Rapicorn

#endif  /* __RAPICORN_SIZE_GROUP_HH__ */
