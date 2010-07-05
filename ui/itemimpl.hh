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
#ifndef __RAPICORN_ITEM_IMPL_HH_
#define __RAPICORN_ITEM_IMPL_HH_

#include <ui/item.hh>
#include <ui/factory.hh>

namespace Rapicorn {

class ItemImpl : public virtual Item {
  Requisition           m_requisition;
  Allocation            m_allocation;
  FactoryContext       *m_factory_context;
protected:
  ItemImpl() : m_factory_context (NULL) { /* removing this breaks g++ pre-4.2.0 20060530 */ }
  virtual void          allocation      (const Allocation &area);
  /* signal methods */
  virtual void          do_invalidate   ();
  virtual void          do_changed      ();
  virtual bool          do_event        (const Event &event);
  virtual String        name            () const;
  virtual void          name            (const String &str);
  virtual FactoryContext*    factory_context        () const;
  virtual void               factory_context        (FactoryContext *fc);
  virtual ColorSchemeType    color_scheme           () const;
  virtual void               color_scheme           (ColorSchemeType cst);
  virtual const Allocation&  allocation             ();
  virtual Requisition        cache_requisition      (Requisition *requisition);
  virtual bool               tune_requisition       (Requisition requisition);
  using                      Item::tune_requisition;
  virtual void               set_allocation         (const Allocation &area);
};

} // Rapicorn

#endif  /* __RAPICORN_ITEM_IMPL_HH_ */
