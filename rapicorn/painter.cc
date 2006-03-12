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
  Dasher dd (dashes, dash_offset);
  int xmin = MAX (MIN (x0, x1), xstart());
  int xmax = MIN (MAX (x0, x1), xbound() - 1);
  if (y >= ystart() && y < ybound())
    {
      if (!dd.total)
        for (int x = xmin; x <= xmax; x++, ++dd)
          set (x, y, c);
      else if (x0 <= x1)
        {
          dd.increment (xmin - x0);
          for (int x = xmin; x <= xmax; x++, ++dd)
            if (dd.on())
              set (x, y, c);
        }
      else
        {
          dd.increment (x0 - xmax);
          for (int x = xmax; x >= xmin; x--, ++dd)
            if (dd.on())
              set (x, y, c);
        }
    }
}

void
Painter::draw_vline (int x, int y0, int y1, Color c, const vector<int> &dashes, int dash_offset)
{
  Dasher dd (dashes, dash_offset);
  int ymin = MAX (MIN (y0, y1), ystart());
  int ymax = MIN (MAX (y0, y1), ybound() - 1);
  if (x >= xstart() && x < xbound())
    {
      if (!dd.total)
        for (int y = ymin; y <= ymax; y++)
          set (x, y, c);
      else if (y0 <= y1)
        {
          dd.increment (ymin - y0);
          for (int y = ymin; y <= ymax; y++, ++dd)
            if (dd.on())
              set (x, y, c);
        }
      else
        {
          dd.increment (y0 - ymax);
          for (int y = ymax; y >= ymin; y--, ++dd)
            if (dd.on())
              set (x, y, c);
        }
    }
}

void
Painter::draw_shadow (int x, int y, int width, int height,
                      Color outer_upper_left, Color inner_upper_left,
                      Color inner_lower_right, Color outer_lower_right)
{
  if (width > 0 && height > 0)
    {
      /* outer lower-right */
      draw_hline (x, x + width - 1, y, outer_lower_right);
      draw_vline (x + width - 1, y, y + height - 1, outer_lower_right);
    }
  if (width > 1 && height > 1)
    {
      /* inner lower-right */
      draw_hline (x + 1, x + width - 2, y + 1, inner_lower_right);
      draw_vline (x + width - 2, y + 1, y + height - 2, inner_lower_right);
    }
  if (width > 0 && height > 0)
    {
      /* outer upper-left */
      draw_hline (x + width - 1, x, y + height - 1, outer_upper_left);
      draw_vline (x, y + height - 1, y, outer_upper_left);
    }
  if (width > 1 && height > 1)
    {
      /* inner upper-left */
      draw_hline (x + width - 2, x + 1, y + height - 2, inner_upper_left);
      draw_vline (x + 1, y + height - 2, y + 1, inner_upper_left);
    }
}

void
Painter::draw_border (int x, int y, int width, int height, Color border, const vector<int> &dashes, int dash_offset)
{
  if (width && height)
    {
      /* adjust the dash offset below by consumed length and by -1 for overlapping corners */
      draw_hline (x, x + width - 1, y, border, dashes, dash_offset);
      draw_vline (x + width - 1, y, y + height - 1, border, dashes, dash_offset + width - 1);
      draw_hline (x + width - 1, x, y + height - 1, border, dashes, dash_offset + width + height - 2);
      draw_vline (x, y + height - 1, y, border, dashes, dash_offset + width + height + width - 3);
    }
}

static inline uint32
quick_rand32 ()
{
  static uint32 accu = 2147483563;
  accu = 1664525 * accu + 1013904223;
  return accu;
}

void
Painter::draw_shaded_rect (int xc0, int yc0, Color color0, int xc1, int yc1, Color color1)
{
  Point c0 (xc0, yc0);
  Color pre0 = color0.premultiplied(), pre1 = color1.premultiplied();
  /* extract alpha, red, green ,blue */
  float Aa = pre1.alpha(), Ar = pre1.red(), Ag = pre1.green(), Ab = pre1.blue();
  float Ba = pre0.alpha(), Br = pre0.red(), Bg = pre0.green(), Bb = pre0.blue();
  int x0 = MIN (xc0, xc1), x1 = MAX (xc0, xc1);
  int y0 = MIN (yc0, yc1), y1 = MAX (yc0, yc1);
  x0 = MAX (x0, xstart());
  x1 = MIN (x1, xbound() - 1);
  y0 = MAX (y0, ystart());
  y1 = MIN (y1, ybound() - 1);
  double max_dist = Point (xc0, yc0).dist (xc1, yc1);
  double wdist = 1 / MAX (1, max_dist);
  float Ca = (Aa - Ba) * wdist, Cr = (Ar - Br) * wdist, Cg = (Ag - Bg) * wdist, Cb = (Ab - Bb) * wdist;
  for (int y = y0; y <= y1; y++)
    {
      uint32 *d = m_plane.poke_span (x0, y, x1 - x0 + 1);
      for (int x = x0; x <= x1; x++)
        {
          double lucent_dist = c0.dist (x, y);
          /* A over B = colorA * alpha + colorB * (1 - alpha) */
          double Dr = Br + Cr * lucent_dist;
          double Dg = Bg + Cg * lucent_dist;
          double Db = Bb + Cb * lucent_dist;
          double Da = Ba + Ca * lucent_dist;
          /* dither */
          union { uint32 r; uint8 a[4]; } z = { quick_rand32() };
          uint32 dr = ftoi (Dr * 0xff) + z.a[3];
          uint32 dg = ftoi (Dg * 0xff) + z.a[2];
          uint32 db = ftoi (Db * 0xff) + z.a[1];
          uint32 da = ftoi (Da * 0xff) + z.a[0];
          /* apply */
          *d++ = Color (dr >> 8, dg >> 8, db >> 8, da >> 8);
        }
    }
  /* timings:
   * span+quick-rand+floatcolor:   	0m1.412s (63.8%)
   * span+quick-rand+dblcolor:   	0m1.464s
   * span+quick-rand:   		0m2.380s
   * poke-span:         		0m3.764s
   * original:            		0m3.904s (100%)
   * sqrt(float):       		0m4.140s
   * coarse2:           		0m4.390s
   * coarse3:           		0m4.460s
   * -ffast-math:       		0m4.418s
   * builtin_hypot:     		0m4.930s
   * hypot:             		0m4.948s
   */
}

void
Painter::draw_filled_rect (int x, int y, int width, int height, Color fill_color)
{
  Color pre = fill_color.premultiplied();
  for (int iy = y; iy < y + height; iy++)
    for (int ix = x; ix < x + width; ix++)
      set_premultiplied (ix, iy, pre);
}


} // Rapicorn
