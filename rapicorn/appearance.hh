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
#ifndef __RAPICORN_APPEARANCE_HH__
#define __RAPICORN_APPEARANCE_HH__

#include <rapicorn/primitives.hh>

namespace Rapicorn {

class Appearance;
class ColorCube {
public:
  virtual Color convert_color   (Color  color) const = 0;
};
class Style : public virtual ReferenceCountImpl {
  Appearance &m_appearance;
  String      m_name;
  typedef const ColorCube & ColorCubeRef;
public:
  explicit              Style           (Appearance     &appearance,
                                         const String   &name);
  String                name            () const                        { return m_name; }
  Style*                create_style    (const String   &style_name);
  Style*                create_style    ()                              { return create_style (name()); }
  virtual               ~Style          ();

  static const ColorCube &STANDARD;
  static const ColorCube &SELECTED;
  static const ColorCube &INPUT;
  virtual Color         cube_color              (ColorCubeRef   cube,
                                                 StateType      state,
                                                 Color          rgb_color,
                                                 ColorType      color_type = COLOR_NONE) const;
  virtual Color         cube_color              (ColorCubeRef   cube,
                                                 StateType      state,
                                                 ColorType      color_type) const;
  static Color          state_convert_color     (StateType      state,
                                                 Color          color,
                                                 ColorType      ctype = COLOR_NONE);
  static Color          insensitive_color       (Color          source_color, ColorType ctype = COLOR_NONE);
  static Color          prelight_color          (Color          source_color, ColorType ctype = COLOR_NONE);
  static Color          impressed_color         (Color          source_color, ColorType ctype = COLOR_NONE);
  static Color          focus_state_color       (Color          source_color, ColorType ctype = COLOR_NONE);
  static Color          default_state_color     (Color          source_color, ColorType ctype = COLOR_NONE);
  static Color          darken_color            (Color          source_color);
  static Color          lighten_color           (Color          source_color);
  /* convenience */
  Color                 standard_color          (StateType      state,
                                                 ColorType      color_type = COLOR_NONE) const
  { return cube_color (STANDARD, state, color_type); }
  Color                 selected_color          (StateType      state,
                                                 ColorType      color_type = COLOR_NONE) const
  { return cube_color (SELECTED, state, color_type); }
  Color                 input_color             (StateType      state,
                                                 ColorType      color_type = COLOR_NONE) const
  { return cube_color (INPUT, state, color_type); }
};

class Appearance : public virtual ReferenceCountImpl {
  const String         &m_name;
  map<String,Style*>    m_styles;
  explicit              Appearance       (const String   &name);
  void                  register_style   (Style          &style);
  void                  unregister_style (Style          &style);
  friend class Style;
public:
  String                name             () const                       { return m_name; }
  virtual Style*        create_style     (const String   &style_name);
  virtual               ~Appearance      ();
  static Appearance*    create_named     (const String   &name);
  static Appearance*    create_default   ();
};

} // Rapicorn

#endif  /* __RAPICORN_APPEARANCE_HH__ */
