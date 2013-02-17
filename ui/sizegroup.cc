// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#include "sizegroup.hh"

namespace Rapicorn {

struct ClassDoctor {
  static void item_set_parent (ItemImpl &item, ContainerImpl *parent) { item.set_parent (parent); }
};

class SizeGroupImpl : public SizeGroup {
  Requisition           req_;
  vector<ItemImpl*>     items_;
public:
  uint                  hgroup_ : 1;
  uint                  vgroup_ : 1;
private:
  uint                  active_ : 1;
  uint                  all_dirty_ : 1;        // need resize
  virtual       ~SizeGroupImpl          ();
  virtual void   add_item               (ItemImpl &item);
  virtual void   remove_item            (ItemImpl &item);
  void           update                 ();
public:
  virtual bool   active                 () const        { return active_; }
  virtual void   active                 (bool isactive) { active_ = isactive; all_dirty_ = false; invalidate_sizes(); }
  void           invalidate_sizes       ();
  explicit                      SizeGroupImpl     (bool hgroup, bool vgroup);
  static vector<SizeGroupImpl*> list_groups       (ItemImpl &item);
  virtual Requisition           group_requisition () { update(); return req_; }
};

SizeGroupImpl::SizeGroupImpl (bool hgroup,
                              bool vgroup) :
  hgroup_ (hgroup), vgroup_ (vgroup),
  active_ (true), all_dirty_ (false)
{}

SizeGroupImpl::~SizeGroupImpl ()
{
  assert (items_.size() == 0);
}

static DataKey<vector<SizeGroupImpl*> > size_group_key;

vector<SizeGroupImpl*>
SizeGroupImpl::list_groups (ItemImpl &item)
{
  return item.get_data (&size_group_key);
}

void
SizeGroupImpl::add_item (ItemImpl &item)
{
  /* add item to size group's list */
  items_.push_back (&item);
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
SizeGroupImpl::remove_item (ItemImpl &item)
{
  ref (this);
  if (active())
    {
      invalidate_sizes();
      item.invalidate_size();
    }
  /* remove item from size group's list */
  bool found_one = false;
  for (uint i = 0; i < items_.size(); i++)
    if (items_[i] == &item)
      {
        items_.erase (items_.begin() + i);
        found_one = true;
        unref (this);
        break;
      }
  if (!found_one)
    critical ("%s: attempt to remove unknown item (%s) from size group: %p", STRFUNC, item.name().c_str(), this);
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
    critical ("%s: attempt to remove unknown size group (%p) from item: %s", STRFUNC, this, item.name().c_str());
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
  while (all_dirty_)
    {
      all_dirty_ = false;
      req_ = Requisition();
      size_request_items (items_, req_);
    }
}

void
SizeGroupImpl::invalidate_sizes()
{
  if (all_dirty_)
    return;
  all_dirty_ = true;
  for (uint i = 0; i < items_.size(); i++)
    items_[i]->invalidate_size();
}

void
SizeGroup::size_request_items (const vector<ItemImpl*> items,
                               Requisition        &max_requisition)
{
  for (uint i = 0; i < items.size(); i++)
    {
      ItemImpl &item = *items[i];
      Requisition ireq = item.inner_size_request();
      max_requisition.width = MAX (max_requisition.width, ireq.width);
      max_requisition.height = MAX (max_requisition.height, ireq.height);
    }
}

void
SizeGroup::delete_item (ItemImpl &item)
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
SizeGroup::invalidate_item (ItemImpl &item)
{
  /* invalidate all active groups */
  vector<SizeGroupImpl*> sgl = SizeGroupImpl::list_groups (item);
  for (uint i = 0; i < sgl.size(); i++)
    if (sgl[i]->active())
      sgl[i]->invalidate_sizes();
}

Requisition
SizeGroup::item_requisition (ItemImpl &item)
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
          if (sgl[i]->hgroup_)
            zreq.width = MAX (zreq.width, gr.width);
          if (sgl[i]->vgroup_)
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
