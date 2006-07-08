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
#ifndef __RAPICORN_ITEM_IMPL_HH_
#define __RAPICORN_ITEM_IMPL_HH_

#include <rapicorn/item.hh>
#include <rapicorn/factory.hh>

namespace Rapicorn {

class ItemImpl : public virtual Item {
  Requisition           m_requisition;
  Allocation            m_allocation;
  String                m_name;
protected:
  virtual void          allocation      (const Allocation &area);
  /* signal methods */
  virtual void          do_invalidate   ();
  virtual void          do_changed      ();
  virtual bool          do_event        (const Event &event);
  using Item::expose;
  virtual void          expose          (const Allocation &area);
  using Item::size_request;
public:
  virtual String        name            () const;
  virtual void          name            (const String &str);
  virtual bool          point           (double     x,  /* global coordinate system */
                                         double     y,
                                         Affine     affine);
  virtual const Requisition& size_request           ();
  virtual const Allocation&  allocation             ();
  virtual bool               tune_requisition       (Requisition requisition);
  using                      Item::tune_requisition;
  virtual void               set_allocation         (const Allocation &area);
};

} // Rapicorn

#endif  /* __RAPICORN_ITEM_IMPL_HH_ */
