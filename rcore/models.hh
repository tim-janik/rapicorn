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
#ifndef __RAPICORN_MODELS_HH__
#define __RAPICORN_MODELS_HH__

#include <rcore/rapicornsignal.hh>
#include <rcore/enumdefs.hh>

namespace Rapicorn {
typedef std::vector<Any> AnySeq;

class Store1;
class Selection1;
class Model1 : public virtual BaseObject { // 1*Type + n*Value
  typedef Signal<Model1, void (uint64,uint64)> RangeSignal;
  Plic::TypeCode  m_type;
  Selection1     &m_selection;
protected:
  void            changed                       (uint64         first,
                                                 uint64         count);
  void            inserted                      (uint64         first,
                                                 uint64         count);
  void            deleted                       (uint64         first,
                                                 uint64         count);
  virtual        ~Model1                        (void);
  virtual void    handle_changed                (uint64, uint64);
  virtual void    handle_inserted               (uint64, uint64);
  virtual void    handle_deleted                (uint64, uint64);
  virtual void    handle_selection_changed      (uint64, uint64);
  virtual Store1* pget_store                    (void) = 0;
  virtual uint64  pcount_rows                   (void) = 0;
  virtual AnySeq  pget_row                      (uint64         row) = 0;
public:
  explicit        Model1                        (Plic::TypeCode row_type,
                                                 SelectionMode  selectionmode);
  Plic::TypeCode  type                          (void) const            { return m_type; }
  int64           count                         (void)                  { return pcount_rows(); }
  AnySeq          get                           (uint64         nth)    { return pget_row (nth); }
  Store1*         store                         (void)                  { return pget_store(); }
  /* notification */
  RangeSignal     sig_changed;
  RangeSignal     sig_inserted;
  RangeSignal     sig_deleted;
  /* selection */
  SelectionMode   selection_mode                (void);
  void            clear_selection               ();
  bool            selected                      (int64          nth);
  int64           next_selected                 (int64          nth);
  int64           prev_selected                 (int64          nth);
  void            toggle_selected               (int64          nth);
  RangeSignal     sig_selection_changed;
};

class Store1 : public virtual BaseObject { // 1*Type + n*Value
protected:
  virtual              ~Store1          (void);
  virtual Model1&       pget_model      (void) = 0;
  virtual void          pchange_rows    (uint64       first,
                                         uint64       count,
                                         const AnySeq *array) = 0;
  virtual void          pinsert_rows    (int64        first,
                                         uint64       count,
                                         const AnySeq *arrays) = 0;
  virtual void          premove_rows    (uint64       first,
                                         uint64       count) = 0;
public:
  explicit              Store1          (void);
  Model1&               model           (void)                  { return pget_model(); }
  Plic::TypeCode        type            (void)                  { return model().type(); }
  int64                 count           (void)                  { return model().count(); }
  AnySeq                get             (uint64       nth)      { return model().get (nth); }
  void                  set             (uint64       nth,
                                         const AnySeq &array)   { pchange_rows (nth, 1, &array); }
  void                  insert          (int64        nth,
                                         const AnySeq &array)   { pinsert_rows (nth, 1, &array); }
  void                  remove          (uint64       nth)      { premove_rows (nth, 1); }
  void                  update          (uint64       nth,
                                         const AnySeq &array)   { pchange_rows (nth, 1, &array); }
  void                  clear           ()                      { premove_rows (0, count()); }
  /* premade stores */
  static Store1*        create_memory_store     (const String  &plor_name,
                                                 Plic::TypeCode row_type,
                                                 SelectionMode  selectionmode);
};

} // Rapicorn

#endif /* __RAPICORN_MODELS_HH__ */
