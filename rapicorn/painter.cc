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
#include "painter.hh"

namespace Rapicorn {

/* auxillary structure to count dash state for stippled outlines */
struct Dasher {
  const vector<int> &dashes;
  int total;
  uint index, counter;
  Dasher (const vector<int> &d,
          int                offset) :
    dashes (d), total (0), index (0), counter (0)
  {
    for (uint i = 0; i < dashes.size(); i++)
      total += dashes[i];
    if (total)
      {
        offset %= total;
        if (offset < 0)
          offset += total;
        increment (offset);
      }
  }
  uint current_dash()
  {
    return index < dashes.size() ? dashes[index] : 0;
  }
  void increment1 ()
  {
    counter++;
    if (counter >= current_dash())
      {
        counter = 0;
        index++;
        index %= MAX (1, dashes.size());
      }
  }
  void increment (uint n)
  {
    if (n && total)
      {
        n %= total;
        while (n >= current_dash() - counter)
          {
            n -= current_dash() - counter;
            counter = 0;
            index++;
            index %= dashes.size();
          }
        while (n--)
          increment1();
      }
  }
  bool on()
  {
    return !(index & 1);
  }
  Dasher& operator++()
  {
    increment1();
    return *this;
  }
};

/* --- Painter --- */
Painter::Painter (Plane &plane) :
  m_plane (plane)
{}

Painter::~Painter ()
{}

void
Painter::draw_hline (int x0, int x1, int y, Color c, const vector<int> &dashes, int dash_offset)
{
}

void
Painter::draw_vline (int x, int y0, int y1, Color c, const vector<int> &dashes, int dash_offset)
{
}

void
Painter::draw_shadow (int x, int y, int width, int height,
                      Color outer_upper_left, Color inner_upper_left,
                      Color inner_lower_right, Color outer_lower_right)
{
}

void
Painter::draw_border (int x, int y, int width, int height, Color border, const vector<int> &dashes, int dash_offset)
{
}

void
Painter::draw_shaded_rect (int xc0, int yc0, Color color0, int xc1, int yc1, Color color1)
{
}

void
Painter::draw_filled_rect (int x, int y, int width, int height, Color fill_color)
{
}

} // Rapicorn
