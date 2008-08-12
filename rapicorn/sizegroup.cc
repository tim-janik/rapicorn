/* Rapicorn
 * Copyright (C) 2008 Tim Janik
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
#include "sizegroup.hh"

namespace Rapicorn {

struct ClassDoctor {
  static void item_set_parent (Item &item, Container *parent) { item.set_parent (parent); }
};

class SizeGroupImpl : public SizeGroup {
  Requisition           m_req;
  vector<Item*>         m_items;
public:
  uint                  m_hgroup : 1;
  uint                  m_vgroup : 1;
private:
  uint                  m_active : 1;
  uint                  m_all_dirty : 1;        // need resize
  virtual       ~SizeGroupImpl          ();
  virtual void   add_item               (Item &item);
  virtual void   remove_item            (Item &item);
  void           update                 ();
public:
  virtual bool   active                 () const        { return m_active; }
  virtual void   active                 (bool isactive) { m_active = isactive; m_all_dirty = false; invalidate_sizes(); }
  void           invalidate_sizes       ();
  explicit                      SizeGroupImpl     (bool hgroup, bool vgroup);
  static vector<SizeGroupImpl*> list_groups       (Item &item);
  virtual Requisition           group_requisition () { update(); return m_req; }
};

SizeGroupImpl::SizeGroupImpl (bool hgroup,
                              bool vgroup) :
  m_hgroup (hgroup), m_vgroup (vgroup),
  m_active (true), m_all_dirty (false)
{}

SizeGroupImpl::~SizeGroupImpl ()
{
  assert (m_items.size() == 0);
}

static DataKey<vector<SizeGroupImpl*> > size_group_key;

vector<SizeGroupImpl*>
SizeGroupImpl::list_groups (Item &item)
{
  return item.get_data (&size_group_key);
}

void
SizeGroupImpl::add_item (Item &item)
{
  /* add item to size group's list */
  m_items.push_back (&item);
  ref (this);
  /* add size group to item's list */
  vector<SizeGroupImpl*> szv = item.get_data (&size_group_key);
  szv.push_back (this);
  item.set_data (&size_group_key, szv);
  if (active())
    {
      invalidate_sizes();
      item.invalidate_size();
    }
}

void
SizeGroupImpl::remove_item (Item &item)
{
  ref (this);
  if (active())
    {
      invalidate_sizes();
      item.invalidate_size();
    }
  /* remove item from size group's list */
  bool found_one = false;
  for (uint i = 0; i < m_items.size(); i++)
    if (m_items[i] == &item)
      {
        m_items.erase (m_items.begin() + i);
        found_one = true;
        unref (this);
        break;
      }
  if (!found_one)
    warning ("%s: attempt to remove unknown item (%s) from size group: %p", STRFUNC, item.name().c_str(), this);
  /* remove size group from item's list */
  found_one = false;
  vector<SizeGroupImpl*> szv = item.get_data (&size_group_key);
  for (uint i = 0; i < szv.size(); i++)
    if (szv[i] == this)
      {
        szv.erase (szv.begin() + i);
        found_one = true;
        break;
      }
  if (!found_one)
    warning ("%s: attempt to remove unknown size group (%p) from item: %s", STRFUNC, this, item.name().c_str());
  if (szv.size() == 0)
    item.delete_data (&size_group_key);
  else
    item.set_data (&size_group_key, szv);
  unref (this);
}

SizeGroup*
SizeGroup::create_hgroup ()
{
  return new SizeGroupImpl (true, false);
}

SizeGroup*
SizeGroup::create_vgroup ()
{
  return new SizeGroupImpl (false, true);
}

void
SizeGroupImpl::update()
{
  while (m_all_dirty)
    {
      m_all_dirty = false;
      m_req = Requisition();
      size_request_items (m_items, m_req);
    }
}

void
SizeGroupImpl::invalidate_sizes()
{
  if (m_all_dirty)
    return;
  m_all_dirty = true;
  for (uint i = 0; i < m_items.size(); i++)
    m_items[i]->invalidate_size();
}

void
SizeGroup::size_request_items (const vector<Item*> items,
                               Requisition        &max_requisition)
{
  for (uint i = 0; i < items.size(); i++)
    {
      Item &item = *items[i];
      Requisition ireq = item.inner_size_request();
      max_requisition.width = MAX (max_requisition.width, ireq.width);
      max_requisition.height = MAX (max_requisition.height, ireq.height);
    }
}

void
SizeGroup::delete_item (Item &item)
{
  /* remove from all groups */
  vector<SizeGroupImpl*> sgl = SizeGroupImpl::list_groups (item);
  for (uint i = 0; i < sgl.size(); i++)
    {
      SizeGroup &sg = *sgl[i];
      sg.remove_item (item);
    }
}

void
SizeGroup::invalidate_item (Item &item)
{
  /* invalidate all active groups */
  vector<SizeGroupImpl*> sgl = SizeGroupImpl::list_groups (item);
  for (uint i = 0; i < sgl.size(); i++)
    if (sgl[i]->active())
      sgl[i]->invalidate_sizes();
}

Requisition
SizeGroup::item_requisition (Item &item)
{
  /* size request all active groups */
  Requisition zreq; // 0,0
  if (item.allocatable())
    {
      vector<SizeGroupImpl*> sgl = SizeGroupImpl::list_groups (item);
      for (uint i = 0; i < sgl.size(); i++)
        {
          Requisition gr = sgl[i]->group_requisition();
          if (!sgl[i]->active())
            continue;
          if (sgl[i]->m_hgroup)
            zreq.width = MAX (zreq.width, gr.width);
          if (sgl[i]->m_vgroup)
            zreq.height = MAX (zreq.height, gr.height);
        }
    }
  /* size request ungrouped/unallocatable items */
  Requisition ireq = item.inner_size_request();
  /* determine final requisition */
  ireq.width = MAX (zreq.width, ireq.width);
  ireq.height = MAX (zreq.height, ireq.height);
  return ireq;
}

} // Rapicorn
