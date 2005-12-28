/* Rapicorn
 * Copyright (C) 2005 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#ifndef __RAPICORN_SLIDER_HH__
#define __RAPICORN_SLIDER_HH__

#include <rapicorn/adjustment.hh>
#include <rapicorn/container.hh>        // FIXME: change if ControlArea is not in container.hh

namespace Rapicorn {

class SliderArea : public virtual ControlArea, public virtual Item {
protected:
  typedef Signal<SliderArea, void ()>   SignalSliderChanged;
  explicit              SliderArea      ();
  virtual void          control         (const String   &command_name,
                                         const String   &arg) = 0;
  virtual void          slider_changed  ();
public:
  virtual Adjustment*   adjustment      () const = 0;
  virtual void          adjustment      (Adjustment     &adjustment) = 0;
  SignalSliderChanged   sig_slider_changed;
};

} // Rapicorn

#endif  /* __RAPICORN_SLIDER_HH__ */
