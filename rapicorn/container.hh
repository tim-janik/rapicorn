/* Rapicorn
 * Copyright (C) 2005 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#ifndef __RAPICORN_CONTAINER_HH__
#define __RAPICORN_CONTAINER_HH__

#include <rapicorn/item.hh>

namespace Rapicorn {

/* --- Container --- */
struct Container : public virtual Item {
  typedef std::map<String,String> PackPropertyList;
protected:
  virtual bool          match_interface (InterfaceMatch &imatch,
                                         const String   &ident);
  virtual bool          add_child       (Item                   &item,
                                         const PackPropertyList &pack_plist = PackPropertyList()) = 0;
  virtual void          remove_child    (Item &item) = 0;
  void                  hide_child      (Item &child) { child.set_flag (HIDDEN_CHILD, false); }
  void                  show_child      (Item &child) { child.set_flag (HIDDEN_CHILD, true); }
  virtual void          dispose_item    (Item &item);
public:
  typedef Walker<Item>  ChildWalker;
  void                  child_container (Container *child_container);
  Container&            child_container ();
  virtual ChildWalker   local_children  () = 0;
  virtual bool          has_children    () = 0;
  void                  add             (Item   &item, const PackPropertyList &pack_plist = PackPropertyList());
  void                  add             (Item   *item, const PackPropertyList &pack_plist = PackPropertyList());
  void                  remove          (Item   &item);
  void                  remove          (Item   *item)          { if (item) remove (*item); else throw NullPointer(); }
  virtual
  const PropertyList&   list_properties (); /* essentially item properties */
  virtual void          point_children  (double                  x,
                                         double                  y,
                                         Affine                  affine,
                                         std::vector<Item*>     &stack);
  virtual void          render          (Display                &display);
  void                  debug_tree      (String indent = String());
  /* child properties */
  struct ChildPacker : public virtual ReferenceCountImpl {
    virtual const PropertyList& list_properties  () = 0;
    virtual void                update           () = 0; /* fetch real pack properties */
    virtual void                commit           () = 0; /* assign pack properties */
    explicit                    ChildPacker      ();
  private:
    /*Copy*/                    ChildPacker      (const ChildPacker&);
    ChildPacker&                operator=        (const ChildPacker&);
  };
  struct Packer {
    /*Con*/             Packer           (ChildPacker     *cp);
    /*Copy*/            Packer           (const Packer    &src);
    void                set_property     (const String    &property_name,
                                          const String    &value,
                                          const nothrow_t &nt = dothrow);
    String              get_property     (const String    &property_name);
    Property*           lookup_property  (const String    &property_name);
    void                apply_properties (const PackPropertyList &pplist);
    const PropertyList& list_properties  ();
    /*Des*/             ~Packer          ();
  private:
    ChildPacker        *m_child_packer;
    ChildPacker&        operator=        (const Packer &src);
    friend              class Container;
  };
  Packer                child_packer    (Item   &item);
  Packer                child_packer    (Item   *item)          { if (item) return child_packer (*item); else throw NullPointer(); }
protected:
  virtual Packer        create_packer   (Item   &item) = 0;
  static ChildPacker*   void_packer     ();
  template<class PackerType>
  PackerType    extract_child_packer    (Packer &packer) { return dynamic_cast<PackerType> (packer.m_child_packer); }
};

/* --- Root --- */
class Root : public virtual Container {
protected:
  explicit      Root            ();
  virtual void  cancel_item_events      (Item               &item) = 0;
public:
  Signal<Container, void (const Allocation&)> sig_expose;
  virtual void  render                  (Plane &plane) = 0;
  /* events */
  virtual bool  dispatch_move_event     (const EventContext &econtext) = 0;
  virtual bool  dispatch_leave_event    (const EventContext &econtext) = 0;
  virtual bool  dispatch_button_event   (const EventContext &econtext,
                                         bool                is_press,
                                         uint                button) = 0;
  virtual bool  dispatch_focus_event    (const EventContext &econtext,
                                         bool                is_in) = 0;
  virtual bool  dispatch_key_event      (const EventContext &econtext,
                                         bool                is_press,
                                         KeyValue            key,
                                         const char         *key_name) = 0;
  virtual bool  dispatch_scroll_event   (const EventContext &econtext,
                                         EventType           scroll_type) = 0;
  virtual void  dispatch_cancel_events  () = 0;
  virtual void  add_grab                (Item               &child) = 0;
  void          add_grab                (Item               *child)     { throw_if_null (child); return add_grab (*child); }
  virtual void  remove_grab             (Item               &child) = 0;
  void          remove_grab             (Item               *child)     { throw_if_null (child); return remove_grab (*child); }
  virtual Item* get_grab                () = 0;
};

} // Rapicorn

#endif  /* __RAPICORN_CONTAINER_HH__ */
