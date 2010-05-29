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
#include "models.hh"

namespace Rapicorn {

/* --- 0-Dim Models --- */

Model0::Model0 (Type t) :
  m_type (t), m_value (t.storage()),
  sig_changed (*this, &Model0::changed)
{}

Model0::~Model0 ()
{}

void
Model0::Model0Value::changed()
{
  ssize_t vvoffset = (ssize_t) ((char*) &((Model0*) 0x10000)->m_value) - 0x10000;
  Model0 *var = (Model0*) ((char*) this - vvoffset);
  var->sig_changed.emit();
}

void
Model0::changed()
{}

/* --- 1-Dim Selections --- */
class Selection1 : public virtual VirtualTypeid {
  friend class          Model1;
  static Selection1&    create_selection        (Model1       &model1,
                                                 SelectionMode selectionmode);
protected:
  Model1               &m_model;
  explicit              Selection1      (Model1 &model) : m_model (model) {}
  virtual SelectionMode mode            (void) const = 0;
  virtual bool          selected        (int64        nth) = 0;
  virtual void          clear           (void) = 0;
  virtual int64         sibling         (int64        nth,
                                         int          dir) = 0;
  virtual void          toggle          (int64        nth) = 0;
  virtual void          constrain       (void) = 0;
};

class Selection1None : public virtual Selection1 {
protected:
  virtual SelectionMode mode            (void) const    { return SELECTION_NONE; }
  virtual bool          selected        (int64  nth)    { return false; }
  virtual void          clear           (void)          {}
  virtual int64         sibling         (int64  nth,
                                         int    dir)    { return -1; }
  virtual void          toggle          (int64  nth)    {}
  virtual void          constrain       (void)          {}
public:
  explicit              Selection1None  (Model1 &model) : Selection1 (model) {}
};

class Selection1Single : public virtual Selection1 {
public:
  explicit              Selection1Single (Model1 &model) : Selection1 (model), m_selection (-1) {}
protected:
  int64                 m_selection;
  virtual SelectionMode mode            (void) const    { return SELECTION_SINGLE; }
  virtual bool          selected        (int64  nth)    { return m_selection >= 0 && m_selection == nth; }
  virtual void
  clear (void)
  {
    if (m_selection >= 0)
      toggle (m_selection);
  }
  virtual int64
  sibling (int64 nth,
           int   dir)
  {
    if (m_selection)
      if (nth < 0 ||
          (dir > 0 && nth < m_selection) ||
          (dir < 0 && nth > m_selection))
        return m_selection;
    return -1;
  }
  virtual void
  toggle (int64 nth)
  {
    const int64 mcount = m_model.count();
    if (nth < 0 || nth >= mcount)
      return;
    int64 old = m_selection;
    if (nth == m_selection)
      m_selection = -1;
    else
      m_selection = nth;
    if (m_selection >= 0)
      m_model.sig_selection_changed.emit (m_selection, 1);
    if (old >= 0)
      m_model.sig_selection_changed.emit (old, 1);
  }
  virtual void
  constrain (void)
  {
    const int64 mcount = m_model.count();
    int64 old = m_selection;
    if (m_selection >= mcount)
      m_selection = -1;
    if (old != m_selection)
      {
        if (m_selection >= 0)
          m_model.sig_selection_changed.emit (m_selection, 1);
        if (old >= 0)
          m_model.sig_selection_changed.emit (old, 1);
      }
  }
};

class Selection1Browse : public Selection1Single {
protected:
  virtual void
  toggle (int64 nth)
  {
    const int64 mcount = m_model.count();
    if (nth < 0 || nth >= mcount)
      return;
    if (nth == m_selection)
      return;
    int64 old = m_selection;
    m_selection = nth;
    if (m_selection >= 0)
      m_model.sig_selection_changed.emit (m_selection, 1);
    if (old >= 0)
      m_model.sig_selection_changed.emit (old, 1);
  }
  virtual void
  constrain (void)
  {
    const int64 mcount = m_model.count();
    int64 old = m_selection;
    if (mcount == 0)
      m_selection = -1;
    else
      m_selection = CLAMP (m_selection, 0, mcount);
    if (old != m_selection)
      {
        if (m_selection >= 0)
          m_model.sig_selection_changed.emit (m_selection, 1);
        if (old >= 0)
          m_model.sig_selection_changed.emit (old, 1);
      }
  }
public:
  explicit Selection1Browse (Model1 &model) : Selection1 (model), Selection1Single (model) {}
};

class Selection1Multiple : public virtual Selection1 {
  vector<bool>  bits;
public:
  explicit              Selection1Multiple (Model1 &model) : Selection1 (model) {}
protected:
  virtual SelectionMode mode            (void) const    { return SELECTION_MULTIPLE; }
  virtual bool          selected        (int64  nth)    { return nth < bits.size() && bits[nth]; }
  virtual void
  clear (void)
  {
    const size_t bsize = bits.size();
    bits.clear();
    if (bsize)
      m_model.sig_selection_changed.emit (0, bsize);
  }
  virtual int64
  sibling (int64 nth,
           int   dir)
  {
    const size_t bsize = bits.size();
    if (nth < 0 || nth >= bsize)
      return -1;
    if (dir > 0)
      for (int64 i = nth + 1; i < bsize; i++)
        if (bits[i])
          return i;
    if (dir < 0)
      for (int64 i = nth - 1; i >= 0; i--)
        if (bits[i])
          return i;
    return -1;
  }
  virtual void
  toggle (int64 nth)
  {
    const int64 mcount = m_model.count();
    if (nth < 0 || nth >= mcount)
      return;
    if (nth >= bits.size())
      bits.resize (nth + 1);
    bits[nth] = !bits[nth];
    m_model.sig_selection_changed.emit (nth, 1);
  }
  virtual void
  constrain (void)
  {
    const int64 mcount = m_model.count();
    if (mcount < bits.size())
      bits.resize (mcount);
  }
};

class Selection1Interval : public virtual Selection1 {
  int64 m_first, m_count;
public:
  explicit              Selection1Interval (Model1 &model) : Selection1 (model), m_first (-1), m_count (0) {}
protected:
  virtual SelectionMode mode            (void) const    { return SELECTION_INTERVAL; }
  virtual bool          selected        (int64  nth)    { return nth >= m_first && nth < m_first + m_count; }
  virtual void
  clear (void)
  {
    int64 oldf = m_first, oldc = m_count;
    m_first = -1;
    m_count = 0;
    if (oldf >= 0 && oldc > 0)
      m_model.sig_selection_changed.emit (oldf, oldc);
  }
  virtual int64
  sibling (int64 nth,
           int   dir)
  {
    const int64 mcount = m_model.count();
    if (nth < 0 || nth >= mcount)
      return -1;
    if (nth < m_first && dir > 0)
      return m_first;
    if (nth >= m_first + m_count && dir < 0)
      return m_first + m_count - 1;
    if (dir > 0 && nth >= m_first && nth + 1 < m_first + m_count)
      return nth + 1;
    if (dir < 0 && nth > m_first && nth < m_first + m_count)
      return nth - 1;
    return -1;
  }
  virtual void
  toggle (int64 nth)
  {
    const int64 mcount = m_model.count();
    if (nth < 0 || nth >= mcount)
      return;
    if (nth + 1 == m_first)
      {
        m_first = nth;
        m_count += 1;
      }
    else if (nth == m_first)
      {
        m_count -= 1;
        m_first = m_count > 0 ? nth + 1 : -1;
      }
    else if (nth + 1 == m_first + m_count)
      {
        m_count -= 1;
        if (m_count < 1)
          m_first = -1;
      }
    else if (nth == m_first + m_count)
      {
        m_count += 1;
      }
    else if (nth > m_first && nth + 1 < m_first + m_count)
      {
        clear();
        return;
      }
    else
      {
        int64 oldf = m_first, oldc = m_count;
        m_first = nth;
        m_count = 1;
        if (oldf >= 0 && oldc > 0)
          m_model.sig_selection_changed.emit (oldf, oldc);
      }
    m_model.sig_selection_changed.emit (nth, 1);
  }
  virtual void
  constrain (void)
  {
    const int64 mcount = m_model.count();
    if (mcount <= m_first)
      {
        m_first = -1;
        m_count = 0;
      }
    else if (mcount < m_first + m_count)
      m_count = mcount - m_first;
  }
};

Selection1&
Selection1::create_selection (Model1       &model1,
                              SelectionMode selectionmode)
{
  Selection1 *self = NULL;
  switch (selectionmode)
    {
    case SELECTION_SINGLE:      self = new Selection1Single (model1); break;
    case SELECTION_BROWSE:      self = new Selection1Browse (model1); break;
    case SELECTION_INTERVAL:    self = new Selection1Interval (model1); break;
    case SELECTION_MULTIPLE:    self = new Selection1Multiple (model1); break;
    default: // silence compiler
    case SELECTION_NONE:        self = new Selection1None (model1); break;
    }
  return *self;
}

/* --- 1-Dim Models --- */
Model1::~Model1 (void)
{
  delete &m_selection;
}

Model1::Model1 (Type          row_type,
                SelectionMode selectionmode) :
  m_type (row_type), m_selection (Selection1::create_selection (*this, selectionmode)),
  sig_changed  (*this, &Model1::handle_changed),
  sig_inserted (*this, &Model1::handle_inserted),
  sig_deleted  (*this, &Model1::handle_deleted),
  sig_selection_changed (*this, &Model1::handle_selection_changed)
{}

void
Model1::changed (uint64 first,
                 uint64 count)
{
  sig_changed.emit (first, count);
}

void
Model1::handle_changed (uint64 first,
                        uint64 count)
{}

void
Model1::inserted (uint64 first,
                  uint64 count)
{
  m_selection.constrain();
  sig_inserted.emit (first, count);
}

void
Model1::handle_inserted (uint64 first,
                         uint64 count)
{}

void
Model1::deleted (uint64 first,
                 uint64 count)
{
  m_selection.constrain();
  sig_deleted.emit (first, count);
}

void
Model1::handle_deleted  (uint64 first,
                         uint64 count)
{}

void
Model1::handle_selection_changed (uint64 first,
                                  uint64 count)
{}

SelectionMode
Model1::selection_mode (void)
{
  return m_selection.mode();
}

void
Model1::clear_selection ()
{
  return m_selection.clear();
}

bool
Model1::selected (int64 nth)
{
  return m_selection.selected (nth);
}

int64
Model1::next_selected (int64 nth)
{
  return m_selection.sibling (nth, +1);
}

int64
Model1::prev_selected (int64 nth)
{
  return m_selection.sibling (nth, -1);
}

void
Model1::toggle_selected (int64 nth)
{
  m_selection.toggle (nth);
}

/* --- 1-Dim Stores --- */

Store1::Store1()
{}

Store1::~Store1()
{}

class MemoryStore1 : public virtual Model1, public virtual Store1 {
  vector<Array*> avector;
  virtual Store1*
  pget_store (void)
  {
    return this;
  }
  virtual Selection1*
  pget_selection (void)
  {
    return NULL;
  }
  virtual uint64
  pcount_rows (void)
  {
    return avector.size();
  }
  virtual Array
  pget_row (uint64 row)
  {
    if (row < avector.size())
      return *avector[row];
    return Array();
  }
  virtual Model1&
  pget_model (void)
  {
    return *this;
  }
  virtual void
  pchange_rows (uint64       first,
                uint64       count,
                const Array *arrays)
  {
    uint64 cnotify = 0;
    for (uint64 row = first; row < first + count; row++)
      if (row < avector.size())
        {
          Array *old = avector[row];
          avector[row] = new Array (arrays[row - first]);
          if (old)
            delete old;
          cnotify++;
        }
    changed (first, cnotify);
  }
  virtual void
  pinsert_rows (int64        first,
                uint64       count,
                const Array *arrays)
  {
    if (first < 0)
      first += avector.size() + 1;
    if (first > avector.size())
      return;
    avector.insert (avector.begin() + first, count, NULL);
    for (uint64 row = first; row < first + count; row++)
      avector[row] = new Array (arrays[row - first]);
    inserted (first, count);
  }
  virtual void
  premove_rows (uint64 first,
                uint64 count)
  {
    if (first >= avector.size())
      return;
    uint64 end = first + count;
    end = MIN (end, avector.size());
    for (uint64 row = first; row < end; row++)
      delete avector[row];
    avector.erase (avector.begin() + first, avector.begin() + end);
    deleted (first, end - first);
  }
  ~MemoryStore1()
  {
    while (avector.size())
      {
        delete avector[avector.size() - 1];
        avector.resize (avector.size() - 1);
      }
  }
public:
  explicit MemoryStore1 (Type          row_type,
                         SelectionMode selectionmode) :
    Model1 (row_type, selectionmode)
  {}
};

Store1*
Store1::create_memory_store (const String &plor_name,
                             Type          row_type,
                             SelectionMode selectionmode)
{
  Store1 *store = new MemoryStore1 (row_type, selectionmode);
  store->plor_name (plor_name);
  return store;
}

} // Rapicorn
