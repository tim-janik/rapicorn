/* Rapicorn
 * Copyright (C) 2005-2006 Tim Janik
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
#ifndef __RAPICORN_TEXT_PANGO_HH__
#define __RAPICORN_TEXT_PANGO_HH__

#include <rapicorn/utilities.hh>
#include <rapicorn/text-editor.hh>

namespace Rapicorn {

class TextField : public virtual Convertible {
public:
  virtual TextMode text_mode    () const = 0;
  virtual void     text_mode    (TextMode      text_mode) = 0;
  virtual String   markup_text  () const = 0;
  virtual void     markup_text  (const String &markup) = 0;
};

#if     RAPICORN_WITH_PANGO
class TextPango : public virtual TextField { // FIXME: move to Text::EditorClient
public:
  virtual void          font_name       (const String &fname) = 0;
  virtual String        font_name       () const = 0;
  virtual AlignType     align           () const = 0;
  virtual void          align           (AlignType at) = 0;
  virtual EllipsizeType ellipsize       () const = 0;
  virtual void          ellipsize       (EllipsizeType et) = 0;
  virtual uint16        spacing         () const = 0;
  virtual void          spacing         (uint16 sp) = 0;
  virtual int16         indent          () const = 0;
  virtual void          indent          (int16 sp) = 0;
  virtual void          text            (const String &text) = 0;
  virtual String        text            () const = 0;
};
#endif  /* RAPICORN_WITH_PANGO */

} // Rapicorn

#endif  /* __RAPICORN_TEXT_PANGO_HH__ */
