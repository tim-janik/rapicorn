/* Rapicorn - experimental UI toolkit
 * Copyright (C) 2005-2008 Tim Janik
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
#include "pixmap.hh"
#include <errno.h>
#include <math.h>

#define MAXDIM                          (20480) // MAXDIM*MAXDIM < 536870912
#define ALIGN_SIZE(size,pow2align)      ((size + (pow2align - 1)) & -pow2align)
#define ALIGN_STRIDE(size)              ALIGN_SIZE (size, 16)

namespace {
using namespace Rapicorn;

static Pixmap*  anon_load_png (const String &filename); /* assigns errno */
static bool     anon_save_png (const String  filename,
                               const Pixbuf &pixbuf,
                               const String &comment);

} // Anon

namespace Rapicorn {

bool
Pixbuf::try_alloc (uint width,
                   uint height)
{
  if (width <= MAXDIM && height <= MAXDIM)
    {
      uint64 rowstride = ALIGN_STRIDE (width);
      uint64 total = rowstride * height;
      uint32 *pixels = new uint32[total];
      if (pixels)
        {
          pixels[0] = 0xff1155bb;
          delete[] pixels;
          return true;
        }
    }
  return false;
}

Pixbuf::Pixbuf (uint _width,
                uint _height) :
  m_pixels (NULL), m_rowstride (ALIGN_STRIDE (_width)),
  m_width (_width), m_height (_height)
{
  assert (_width <= MAXDIM);
  assert (_height <= MAXDIM);
  m_pixels = new uint32[m_rowstride * m_height];
  if (!m_pixels)
    error ("%s(): failed to allocate %u bytes for %ux%u pixels",
           STRFUNC, sizeof (uint32[m_rowstride * m_height]),
           _width, _height);
  memset (m_pixels, 0, sizeof (uint32[m_rowstride * m_height]));
}

const uint32*
Pixbuf::row (uint y) const /* endian dependant ARGB integers */
{
  if (y >= (uint) m_height)
    return NULL;
  return &m_pixels[m_rowstride * y];
}

Pixbuf::~Pixbuf ()
{
  m_rowstride = 0;
  delete[] m_pixels;
  m_pixels = NULL;
}

#define SQR(x)  ((x) * (x))

bool
Pixbuf::compare (const Pixbuf &other,
                 uint x, uint y, int width, int height,
                 uint tx, uint ty,
                 double *averrp, double *maxerrp,
                 double *nerrp, double *npixp) const
{
  if (averrp) *averrp = 0;
  if (maxerrp) *maxerrp = 0;
  if (nerrp) *nerrp = 0;
  if (npixp) *npixp = 0;
  if (x >= (uint) m_width || tx >= (uint) other.m_width ||
      y >= (uint) m_height || ty >= (uint) other.m_height)
    return false;
  if (width < 0)
    width = m_width;
  if (height < 0)
    height = m_height;
  width = MIN (MIN (m_width - (int) x, other.m_width - (int) tx), width);
  height = MIN (MIN (m_height - (int) y, other.m_height - (int) ty), height);
  const uint npix = height * width;
  uint nerr = 0;
  int maxerr2 = 0;
  for (int k = 0; k < height; k++)
    {
      const uint32 *r1 = row (y + k);
      const uint32 *r2 = other.row (ty + k);
      for (int j = 0; j < width; j++)
        if (r1[x + j] != r2[tx + j])
          {
            int8 *p1 = (int8*) &r1[x + j], *p2 = (int8*) &r2[tx + j];
            maxerr2 = MAX (maxerr2,
                           SQR (p1[0] - p2[0]) + SQR (p1[1] - p2[1]) +
                           SQR (p1[2] - p2[2]) + SQR (p1[3] - p2[3]));
            nerr++;
          }
    }
  double maxerr = sqrt (maxerr2);
  if (averrp)
    *averrp = maxerr / npix;
  if (maxerrp)
    *maxerrp = maxerr;
  if (nerrp)
    *nerrp = nerr;
  if (npixp)
    *npixp = npix;
  return nerr != 0;
}

Pixmap::Pixmap (uint _width,
                uint _height) :
  Pixbuf (_width, _height)
{}

void
Pixmap::comment (const String &_comment)
{
  m_comment = _comment;
}

Pixmap*
Pixmap::load_png (const String &filename, /* assigns errno */
                  bool          tryrepair)
{
  Pixmap *pixmap = anon_load_png (filename);
  if (pixmap && errno && !tryrepair)
    {
      int saved = errno;
      unref (ref_sink (pixmap));
      pixmap = NULL;
      errno = saved;
    }
  return pixmap;
}

bool
Pixmap::save_png (const String &filename) /* assigns errno */
{
  return anon_save_png (filename, *this, m_comment);
}

Pixmap::~Pixmap ()
{
  m_comment.erase();
}

static void
pixmap_fill (Pixmap *pixmap,
             uint32  pixel)
{
  for (int y = 0; y < pixmap->height(); y++)
    {
      uint32 *row = pixmap->row (y);
      for (int x = 0; x < pixmap->width(); x++)
        row[x] = pixel;
    }
}

static void
pixmap_border (Pixmap *pixmap,
               uint32  pixel)
{
  uint32 *row = pixmap->row (0);
  for (int i = 0; i < pixmap->width(); i++)
    row[i] = pixel;
  row = pixmap->row (pixmap->height() - 1);
  for (int i = 0; i < pixmap->width(); i++)
    row[i] = pixel;
  for (int i = 1; i < pixmap->height() - 1; i++)
    {
      row = pixmap->row (i);
      row[0] = pixel;
      row[pixmap->width() - 1] = pixel;
    }
}

} // Rapicorn

#include <png.h>

namespace {

static bool
text_convert (const char        *to_charset,    // FIXME
              std::string       &output_string,
              const char        *from_charset,
              const std::string &input_string,
              const char        *fallback_charset = "ISO-8859-1",
              const std::string &output_mark = "")
{
  output_string = input_string;
  return true;
}

struct PngContext {
  Pixmap    *pixmap;
  int        error;
  FILE      *fp;
  png_byte **rows;
  PngContext() : pixmap (NULL), error (0), fp (NULL), rows (NULL) {}
};

static int /* returns errno; longjmp() may jump out of this function */
pngcontext_configure_4argb (png_structp  png_ptr,
                            png_infop    info_ptr,
                            uint        *widthp,
                            uint        *heightp)
{
  int bit_depth = png_get_bit_depth (png_ptr, info_ptr);
  /* check bit-depth to be only 1, 2, 4, 8, 16 to prevent libpng errors */
  if (bit_depth < 1 || bit_depth > 16 || (bit_depth & (bit_depth - 1)))
    return EINVAL;
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
  if (pwidth < 1 || pheight < 1 || pwidth > MAXDIM | pheight > MAXDIM)
    return ENOMEM;
  if (bit_depth != 8 ||
      color_type != PNG_COLOR_TYPE_RGB_ALPHA ||
      png_get_channels (png_ptr, info_ptr) != 4)
    return ENOSYS;
  return 0;
}

static void /* longjmp() may jump out of this function */
pngcontext_read_image (PngContext &pcontext,
                       png_structp png_ptr,
                       png_infop   info_ptr)
{
  /* setup PNG io */
  png_init_io (png_ptr, pcontext.fp);
  png_set_sig_bytes (png_ptr, 8);                       /* advance by the 8 signature bytes we read earlier */
  png_set_user_limits (png_ptr, 0x7ffffff, 0x7ffffff);  /* we check the size ourselves */
  /* setup from meta data */
  png_read_info (png_ptr, info_ptr);
  uint width, height;
  int conferr = pngcontext_configure_4argb (png_ptr, info_ptr, &width, &height);
  if (conferr)
    {
      pcontext.error = conferr;
      return;
    }
  /* setup pixmap and rows */
  pcontext.rows = new png_bytep[height];
  pcontext.pixmap = Pixbuf::try_alloc (width, height) ? new Pixmap (width, height) : NULL;
  if (!pcontext.rows || !pcontext.pixmap)
    {
      pcontext.error = ENOMEM;
      return;
    }
  pixmap_fill (pcontext.pixmap, 0);             /* transparent background for partial images */
  pixmap_border (pcontext.pixmap, 0x80ff0000);  /* show red error border for partial images */
  for (uint i = 0; i < height; i++)
    pcontext.rows[i] = (png_byte*) pcontext.pixmap->row (i);
  /* read image rows */
  png_read_image (png_ptr, pcontext.rows);
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
            if (text_convert ("UTF-8", output, "ISO-8859-1", input) && output.size())
              {
                pcontext.pixmap->comment (output);
                if (is_comment)
                  break;
              }
          }
      }
  pcontext.error = 0;
}

static void
pngcontext_error (png_structp     png_ptr,
                  png_const_charp error_msg)
{
  PngContext *pcontext = (PngContext*) png_get_error_ptr (png_ptr);
  if (!pcontext->error)
    pcontext->error = EIO;
  longjmp (png_ptr->jmpbuf, 1);
}

static void
pngcontext_nop (png_structp     png_ptr,
                png_const_charp error_msg)
{}

static Pixmap*
anon_load_png (const String &filename) /* assigns errno */
{
  PngContext pcontext;
  /* open image */
  pcontext.fp = fopen (filename.c_str(), "rb");
  if (!pcontext.fp)
    return NULL; // errno set by fopen()
  /* read and check PNG signature */
  uint8 sigbuffer[8];
  if (fread (sigbuffer, 1, 8, pcontext.fp) != 8 || png_sig_cmp (sigbuffer, 0, 8) != 0)
    {
      fclose (pcontext.fp);
      errno = ENODATA; // not a PNG
      return NULL;
    }
  /* allocate resources */
  pcontext.error = ENOMEM;
  png_structp png_ptr = png_create_read_struct (PNG_LIBPNG_VER_STRING, &pcontext, pngcontext_error, pngcontext_nop);
  png_infop info_ptr = !png_ptr ? NULL : png_create_info_struct (png_ptr);
  if (!info_ptr)
    {
      if (png_ptr)
        png_destroy_read_struct (&png_ptr, NULL, NULL);
      fclose (pcontext.fp);
      errno = pcontext.error;
      return NULL;
    }
  /* save stack for longjmp() in png_loader_error() */
  if (setjmp (png_ptr->jmpbuf) == 0)
    {
      /* read pixel image */
      pcontext.fp = pcontext.fp;
      pcontext.error = EINVAL;
      pngcontext_read_image (pcontext, png_ptr, info_ptr);
    }
  /* cleanup */
  png_destroy_read_struct (&png_ptr, &info_ptr, NULL);
  fclose (pcontext.fp);
  if (pcontext.rows)
    delete[] pcontext.rows;
  errno = pcontext.error; // maybe set even if pcontext.pixmap!=NULL
  return pcontext.pixmap;
}

static bool
anon_save_png (const String  filename,
               const Pixbuf &pixbuf,
               const String &comment)
{
  const int w = pixbuf.width();
  const int h = pixbuf.height();
  const int depth = 8;
  /* allocate resources */
  PngContext pcontext;
  pcontext.error = ENOMEM;
  png_structp png_ptr = png_create_write_struct (PNG_LIBPNG_VER_STRING, &pcontext,
                                                 pngcontext_error, pngcontext_nop);
  png_infop info_ptr = !png_ptr ? NULL : png_create_info_struct (png_ptr);
  if (!info_ptr)
    {
      if (png_ptr)
        png_destroy_write_struct (&png_ptr, &info_ptr);
      errno = pcontext.error;
      return false;
    }
  /* setup key=value pairs for PNG text section */
  StringVector sv;
  if (comment.size())
    sv.insert (sv.begin(), "Comment=" + comment);
  StringVector keys, values;
  for (uint i = 0; i < sv.size(); i++)
    {
      uint as = sv[i].find ('=');
      if (as > 0 && as <= 79 && sv[i].size() > as + 1)
        {
          String utf8string = sv[i].substr (as + 1);
          String iso_string;
          if (text_convert ("ISO-8859-1", iso_string, "UTF-8", utf8string) && iso_string.size())
            {
              keys.push_back (sv[i].substr (0, as));
              values.push_back (iso_string);
            }
        }
    }
  /* save stack for longjmp() in png_saving_error() */
  pcontext.error = EIO;
  png_textp text_ptr = NULL;
  pcontext.fp = fopen (filename.c_str(), "wb");
  if (pcontext.fp && setjmp (png_ptr->jmpbuf) == 0)
    {
      /* write pixel image */
      pcontext.error = EIO;
      text_ptr = new png_text[sv.size()];
      memset (text_ptr, 0, sizeof (*text_ptr) * sv.size());
      uint num_text;
      for (num_text = 0; num_text < keys.size(); num_text++)
        {
          text_ptr[num_text].compression = PNG_TEXT_COMPRESSION_zTXt;
          text_ptr[num_text].key = const_cast<char*> (keys[num_text].c_str());
          text_ptr[num_text].text = const_cast<char*> (values[num_text].c_str());
          text_ptr[num_text].text_length = values[num_text].size();
        }
      png_set_text (png_ptr, info_ptr, text_ptr, num_text);
      png_set_compression_level (png_ptr, 9);
      png_set_IHDR (png_ptr, info_ptr, w, h, depth,
                    PNG_COLOR_TYPE_RGB_ALPHA, PNG_INTERLACE_NONE,
                    PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
      png_color_8 sig_bit;
      sig_bit.red = sig_bit.green = sig_bit.blue = sig_bit.alpha = depth;
      png_set_sBIT (png_ptr, info_ptr, &sig_bit);
      png_set_shift (png_ptr, &sig_bit);
      png_set_packing (png_ptr);
#if     __BYTE_ORDER == __LITTLE_ENDIAN
      png_set_bgr (png_ptr);                            /* request ARGB as little-endian: BGRA */
#elif   __BYTE_ORDER == __BIG_ENDIAN
      png_set_swap_alpha (png_ptr);                     /* request big-endian ARGB */
#else
#error  Unknown endianess type
#endif
      /* perform image IO */
      png_init_io (png_ptr, pcontext.fp);
      png_write_info (png_ptr, info_ptr);
      for (int y = 0; y < h; y++)
        {
          png_bytep row_ptr = (png_bytep) pixbuf.row (y);
          png_write_rows (png_ptr, &row_ptr, 1);
        }
      png_write_end (png_ptr, info_ptr);
      pcontext.error = 0;
    }
  png_destroy_write_struct (&png_ptr, &info_ptr);
  if (text_ptr)
    delete[] text_ptr;
  if (pcontext.fp)
    {
      if (fclose (pcontext.fp) != 0 && !pcontext.error)
        pcontext.error = EIO;
      if (pcontext.error)
        unlink (filename.c_str());
    }
  errno = pcontext.error;
  return pcontext.error == 0;
}


} // Anon
