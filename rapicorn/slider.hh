/* Rapicorn
 * Copyright (C) 2005 Tim Janik
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
#ifndef __RAPICORN_SLIDER_HH__
#define __RAPICORN_SLIDER_HH__

#include <rapicorn/adjustment.hh>
#include <rapicorn/container.hh>

namespace Rapicorn {

class SliderArea : public virtual Container {
  bool                  move              (int);
protected:
  virtual const CommandList&    list_commands   ();
  virtual const PropertyList&   list_properties ();
  explicit              SliderArea        ();
  virtual void          control           (const String   &command_name,
                                           const String   &arg) = 0;
  virtual void          slider_changed    ();
  typedef Signal<SliderArea, void ()>     SignalSliderChanged;
public:
  virtual bool          flipped           () const = 0;
  virtual void          flipped           (bool flip) = 0;
  virtual Adjustment*   adjustment        () const = 0;
  virtual void          adjustment        (Adjustment     &adjustment) = 0;
  virtual
  AdjustmentSourceType  adjustment_source () const = 0;
  virtual void          adjustment_source (AdjustmentSourceType adj_source) = 0;
  SignalSliderChanged   sig_slider_changed;
};

} // Rapicorn

#endif  /* __RAPICORN_SLIDER_HH__ */
