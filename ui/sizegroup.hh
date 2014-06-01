// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __RAPICORN_SIZE_GROUP_HH__
#define __RAPICORN_SIZE_GROUP_HH__

#include <ui/widget.hh>

namespace Rapicorn {

class WidgetGroup : public virtual ReferenceCountable {
  virtual            ~WidgetGroup       ();
  explicit            WidgetGroup       (const String &name, WidgetGroupType type);
  const String        name_;
  const WidgetGroupType type_;
  vector<WidgetImpl*> widgets_;
public:
  typedef vector<WidgetGroup*> GroupVector;
  String              name          () const { return name_; }                  ///< Get WidgetGroup name
  WidgetGroupType     type          () const { return type_; }                  ///< Get WidgetGroup  type
  void                add_widget    (WidgetImpl &widget);                       ///< Add a widget to group
  void                remove_widget (WidgetImpl &widget);                       ///< Remove a widget from group
  static GroupVector  list_groups   (WidgetImpl &widget);                       ///< List all groups a widget has been added to
  static WidgetGroup* create        (const String &name, WidgetGroupType type); ///< Create WidgetGroup from @a name and @a type
};

/* --- SizeGroup --- */
class SizeGroup : public virtual ReferenceCountable {
  friend            class ClassDoctor;
  friend            class WidgetImpl;
protected:
  void                size_request_widgets (const vector<WidgetImpl*> widgets,
                                          Requisition        &max_requisition);
  virtual Requisition group_requisition  () = 0;
  static void         delete_widget        (WidgetImpl &widget);
  static void         invalidate_widget    (WidgetImpl &widget);
  static Requisition  widget_requisition   (WidgetImpl &widget);
public:
  static SizeGroup*   create_hgroup      ();
  static SizeGroup*   create_vgroup      ();
  virtual bool        active             () const = 0;
  virtual void        active             (bool isactive) = 0;
  virtual void        add_widget           (WidgetImpl &widget) = 0;
  virtual void        remove_widget        (WidgetImpl &widget) = 0;
};

} // Rapicorn

#endif  /* __RAPICORN_SIZE_GROUP_HH__ */
