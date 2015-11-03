// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#include "sizegroup.hh"

namespace Rapicorn {

// == WidgetGroup ==
WidgetGroupP
WidgetGroup::create (const String &name, WidgetGroupType type)
{
  assert_return (name.empty() == false, NULL);
  if (type == WIDGET_GROUP_HSIZE || type == WIDGET_GROUP_VSIZE)
    return FriendAllocator<SizeGroup>::make_shared (name, type);
  else
    return FriendAllocator<WidgetGroup>::make_shared (name, type);
}

WidgetGroup::WidgetGroup (const String &name, WidgetGroupType type) :
  name_ (name), type_ (type)
{}

WidgetGroup::~WidgetGroup()
{
  assert (widgets_.size() == 0);
}

static DataKey<WidgetGroup::GroupVector> widget_group_key;

vector<WidgetGroupP>
WidgetGroup::list_groups (WidgetImpl &widget)
{
  return widget.get_data (&widget_group_key);
}

void
WidgetGroup::add_widget (WidgetImpl &widget)
{
  // add widget to group's list
  widgets_.push_back (&widget);
  // add group to widget's list
  GroupVector wgv = widget.get_data (&widget_group_key);
  wgv.push_back (shared_ptr_cast<WidgetGroup> (this));
  widget.set_data (&widget_group_key, wgv);
  widget_transit (widget);
}

void
WidgetGroup::remove_widget (WidgetImpl &widget)
{
  widget_transit (widget);
  // remove widget from group's list
  bool found_one = false;
  for (uint i = 0; i < widgets_.size(); i++)
    if (widgets_[i] == &widget)
      {
        widgets_.erase (widgets_.begin() + i);
        found_one = true;
        break;
      }
  if (!found_one)
    {
      critical ("attempt to remove unknown widget (%s) from group: %p", widget.name().c_str(), this);
      return;
    }
  // remove group from widget's list */
  found_one = false;
  GroupVector wgv = widget.get_data (&widget_group_key);
  for (uint i = 0; i < wgv.size(); i++)
    if (this == &*wgv[i])
      {
        wgv.erase (wgv.begin() + i);
        found_one = true;
        break;
      }
  if (!found_one)
    fatal ("failed to remove size group (%p) from widget: %s", this, widget.name().c_str());
  if (wgv.size() == 0)
    widget.delete_data (&widget_group_key);
  else
    widget.set_data (&widget_group_key, wgv);
}

void
WidgetGroup::delete_widget (WidgetImpl &widget)
{
  // remove from all groups
  GroupVector wgl = list_groups (widget);
  for (size_t i = 0; i < wgl.size(); i++)
    wgl[i]->remove_widget (widget);
}

void
WidgetGroup::invalidate_widget (WidgetImpl &widget)
{
  // invalidate all enabled groups
  GroupVector wgl = list_groups (widget);
  for (size_t i = 0; i < wgl.size(); i++)
    wgl[i]->widget_invalidated (widget);
}

// == SizeGroup ==
SizeGroup::SizeGroup (const String &name, WidgetGroupType type) :
  WidgetGroup (name, type),
  enabled_ (true), all_dirty_ (false)
{}

void
SizeGroup::widget_invalidated (WidgetImpl &widget)
{
  invalidate_sizes();
}

void
SizeGroup::widget_transit (WidgetImpl &widget)
{
  invalidate_sizes();
  if (enabled())
    widget.invalidate_size();
}

void
SizeGroup::invalidate_sizes()
{
  if (enabled())
    {
      if (all_dirty_)
        return;
      all_dirty_ = true;
      for (size_t i = 0; i < widgets_.size(); i++)
        widgets_[i]->invalidate_size();
    }
}

const Requisition&
SizeGroup::group_requisition ()
{
  while (all_dirty_)
    {
      all_dirty_ = false;
      req_ = Requisition();
      // beware, widgets_ vector _could_ change during loop
      for (size_t i = 0; i < widgets_.size(); i++)
        {
          WidgetImpl &widget = *widgets_[i];
          if (widget.visible())
            {
              const Requisition ireq = widget.inner_size_request();
              req_.width = MAX (req_.width, ireq.width);
              req_.height = MAX (req_.height, ireq.height);
            }
        }
    }
  return req_;
}

Requisition
SizeGroup::widget_requisition (WidgetImpl &widget)
{
  // size request all enabled groups
  Requisition zreq; // 0,0
  if (widget.visible())
    {
      GroupVector wgl = list_groups (widget);
      for (size_t i = 0; i < wgl.size(); i++)
        if (wgl[i]->type() == WIDGET_GROUP_HSIZE || wgl[i]->type() == WIDGET_GROUP_VSIZE)
          {
            SizeGroupP sg = shared_ptr_cast<SizeGroup> (wgl[i]);
            if (!sg->enabled())
              continue;
            Requisition gr = sg->group_requisition();
            if (sg->type() == WIDGET_GROUP_HSIZE)
              zreq.width = MAX (zreq.width, gr.width);
            if (sg->type() == WIDGET_GROUP_VSIZE)
              zreq.height = MAX (zreq.height, gr.height);
          }
    }
  // size request ungrouped/invisible widgets
  Requisition ireq = widget.inner_size_request();
  // determine final requisition
  ireq.width = MAX (zreq.width, ireq.width);
  ireq.height = MAX (zreq.height, ireq.height);
  return ireq;
}

} // Rapicorn
