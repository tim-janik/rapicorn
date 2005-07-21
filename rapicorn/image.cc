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
#include "image.hh"
#include "itemimpl.hh"
#include "painter.hh"
#include "factory.hh"

namespace Rapicorn {

static const uint8* get_broken16_pixdata (void);

class ImageImpl : public virtual ItemImpl, public virtual Image {
  const PixelImage *m_pimage;
  int               m_xoffset, m_yoffset;
public:
  explicit ImageImpl() :
    m_pimage (NULL),
    m_xoffset (0),
    m_yoffset (0)
  {}
  void
  reset ()
  {
    m_xoffset = m_yoffset = 0;
    if (m_pimage)
      {
        const PixelImage *mpi = m_pimage;
        m_pimage = NULL;
        mpi->unref();
      }
    invalidate();
  }
  ~ImageImpl()
  {}
  virtual void          image_file        (const String &filename)      { load_image_file (filename); }
  virtual void          builtin_pixstream (const String &builtin_name)  { load_image_file (builtin_name); } // FIXME
  virtual ErrorType
  load_image_file (const String &filename)
  {
    reset();
    ErrorType error = UNKNOWN_FORMAT;
    if (error)
      load_pixstream (get_broken16_pixdata());
    return error;
  }
  virtual ErrorType
  load_pixstream (const uint8 *gdkp_pixstream)
  {
    reset();
    ErrorType error = NONE;
    const PixelImage *pimage = pixel_image_from_pixstream (gdkp_pixstream, &error);
    if (pimage)
      {
        pimage->ref_sink();
        error = load_pixel_image (pimage);
        pimage->unref();
      }
    else
      load_pixstream (get_broken16_pixdata());
    return error;
  }
  virtual ErrorType
  load_pixel_image (const PixelImage *pimage)
  {
    reset();
    if (!pimage)
      {
        /* no setting a "broken" image here, to allow
         * load_pixel_image(NULL) for setting up an empty image
         */
        return EXCESS_DIMENSIONS;
      }
    const PixelImage *oi = m_pimage;
    m_pimage = pimage;
    m_pimage->ref_sink();
    if (oi)
      oi->unref();
    return NONE;
  }
protected:
  virtual void
  size_request (Requisition &requisition)
  {
    if (m_pimage)
      {
        requisition.width += m_pimage->width();
        requisition.height += m_pimage->height();
      }
  }
  virtual void
  size_allocate (Allocation area)
  {
    allocation (area);
    if (m_pimage)
      {
        m_xoffset = (area.width - m_pimage->width()) / 2;
        m_yoffset = (area.height - m_pimage->height()) / 2;
      }
    else
      m_xoffset = m_yoffset = 0;
  }
public:
  void
  render (Display &display)
  {
    int x = allocation().x, y = allocation().y, width = allocation().width, height = allocation().height;
    Plane &plane = display.create_plane();
    Painter painter (plane);
    painter.draw_shadow (x, y, width, height, dark_glint(), dark_shadow(), dark_glint(), dark_shadow());
  }
  virtual const PropertyList&
  list_properties()
  {
    static Property *properties[] = {
      MakeProperty (Image, image_file, _("Image Filename"), _("Load an image from a file, only PNG images can be loaded."), "", "rw"),
      MakeProperty (Image, builtin_pixstream, _("Builtin Pixstream"), _("Load an image from a builtin pixel stream."), "", "rw"),
    };
    static const PropertyList property_list (properties, Item::list_properties());
    return property_list;
  }
};
static const ItemFactory<ImageImpl> image_factory ("Rapicorn::Image");

class PixelImageImpl : virtual public PixelImage {
  uint m_width, m_height;
  uint32 *m_pixels;
public:
  PixelImageImpl() :
    m_width (0),
    m_height (0),
    m_pixels (NULL)
  {}
  ~PixelImageImpl()
  {
    if (m_pixels)
      delete[] m_pixels;
  }
  void
  resize (uint iwidth,
          uint iheight)
  {
    if (m_pixels)
      delete[] m_pixels;
    m_width = iwidth;
    m_height = iheight;
    m_pixels = new uint32[iwidth * iheight];
  }
  uint32*
  pixels ()
  {
    return m_pixels;
  }
  virtual uint          width  () const       { return m_width; }
  virtual uint          height () const       { return m_height; }
  virtual const uint32* row    (uint y) const { return m_pixels + y * width() * 4; }
};

static Image::ErrorType
fill_pixel_image_from_pixstream (bool                has_alpha,
                                 bool                rle_encoded,
                                 uint                pixdata_width,
                                 uint                pixdata_height,
                                 const uint8        *encoded_pixdata,
                                 PixelImageImpl     &pimage)
{
  if (pixdata_width < 1 || pixdata_width > 16777216 ||
      pixdata_height < 1 || pixdata_height > 16777216)
    return Image::EXCESS_DIMENSIONS;
  uint bpp = has_alpha ? 4 : 3;
  if (!encoded_pixdata)
    return Image::DATA_CORRUPT;

  pimage.resize (pixdata_width, pixdata_height);

  if (rle_encoded)
    {
      const uint8 *rle_buffer = encoded_pixdata;
      uint32 *image_buffer = pimage.pixels();
      uint32 *image_limit = image_buffer + pixdata_width * pixdata_height;

      while (image_buffer < image_limit)
        {
          uint length = *(rle_buffer++);
          if ((length & ~128) == 0)
            return Image::DATA_CORRUPT;
          bool check_overrun = false;
          if (length & 128)
            {
              length = length - 128;
              check_overrun = image_buffer + length > image_limit;
              if (check_overrun)
                length = image_limit - image_buffer;
              if (bpp < 4)
                while (length--)
                  *image_buffer++ = (0xff << 24) | (rle_buffer[0] << 16) | (rle_buffer[1] << 8) | rle_buffer[2];
              else
                while (length--)
                  *image_buffer++ = (rle_buffer[3] << 24) | (rle_buffer[0] << 16) | (rle_buffer[1] << 8) | rle_buffer[2];
              rle_buffer += bpp;
            }
          else
            {
              check_overrun = image_buffer + length > image_limit;
              if (check_overrun)
                length = image_limit - image_buffer;
              if (bpp < 4)
                while (length--)
                  {
                    *image_buffer++ = (0xff << 24) | (rle_buffer[0] << 16) | (rle_buffer[1] << 8) | rle_buffer[2];
                    rle_buffer += 3;
                  }
              else
                while (length--)
                  {
                    *image_buffer++ = (rle_buffer[3] << 24) | (rle_buffer[0] << 16) | (rle_buffer[1] << 8) | rle_buffer[2];
                    rle_buffer += 4;
                  }
            }
          if (check_overrun)
            return Image::DATA_CORRUPT;
        }
    }
  else
    for (uint y = 0; y < pixdata_height; y++)
      {
        uint32 *row = pimage.pixels() + y * pimage.width() * 4;
        uint length = pixdata_width;
        if (bpp < 4)
          while (length--)
            {
              *row++ = (0xff << 24) | (encoded_pixdata[0] << 16) | (encoded_pixdata[1] << 8) | encoded_pixdata[2];
              encoded_pixdata += 3;
            }
        else
          while (length--)
            {
              *row++ = (encoded_pixdata[3] << 24) | (encoded_pixdata[0] << 16) | (encoded_pixdata[1] << 8) | encoded_pixdata[2];
              encoded_pixdata += 4;
            }
      }

  return Image::NONE;
}

static inline const uint8*
get_uint32 (const uint8 *stream, uint *result)
{
  *result = (stream[0] << 24) + (stream[1] << 16) + (stream[2] << 8) + stream[3];
  return stream + 4;
}

const PixelImage*
Image::pixel_image_from_pixstream (const uint8 *gdkp_pixstream,
                                   ErrorType   *error_type)
{
  ErrorType dummy = DATA_CORRUPT;
  error_type = error_type ? error_type : &dummy;
  return_val_if_fail (gdkp_pixstream != NULL, NULL);

  *error_type = UNKNOWN_FORMAT;
  const uint8 *s = gdkp_pixstream;
  if (strncmp ((char*) s, "GdkP", 4) != 0)
    return NULL;
  s += 4;

  *error_type = UNKNOWN_FORMAT;
  uint len;
  s = get_uint32 (s, &len);
  if (len < 24)
    return NULL;

  *error_type = UNKNOWN_FORMAT;
  uint type;
  s = get_uint32 (s, &type);
  if (type != 0x02010001 &&     /* RLE/8bit/RGB */
      type != 0x01010001 &&     /* RAW/8bit/RGB */
      type != 0x02010002 &&     /* RLE/8bit/RGBA */
      type != 0x01010002)       /* RAW/8bit/RGBA */
    return NULL;

  *error_type = EXCESS_DIMENSIONS;
  uint rowstride, width, height;
  s = get_uint32 (s, &rowstride);
  s = get_uint32 (s, &width);
  s = get_uint32 (s, &height);
  if (width < 1 || height < 1)
    return NULL;

  PixelImageImpl *pimage = new PixelImageImpl();
  *error_type = fill_pixel_image_from_pixstream ((type & 0xff) == 0x02,
                                                 (type >> 24) == 0x02,
                                                 width, height, s,
                                                 *pimage);
  if (!*error_type)
    return pimage;
  pimage->ref_sink();
  pimage->unref();
  return NULL;
}

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
      "\221\0\0\0\0\1\0\0\0@\214\0\0\0\200\1\0\0\0@\202\0\0\0\0\1\0\0\0\200"
      "\214\0\0\0\377\1\0\0\0\200\202\0\0\0\0\2\0\0\0\200\0\0\0\377\204\235"
      "\264\377\377\2\244\272\377\306\245\272\377\277\204\235\264\377\377\2"
      "\0\0\0\377\0\0\0~\202\0\0\0\0\2\0\0\0\200\0\0\0\377\202\235\264\377\377"
      "\2\237\265\377\360\256\301\377p\202\0\0\0\0\1\235\264\377\375\203\235"
      "\264\377\377\2\0\0\0\273\0\0\0\17\202\0\0\0\0\5\0\0\0\200\0\0\0\377\235"
      "\264\377\377\246\273\377\270\264\305\377\36\203\0\0\0\0\1\256\301\377"
      "j\202\235\264\377\377\2\250\274\377\243\0\0\0\7\203\0\0\0\0\3\0\0\0\200"
      "\0\0\0\351\260\303\377^\205\0\0\0\0\3\277\316\377\6\243\271\377\320\253"
      "\276\377\217\205\0\0\0\0\2\0\0\0<\0\0\0\26\207\0\0\0\0\1\263\305\377"
      "\24\213\0\0\0\0\2\200\377w!m\377b^\206\0\0\0\0\1\0\0\0\14\205\0\0\0\0"
      "\5\210\377\177\2d\377Xv\34\377\13\364\27\377\6\371}\377s.\204\0\0\0\0"
      "\2\0\0\0d\0\0\0{\204\0\0\0\0\2}\377s.8\377)\315\203\22\377\0\377\2""2"
      "\377\"\326\210\377\177\5\202\0\0\0\0\3d\377Yu\0\0\0\377\0\0\0\200\202"
      "\0\0\0\0\3\0\0\0\12\0\0\0\205\25\377\4\373\205\22\377\0\377\6\\\377P"
      "\211\0\0\0\0^\377R\205\22\377\0\377\0\0\0\377\0\0\0\200\202\0\0\0\0\2"
      "\0\0\0\177\0\0\0\377\207\22\377\0\377\1T\377G\234\202\22\377\0\377\2"
      "\0\0\0\377\0\0\0\200\202\0\0\0\0\1\0\0\0\200\214\0\0\0\377\1\0\0\0\200"
      "\202\0\0\0\0\1\0\0\0@\214\0\0\0\200\1\0\0\0@\221\0\0\0\0" };
  return broken16_pixdata;
}

} // Rapicorn
