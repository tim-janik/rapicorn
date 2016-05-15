// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#include "selob.hh"
#include "container.hh"
#include "factory.hh"

namespace Rapicorn {

struct ClassDoctor {
  static inline Selector::Selob* widget_pseudo_selector (Selector::Selob &selob, WidgetImpl &widget, const String &ident, const String &arg, String &error)
  { return widget.pseudo_selector (selob, ident, arg, error); }
};

namespace Selector {

SelobWidget::SelobWidget (SelobAllocator &allocator, WidgetImpl &widget) :
  widget_ (shared_ptr_cast<WidgetImpl> (&widget)), parent_ (NULL), n_children_ (-1), allocator_ (allocator)
{}

SelobWidget::~SelobWidget ()
{}

String
SelobWidget::get_id ()
{
  return widget_->id();
}

String
SelobWidget::get_type ()
{
  return Factory::factory_context_type (widget_->factory_context());
}

const StringVector&
SelobWidget::get_type_list()
{
  return Factory::factory_context_tags (widget_->factory_context());
}

bool
SelobWidget::has_property (const String &name)
{
  return NULL != widget_->lookup_property (name);
}

String
SelobWidget::get_property (const String &name)
{
  return widget_->get_property (name);
}

void
SelobWidget::cache_parent ()
{
  if (UNLIKELY (!parent_))
    {
      ContainerImpl *parent = widget_->parent();
      if (parent)
        parent_ = allocator_.widget_selob (*parent);
    }
}

Selob*
SelobWidget::get_parent ()
{
  cache_parent();
  return parent_;
}

Selob*
SelobWidget::get_sibling (int64 dir)
{
  ContainerImpl *container = widget_->parent();
  if (container && dir < 0)
    {
      WidgetImpl *last = NULL;
      for (auto childp : *container)
        {
          if (widget_ == childp)
            break;
          last = &*childp;
        }
      if (last)
        return allocator_.widget_selob (*last);
    }
  else if (container && dir > 0)
    {
      for (WidgetImplP *iter = container->begin(); iter < container->end(); iter++)
        if (widget_ == *iter)
          {
            iter++;
            if (iter < container->end())
              return allocator_.widget_selob (**iter);
            break;
          }
    }
  return NULL;
}

void
SelobWidget::cache_n_children()
{
  if (UNLIKELY (n_children_ < 0))
    {
      ContainerImpl *container = widget_->as_container_impl();
      n_children_ = container ? container->n_children() : 0;
    }
}

bool
SelobWidget::has_children ()
{
  cache_n_children();
  return n_children_ > 0;
}

int64
SelobWidget::n_children ()
{
  cache_n_children();
  return n_children_;
}

Selob*
SelobWidget::get_child (int64 index)
{
  if (n_children_ == 0)
    return NULL;
  ContainerImpl *container = widget_->as_container_impl();
  if (container)
    {
      WidgetImpl *child = container->nth_child (index);
      if (child)
        return allocator_.widget_selob (*child);
    }
  return NULL;
}

bool
SelobWidget::is_nth_child (int64 nth1based)
{
  ContainerImpl *container = widget_->parent();
  if (container && nth1based > 0)
    return container->nth_child (nth1based - 1) == &*widget_;
  else if (container && nth1based < 0)
    {
      const size_t total = container->n_children();
      if (total >= size_t (-nth1based))
        return container->nth_child (total + nth1based) == &*widget_;
    }
  return false;
}

Selob*
SelobWidget::pseudo_selector (const String &ident, const String &arg, String &error)
{
  return widget_->pseudo_selector (*this, string_tolower (ident), arg, error);
}

SelobAllocator::SelobAllocator ()
{}

SelobAllocator::~SelobAllocator ()
{
  while (selobs_.size())
    {
      SelobWidget *selob = selobs_.back();
      selobs_.pop_back();
      delete selob;
    }
}

SelobWidget*
SelobAllocator::widget_selob (WidgetImpl &widget)
{
  assert_return (&widget != NULL, NULL);
  SelobWidget *selob = new SelobWidget (*this, widget);
  selobs_.push_back (selob);
  return selob;
}

WidgetImpl*
SelobAllocator::selob_widget (Selob &selob)
{
  SelobWidget *selobwidget = dynamic_cast<SelobWidget*> (&selob);
  return selobwidget ? &*selobwidget->widget_ : NULL;
}

SelobAllocator*
SelobAllocator::selob_allocator (Selob &selob)
{
  SelobWidget *selobwidget = dynamic_cast<SelobWidget*> (&selob);
  return selobwidget ? &selobwidget->allocator_ : NULL;
}

SelobListModel::SelobListModel (SelobAllocator &allocator, ListModelIface &lmodel) :
  lmodel_ (lmodel), f_row_constraint (0), f_col_constraint (0), f_value_constraint (0), f_type_constraint (0)
{}

SelobListModel::~SelobListModel ()
{}

String
SelobListModel::get_id ()
{
  return "";
}

String
SelobListModel::get_type ()
{
  return "Rapicorn::ListModel";
}

const StringVector&
SelobListModel::get_type_list()
{
  if (type_list_.empty())
    type_list_.push_back (get_type());
  return type_list_;
}

Selob*
SelobListModel::pseudo_selector (const String &ident, const String &arg, String &error)
{
  if (!(f_col_constraint || f_row_constraint) && ident == "::cell")
    {
      f_row_constraint = true;
      f_col_constraint = true;
      StringVector sv = string_split (arg, ",");
      row_constraint_ = sv.size() >= 1 ? sv[0] : "";
      col_constraint_ = sv.size() >= 2 ? sv[1] : "";
      return this;
    }
  else if (!f_row_constraint && ident == "::row")
    { f_row_constraint = true; row_constraint_ = arg; return this; }
  else if (!f_col_constraint && (ident == "::col" || ident == "::column"))
    { f_col_constraint = true; col_constraint_ = arg; return this; }
  return NULL;
}

} // Selector
} // Rapicorn
