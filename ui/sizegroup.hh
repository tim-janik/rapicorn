// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __RAPICORN_SIZE_GROUP_HH__
#define __RAPICORN_SIZE_GROUP_HH__

#include <ui/item.hh>

namespace Rapicorn {

/* --- SizeGroup --- */
class SizeGroup : public virtual BaseObject {
  friend            class ClassDoctor;
  friend            class ItemImpl;
protected:
  void                size_request_items (const vector<ItemImpl*> items,
                                          Requisition        &max_requisition);
  virtual Requisition group_requisition  () = 0;
  static void         delete_item        (ItemImpl &item);
  static void         invalidate_item    (ItemImpl &item);
  static Requisition  item_requisition   (ItemImpl &item);
public:
  static SizeGroup*   create_hgroup      ();
  static SizeGroup*   create_vgroup      ();
  virtual bool        active             () const = 0;
  virtual void        active             (bool isactive) = 0;
  virtual void        add_item           (ItemImpl &item) = 0;
  virtual void        remove_item        (ItemImpl &item) = 0;
};

} // Rapicorn

#endif  /* __RAPICORN_SIZE_GROUP_HH__ */
