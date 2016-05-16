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
  else if (type == WIDGET_GROUP_RADIO)
    return FriendAllocator<RadioGroup>::make_shared (name, type);
  else
    fatal ("invalid WidgetGroupType value: %d", type);
}

WidgetGroup::WidgetGroup (const String &name, WidgetGroupType type) :
  name_ (name), type_ (type)
{}

WidgetGroup::~WidgetGroup()
{
  assert (widgets_.size() == 0);
}

const vector<WidgetGroupP>&
WidgetGroup::list_groups (const WidgetImpl &widget)
{
  return widget.pack_info().widget_groups;
}

void
WidgetGroup::add_widget (WidgetImpl &widget)
{
  // add widget to group's list
  widgets_.push_back (&widget);
  // add group to widget's list
  WidgetGroupVector &wgv = widget.widget_pack_info().widget_groups;
  wgv.push_back (shared_ptr_cast<WidgetGroup> (this));
  adding_widget (widget);
}

void
WidgetGroup::remove_widget (WidgetImpl &widget)
{
  removing_widget (widget);
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
      critical ("attempt to remove unknown widget (%s) from group: %p", widget.id().c_str(), this);
      return;
    }
  // remove group from widget's list */
  found_one = false;
  WidgetGroupVector &wgv = widget.widget_pack_info().widget_groups;
  for (size_t i = 0; i < wgv.size(); i++)
    if (this == &*wgv[i])
      {
        wgv.erase (wgv.begin() + i);
        found_one = true;
        break;
      }
  if (!found_one)
    fatal ("failed to remove size group (%p) from widget: %s", this, widget.id());
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

void
WidgetGroup::toggled_widget (WidgetImpl &widget)
{
  // notify all groups
  GroupVector wgl = list_groups (widget);
  for (size_t i = 0; i < wgl.size(); i++)
    wgl[i]->widget_toggled (widget);
}

// == RadioGroup ==
RadioGroup::RadioGroup (const String &name, WidgetGroupType type) :
  WidgetGroup (name, type), updating_toggles_ (false)
{}

void
RadioGroup::adding_widget (WidgetImpl &widget)
{
  widget_toggled (widget);
}

void
RadioGroup::removing_widget (WidgetImpl &widget)
{
  // ensure radio group has a selection after removal
  if (widget.toggled() && widgets_.size() > 2)
    for (size_t i = 0; i < widgets_.size(); i++)
      if (widgets_[i] != &widget)
        {
          widgets_[i]->toggled (true);
          break;
        }
}

void
RadioGroup::widget_toggled (WidgetImpl &widget)
{
  // avoid messing with our own updates
  if (updating_toggles_)
    return;
  updating_toggles_ = true;
  // widget was just toggled, enforce radio group invariant
  if (widget.toggled())
    {
      // ensure no other widget in the group is toggled
      for (size_t i = 0; i < widgets_.size(); i++)
        if (widgets_[i] != &widget && widgets_[i]->toggled())
          widgets_[i]->toggled (false);
    }
  else // widget.toggled() == false
    {
      // ensure *some* widget is toggled
      bool seen_toggled = false;
      for (size_t i = 0; i < widgets_.size(); i++)
        if (widgets_[i]->toggled())
          {
            seen_toggled = true;
            break;
          }
      // simply default to picking the first
      if (!seen_toggled)
        for (size_t i = 0; i < widgets_.size(); i++)
          if (widgets_[i] != &widget)
            {
              widgets_[i]->toggled (true);
              break;
            }
    }
  updating_toggles_ = false;
}

bool
RadioGroup::widget_may_toggle (const WidgetImpl &widget)
{
  if (!widget.toggled())
    return true;
  // check all groups
  bool in_toggle_update = false, seen_sole_selection = false;
  GroupVector wgl = list_groups (widget);
  for (size_t i = 0; i < wgl.size(); i++)
    if (wgl[i]->type() == WIDGET_GROUP_RADIO)
      {
        RadioGroup *rgroup = dynamic_cast<RadioGroup*> (&*wgl[i]);
        if (rgroup->updating_toggles_)
          in_toggle_update = true;
        if (rgroup->widgets_.size() > 1)
          seen_sole_selection = true;
      }
  if (!in_toggle_update && seen_sole_selection)
    return false; // must not toggle off selected radio
  return true;
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
