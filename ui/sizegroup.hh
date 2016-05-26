// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#ifndef __RAPICORN_SIZE_GROUP_HH__
#define __RAPICORN_SIZE_GROUP_HH__

#include <ui/widget.hh>

namespace Rapicorn {

// == WidgetGroup ==
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
  virtual void        widget_invalidated (WidgetImpl &widget) {}
  virtual void        widget_toggled     (WidgetImpl &widget) {}
  virtual void        adding_widget      (WidgetImpl &widget) = 0;
  virtual void        removing_widget    (WidgetImpl &widget) = 0;
public:
  typedef vector<WidgetGroupP> GroupVector;
  String              name              () const { return name_; }                  ///< Get WidgetGroup name
  WidgetGroupType     type              () const { return type_; }                  ///< Get WidgetGroup  type
  void                add_widget        (WidgetImpl &widget);                       ///< Add a widget to group
  void                remove_widget     (WidgetImpl &widget);                       ///< Remove a widget from group
  static void         delete_widget     (WidgetImpl &widget);
  static void         invalidate_widget (WidgetImpl &widget);
  static void         toggled_widget    (WidgetImpl &widget);
  static WidgetGroupP create            (const String &name, WidgetGroupType type); ///< Create WidgetGroup from @a name and @a type
  static const GroupVector& list_groups (const WidgetImpl &widget);                 ///< List all groups a widget has been added to
};

// == RadioGroup ==
class RadioGroup;
typedef std::shared_ptr<RadioGroup> RadioGroupP;
typedef std::weak_ptr  <RadioGroup> RadioGroupW;

class RadioGroup : public virtual WidgetGroup {
  friend              class WidgetImpl;
  friend class        FriendAllocator<RadioGroup>;
  bool                updating_toggles_;
  explicit            RadioGroup                (const String &name, WidgetGroupType type);
protected:
  virtual void        adding_widget             (WidgetImpl &widget);
  virtual void        removing_widget           (WidgetImpl &widget);
  virtual void        widget_toggled            (WidgetImpl &widget);
public:
  static bool         widget_may_toggle         (const WidgetImpl &widget);
};

// == SizeGroup ==
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
  void                widget_transit            (WidgetImpl &widget);
protected:
  virtual void        adding_widget             (WidgetImpl &widget) { widget_transit (widget); }
  virtual void        removing_widget           (WidgetImpl &widget) { widget_transit (widget); }
  virtual void        widget_invalidated        (WidgetImpl &widget);
public:
  virtual bool        enabled                   () const         { return enabled_; }
  virtual void        enabled                   (bool isenabled) { enabled_ = isenabled; all_dirty_ = false; invalidate_sizes(); }
};

} // Rapicorn

#endif  /* __RAPICORN_SIZE_GROUP_HH__ */
