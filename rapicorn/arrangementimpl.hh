/* Rapicorn
 * Copyright (C) 2005 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * A copy of the GNU Lesser General Public License should ship along
 * with this library; if not, see http://www.gnu.org/copyleft/.
 */
#ifndef __RAPICORN_ARRANGEMENT_IMPL_HH__
#define __RAPICORN_ARRANGEMENT_IMPL_HH__

#include <rapicorn/arrangement.hh>
#include <rapicorn/containerimpl.hh>

namespace Rapicorn {

class ArrangementImpl : public virtual Arrangement, public virtual MultiContainerImpl {
  Point         m_origin;
  float         m_origin_hanchor;
  float         m_origin_vanchor;
  Rect          m_child_area;
public:
  explicit                      ArrangementImpl         ();
  virtual                       ~ArrangementImpl        ();
  virtual Point                 origin                  ()                      { return m_origin; }
  virtual void                  origin                  (Point p)               { m_origin = p; invalidate(); }
  virtual float                 origin_hanchor          ()                      { return m_origin_hanchor; }
  virtual void                  origin_hanchor          (float align)           { m_origin_hanchor = align; invalidate(); }
  virtual float                 origin_vanchor          ()                      { return m_origin_vanchor; }
  virtual void                  origin_vanchor          (float align)           { m_origin_vanchor = align; invalidate(); }
  virtual Rect                  child_area              ();
protected:
  virtual void                  size_request            (Requisition &requisition);
  virtual void                  size_allocate           (Allocation area);
  /* child location */
  struct Location {
    double pos_x, pos_y;
    float  pos_hanchor;
    float  pos_vanchor;
    explicit                    Location                ();
  };
  static Location               child_location          (Item &child);
  static void                   child_location          (Item &child, Location loc);
  static DataKey<Location>      child_location_key;
  Allocation                    local_child_allocation  (Item  &child,
                                                         double width,
                                                         double height);
  /* pack properties */
  class ArrangementPacker : public virtual ChildPacker {
    Item     &item;
    Location loc;
  public:
    explicit                    ArrangementPacker       (Item &citem);
    virtual                     ~ArrangementPacker      ();
    virtual
    const PropertyList&         list_properties         ();
    virtual void                update                  (); /* fetch real pack properties */
    virtual void                commit                  (); /* assign pack properties */
    Point                       position                ()        { return Point (loc.pos_x, loc.pos_y); }
    void                        position                (Point p) { loc.pos_x = p.x; loc.pos_y = p.y; }
    float                       hanchor                 () const { return loc.pos_hanchor; }
    void 		        hanchor                 (float align) { loc.pos_hanchor = CLAMP (align, 0, 1); }
    float                       vanchor                 () const { return loc.pos_vanchor; }
    void 		        vanchor                 (float align) { loc.pos_vanchor = CLAMP (align, 0, 1); }
  };
  virtual Packer                create_packer           (Item &item);
};

} // Rapicorn

#endif  /* __RAPICORN_ARRANGEMENT_IMPL_HH__ */
