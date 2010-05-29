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

#include <rcore/types.hh>
#include <rcore/values.hh>

namespace Rapicorn {

/* --- Data Models --- */
class Model0 : public virtual BaseObject { // 1*Type + 1*Value
  class Model0Value : public BaseValue {
    virtual void        changed         ();
  public:
    explicit            Model0Value     (StorageType s) : BaseValue (s) {}
  };
  Type                  m_type;
  Model0Value           m_value;
protected:
  virtual              ~Model0          (void);
  virtual void          changed         (void);
public:
  explicit              Model0          (Type t);
  Type                  type            (void)          { return m_type; }
  /* notification */
  Signal<Model0, void ()> sig_changed;
  /* accessors */
  template<typename T> void  set        (T tvalue)      { m_value.set (tvalue); }
  template<typename T> void  operator=  (T tvalue)      { m_value.set (tvalue); }
  template<typename T> T     get        ()              { return m_value.convert ((T*) 0); }
  template<typename T> /*T*/ operator T ()              { return m_value.convert ((T*) 0); }
};
typedef class Model0 Variable;

class Store1;
class Selection1;
class Model1 : public virtual BaseObject { // 1*Type + n*Value
  typedef Signal<Model1, void (uint64,uint64)> RangeSignal;
  Type            m_type;
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
  virtual Array   pget_row                      (uint64         row) = 0;
public:
  explicit        Model1                        (Type           row_type,
                                                 SelectionMode  selectionmode);
  Type            type                          (void) const            { return m_type; }
  int64           count                         (void)                  { return pcount_rows(); }
  Array           get                           (uint64         nth)    { return pget_row (nth); }
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
                                         const Array *array) = 0;
  virtual void          pinsert_rows    (int64        first,
                                         uint64       count,
                                         const Array *arrays) = 0;
  virtual void          premove_rows    (uint64       first,
                                         uint64       count) = 0;
public:
  explicit              Store1          (void);
  Model1&               model           (void)                  { return pget_model(); }
  Type                  type            (void)                  { return model().type(); }
  int64                 count           (void)                  { return model().count(); }
  Array                 get             (uint64       nth)      { return model().get (nth); }
  void                  set             (uint64       nth,
                                         const Array &array)    { pchange_rows (nth, 1, &array); }
  void                  insert          (int64        nth,
                                         const Array &array)    { pinsert_rows (nth, 1, &array); }
  void                  remove          (uint64       nth)      { premove_rows (nth, 1); }
  void                  update          (uint64       nth,
                                         const Array &array)    { pchange_rows (nth, 1, &array); }
  void                  clear           ()                      { premove_rows (0, count()); }
  /* premade stores */
  static Store1*        create_memory_store     (Type          row_type,
                                                 SelectionMode selectionmode);
};

} // Rapicorn

#endif /* __RAPICORN_MODELS_HH__ */
