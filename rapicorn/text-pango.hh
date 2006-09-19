/* Rapicorn
 * Copyright (C) 2005-2006 Tim Janik
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
#ifndef __RAPICORN_TEXT_PANGO_HH__
#define __RAPICORN_TEXT_PANGO_HH__

#include <rapicorn/utilities.hh>
#include <rapicorn/text-editor.hh>

#if     RAPICORN_WITH_PANGO
namespace Rapicorn {

class TextPango : public virtual Convertible { // FIXME: move to Text::EditorClient
public:
  virtual void          font_name       (const String &fname) = 0;
  virtual String        font_name       () const = 0;
  virtual AlignType     align           () const = 0;
  virtual void          align           (AlignType at) = 0;
  virtual WrapType      wrap            () const = 0;
  virtual void          wrap            (WrapType wt) = 0;
  virtual EllipsizeType ellipsize       () const = 0;
  virtual void          ellipsize       (EllipsizeType et) = 0;
  virtual uint16        spacing         () const = 0;
  virtual void          spacing         (uint16 sp) = 0;
  virtual int16         indent          () const = 0;
  virtual void          indent          (int16 sp) = 0;
  virtual void          text            (const String &text) = 0;
  virtual String        text            () const = 0;
};

} // Rapicorn
#endif  /* RAPICORN_WITH_PANGO */

#endif  /* __RAPICORN_TEXT_PANGO_HH__ */
