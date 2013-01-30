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
  m_item (ref (item)), m_parent (NULL), m_n_children (-1), m_allocator (allocator)
{}

SelobItem::~SelobItem ()
{
  unref (m_item);
}

String
SelobItem::get_id ()
{
  return m_item.name();
}

String
SelobItem::get_type ()
{
  return Factory::factory_context_type (m_item.factory_context());
}

const StringVector&
SelobItem::get_type_list()
{
  return Factory::factory_context_tags (m_item.factory_context());
}

bool
SelobItem::has_property (const String &name)
{
  return NULL != m_item.lookup_property (name);
}

String
SelobItem::get_property (const String &name)
{
  return m_item.get_property (name);
}

void
SelobItem::cache_parent ()
{
  if (UNLIKELY (!m_parent))
    {
      ContainerImpl *parent = m_item.parent();
      if (parent)
        m_parent = m_allocator.item_selob (*parent);
    }
}

Selob*
SelobItem::get_parent ()
{
  cache_parent();
  return m_parent;
}

Selob*
SelobItem::get_sibling (int64 dir)
{
  ContainerImpl *container = m_item.parent();
  if (container && dir < 0)
    {
      ItemImpl *last = NULL;
      for (ContainerImpl::ChildWalker cw = container->local_children(); cw.has_next(); last = &*cw, cw++)
        if (&m_item == &*cw)
          break;
      if (last)
        return m_allocator.item_selob (*last);
    }
  else if (container && dir > 0)
    {
      for (ContainerImpl::ChildWalker cw = container->local_children(); cw.has_next(); cw++)
        if (&m_item == &*cw)
          {
            cw++;
            if (cw.has_next())
              return m_allocator.item_selob (*cw);
            break;
          }
    }
  return NULL;
}

void
SelobItem::cache_n_children()
{
  if (UNLIKELY (m_n_children < 0))
    {
      ContainerImpl *container = dynamic_cast<ContainerImpl*> (&m_item);
      m_n_children = container ? container->n_children() : 0;
    }
}

bool
SelobItem::has_children ()
{
  cache_n_children();
  return m_n_children > 0;
}

int64
SelobItem::n_children ()
{
  cache_n_children();
  return m_n_children;
}

Selob*
SelobItem::get_child (int64 index)
{
  if (m_n_children == 0)
    return NULL;
  ContainerImpl *container = dynamic_cast<ContainerImpl*> (&m_item);
  if (container)
    {
      ItemImpl *child = container->nth_child (index);
      if (child)
        return m_allocator.item_selob (*child);
    }
  return NULL;
}

bool
SelobItem::is_nth_child (int64 nth1based)
{
  ContainerImpl *container = m_item.parent();
  if (container && nth1based > 0)
    return container->nth_child (nth1based - 1) == &m_item;
  else if (container && nth1based < 0)
    {
      const size_t total = container->n_children();
      if (total >= size_t (-nth1based))
        return container->nth_child (total + nth1based) == &m_item;
    }
  return false;
}

Selob*
SelobItem::pseudo_selector (const String &ident, const String &arg, String &error)
{
  return ClassDoctor::item_pseudo_selector (*this, m_item, string_tolower (ident), arg, error);
}

SelobAllocator::SelobAllocator ()
{}

SelobAllocator::~SelobAllocator ()
{
  while (m_selobs.size())
    {
      SelobItem *selob = m_selobs.back();
      m_selobs.pop_back();
      delete selob;
    }
}

SelobItem*
SelobAllocator::item_selob (ItemImpl &item)
{
  assert_return (&item != NULL, NULL);
  SelobItem *selob = new SelobItem (*this, item);
  m_selobs.push_back (selob);
  return selob;
}

ItemImpl*
SelobAllocator::selob_item (Selob &selob)
{
  SelobItem *selobitem = dynamic_cast<SelobItem*> (&selob);
  return selobitem ? &selobitem->m_item : NULL;
}

SelobAllocator*
SelobAllocator::selob_allocator (Selob &selob)
{
  SelobItem *selobitem = dynamic_cast<SelobItem*> (&selob);
  return selobitem ? &selobitem->m_allocator : NULL;
}

SelobListModel::SelobListModel (SelobAllocator &allocator, ListModelIface &lmodel) :
  m_lmodel (lmodel), f_row_constraint (0), f_col_constraint (0), f_value_constraint (0), f_type_constraint (0)
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
      m_row_constraint = sv.size() >= 1 ? sv[0] : "";
      m_col_constraint = sv.size() >= 2 ? sv[1] : "";
      return this;
    }
  else if (!f_row_constraint && ident == "::row")
    { f_row_constraint = true; m_row_constraint = arg; return this; }
  else if (!f_col_constraint && (ident == "::col" || ident == "::column"))
    { f_col_constraint = true; m_col_constraint = arg; return this; }
  return NULL;
}

} // Selector
} // Rapicorn
