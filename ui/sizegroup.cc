// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#include "sizegroup.hh"

namespace Rapicorn {

// == WidgetGroup ==
WidgetGroup*
WidgetGroup::create (const String &name)
{
  assert_return (name.empty() == false, NULL);
  return new WidgetGroup (name);
}

WidgetGroup::WidgetGroup (const String &name) :
  name_ (name)
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

// == SizeGroupImpl ==
class SizeGroupImpl : public SizeGroup {
  Requisition           req_;
  vector<WidgetImpl*>     widgets_;
public:
  uint                  hgroup_ : 1;
  uint                  vgroup_ : 1;
private:
  uint                  active_ : 1;
  uint                  all_dirty_ : 1;        // need resize
  virtual       ~SizeGroupImpl          ();
  virtual void   add_widget               (WidgetImpl &widget);
  virtual void   remove_widget            (WidgetImpl &widget);
  void           update                 ();
public:
  virtual bool   active                 () const        { return active_; }
  virtual void   active                 (bool isactive) { active_ = isactive; all_dirty_ = false; invalidate_sizes(); }
  void           invalidate_sizes       ();
  explicit                      SizeGroupImpl     (bool hgroup, bool vgroup);
  static vector<SizeGroupImpl*> list_groups       (WidgetImpl &widget);
  virtual Requisition           group_requisition () { update(); return req_; }
};

SizeGroupImpl::SizeGroupImpl (bool hgroup,
                              bool vgroup) :
  hgroup_ (hgroup), vgroup_ (vgroup),
  active_ (true), all_dirty_ (false)
{}

SizeGroupImpl::~SizeGroupImpl ()
{
  assert (widgets_.size() == 0);
}

static DataKey<vector<SizeGroupImpl*> > size_group_key;

vector<SizeGroupImpl*>
SizeGroupImpl::list_groups (WidgetImpl &widget)
{
  return widget.get_data (&size_group_key);
}

void
SizeGroupImpl::add_widget (WidgetImpl &widget)
{
  /* add widget to size group's list */
  widgets_.push_back (&widget);
  ref (this);
  /* add size group to widget's list */
  vector<SizeGroupImpl*> szv = widget.get_data (&size_group_key);
  szv.push_back (this);
  widget.set_data (&size_group_key, szv);
  if (active())
    {
      invalidate_sizes();
      widget.invalidate_size();
    }
}

void
SizeGroupImpl::remove_widget (WidgetImpl &widget)
{
  ref (this);
  if (active())
    {
      invalidate_sizes();
      widget.invalidate_size();
    }
  /* remove widget from size group's list */
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
    critical ("%s: attempt to remove unknown widget (%s) from size group: %p", STRFUNC, widget.name().c_str(), this);
  /* remove size group from widget's list */
  found_one = false;
  vector<SizeGroupImpl*> szv = widget.get_data (&size_group_key);
  for (uint i = 0; i < szv.size(); i++)
    if (szv[i] == this)
      {
        szv.erase (szv.begin() + i);
        found_one = true;
        break;
      }
  if (!found_one)
    critical ("%s: attempt to remove unknown size group (%p) from widget: %s", STRFUNC, this, widget.name().c_str());
  if (szv.size() == 0)
    widget.delete_data (&size_group_key);
  else
    widget.set_data (&size_group_key, szv);
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
      size_request_widgets (widgets_, req_);
    }
}

void
SizeGroupImpl::invalidate_sizes()
{
  if (all_dirty_)
    return;
  all_dirty_ = true;
  for (uint i = 0; i < widgets_.size(); i++)
    widgets_[i]->invalidate_size();
}

void
SizeGroup::size_request_widgets (const vector<WidgetImpl*> widgets,
                               Requisition        &max_requisition)
{
  for (uint i = 0; i < widgets.size(); i++)
    {
      WidgetImpl &widget = *widgets[i];
      Requisition ireq = widget.inner_size_request();
      max_requisition.width = MAX (max_requisition.width, ireq.width);
      max_requisition.height = MAX (max_requisition.height, ireq.height);
    }
}

void
SizeGroup::delete_widget (WidgetImpl &widget)
{
  /* remove from all groups */
  vector<SizeGroupImpl*> sgl = SizeGroupImpl::list_groups (widget);
  for (uint i = 0; i < sgl.size(); i++)
    {
      SizeGroup &sg = *sgl[i];
      sg.remove_widget (widget);
    }
}

void
SizeGroup::invalidate_widget (WidgetImpl &widget)
{
  /* invalidate all active groups */
  vector<SizeGroupImpl*> sgl = SizeGroupImpl::list_groups (widget);
  for (uint i = 0; i < sgl.size(); i++)
    if (sgl[i]->active())
      sgl[i]->invalidate_sizes();
}

Requisition
SizeGroup::widget_requisition (WidgetImpl &widget)
{
  // size request all active groups
  Requisition zreq; // 0,0
  if (widget.visible())
    {
      vector<SizeGroupImpl*> sgl = SizeGroupImpl::list_groups (widget);
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
  // size request ungrouped/invisible widgets
  Requisition ireq = widget.inner_size_request();
  // determine final requisition
  ireq.width = MAX (zreq.width, ireq.width);
  ireq.height = MAX (zreq.height, ireq.height);
  return ireq;
}

} // Rapicorn
