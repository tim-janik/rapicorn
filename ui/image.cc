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
#include "image.hh"
#include "painter.hh"
#include "factory.hh"
#include <errno.h>

namespace Rapicorn {

const PropertyList&
Image::list_properties()
{
  static Property *properties[] = {
    MakeProperty (Image, image_file, _("Image Filename"), _("Load an image from a file, only PNG images can be loaded."), "w"),
    MakeProperty (Image, stock_pixmap, _("Stock Image"), _("Load an image from stock, providing a stock name."), "w"),
  };
  static const PropertyList property_list (properties, ItemImpl::list_properties());
  return property_list;
}

static const uint8* get_broken16_pixdata (void);

static cairo_surface_t*
cairo_surface_from_pixmap (Pixmap &pixmap)
{
  int stride = 0;
  uint32 *data = pixmap.data (&stride);
  cairo_surface_t *isurface =
    cairo_image_surface_create_for_data ((unsigned char*) data,
                                         CAIRO_FORMAT_ARGB32,
                                         pixmap.width(),
                                         pixmap.height(),
                                         stride);
  return_val_if_fail (CAIRO_STATUS_SUCCESS == cairo_surface_status (isurface), NULL);
  return isurface;
}

class ImageImpl : public virtual ItemImpl, public virtual Image {
  Pixmap           *m_pixmap;
  int               m_xoffset, m_yoffset;
public:
  explicit ImageImpl() :
    m_pixmap (NULL),
    m_xoffset (0),
    m_yoffset (0)
  {}
  void
  reset ()
  {
    m_xoffset = m_yoffset = 0;
    if (m_pixmap)
      {
        Pixmap *op = m_pixmap;
        m_pixmap = NULL;
        unref (op);
      }
    invalidate();
  }
  ~ImageImpl()
  {}
  virtual void
  pixmap (Pixmap *pixmap)
  {
    if (pixmap)
      ref_sink (pixmap);
    reset();
    m_pixmap = pixmap;
  }
  virtual Pixmap*
  pixmap (void)
  {
    return m_pixmap;
  }
  virtual void
  stock_pixmap (const String &stock_name)
  {
    Pixmap *pixmap = Pixmap::stock (stock_name);
    int saved = errno;
    if (!pixmap)
      {
        pixmap = Pixmap::pixstream (get_broken16_pixdata());
        assert (pixmap);
      }
    ref_sink (pixmap);
    this->pixmap (pixmap);
    unref (pixmap);
    errno = saved;
  }
  virtual void
  image_file (const String &filename)
  {
    Pixmap *pixmap = Pixmap::load_png (filename, true);
    int saved = errno;
    if (!pixmap)
      {
        pixmap = Pixmap::pixstream (get_broken16_pixdata());
        assert (pixmap);
      }
    ref_sink (pixmap);
    this->pixmap (pixmap);
    unref (pixmap);
    errno = saved;
  }
protected:
  virtual void
  size_request (Requisition &requisition)
  {
    if (m_pixmap)
      {
        requisition.width += m_pixmap->width();
        requisition.height += m_pixmap->height();
      }
  }
  virtual void
  size_allocate (Allocation area, bool changed)
  {
    if (m_pixmap)
      {
        m_xoffset = iround (0.5 * (area.width - m_pixmap->width()));
        m_yoffset = iround (0.5 * (area.height - m_pixmap->height()));
      }
    else
      m_xoffset = m_yoffset = 0;
  }
public:
  virtual void
  render (RenderContext    &rcontext,
          const Allocation &area)
  {
    if (m_pixmap)
      {
        int ix = iround (allocation().x + m_xoffset), iy = iround (allocation().y + m_yoffset);
        int ih = m_pixmap->height();
        cairo_t *cr = cairo_context (rcontext, Rect (ix, iy, m_pixmap->width(), ih));
        cairo_surface_t *isurface = cairo_surface_from_pixmap (*m_pixmap);
        cairo_set_source_surface (cr, isurface, 0, 0); // (ix,iy) are set in the matrix below
        cairo_matrix_t matrix;
        cairo_matrix_init_identity (&matrix);
        cairo_matrix_translate (&matrix, -ix, iy + m_pixmap->height());
        cairo_matrix_scale (&matrix, 1, -1);
        cairo_pattern_set_matrix (cairo_get_source (cr), &matrix);
        cairo_paint (cr);
        cairo_surface_destroy (isurface);
      }
  }
};
static const ItemFactory<ImageImpl> image_factory ("Rapicorn::Factory::Image");

static const uint8*
get_broken16_pixdata (void)
{
  /* GdkPixbuf RGBA C-Source image dump 1-byte-run-length-encoded */
#ifdef __SUNPRO_C
#pragma align 4 (broken16_pixdata)
#endif
#ifdef __GNUC__
  static const uint8 broken16_pixdata[] __attribute__ ((__aligned__ (4))) = 
#else
    static const uint8 broken16_pixdata[] = 
#endif
    { ""
      /* Pixbuf magic (0x47646b50) */
      "GdkP"
      /* length: header (24) + pixel_data (485) */
      "\0\0\1\375"
      /* pixdata_type (0x2010002) */
      "\2\1\0\2"
      /* rowstride (64) */
      "\0\0\0@"
      /* width (16) */
      "\0\0\0\20"
      /* height (16) */
      "\0\0\0\20"
      /* pixel_data: */
      "\221\0\0\0\0\203\0\0\0\377\2>\0\10\377\0\0\0\377\202\0\0\0\0\1\253\0"
      "\30\\\203\0\0\0\377\2\0\0\0\177\0\0\0\77\203\0\0\0\0\1\0\0\0\377\202"
      "\377\377\377\377\2\377r\205\377\377Jc\262\202\0\0\0\0\7\377D_\301\377"
      "\201\222\377\377\377\377\377\0\0\0\377\300\300\300\377\0\0\0\177\0\0"
      "\0\77\202\0\0\0\0\1\0\0\0\377\202\377\377\377\377\2\377\"A\377\377Kd"
      "D\202\0\0\0\0\7\377\32;\367\377\317\325\377\377\377\377\377\0\0\0\377"
      "\377\377\377\377\300\300\300\377\0\0\0\177\202\0\0\0\0\4\0\0\0\377\377"
      "\377\377\377\377\324\332\377\377\25""6\371\202\0\0\0\0\2\377Kd=\377\33"
      ";\377\202\377\377\377\377\204\0\0\0\377\202\0\0\0\0\4\0\0\0\377\377\377"
      "\377\377\377\206\227\377\377A\\\307\202\0\0\0\0\2\377.K\224\377k\177"
      "\377\205\377\377\377\377\1\0\0\0\377\202\0\0\0\0\11\0\0\0\377\377\377"
      "\377\377\377\257\272\377\377\5(\363\377\0$8\0\0\0\0\377\0$P\377\5(\363"
      "\377\305\315\370\204\377\377\377\377\1\0\0\0\377\202\0\0\0\0\1\0\0\0"
      "\377\202\377\377\377\377\7\377\273\305\366\377\4'\366\377\0$R\0\0\0\0"
      "\377\0$A\377\2%\364\377\247\264\360\203\377\377\377\377\1\0\0\0\377\202"
      "\0\0\0\0\1\0\0\0\377\203\377\377\377\377\7\377\325\333\376\377\12,\365"
      "\377\0$w\0\0\0\0\377\0$)\377\2%\355\377|\216\350\202\377\377\377\377"
      "\1\0\0\0\377\202\0\0\0\0\1\0\0\0\377\204\377\377\377\377\11\377\366\367"
      "\377\377(F\352\377\1%\251\377\0$\10\377\0$\11\377\3'\310\377F`\350\377"
      "\367\370\377\0\0\0\377\202\0\0\0\0\1\0\0\0\377\204\377\377\377\377\11"
      "\377\365\366\377\3777S\361\377\4'\256\377\0$\6\377\0$\12\377\5(\301\377"
      "F`\354\377\371\371\377\0\0\0\377\202\0\0\0\0\1\0\0\0\377\203\377\377"
      "\377\377\12\377\356\360\377\377+H\355\377\1%\252\377\0$\2\377\0$\21\377"
      "\6)\312\377Jd\357\377\375\375\377\377\377\377\377\0\0\0\377\202\0\0\0"
      "\0\1\0\0\0\377\202\377\377\377\377\10\377\356\360\377\377\35=\360\377"
      "\1%\216\377\0$\1\377\0$\21\377\3'\327\377h}\357\377\376\376\377\202\377"
      "\377\377\377\1\0\0\0\377\202\0\0\0\0\11\0\0\0\377\377\377\377\377\377"
      "\340\344\377\377\15/\365\377\2%z\0\0\0\0\377\0$\37\377\3&\353\377\200"
      "\222\364\204\377\377\377\377\1\0\0\0\377\202\0\0\0\0\7\0\0\0\377&\0\5"
      "\377\0\0\0\377\326\0\36p\0\0\0\0\377\0$&\370\0#\351\207\0\0\0\377\221"
      "\0\0\0\0"
    };
  return broken16_pixdata;
}

} // Rapicorn
