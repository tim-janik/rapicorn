// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#include "selob.hh"
#include "container.hh"
#include "factory.hh"

namespace Rapicorn {

struct ClassDoctor {
  static inline Selector::Selob* item_pseudo_selector (Selector::Selob &selob, ItemImpl &item, const String &ident, const String &arg, String &error)
  { return item.pseudo_selector (selob, ident, arg, error); }
};

namespace Selector {

SelobItem::SelobItem (SelobAllocator &allocator, ItemImpl &item) :
  item_ (ref (item)), parent_ (NULL), n_children_ (-1), allocator_ (allocator)
{}

SelobItem::~SelobItem ()
{
  unref (item_);
}

String
SelobItem::get_id ()
{
  return item_.name();
}

String
SelobItem::get_type ()
{
  return Factory::factory_context_type (item_.factory_context());
}

const StringVector&
SelobItem::get_type_list()
{
  return Factory::factory_context_tags (item_.factory_context());
}

bool
SelobItem::has_property (const String &name)
{
  return NULL != item_.lookup_property (name);
}

String
SelobItem::get_property (const String &name)
{
  return item_.get_property (name);
}

void
SelobItem::cache_parent ()
{
  if (UNLIKELY (!parent_))
    {
      ContainerImpl *parent = item_.parent();
      if (parent)
        parent_ = allocator_.item_selob (*parent);
    }
}

Selob*
SelobItem::get_parent ()
{
  cache_parent();
  return parent_;
}

Selob*
SelobItem::get_sibling (int64 dir)
{
  ContainerImpl *container = item_.parent();
  if (container && dir < 0)
    {
      ItemImpl *last = NULL;
      for (ContainerImpl::ChildWalker cw = container->local_children(); cw.has_next(); last = &*cw, cw++)
        if (&item_ == &*cw)
          break;
      if (last)
        return allocator_.item_selob (*last);
    }
  else if (container && dir > 0)
    {
      for (ContainerImpl::ChildWalker cw = container->local_children(); cw.has_next(); cw++)
        if (&item_ == &*cw)
          {
            cw++;
            if (cw.has_next())
              return allocator_.item_selob (*cw);
            break;
          }
    }
  return NULL;
}

void
SelobItem::cache_n_children()
{
  if (UNLIKELY (n_children_ < 0))
    {
      ContainerImpl *container = dynamic_cast<ContainerImpl*> (&item_);
      n_children_ = container ? container->n_children() : 0;
    }
}

bool
SelobItem::has_children ()
{
  cache_n_children();
  return n_children_ > 0;
}

int64
SelobItem::n_children ()
{
  cache_n_children();
  return n_children_;
}

Selob*
SelobItem::get_child (int64 index)
{
  if (n_children_ == 0)
    return NULL;
  ContainerImpl *container = dynamic_cast<ContainerImpl*> (&item_);
  if (container)
    {
      ItemImpl *child = container->nth_child (index);
      if (child)
        return allocator_.item_selob (*child);
    }
  return NULL;
}

bool
SelobItem::is_nth_child (int64 nth1based)
{
  ContainerImpl *container = item_.parent();
  if (container && nth1based > 0)
    return container->nth_child (nth1based - 1) == &item_;
  else if (container && nth1based < 0)
    {
      const size_t total = container->n_children();
      if (total >= size_t (-nth1based))
        return container->nth_child (total + nth1based) == &item_;
    }
  return false;
}

Selob*
SelobItem::pseudo_selector (const String &ident, const String &arg, String &error)
{
  return ClassDoctor::item_pseudo_selector (*this, item_, string_tolower (ident), arg, error);
}

SelobAllocator::SelobAllocator ()
{}

SelobAllocator::~SelobAllocator ()
{
  while (selobs_.size())
    {
      SelobItem *selob = selobs_.back();
      selobs_.pop_back();
      delete selob;
    }
}

SelobItem*
SelobAllocator::item_selob (ItemImpl &item)
{
  assert_return (&item != NULL, NULL);
  SelobItem *selob = new SelobItem (*this, item);
  selobs_.push_back (selob);
  return selob;
}

ItemImpl*
SelobAllocator::selob_item (Selob &selob)
{
  SelobItem *selobitem = dynamic_cast<SelobItem*> (&selob);
  return selobitem ? &selobitem->item_ : NULL;
}

SelobAllocator*
SelobAllocator::selob_allocator (Selob &selob)
{
  SelobItem *selobitem = dynamic_cast<SelobItem*> (&selob);
  return selobitem ? &selobitem->allocator_ : NULL;
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
