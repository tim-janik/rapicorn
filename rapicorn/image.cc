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
#include "itemimpl.hh"
#include "painter.hh"
#include "factory.hh"
#include <ext/hash_map>

namespace { // Anon
using namespace Rapicorn;

static Rapicorn::Image::ErrorType       rapicorn_load_png_image (const char            *file_name,
                                                                 Rapicorn::PixelImage **imagep,
                                                                 std::string           &comment);
} // Anon

namespace Rapicorn {

struct HashName {
  size_t
  operator() (const String &name) const
  {
    /* 31 quantization steps hash function */
    const size_t len = name.size();
    size_t h = 0;
    for (uint i = 0; i < len; i++)
      h = (h << 5) - h + name[i];
    return h;
  }
};

struct EqualNames {
  bool
  operator() (const String &name1,
              const String &name2) const
  {
    return name1 == name2;
  }
};

static __gnu_cxx::hash_map <const String, const uint8*, HashName, EqualNames> builtin_pixstreams;

const uint8*
Image::lookup_builtin_pixstream (const String &builtin_name)
{
  return builtin_pixstreams[builtin_name];
}

void
Image::register_builtin_pixstream (const String       &builtin_name,
                                   const uint8 * const static_const_pixstream)
{
  builtin_pixstreams[builtin_name] = static_const_pixstream;
}

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
  virtual void
  builtin_pixstream (const String &builtin_name)
  {
    const uint8 *pixstream = lookup_builtin_pixstream (builtin_name.c_str());
    if (!pixstream)
      pixstream = get_broken16_pixdata();
    load_pixstream (pixstream);
  }
  virtual ErrorType
  load_image_file (const String &filename)
  {
    reset();
    PixelImage *pimage = NULL;
    String comment;
    ErrorType error = rapicorn_load_png_image (filename.c_str(), &pimage, comment);
    // diag ("Image: load \"%s\": %d comment=%s", filename.c_str(), error, comment.c_str());
    if (pimage)
      {
        pimage->ref_sink();
        if (error)      /* return first error */
          load_pixel_image (pimage);
        else
          error = load_pixel_image (pimage);
        pimage->unref();
      }
    else
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
        /* not setting a "broken" image here, to allow
         * load_pixel_image (NULL) for setting up an empty image
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
        m_xoffset = iround (0.5 * (area.width - m_pimage->width()));
        m_yoffset = iround (0.5 * (area.height - m_pimage->height()));
      }
    else
      m_xoffset = m_yoffset = 0;
  }
public:
  virtual void
  render (Display &display)
  {
    if (m_pimage)
      {
        int ix = iround (allocation().x + m_xoffset), iy = iround (allocation().y + m_yoffset);
        int ih = m_pimage->height();
        display.push_clip_rect (ix, iy, m_pimage->width(), ih);
        Plane &plane = display.create_plane();
        for (int y = plane.ystart(); y < plane.ybound(); y++)
          {
            uint nth = y - iy;
            const uint32 *row = m_pimage->row (ih - (1 + nth));
            row += plane.xstart() - ix;
            uint32 *span = plane.poke_span (plane.xstart(), y, plane.width());
            uint32 *limit = span + plane.width();
            while (span < limit)
              *span++ = Color (*row++).premultiplied();
          }
        display.pop_clip_rect();
      }
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
static const ItemFactory<ImageImpl> image_factory ("Rapicorn::Factory::Image");

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
    m_pixels = NULL;
  }
  void
  resize (uint iwidth,
          uint iheight)
  {
    if (m_pixels)
      delete[] m_pixels;
    m_width = iwidth;
    m_height = iheight;
    m_pixels = new uint32[iwidth * iheight]; /* may fail and set m_pixels = NULL; */
  }
  void
  fill (uint32 pixel)
  {
    if (m_pixels)
      {
        if (pixel == 0)
          memset (m_pixels, 0, m_height * m_width * sizeof (m_pixels[0]));
        else
          for (uint i = 0; i < m_width * m_height; i++)
            m_pixels[i] = pixel;
      }
  }
  void
  border (uint32 pixel)
  {
    if (m_pixels)
      {
        for (uint i = 0; i < m_width; i++)
          m_pixels[i] = m_pixels[i + (m_height - 1) * m_width] = pixel;
        for (uint i = 0; i < m_height; i++)
          m_pixels[i * m_width] = m_pixels[i * m_width + m_width - 1] = pixel;
      }
  }
  uint32*
  pixels ()
  {
    return m_pixels;
  }
  virtual int           width  () const       { return m_width; }
  virtual int           height () const       { return m_height; }
  virtual const uint32* row    (uint y) const { return m_pixels + y * width(); }
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
        uint32 *row = pimage.pixels() + y * pimage.width();
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


/* --- libpng wrapper code --- */
#include <png.h>
namespace {

static Rapicorn::Image::ErrorType /* longjmp() may jump across this function */
png_loader_configure_4argb (png_structp  png_ptr,
                            png_infop    info_ptr,
                            uint        *widthp,
                            uint        *heightp)
{
  int bit_depth = png_get_bit_depth (png_ptr, info_ptr);
  /* check bit-depth to be only 1, 2, 4, 8, 16 to prevent libpng errors */
  if (bit_depth < 1 || bit_depth > 16 || (bit_depth & (bit_depth - 1)))
    return Rapicorn::Image::UNKNOWN_FORMAT;
  int color_type = png_get_color_type (png_ptr, info_ptr);
  int interlace_type = png_get_interlace_type (png_ptr, info_ptr);
  /* beware, ordering of the following transformation requests matters */
  if (color_type == PNG_COLOR_TYPE_PALETTE)
    png_set_palette_to_rgb (png_ptr);                           /* request RGB format */
  if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
    png_set_gray_1_2_4_to_8 (png_ptr);                          /* request 8bit per sample */
  if (png_get_valid (png_ptr, info_ptr, PNG_INFO_tRNS))
    png_set_tRNS_to_alpha (png_ptr);                            /* request transparency as alpha channel */
  if (bit_depth == 16)
    png_set_strip_16 (png_ptr);                                 /* request 8bit per sample */
  if (bit_depth < 8)
    png_set_packing (png_ptr);                                  /* request 8bit per sample */
  if (color_type == PNG_COLOR_TYPE_GRAY ||
      color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
    png_set_gray_to_rgb (png_ptr);                              /* request RGB format */
#if     __BYTE_ORDER == __LITTLE_ENDIAN
  if (color_type == PNG_COLOR_TYPE_RGB ||
      color_type == PNG_COLOR_TYPE_RGB_ALPHA)
    png_set_bgr (png_ptr);                                      /* request ARGB as little-endian: BGRA */
  if (color_type == PNG_COLOR_TYPE_RGB ||
      color_type == PNG_COLOR_TYPE_GRAY)
    png_set_add_alpha (png_ptr, 0xff, PNG_FILLER_AFTER);        /* request ARGB as little-endian: BGRA */
#elif   __BYTE_ORDER == __BIG_ENDIAN
  if (color_type == PNG_COLOR_TYPE_RGB_ALPHA)
    png_set_swap_alpha (png_ptr);                               /* request big-endian ARGB */
  if (color_type == PNG_COLOR_TYPE_RGB ||
      color_type == PNG_COLOR_TYPE_GRAY)
    png_set_add_alpha (png_ptr, 0xff, PNG_FILLER_BEFORE);       /* request big-endian ARGB */
#else
#error  Unknown endianess type
#endif
  if (interlace_type != PNG_INTERLACE_NONE)
    png_set_interlace_handling (png_ptr);                       /* request non-interlaced image */
  /* reflect the transformations */
  png_read_update_info (png_ptr, info_ptr);
  png_uint_32 pwidth = 0, pheight = 0;
  int compression_type = 0, filter_type = 0;
  png_get_IHDR (png_ptr, info_ptr, &pwidth, &pheight,
                &bit_depth, &color_type, &interlace_type,
                &compression_type, &filter_type);
  *widthp = pwidth;
  *heightp = pheight;
  /* sanity checks */
  if (pwidth < 1 || pheight < 1 || pwidth > 100000 | pheight > 100000)
    return Rapicorn::Image::EXCESS_DIMENSIONS;
  if (bit_depth != 8 ||
      color_type != PNG_COLOR_TYPE_RGB_ALPHA ||
      png_get_channels (png_ptr, info_ptr) != 4)
    return Rapicorn::Image::UNKNOWN_FORMAT;
  return Rapicorn::Image::NONE;
}

struct PngImage {
  Rapicorn::Image::ErrorType error;
  FILE                      *fp;
  Rapicorn::PixelImageImpl  *image;
  uint8                    **rows;
  std::string                comment;
  explicit PngImage() :
    error (Rapicorn::Image::NONE),
    fp (NULL), image (NULL), rows (NULL)
  {}
};

static void /* longjmp() may jump across this function */
png_loader_read_image (PngImage   &pimage,
                       png_structp png_ptr,
                       png_infop   info_ptr)
{
  /* setup PNG io */
  png_init_io (png_ptr, pimage.fp);
  png_set_sig_bytes (png_ptr, 8);                       /* advance by the 8 signature bytes we read earlier */
  png_set_user_limits (png_ptr, 0x7ffffff, 0x7ffffff);  /* we check the size ourselves */
  /* setup from meta data */
  png_read_info (png_ptr, info_ptr);
  uint width, height;
  Rapicorn::Image::ErrorType error = png_loader_configure_4argb (png_ptr, info_ptr, &width, &height);
  if (error)
    {
      pimage.error = error;
      return;
    }
  /* setup rows */
  pimage.image = new Rapicorn::PixelImageImpl();
  if (pimage.image)
    {
      pimage.image->resize (width, height);
      uint8 *pixels = (uint8*) pimage.image->pixels();
      if (pixels)
        {
          uint rowstride = width * 4;
          pimage.image->fill (0);               /* transparent background for partial images */
          pimage.image->border (0x80ff0000);    /* show red error border for partial images */
          pimage.rows = new png_bytep[height];
          if (pimage.rows)
            for (uint i = 0; i < height; i++)
              pimage.rows[i] = pixels + i * rowstride;
        }
      else      /* not enough pixel memory */
        {
          delete pimage.image;
          pimage.image = NULL;
        }
    }
  if (!pimage.rows)
    {
      pimage.error = Rapicorn::Image::EXCESS_DIMENSIONS;
      return;
    }
  /* read in image */
  png_read_image (png_ptr, pimage.rows);
  png_read_end (png_ptr, info_ptr);
  /* read out comments */
  png_textp text_ptr = NULL;
  int num_texts = 0;
  if (png_get_text (png_ptr, info_ptr, &text_ptr, &num_texts))
    for (int i = 0; i < num_texts; i++)
      {
        bool is_comment = strcasecmp (text_ptr[i].key, "Comment") == 0;
        if (is_comment || strcasecmp (text_ptr[i].key, "Description") == 0)
          {
            std::string output, input (text_ptr[i].text);
            if (Rapicorn::text_convert ("UTF-8", output, "ISO-8859-1", input) &&
                output[0] != 0)
              {
                pimage.comment = output;
                if (is_comment)
                  break;
              }
          }
      }
  pimage.error = Rapicorn::Image::NONE;
}

static void
png_loader_error (png_structp     png_ptr,
                  png_const_charp error_msg)
{
  PngImage *pimage = (PngImage*) png_get_error_ptr (png_ptr);
  pimage->error = Rapicorn::Image::DATA_CORRUPT;
  longjmp (png_ptr->jmpbuf, 1);
}

static void
png_loader_nop (png_structp     png_ptr,
                png_const_charp error_msg)
{}

static Rapicorn::Image::ErrorType
rapicorn_load_png_image (const char            *file_name,
                         Rapicorn::PixelImage **imagep,
                         std::string           &comment)
{
  *imagep = NULL;
  comment = "";
  /* open image */
  FILE *fp = fopen (file_name, "rb");
  if (!fp)
    return Rapicorn::Image::READ_FAILED;
  /* read 8 PNG signature bytes */
  uint8 sigbuffer[8];
  if (fread (sigbuffer, 1, 8, fp) != 8)
    {
      fclose (fp);
      return Rapicorn::Image::READ_FAILED;
    }
  /* check signature */
  if (png_sig_cmp (sigbuffer, 0, 8) != 0)
    {
      fclose (fp);
      return Rapicorn::Image::UNKNOWN_FORMAT;
    }
  /* allocate resources */
  PngImage pimage;
  pimage.error = Rapicorn::Image::EXCESS_DIMENSIONS;
  png_structp png_ptr = png_create_read_struct (PNG_LIBPNG_VER_STRING, &pimage, png_loader_error, png_loader_nop);
  png_infop info_ptr = !png_ptr ? NULL : png_create_info_struct (png_ptr);
  if (!info_ptr)
    {
      if (png_ptr)
        png_destroy_read_struct (&png_ptr, NULL, NULL);
      fclose (fp);
      return pimage.error;
    }
  /* save stack for longjmp() in png_loader_error() */
  if (setjmp (png_ptr->jmpbuf) == 0)
    {
      /* read pixel image */
      pimage.fp = fp;
      pimage.error = Rapicorn::Image::DATA_CORRUPT;
      png_loader_read_image (pimage, png_ptr, info_ptr);
    }
  /* cleanup */
  png_destroy_read_struct (&png_ptr, &info_ptr, NULL);
  if (pimage.rows)
    delete[] pimage.rows;
  fclose (fp);
  /* apply return values */
  *imagep = pimage.image;       /* may be != NULL even on errors */
  comment = pimage.comment;
  /* pimage.error indicates success */
  return pimage.error;
}

} // Anon
