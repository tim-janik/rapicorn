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
class Style : public virtual ReferenceCountImpl {
  Appearance &m_appearance;
  String      m_name;
public:
  explicit              Style           (Appearance     &appearance,
                                         const String   &name);
  String                name            () const                        { return m_name; }
  virtual Color         color           (StateType       state,
                                         const String   &color_name,
                                         ColorType       ctype);
  Color                 color           (StateType       state,
                                         ColorType       ctype)         { return color (state, "", ctype); }
  Style*                create_style    (const String   &style_name);
  Style*                create_style    ()                              { return create_style (name()); }
  virtual               ~Style          ();
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
