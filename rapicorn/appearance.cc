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

static Color
normal_color (const String   &color_name,
              ColorType       ctype)
{
  switch (ctype)
    {
    default:
    case COLOR_NONE:                    return 0x00000000;
    case COLOR_FOREGROUND:              return 0xff000000;
    case COLOR_BACKGROUND:              return 0xffc0c0c0;
    case COLOR_SELECTED_FOREGROUND:     return 0xffffffff;
    case COLOR_SELECTED_BACKGROUND:     return 0xff000077;
    case COLOR_FOCUS:                   return 0xff000060;
    case COLOR_DEFAULT:                 return 0xffd0d000;
    case COLOR_LIGHT_GLINT:             return 0xffe0e0e0;
    case COLOR_LIGHT_SHADOW:            return 0xffb0b0b0;
    case COLOR_DARK_GLINT:              return 0xffb0b0b0;
    case COLOR_DARK_SHADOW:             return 0xff808080;
    case COLOR_WHITE:                   return 0xffffffff;
    case COLOR_BLACK:                   return 0xff000000;
    case COLOR_RED:                     return 0xffff0000;
    case COLOR_YELLOW:                  return 0xffffff00;
    case COLOR_GREEN:                   return 0xff00ff00;
    case COLOR_CYAN:                    return 0xff00ffff;
    case COLOR_BLUE:                    return 0xff0000ff;
    case COLOR_MAGENTA:                 return 0xffff00ff;
    }
}

static Color
insensitive_color (Color           c,
                   const String   &color_name,
                   ColorType       ctype)
{
  double hue, saturation, value;
  c.get_hsv (&hue, &saturation, &value);
  saturation *= 0.8;
  value *= 1.1; // FIXME: adjust lightness
  c.set_hsv (hue, saturation, MIN (1, value));
  return c;
}

static Color
prelight_color (Color           c,
                const String   &color_name,
                ColorType       ctype)
{
  switch (ctype)
    {
    case COLOR_FOREGROUND: case COLOR_BACKGROUND:
    case COLOR_SELECTED_FOREGROUND: case COLOR_SELECTED_BACKGROUND:
    case COLOR_FOCUS: case COLOR_DEFAULT:
      break;
    default:
      double hue, saturation, value;
      c.get_hsv (&hue, &saturation, &value);
      saturation *= 1.15;
      c.set_hsv (hue, MIN (1, saturation), value);
      break;
    }
  return c;
}

static Color
impressed_color (Color           c,
                 const String   &color_name,
                 ColorType       ctype)
{
  switch (ctype)
    {
    case COLOR_FOREGROUND: case COLOR_BACKGROUND:
    case COLOR_SELECTED_FOREGROUND: case COLOR_SELECTED_BACKGROUND:
    case COLOR_FOCUS: case COLOR_DEFAULT:
      break;
    default:
      double hue, saturation, value;
      c.get_hsv (&hue, &saturation, &value);
      value *= 0.8;
      c.set_hsv (hue, saturation, value);
      break;
    }
  return c;
}

static Color
has_focus_color (Color           c,
                 const String   &color_name,
                 ColorType       ctype)
{
  return c;
}

static Color
has_default_color (Color           c,
                   const String   &color_name,
                   ColorType       ctype)
{
  return c;
}

Color
Style::color (StateType     state,
              const String &color_name,
              ColorType     ctype)
{
  Color c = normal_color (color_name, ctype);
  if (state & STATE_IMPRESSED)
    c = impressed_color (c, color_name, ctype);
  if (state & STATE_FOCUS)
    c = has_focus_color (c, color_name, ctype);
  if (state & STATE_DEFAULT)
    c = has_default_color (c, color_name, ctype);
  if (state & STATE_INSENSITIVE)
    return insensitive_color (c, color_name, ctype);
  if (state & STATE_PRELIGHT)
    c = prelight_color (c, color_name, ctype);
  return c;
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
