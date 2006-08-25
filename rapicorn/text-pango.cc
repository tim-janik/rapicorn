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
#include "pangolabel.hh"
#if     RAPICORN_WITH_PANGO
#include <pango/pangoft2.h>
#include "factory.hh"
#include "itemimpl.hh"
#include "viewport.hh"  // for rapicorn_gtk_threads_enter / rapicorn_gtk_threads_leave

#define PANGO_ISCALE    (1.0 / PANGO_SCALE)

namespace Rapicorn {

/* --- Pango support code --- */
static const char*
default_text_language()
{
  static const char *language = NULL;
  if (!language)
    {
      String lc_ctype = setlocale (LC_CTYPE, NULL);
      String::size_type sep1 = lc_ctype.find ('.');
      String::size_type sep2 = lc_ctype.find ('@');
      String::size_type sep = MIN (sep1, sep2);
      if (sep != lc_ctype.npos)
        lc_ctype[sep] = 0;
      language = strdup (lc_ctype.c_str());
    }
  return language;
}

static PangoDirection
default_pango_direction()
{
  /* TRANSLATORS: Leave as language-direction:LTR for standard left-to-right (text)
   * layout direction. Translate to language-direction:RTL for right-to-left layout.
   */
  const char *ltr_dir = "language-direction:LTR", *rtl_dir = "language-direction:RTL", *default_dir = _("language-direction:LTR");
  PangoDirection pdirection = PANGO_DIRECTION_LTR;
  if (strcmp (default_dir, rtl_dir) == 0)
    pdirection = PANGO_DIRECTION_RTL;
  else if (strcmp (default_dir, ltr_dir) == 0)
    pdirection = PANGO_DIRECTION_LTR;
  else
    warning ("invalid translation of \"%s\": %s", ltr_dir, default_dir);
  return pdirection;
}

static Point
default_pango_dpi ()
{
  return Point (96, 96);
}

static const PangoFontDescription*
default_pango_font_description()
{
  static const char *default_font_name = "Sans 10";
  static PangoFontDescription *font_desc = NULL;
  if (!font_desc)
    {
      font_desc = pango_font_description_from_string (default_font_name);
      if (!pango_font_description_get_family (font_desc))
        pango_font_description_set_family (font_desc, "Sans");
      if (pango_font_description_get_size (font_desc) <= 0)
        pango_font_description_set_size (font_desc, 10 * PANGO_SCALE);
    }
  return font_desc;
}

static void
default_font_config_func (FcPattern *pattern,
                          gpointer   data)
{
  bool antialias = true, hinting = true, autohint = false;
  int hint_style = 4; /* 0:unset, 1:none, 2:slight, 3:medium, 4:full */
  int subpxorder = 1; /* 0:unset, 1:none, 2:RGB, 3:BGR, 4:VRGB, 5:VBGR */
  // subpixel order != none needs FT_PIXEL_MODE_LCD or FT_PIXEL_MODE_LCD_V
  FcValue v;
  
  if (FcPatternGet (pattern, FC_ANTIALIAS, 0, &v) == FcResultNoMatch)
    FcPatternAddBool (pattern, FC_ANTIALIAS, antialias);
  if (FcPatternGet (pattern, FC_HINTING, 0, &v) == FcResultNoMatch)
    FcPatternAddBool (pattern, FC_HINTING, hinting);
  if (hint_style && FcPatternGet (pattern, FC_HINT_STYLE, 0, &v) == FcResultNoMatch)
    {
      switch (hint_style)
        {
        case 1: FcPatternAddInteger (pattern, FC_HINT_STYLE, FC_HINT_NONE);   break;
        case 2: FcPatternAddInteger (pattern, FC_HINT_STYLE, FC_HINT_SLIGHT); break;
        case 3: FcPatternAddInteger (pattern, FC_HINT_STYLE, FC_HINT_MEDIUM); break;
        case 4: FcPatternAddInteger (pattern, FC_HINT_STYLE, FC_HINT_FULL);   break;
        default:      break;
        }
    }
  if (FcPatternGet (pattern, FC_AUTOHINT, 0, &v) == FcResultNoMatch)
    FcPatternAddBool (pattern, FC_AUTOHINT, autohint);
  if (subpxorder && FcPatternGet (pattern, FC_RGBA, 0, &v) == FcResultNoMatch)
    {
      switch (subpxorder)
        {
        case 1: FcPatternAddInteger (pattern, FC_RGBA, FC_RGBA_NONE); break;
        case 2: FcPatternAddInteger (pattern, FC_RGBA, FC_RGBA_RGB);  break;
        case 3: FcPatternAddInteger (pattern, FC_RGBA, FC_RGBA_BGR);  break;
        case 4: FcPatternAddInteger (pattern, FC_RGBA, FC_RGBA_VRGB); break;
        case 5: FcPatternAddInteger (pattern, FC_RGBA, FC_RGBA_VBGR); break;
        default:      break;
        }
    }
}

static PangoAlignment
pango_alignment_from_align_type (AlignType at)
{
  switch (at)
    {
    default:
    case ALIGN_LEFT:    return PANGO_ALIGN_LEFT;
    case ALIGN_CENTER:  return PANGO_ALIGN_CENTER;
    case ALIGN_RIGHT:   return PANGO_ALIGN_RIGHT;
    }
}

#if 0 // unused
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
#endif

static PangoWrapMode
pango_wrap_mode_from_wrap_type (WrapType wt)
{
  switch (wt)
    {
    default:
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
    default:
    case ELLIPSIZE_NONE:      return PANGO_ELLIPSIZE_NONE;
    case ELLIPSIZE_START:     return PANGO_ELLIPSIZE_START;
    case ELLIPSIZE_MIDDLE:    return PANGO_ELLIPSIZE_MIDDLE;
    case ELLIPSIZE_END:       return PANGO_ELLIPSIZE_END;
    }
}

#if 0 // unused
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
#endif

/* --- LayoutCache --- */
#define RETURN_LESS_IF_UNEQUAL(lhs,rhs) { if (lhs < rhs) return 1; else if (rhs < lhs) return 0; }
class LayoutCache {
  static const uint   cache_threshold   = 100; /* threshold beyond which the cache needs trimming */
  static const double cache_probability = 0.5; /* probability for keeping an element alive */
  struct ContextKey {
    PangoDirection direction;
    String         language;
    Point          dpi;
    inline bool
    operator< (const ContextKey &rhs) const
    {
      const ContextKey &lhs = *this;
      RETURN_LESS_IF_UNEQUAL (lhs.direction, rhs.direction);
      RETURN_LESS_IF_UNEQUAL (lhs.language,  rhs.language);
      RETURN_LESS_IF_UNEQUAL (lhs.dpi.x,     rhs.dpi.x);
      RETURN_LESS_IF_UNEQUAL (lhs.dpi.y,     rhs.dpi.y);
      return 0;
    }
  };
  std::map<ContextKey, PangoContext*> context_cache;
  PangoContext*
  retrieve_context (ContextKey key)
  {
    PangoContext *pcontext = context_cache[key];
    if (!pcontext)
      {
        PangoFontMap *fontmap = pango_ft2_font_map_new();
        pango_ft2_font_map_set_default_substitute (PANGO_FT2_FONT_MAP (fontmap), default_font_config_func, NULL, NULL);
        pango_ft2_font_map_set_resolution (PANGO_FT2_FONT_MAP (fontmap), key.dpi.x, key.dpi.y);
        pcontext = pango_ft2_font_map_create_context (PANGO_FT2_FONT_MAP (fontmap));
        g_object_unref (fontmap);
        pango_context_set_base_dir (pcontext, key.direction);
        pango_context_set_language (pcontext, pango_language_from_string (key.language.c_str()));
        pango_context_set_font_description (pcontext, default_pango_font_description());
        /* trim cache before insertion */
        if (context_cache.size() >= cache_threshold)
          {
            std::map<ContextKey, PangoContext*>::iterator it = context_cache.begin();
            while (it != context_cache.end())
              {
                std::map<ContextKey, PangoContext*>::iterator last = it++;
                if (drand48() > cache_probability)
                  {
                    if (last->second)   // maybe NULL due to failing lookups
                      g_object_unref (last->second);
                    context_cache.erase (last);
                  }
              }
          }
        context_cache[key] = pcontext; // takes over initial ref()
      }
    return pcontext;
  }
public:
  PangoLayout*
  fetch_layout (String         font_description,
                AlignType      align,
                WrapType       wrap,
                EllipsizeType  ellipsize,
                int            indent,
                int            spacing)
  {
    ContextKey key;
    key.direction = default_pango_direction();
    key.language = default_text_language();
    key.dpi = default_pango_dpi();
    PangoContext *pcontext = retrieve_context (key);
    PangoFontDescription *pfdesc;
    if (font_description.size())
      pfdesc = pango_font_description_from_string (font_description.c_str());
    else
      pfdesc = pango_font_description_copy_static (default_pango_font_description());
    gchar *gstr = pango_font_description_to_string (pfdesc);
    font_description = gstr;
    g_free (gstr);
    PangoLayout *playout = pango_layout_new (pcontext);
    pango_layout_set_alignment (playout, pango_alignment_from_align_type (align));
    pango_layout_set_wrap (playout, pango_wrap_mode_from_wrap_type (wrap));
    pango_layout_set_ellipsize (playout, pango_ellipsize_mode_from_ellipsize_type (ellipsize));
    pango_layout_set_indent (playout, indent);
    pango_layout_set_spacing (playout, spacing);
    pango_layout_set_attributes (playout, NULL);
    pango_layout_set_tabs (playout, NULL);
    pango_layout_set_width (playout, -1);
    pango_layout_set_text (playout, "", 0);
    pango_font_description_merge_static (pfdesc, default_pango_font_description(), FALSE);
    pango_layout_set_font_description (playout, pfdesc);
    pango_font_description_free (pfdesc);
    return playout;
  }
  void
  return_layout (PangoLayout *playout)
  {
    g_object_unref (playout);
  }
  static String
  font_description_from_layout (PangoLayout *playout)
  {
    const PangoFontDescription *cfdesc = pango_layout_get_font_description (playout);
    if (!cfdesc)
      cfdesc = pango_context_get_font_description (pango_layout_get_context (playout));
    gchar *gstr = pango_font_description_to_string (cfdesc);
    String font_desc = gstr;
    g_free (gstr);
    return font_desc;
  }
  static double
  dot_size_from_layout (PangoLayout *playout)
  {
    const PangoFontDescription *cfdesc = pango_layout_get_font_description (playout);
    if (!cfdesc)
      cfdesc = pango_context_get_font_description (pango_layout_get_context (playout));
    // FIXME: hardcoded dpi, assumes relative size
    double dot_size = (pango_font_description_get_size (cfdesc) * default_pango_dpi().y) / (PANGO_SCALE * 72 * 6);
    return max (1, iround (dot_size));
  }
};
static LayoutCache global_layout_cache; // protected by rapicorn_gtk_threads_enter / rapicorn_gtk_threads_leave

/* --- PangoLabelImpl --- */
class PangoLabelImpl : public virtual ItemImpl, public virtual PangoLabel {
  String                m_text;
  String                m_font_desc;
  AlignType             m_align;
  WrapType              m_wrap;
  EllipsizeType         m_ellipsize;
  uint16                m_indent, m_spacing;
  PangoLayout*
  acquire_layout() const
  {
    rapicorn_gtk_threads_enter();
    PangoLayout *playout = global_layout_cache.fetch_layout (m_font_desc, m_align, m_wrap, m_ellipsize, m_indent, m_spacing);
    pango_layout_set_attributes (playout, NULL);
    pango_layout_set_tabs (playout, NULL);
    pango_layout_set_width (playout, -1);
    pango_layout_set_text (playout, m_text.c_str(), -1);
    return playout;
  }
  void
  release_layout (PangoLayout *&playout) const
  {
    pango_layout_set_attributes (playout, NULL);
    pango_layout_set_tabs (playout, NULL);
    pango_layout_set_width (playout, -1);
    pango_layout_set_text (playout, "", 0);
    global_layout_cache.return_layout (playout);
    rapicorn_gtk_threads_leave();
    playout = NULL;
  }
public:
  PangoLabelImpl() :
    m_align (ALIGN_LEFT), m_wrap (WRAP_WORD), m_ellipsize (ELLIPSIZE_END),
    m_indent (0), m_spacing (0)
  {}
  virtual void
  font_name (const String &font_name)
  {
    m_font_desc = font_name;
    invalidate();
  }
  virtual String
  font_name() const
  {
    PangoLayout *playout = acquire_layout();
    String font_desc = LayoutCache::font_description_from_layout (playout);
    release_layout (playout);
    return font_desc;
  }
  virtual AlignType     align     () const           { return m_align; }
  virtual void          align     (AlignType at)     { m_align = at; invalidate(); }
  virtual WrapType      wrap      () const           { return m_wrap; }
  virtual void          wrap      (WrapType wt)      { m_wrap = wt; invalidate(); }
  virtual EllipsizeType ellipsize () const           { return m_ellipsize; }
  virtual void          ellipsize (EllipsizeType et) { m_ellipsize = et; invalidate(); }
  virtual int16         indent    () const           { return m_indent; }
  virtual void          indent    (int16 ind)        { m_indent = ind; invalidate(); }
  virtual uint16        spacing   () const           { return m_spacing; }
  virtual void          spacing   (uint16 sp)        { m_spacing = sp; invalidate(); }
  virtual void          text      (const String &tx) { m_text = tx; invalidate(); }
  virtual String        text      () const           { return m_text; }
protected:
  virtual void
  size_request (Requisition &requisition)
  {
    PangoRectangle rect = { 0, 0 };
    PangoLayout *playout = acquire_layout();
    pango_layout_set_width (playout, -1);
    pango_layout_get_extents (playout, NULL, &rect);
    requisition.width = 1 + PANGO_PIXELS (rect.width);
    requisition.height = 1 + PANGO_PIXELS (rect.height);
    release_layout (playout);
  }
  virtual void
  size_allocate (Allocation area)
  {
    allocation (area);
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
  render (PangoLayout  *playout,
          Plane        &plane,
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
    /* render text to alpha bitmap */
    FT_Bitmap bitmap;
    bitmap.buffer = NULL;
    bitmap.rows = height;
    bitmap.width = width;
    bitmap.pitch = (bitmap.width + 3) & ~3;
    bitmap.buffer = new uint8[bitmap.rows * bitmap.pitch];
    memset (bitmap.buffer, 0, bitmap.rows * bitmap.pitch);
    bitmap.num_grays = 256;
    bitmap.pixel_mode = FT_PIXEL_MODE_GRAY;
    pango_ft2_render_layout (&bitmap, playout, 0, 0);
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
  render (Display &display)
  {
    PangoLayout *playout = acquire_layout();
    int x = allocation().x, y = allocation().y, width = allocation().width, height = allocation().height;
    bool vellipsize = ellipsize() != ELLIPSIZE_NONE && height < requisition().height;
    width -= 1;
    height -= 1;
    if (width < 1 || height < 1)
      return;
    if (m_ellipsize != ELLIPSIZE_NONE)
      pango_layout_set_width (playout, width * PANGO_SCALE);
    PangoRectangle rect = { 0, 0 };
    pango_layout_get_extents (playout, NULL, &rect);
    int pixels = PANGO_PIXELS (rect.height);
    if (pixels < height)
      {
        int extra = height - pixels;
        y += extra / 2;
        height -= extra;
      }
    uint dot_size = 0;
    if (vellipsize)
      dot_size = LayoutCache::dot_size_from_layout (playout);
    Plane &plane = display.create_plane ();
    if (insensitive())
      {
        x += 1;
        render (playout, plane, Affine(), x, y, width, height, dot_size, white());
        x -= 1;
        y += 1;
        Plane emboss (Plane::init_from_size (plane));
        render (playout, emboss, Affine(), x, y, width, height, dot_size, dark_shadow());
        plane.combine (emboss, COMBINE_OVER);
      }
    else
      {
        x += 1;
        y += 1;
        render (playout, plane, Affine(), x, y, width, height, dot_size, foreground());
      }
    release_layout (playout);
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
      MakeProperty (PangoLabel, font_name, _("Font Name"),      _("The type of font to use"), "", "rw"),
    };
    static const PropertyList property_list (properties, Item::list_properties());
    return property_list;
  }
};

static const ItemFactory<PangoLabelImpl> pango_label_factory ("Rapicorn::PangoLabel");

} // Rapicorn
#endif  /* RAPICORN_WITH_PANGO */
