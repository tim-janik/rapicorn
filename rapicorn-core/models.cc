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

Model1::~Model1 (void)
{}

Model1::Model1 (Type row_type) :
  m_type (row_type),
  sig_changed  (*this, &Model1::handle_changed),
  sig_inserted (*this, &Model1::handle_inserted),
  sig_deleted  (*this, &Model1::handle_deleted)
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
  sig_deleted.emit (first, count);
}

void
Model1::handle_deleted  (uint64 first,
                         uint64 count)
{}

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
  explicit MemoryStore1 (Type row_type) :
    Model1 (row_type)
  {}
};

Store1*
Store1::create_memory_store (Type row_type)
{
  return new MemoryStore1 (row_type);
}

} // Rapicorn
