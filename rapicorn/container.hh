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
  friend              class Item;
  friend              class Root;
  void                uncross_descendant(Item          &descendant);
  void                item_cross_link   (Item           &owner,
                                         Item           &link,
                                         const ItemSlot &uncross);
  void                item_cross_unlink (Item           &owner,
                                         Item           &link,
                                         const ItemSlot &uncross);
  void                item_uncross_links(Item           &owner,
                                         Item           &link);
protected:
  virtual bool        match_interface   (InterfaceMatch &imatch,
                                         const String   &ident);
  virtual void        add_child         (Item           &item) = 0;
  virtual void        remove_child      (Item           &item) = 0;
  virtual void        unparent_child    (Item           &item);
  virtual void        dispose_item      (Item           &item);
  virtual void        hierarchy_changed (Item           *old_toplevel);
  virtual bool        move_focus        (FocusDirType    fdir);
  void                set_focus_child   (Item           *item);
  Item*               get_focus_child   ();
  void                expose_enclosure  (); /* expose without children */
public:
  typedef Walker<Item>  ChildWalker;
  void                  child_container (Container      *child_container);
  Container&            child_container ();
  virtual ChildWalker   local_children  () = 0;
  virtual bool          has_children    () = 0;
  void                  remove          (Item           &item);
  void                  remove          (Item           *item)  { if (item) remove (*item); else throw NullPointer(); }
  void                  add             (Item                   &item,
                                         const PackPropertyList &pack_plist = PackPropertyList(),
                                         PackPropertyList       *unused_props = NULL);
  void                  add             (Item                   *item,
                                         const PackPropertyList &pack_plist = PackPropertyList(),
                                         PackPropertyList       *unused_props = NULL);
  virtual Affine        child_affine    (Item                   &item);
  virtual
  const PropertyList&   list_properties (); /* essentially chaining to Item:: */
  const CommandList&    list_commands   (); /* essentially chaining to Item:: */
  virtual void          point_children  (Point                   p, /* item coordinates relative */
                                         std::vector<Item*>     &stack);
  void             root_point_children  (Point                   p, /* root coordinates relative */
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
    BIRNET_PRIVATE_CLASS_COPY (ChildPacker);
  };
  struct Packer {
    /*Con*/             Packer           (ChildPacker     *cp);
    /*Copy*/            Packer           (const Packer    &src);
    void                set_property     (const String    &property_name,
                                          const String    &value,
                                          const nothrow_t &nt = dothrow);
    String              get_property     (const String    &property_name);
    Property*           lookup_property  (const String    &property_name);
    void                apply_properties (const PackPropertyList &pplist,
                                          PackPropertyList       *unused_props = NULL);
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

} // Rapicorn

#endif  /* __RAPICORN_CONTAINER_HH__ */
