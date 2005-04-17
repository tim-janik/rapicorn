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
#include "pangolabel.hh"
#include <pango/pangoft2.h>
#include "factory.hh"
#include "itemimpl.hh"

namespace Rapicorn {

#define PANGO_ISCALE    (1.0 / PANGO_SCALE)

class PangoLabelImpl : public virtual ItemImpl, public virtual PangoLabel {
  String                m_text;
  PangoLayout          *m_layout;
  PangoFontDescription *m_font_desc;
  PangoAttrList        *m_attr_list;
  WrapType              m_wrap;
  static PangoLanguage*
  default_pango_language()
  {
    String lc_ctype = setlocale (LC_CTYPE, NULL);
    uint sep1 = lc_ctype.find ('.');
    uint sep2 = lc_ctype.find ('@');
    uint sep = MIN (sep1, sep2);
    if (sep != lc_ctype.npos)
      lc_ctype[sep] = 0;
    PangoLanguage *pango_language = pango_language_from_string (lc_ctype.c_str());
    return pango_language; /* singleton reference */
  }
  static PangoDirection
  default_text_direction()
  {
    /* TRANSLATORS: Leave as language-direction:LTR for standard left-to-right (text)
     * layout direction. Translate to language-direction:RTL for right-to-left layout.
     */
    const char *ltr_dir = "language-direction:LTR", *rtl_dir = "language-direction:RTL", *default_dir = _("language-direction:LTR");
    PangoDirection pango_direction = PANGO_DIRECTION_LTR;
    if (strcmp (default_dir, rtl_dir) == 0)
      pango_direction = PANGO_DIRECTION_RTL;
    else if (strcmp (default_dir, ltr_dir) == 0)
      pango_direction = PANGO_DIRECTION_LTR;
    else
      warning ("invalid translation of \"%s\": %s", ltr_dir, default_dir);
    return pango_direction;
  }
  static const char*
  default_font_name()
  {
    return "Sans 10";
  }
  static PangoFontDescription*
  create_default_font_description()
  {
    PangoFontDescription *font_desc = pango_font_description_from_string (default_font_name());
    if (!pango_font_description_get_family (font_desc))
      pango_font_description_set_family (font_desc, "Sans");
    if (pango_font_description_get_size (font_desc) <= 0)
      pango_font_description_set_size (font_desc, 10 * PANGO_SCALE);
    return font_desc;
  }
  static Point
  get_dpi_for_pango ()
  {
    return Point (96, 96);
  }
  static PangoLayout*
  create_layout()
  {
    Point dpi = get_dpi_for_pango();
    PangoContext *context = pango_ft2_get_context (dpi.x, dpi.y);
    pango_context_set_base_dir (context, default_text_direction());
    pango_context_set_language (context, default_pango_language());
    PangoFontDescription *font_desc = create_default_font_description();
    pango_context_set_font_description (context, font_desc);
    pango_font_description_free (font_desc);
    PangoLayout *layout = pango_layout_new (context);
    g_object_unref (context);
    pango_layout_set_alignment (layout, PANGO_ALIGN_LEFT);
    pango_layout_set_wrap (layout, pango_wrap_mode_from_wrap_type (WRAP_WORD));
    pango_layout_set_ellipsize (layout, PANGO_ELLIPSIZE_END);
    return layout;
  }
  static PangoAlignment
  pango_alignment_from_align_type (AlignType at)
  {
    switch (at)
      {
      case ALIGN_LEFT:    return PANGO_ALIGN_LEFT;
      case ALIGN_CENTER:  return PANGO_ALIGN_CENTER;
      case ALIGN_RIGHT:   return PANGO_ALIGN_RIGHT;
      }
  }
  static AlignType
  align_type_from_pango_alignment (PangoAlignment pa)
  {
    switch (pa)
      {
      case PANGO_ALIGN_LEFT:    return ALIGN_LEFT;
      case PANGO_ALIGN_CENTER:  return ALIGN_CENTER;
      case PANGO_ALIGN_RIGHT:   return ALIGN_RIGHT;
      }
  }
  static PangoWrapMode
  pango_wrap_mode_from_wrap_type (WrapType wt)
  {
    switch (wt)
      {
      case WRAP_NONE:   return PANGO_WRAP_CHAR;
      case WRAP_CHAR:   return PANGO_WRAP_WORD_CHAR;
      case WRAP_WORD:   return PANGO_WRAP_WORD;
      }
  }
  static PangoEllipsizeMode
  pango_ellipsize_mode_from_ellipsize_type (EllipsizeType et)
  {
    switch (et)
      {
      case ELLIPSIZE_NONE:      return PANGO_ELLIPSIZE_NONE;
      case ELLIPSIZE_START:     return PANGO_ELLIPSIZE_START;
      case ELLIPSIZE_MIDDLE:    return PANGO_ELLIPSIZE_MIDDLE;
      case ELLIPSIZE_END:       return PANGO_ELLIPSIZE_END;
      }
  }
  static EllipsizeType
  ellipsize_type_from_pango_ellipsize_mode (PangoEllipsizeMode em)
  {
    switch (em)
      {
      case PANGO_ELLIPSIZE_NONE:        return ELLIPSIZE_NONE;
      case PANGO_ELLIPSIZE_START:       return ELLIPSIZE_START;
      case PANGO_ELLIPSIZE_MIDDLE:      return ELLIPSIZE_MIDDLE;
      case PANGO_ELLIPSIZE_END:         return ELLIPSIZE_END;
      }
  }
  void
  apply_font_desc()
  {
    PangoFontDescription *font_desc = create_default_font_description();
    if (m_font_desc)
      pango_font_description_merge (font_desc, m_font_desc, TRUE);
    pango_layout_set_font_description (m_layout, font_desc);
    pango_font_description_free (font_desc);
  }
public:
  explicit PangoLabelImpl() :
    m_layout (create_layout()),
    m_font_desc (NULL),
    m_attr_list (NULL),
    m_wrap (WRAP_WORD)
  {
  }
  ~PangoLabelImpl()
  {
    g_object_unref (m_layout);
    if (m_attr_list)
      pango_attr_list_unref (m_attr_list);
    if (m_font_desc)
      pango_font_description_free (m_font_desc);
  }
  virtual void
  font_name (const String &fname)
  {
    PangoFontDescription *font_desc = pango_font_description_from_string (fname.c_str());
    if (m_font_desc)
      pango_font_description_free (m_font_desc);
    m_font_desc = font_desc;
    apply_font_desc ();
    invalidate();
  }
  virtual String
  font_name() const
  {
    PangoFontDescription *font_desc = create_default_font_description();
    if (m_font_desc)
      pango_font_description_merge (font_desc, m_font_desc, TRUE);
    char *fname = pango_font_description_to_string (font_desc);
    String font (fname);
    g_free (fname);
    pango_font_description_free (font_desc);
    return fname;
  }
  virtual AlignType
  align () const
  {
    return align_type_from_pango_alignment (pango_layout_get_alignment (m_layout));
  }
  virtual void
  align (AlignType at)
  {
    pango_layout_set_alignment (m_layout, pango_alignment_from_align_type (at));
    invalidate();
  }
  virtual WrapType
  wrap () const
  {
    return m_wrap;
  }
  virtual void
  wrap (WrapType wt)
  {
    if (m_wrap != wt)
      {
        m_wrap = wt;
        pango_layout_set_wrap (m_layout, pango_wrap_mode_from_wrap_type (m_wrap));
        invalidate();
      }
  }
  virtual EllipsizeType
  ellipsize () const
  {
    return ellipsize_type_from_pango_ellipsize_mode (pango_layout_get_ellipsize (m_layout));
  }
  virtual void
  ellipsize (EllipsizeType et)
  {
    pango_layout_set_ellipsize (m_layout, pango_ellipsize_mode_from_ellipsize_type (et));
  }
  virtual uint16
  spacing () const
  {
    return pango_layout_get_spacing (m_layout);
  }
  virtual void
  spacing (uint16 sp)
  {
    pango_layout_set_spacing (m_layout, sp);
  }
  virtual int16
  indent () const
  {
    return pango_layout_get_indent (m_layout);
  }
  virtual void
  indent (int16 sp)
  {
    pango_layout_set_indent (m_layout, sp);
  }
  virtual void
  text (const String &txt)
  {
    if (m_text != txt)
      {
        m_text = txt;
        pango_layout_set_text (m_layout, m_text.c_str(), -1);
        invalidate();
      }
  }
  virtual String
  text () const
  {
    return m_text;
  }
protected:
  virtual void
  size_request (Requisition &requisition)
  {
    PangoRectangle rect = { 0, 0 };
    pango_layout_set_width (m_layout, -1);
    pango_layout_get_extents (m_layout, NULL, &rect);
    requisition.width = 1 + PANGO_PIXELS (rect.width);
    requisition.height = 1 + PANGO_PIXELS (rect.height);
  }
  virtual void
  size_allocate (Allocation area)
  {
    allocation (area);
    if (ellipsize() != ELLIPSIZE_NONE)
      {
        pango_layout_set_width (m_layout, (max (1, allocation().width) - 1) * PANGO_SCALE);
        pango_layout_set_text (m_layout, m_text.c_str(), -1);
      }
  }
  static const PangoFontDescription*
  layout_get_font_description (PangoLayout *layout)
  {
    const PangoFontDescription *font_desc = pango_layout_get_font_description (layout);
    if (font_desc)
      return font_desc;
    return pango_context_get_font_description (pango_layout_get_context (layout));
  }
public:
  void
  render_dot (Plane        &plane,
              Affine        affine,
              int           x,
              int           y,
              int           width,
              int           height,
              int           dot_size,
              Color         col)
  {
    /* render bitmap to plane */
    Color fg (col.premultiplied());
    int offset = height - dot_size;
    y += offset / 2;
    height = dot_size;
    offset = width - dot_size;
    x += offset / 2;
    width = dot_size;
    int xmin = MAX (x, plane.xstart()), xbound = MIN (x + width, plane.xbound());
    int ymin = MAX (y, plane.ystart()), ybound = MIN (y + height, plane.ybound());
    int xspan = xbound - xmin;
    for (int iy = ymin; iy < ybound; iy++)
      {
        uint32 *pp = plane.peek (xmin - plane.xstart(), iy - plane.ystart()), *p = pp;
        while (p < pp + xspan)
          *p++ = fg;
      }
  }
  void
  render (Plane        &plane,
          Affine        affine,
          int           x,
          int           y,
          int           width,
          int           height,
          int           dot_size,
          Color         col)
  {
    if (dot_size)
      {
        render_dot (plane, affine, x, y, width / 3, height, dot_size, col);
        render_dot (plane, affine, x + width / 3, y, width / 3, height, dot_size, col);
        render_dot (plane, affine, x + 2 * width / 3, y, width / 3, height, dot_size, col);
        return;
      }
    if (ellipsize() != ELLIPSIZE_NONE) // FIXME: size request/alloc workaround
      pango_layout_set_width (m_layout, width * PANGO_SCALE);
    /* render text to alpha bitmap */
    FT_Bitmap bitmap;
    bitmap.buffer = NULL;
    bitmap.rows = height;
    bitmap.width = width;
    bitmap.pitch = (bitmap.width + 3) & ~3;
    bitmap.buffer = new uint8[bitmap.rows * bitmap.pitch];
    memset (bitmap.buffer, 0, bitmap.rows * bitmap.pitch);
    bitmap.num_grays = 256;
    bitmap.pixel_mode = ft_pixel_mode_grays;
    pango_ft2_render_layout (&bitmap, m_layout, 0, 0);
    /* render bitmap to plane */
    Color fg (col.premultiplied());
    uint8 red = fg.red(), green = fg.green(), blue = fg.blue();
    int xmin = MAX (x, plane.xstart()), xbound = MIN (x + width, plane.xbound());
    int ymin = MAX (y, plane.ystart()), ybound = MIN (y + height, plane.ybound());
    int xspan = xbound - xmin;
    for (int iy = ymin; iy < ybound; iy++)
      {
        uint32 *pp = plane.peek (xmin - plane.xstart(), iy - plane.ystart()), *p = pp;
        uint8  *ba = &bitmap.buffer[(bitmap.rows - 1 - iy + y) * bitmap.pitch + xmin - x];
        while (p < pp + xspan)
          {
            uint8 alpha = *ba++;
            uint8 r = Color::IMUL (red, alpha), g = Color::IMUL (green, alpha), b = Color::IMUL (blue, alpha);
            *p++ = Color (r, g, b, alpha);
          }
      }
    /* cleanup */
    delete[] bitmap.buffer;
  }
  void
  render (Plane        &plane,
          Affine        affine)
  {
    int x = allocation().x, y = allocation().y, width = allocation().width, height = allocation().height;
    bool vellipsize = ellipsize() != ELLIPSIZE_NONE && height < requisition().height;
    width -= 1;
    height -= 1;
    if (width < 1 || height < 1)
      return;
    PangoRectangle rect = { 0, 0 };
    pango_layout_get_extents (m_layout, NULL, &rect);
    int pixels = PANGO_PIXELS (rect.height);
    if (pixels < height)
      {
        int extra = height - pixels;
        y += extra / 2;
        height -= extra;
      }
    uint dot_size = 0;
    if (vellipsize)
      dot_size = max (1, iround (pango_font_description_get_size (layout_get_font_description (m_layout)) *
                                 get_dpi_for_pango().y) / (PANGO_SCALE * 72 * 6));
    if (insensitive())
      {
        x += 1;
        render (plane, affine, x, y, width, height, dot_size, white());
        x -= 1;
        y += 1;
        Plane emboss = Plane::create_from_size (plane);
        render (emboss, affine, x, y, width, height, dot_size, dark_shadow());
        plane.combine (emboss, COMBINE_OVER);
      }
    else
      {
        x += 1;
        y += 1;
        render (plane, affine, x, y, width, height, dot_size, foreground());
      }
  }
  virtual const PropertyList&
  list_properties()
  {
    static Property *properties[] = {
      MakeProperty (PangoLabel, align,     _("Align Type"), _("The kind of text alignment"), ALIGN_LEFT, "rw"),
      MakeProperty (PangoLabel, wrap,      _("Wrap Type"),      _("The type of wrapping applied when breaking lines"), WRAP_WORD, "rw"),
      MakeProperty (PangoLabel, ellipsize, _("Ellipsize Type"), _("Whether and where to reduce the text size"), ELLIPSIZE_END, "rw"),
      MakeProperty (PangoLabel, spacing,   _("Spacing"),        _("The amount of space between lines"), 0, 0, 65535, 0, "rw"),
      MakeProperty (PangoLabel, indent,    _("Indent"),         _("The paragraph indentation"), 0, -32767, 32767, 0, "rw"),
      MakeProperty (PangoLabel, text,      _("Text"),           _("The text to display"), "", "rw"),
      MakeProperty (PangoLabel, font_name, _("Font Name"),      _("The type of font to use"), "Sans 10", "rw"),
    };
    static const PropertyList property_list (properties, Item::list_properties());
    return property_list;
  }
};

static const ItemFactory<PangoLabelImpl> pango_label_factory ("Rapicorn::PangoLabel");

} // Rapicorn
