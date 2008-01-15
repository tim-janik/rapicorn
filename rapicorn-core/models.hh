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

#include <rapicorn-core/values.hh>

namespace Rapicorn {

/* --- Data Models --- */
class Model0 : public virtual ReferenceCountImpl { // 1*Type + 1*Value
  class Model0Value : public BaseValue {
    virtual void                changed         ();
  public:
    explicit                    Model0Value     (Type::Storage s) : BaseValue (s) {}
  };
protected:
  virtual void          changed         ();
  virtual              ~Model0        ();
public:
  explicit              Model0        (Type t);
  Type                  type;
  Model0Value           value;
  /* notification */
  Signal<Model0, void ()> sig_changed;
  /* accessors */
  template<typename T> void  set        (T tvalue)      { value.set (tvalue); }
  template<typename T> void  operator=  (T tvalue)      { value.set (tvalue); }
  template<typename T> T     get        ()              { return value.convert ((T*) 0); }
  template<typename T> /*T*/ operator T ()              { return value.convert ((T*) 0); }
};
typedef class Model0 Variable;

} // Rapicorn

#endif /* __RAPICORN_MODELS_HH__ */
