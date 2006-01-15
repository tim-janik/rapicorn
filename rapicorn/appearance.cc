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
#include "appearance.hh"

namespace Rapicorn {

Style::Style (Appearance     &appearance,
              const String   &name) :
  m_appearance (appearance),
  m_name (name)
{
  m_appearance.register_style (*this);
}

Color
Style::lighten_color (Color source_color)
{
  Color c = source_color;
  double hue, saturation, value;
  c.get_hsv (&hue, &saturation, &value);
  value *= 1.1;
  c.set_hsv (hue, saturation, value);
  return c;
}

Color
Style::darken_color (Color source_color)
{
  Color c = source_color;
  double hue, saturation, value;
  c.get_hsv (&hue, &saturation, &value);
  value *= 0.9;
  c.set_hsv (hue, saturation, value);
  return c;
}

Color
Style::insensitive_color (Color source_color,
                          ColorType ctype)
{
  Color c = source_color;
  double hue, saturation, value;
  c.get_hsv (&hue, &saturation, &value);
  saturation *= 0.8;
  value *= 1.1; // FIXME: adjust lightness
  c.set_hsv (hue, saturation, MIN (1, value));
  return c;
}

Color
Style::prelight_color (Color source_color,
                       ColorType ctype)
{
  Color c = source_color;
  switch (ctype)
    {
    case COLOR_FOREGROUND:
    case COLOR_BACKGROUND:
    case COLOR_FOCUS:
    case COLOR_DEFAULT:
      return c;
    default: ;
    }
  double hue, saturation, value;
  c.get_hsv (&hue, &saturation, &value);
  saturation *= 1.15;
  c.set_hsv (hue, MIN (1, saturation), value);
  return c;
}

Color
Style::impressed_color (Color source_color,
                        ColorType ctype)
{
  Color c = source_color;
  switch (ctype)
    {
    case COLOR_FOREGROUND:
    case COLOR_BACKGROUND:
    case COLOR_FOCUS:
    case COLOR_DEFAULT:
      return c;
    default: ;
    }
  double hue, saturation, value;
  c.get_hsv (&hue, &saturation, &value);
  value *= 0.8;
  c.set_hsv (hue, saturation, value);
  return c;
}

Color
Style::focus_state_color (Color source_color,
                          ColorType ctype)
{
  Color c = source_color;
  return c;
}

Color
Style::default_state_color (Color source_color,
                            ColorType ctype)
{
  Color c = source_color;
  return c;
}

Color
Style::state_convert_color (StateType      state,
                            Color          color,
                            ColorType      ctype)
{
  Color c = color;
  if (state & STATE_IMPRESSED)
    c = impressed_color (c, ctype);
  if (state & STATE_FOCUS)
    c = focus_state_color (c, ctype);
  if (state & STATE_DEFAULT)
    c = default_state_color (c, ctype);
  if (state & STATE_INSENSITIVE)
    return insensitive_color (c, ctype);        /* ignore prelight if insensitive */
  if (state & STATE_PRELIGHT)
    c = prelight_color (c, ctype);
  return c;
}

Color
Style::cube_color (Style::ColorCubeRef   cube,
                   StateType      state,
                   Color          rgb_color,
                   ColorType      color_type) const
{
  Color rgb = cube.convert_color (rgb_color);
  rgb = state_convert_color (state, rgb, color_type);
  return rgb;
}

Color
Style::cube_color (Style::ColorCubeRef   cube,
                   StateType      state,
                   ColorType      color_type) const
{
  Color c;
  switch (color_type)
    {
    default:
    case COLOR_NONE:                    c = 0x00000000; break;
    case COLOR_FOREGROUND:              c = 0xff000000; break;
    case COLOR_BACKGROUND:              c = 0xffc0c0c0; break;
    case COLOR_FOCUS:                   c = 0xff000060; break;
    case COLOR_DEFAULT:                 c = 0xffd0d000; break;
    case COLOR_LIGHT_GLINT:             c = 0xffe0e0e0; break;
    case COLOR_LIGHT_SHADOW:            c = 0xffb0b0b0; break;
    case COLOR_DARK_GLINT:              c = 0xffb0b0b0; break;
    case COLOR_DARK_SHADOW:             c = 0xff808080; break;
    case COLOR_WHITE:                   c = 0xffffffff; break;
    case COLOR_BLACK:                   c = 0xff000000; break;
    case COLOR_RED:                     c = 0xffff0000; break;
    case COLOR_YELLOW:                  c = 0xffffff00; break;
    case COLOR_GREEN:                   c = 0xff00ff00; break;
    case COLOR_CYAN:                    c = 0xff00ffff; break;
    case COLOR_BLUE:                    c = 0xff0000ff; break;
    case COLOR_MAGENTA:                 c = 0xffff00ff; break;
    }
  return cube_color (cube, state, c, color_type);
}

Style*
Style::create_style (const String   &style_name)
{
  return m_appearance.create_style (style_name);
}

Style::~Style ()
{
  m_appearance.unregister_style (*this);
}

class StandardCube : public virtual ColorCube {
public:
  virtual Color
  convert_color   (Color  color) const
  {
    Color c = color;
    return c;
  }
};
static StandardCube standard_cube;

const ColorCube &Style::STANDARD = standard_cube;
const ColorCube &Style::SELECTED = standard_cube;
const ColorCube &Style::INPUT = standard_cube;

Appearance::Appearance (const String &name) :
  m_name (name)
{}

Style*
Appearance::create_style (const String &style_name)
{
  Style *style = m_styles[style_name];
  if (style)
    style->ref();
  else
    style = new Style (*this, "normal");
  return style;
}

void
Appearance::register_style (Style &style)
{
  Style *old = m_styles[style.name()];
  if (!old)
    ref();
  m_styles[style.name()] = &style;
}

void
Appearance::unregister_style (Style &style)
{
  Style *old = m_styles[style.name()];
  if (old == &style)
    {
      m_styles[style.name()] = NULL;
      unref();
    }
}

Appearance::~Appearance ()
{
  for (map<String,Style*>::iterator it = m_styles.begin(); it != m_styles.end(); it++)
    if (it->second)
      warning ("leaking: (Style*) %p", (Style*) it->second);
}

Appearance*
Appearance::create_default ()
{
  static Appearance *default_appearance = NULL;
  if (!default_appearance)
    {
      default_appearance = new Appearance ("default");
      default_appearance->ref_sink();
    }
  return ref (default_appearance);
}

Appearance*
Appearance::create_named (const String   &name)
{
  return create_default();
}

} // Rapicorn
