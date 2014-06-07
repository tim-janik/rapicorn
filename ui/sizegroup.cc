// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#include "sizegroup.hh"

namespace Rapicorn {

// == WidgetGroup ==
WidgetGroup*
WidgetGroup::create (const String &name, WidgetGroupType type)
{
  assert_return (name.empty() == false, NULL);
  if (type == WIDGET_GROUP_HSIZE || type == WIDGET_GROUP_VSIZE)
    return new SizeGroup (name, type);
  else
    return new WidgetGroup (name, type);
}

WidgetGroup::WidgetGroup (const String &name, WidgetGroupType type) :
  name_ (name), type_ (type)
{}

WidgetGroup::~WidgetGroup()
{
  assert (widgets_.size() == 0);
}

static DataKey<WidgetGroup::GroupVector> widget_group_key;

vector<WidgetGroup*>
WidgetGroup::list_groups (WidgetImpl &widget)
{
  return widget.get_data (&widget_group_key);
}

void
WidgetGroup::add_widget (WidgetImpl &widget)
{
  ref (this);
  // add widget to group's list
  widgets_.push_back (&widget);
  // add group to widget's list
  GroupVector wgv = widget.get_data (&widget_group_key);
  wgv.push_back (this);
  widget.set_data (&widget_group_key, wgv);
  // FIXME: invalidate size_group
}

void
WidgetGroup::remove_widget (WidgetImpl &widget)
{
  ref (this);
  // FIXME: invalidate size_group
  // remove widget from group's list
  bool found_one = false;
  for (uint i = 0; i < widgets_.size(); i++)
    if (widgets_[i] == &widget)
      {
        widgets_.erase (widgets_.begin() + i);
        found_one = true;
        unref (this);
        break;
      }
  if (!found_one)
    {
      critical ("attempt to remove unknown widget (%s) from group: %p", widget.name().c_str(), this);
      unref (this);
      return;
    }
  // remove group from widget's list */
  found_one = false;
  GroupVector wgv = widget.get_data (&widget_group_key);
  for (uint i = 0; i < wgv.size(); i++)
    if (wgv[i] == this)
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
  unref (this);
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
  // invalidate all active groups
  GroupVector wgl = list_groups (widget);
  for (size_t i = 0; i < wgl.size(); i++)
    wgl[i]->widget_invalidated (widget);
}

// == SizeGroup ==
SizeGroup::SizeGroup (const String &name, WidgetGroupType type) :
  WidgetGroup (name, type),
  active_ (true), all_dirty_ (false)
{}

void
SizeGroup::widget_transit (WidgetImpl &widget)
{
  if (active())
    {
      invalidate_sizes();
      widget.invalidate_size();
    }
}

void
SizeGroup::widget_invalidated (WidgetImpl &widget)
{
  if (active())
    invalidate_sizes();
}

void
SizeGroup::update()
{
  while (all_dirty_)
    {
      all_dirty_ = false;
      req_ = Requisition();
      size_request_widgets (widgets_, req_);
    }
}

void
SizeGroup::invalidate_sizes()
{
  if (all_dirty_)
    return;
  all_dirty_ = true;
  for (size_t i = 0; i < widgets_.size(); i++)
    widgets_[i]->invalidate_size();
}

void
SizeGroup::size_request_widgets (const vector<WidgetImpl*> widgets, Requisition &max_requisition)
{
  for (size_t i = 0; i < widgets.size(); i++)
    {
      WidgetImpl &widget = *widgets[i];
      const Requisition ireq = widget.inner_size_request();
      max_requisition.width = MAX (max_requisition.width, ireq.width);
      max_requisition.height = MAX (max_requisition.height, ireq.height);
    }
}

Requisition
SizeGroup::widget_requisition (WidgetImpl &widget)
{
  // size request all active groups
  Requisition zreq; // 0,0
  if (widget.visible())
    {
      GroupVector wgl = list_groups (widget);
      for (size_t i = 0; i < wgl.size(); i++)
        if (wgl[i]->type() == WIDGET_GROUP_HSIZE || wgl[i]->type() == WIDGET_GROUP_VSIZE)
          {
            SizeGroup *sg = dynamic_cast<SizeGroup*> (wgl[i]);
            Requisition gr = sg->group_requisition();
            if (!sg->active())
              continue;
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
