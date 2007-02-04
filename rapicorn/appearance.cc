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
#include "appearance.hh"

namespace Rapicorn {

const Style::ColorSchemeKind &Style::STANDARD = *(ColorSchemeKind*) 0xC0FFEE41;
const Style::ColorSchemeKind &Style::SELECTED = *(ColorSchemeKind*) 0xC0FFEE42;
const Style::ColorSchemeKind &Style::INPUT = *(ColorSchemeKind*) 0xC0FFEE43;

Style::Style (Appearance     &appearance,
              const String   &name) :
  m_appearance (appearance),
  m_name (name)
{
  m_appearance.register_style (*this);
}

Style*
Style::create_style (const String   &style_name)
{
  return m_appearance.create_style (style_name);
}

const ColorScheme&
Style::color_scheme (const ColorSchemeKind &kind) const
{
  return ColorScheme::default_scheme();
}

Color
Style::standard_color (StateType      state,
                       ColorType      color_type) const
{
  return color_scheme (STANDARD).make_color (color_type);
}

Color
Style::selected_color (StateType      state,
                       ColorType      color_type) const
{
  return color_scheme (SELECTED).make_color (color_type);
}

Color
Style::input_color (StateType      state,
                    ColorType      color_type) const
{
  return color_scheme (INPUT).make_color (color_type);
}

Color
Style::resolve_color (const String &color_name,
                      StateType     state,
                      ColorType     color_type)
{
  EnumTypeColorType ect;
  if (color_name[0] == '#')
    {
      uint32 argb = string_to_int (&color_name[1], 16);
      /* invert alpha */
      Color c (argb);
      c.alpha (0xff - c.alpha());
      return c; // return argb;
    }
  const EnumClass::Value *value = ect.find_first (color_name);
  if (value)
    return standard_color (state, ColorType (value->value));
  else
    {
      Color parsed_color = Color::from_name (color_name);
      if (!parsed_color)
        parsed_color = standard_color (state, color_type);
      return parsed_color;
    }
}

Style::~Style ()
{
  m_appearance.unregister_style (*this);
}

Color
ColorScheme::make_light_color (Color source_color) const
{
  Color c = source_color;
  double hue, saturation, value;
  c.get_hsv (&hue, &saturation, &value);
  value *= 1.1;
  c.set_hsv (hue, saturation, value);
  return c;
}

Color
ColorScheme::make_dark_color (Color source_color) const
{
  Color c = source_color;
  double hue, saturation, value;
  c.get_hsv (&hue, &saturation, &value);
  value *= 0.9;
  c.set_hsv (hue, saturation, value);
  return c;
}

Color
ColorScheme::make_insensitive_color (Color     source_color,
                                     ColorType ctype) const
{
  Color c = source_color;
  double hue, saturation, value;
  c.get_hsv (&hue, &saturation, &value);
  saturation *= 0.8;
  value *= 1.075;
  c.set_hsv (hue, saturation, MIN (1, value));
  return c;
}

Color
ColorScheme::make_prelight_color (Color     source_color,
                                  ColorType ctype) const
{
  Color c = source_color;
  switch (ctype)
    {
    case COLOR_FOREGROUND:
    case COLOR_FOCUS:
    case COLOR_DEFAULT:
      return c;
    default: ;
    }
  double hue, saturation, value;
  c.get_hsv (&hue, &saturation, &value);
  saturation *= 1.2;
  c.set_hsv (hue, MIN (1, saturation), value);
  return c;
}

Color
ColorScheme::make_impressed_color (Color     source_color,
                                   ColorType ctype) const
{
  Color c = source_color;
  switch (ctype)
    {
    case COLOR_FOREGROUND:
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
ColorScheme::make_focus_state_color (Color     source_color,
                                     ColorType ctype) const
{
  Color c = source_color;
  return c;
}

Color
ColorScheme::make_default_state_color (Color     source_color,
                                       ColorType ctype) const
{
  Color c = source_color;
  return c;
}

Color
ColorScheme::make_state_color (StateType      state,
                               Color          color,
                               ColorType      ctype) const
{
  Color c = color;
  if (state & STATE_IMPRESSED)
    c = make_impressed_color (c, ctype);
  if (state & STATE_DEFAULT)
    c = make_default_state_color (c, ctype);
  if (state & STATE_FOCUS)
    c = make_focus_state_color (c, ctype);
  if (state & STATE_INSENSITIVE)
    c = make_insensitive_color (c, ctype);
  if ((state & STATE_PRELIGHT) &&
      !(state & STATE_INSENSITIVE))     /* ignore prelight if insensitive */
    c = make_prelight_color (c, ctype);
  return c;
}

Color
ColorScheme::generic_color (Color source_color) const
{
  return source_color;
}

const ColorScheme&
ColorScheme::default_scheme ()
{
  static const class DefaultColorScheme : public virtual ColorScheme {
    virtual Color
    make_color (ColorType color_type) const
    {
      Color c;
#if 1
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
        }
#elif 0
      switch (color_type)
        {
          double hue, saturation, value;
        default:
        case COLOR_NONE:                    c = 0x00000000; break;
        case COLOR_FOREGROUND:              c = 0xff000000; break;
        case COLOR_BACKGROUND:              c = 0xff6060c0; break;
        case COLOR_FOCUS:                   c = 0xff000000; break;
        case COLOR_DEFAULT:                 c = 0xffd0d000; break;
        case COLOR_LIGHT_GLINT:
          c = make_color (COLOR_BACKGROUND);
          c.get_hsv (&hue, &saturation, &value);
          value *= 1.3;
          c.set_hsv (hue, saturation, value);
          break;
        case COLOR_LIGHT_SHADOW:
          c = make_color (COLOR_BACKGROUND);
          c.get_hsv (&hue, &saturation, &value);
          value *= 1.15;
          c.set_hsv (hue, saturation, value);
          break;
        case COLOR_DARK_GLINT:
          c = make_color (COLOR_BACKGROUND);
          c.get_hsv (&hue, &saturation, &value);
          value *= 0.8;
          c.set_hsv (hue, saturation, value);
          break;
        case COLOR_DARK_SHADOW:
          c = make_color (COLOR_BACKGROUND);
          c.get_hsv (&hue, &saturation, &value);
          value *= 0.7;
          c.set_hsv (hue, saturation, value);
          break;
        }
#endif
      return Color (c);
    }
  } default_color_scheme;
  return default_color_scheme;
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
