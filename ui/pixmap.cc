// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#define __RAPICORN_IDL_ALIASES__ 0      // provide no ClnT or SrvT aliases
#include "serverapi.hh" // to instantiate template class PixmapT<Rapicorn::SrvT_Pixbuf>;
#include "clientapi.hh" // to instantiate template class PixmapT<Rapicorn::ClnT_Pixbuf>;
#include <errno.h>
#include <math.h>
#include <cstring>

#define MAXDIM                          (20480) // MAXDIM*MAXDIM < 536870912
#define ALIGN_SIZE(size,pow2align)      ((size + (pow2align - 1)) & -pow2align)

namespace {
using namespace Rapicorn;

template<class Pixbuf> static bool
anon_load_png (PixmapT<Pixbuf> pixmap, size_t nbytes, const char *bytes); /* assigns errno */

} // Anon

namespace Rapicorn {

template<class Pixbuf>
PixmapT<Pixbuf>::PixmapT() :
  pixbuf_ (new Pixbuf())
{}

template<class Pixbuf>
PixmapT<Pixbuf>::PixmapT (uint w, uint h) :
  pixbuf_ (new Pixbuf())
{
  assert (w <= MAXDIM);
  assert (h <= MAXDIM);
  pixbuf_->resize (w, h);
}

template<class Pixbuf>
PixmapT<Pixbuf>::PixmapT (const Pixbuf &source) :
  pixbuf_ (new Pixbuf())
{
  *pixbuf_ = source;
}

template<class Pixbuf>
PixmapT<Pixbuf>::PixmapT (Blob &blob) :
  pixbuf_ (new Pixbuf())
{
  errno = ENOENT;
  if (blob.size() && load_png (blob.size(), blob.data()))
    errno = 0;
  if (errno)
    RAPICORN_DIAG ("PixmapT: failed to load %s: %s", CQUOTE (blob.name()), strerror (errno));
}

template<class Pixbuf>
PixmapT<Pixbuf>::PixmapT (const String &res_png) :
  pixbuf_ (new Pixbuf())
{
  errno = ENOENT;
  Blob blob = Res (res_png);
  if (blob.size() && load_png (blob.size(), blob.data()))
    errno = 0;
  if (errno)
    RAPICORN_DIAG ("PixmapT: failed to load %s: %s", CQUOTE (res_png), strerror (errno));
}

template<class Pixbuf>
PixmapT<Pixbuf>&
PixmapT<Pixbuf>::operator= (const Pixbuf &source)
{
  *pixbuf_ = source;
  return *this;
}

template<class Pixbuf> void
PixmapT<Pixbuf>::resize (uint width, uint height)
{
  assert (width <= MAXDIM);
  assert (height <= MAXDIM);
  pixbuf_->resize (width, height);
}

template<class Pixbuf> bool
PixmapT<Pixbuf>::try_resize (uint width, uint height)
{
  if (width && height && width <= MAXDIM && height <= MAXDIM)
    {
      //uint64 rowstride = align_rowstride (width, alignment);
      //uint64 total = rowstride * height;
      const size_t total = width * size_t (height);
      uint32 *pixels = new uint32[total];
      if (pixels)
        {
          pixels[0] = 0xff1155bb;
          delete[] pixels;
          resize (width, height);
          return true;
        }
    }
  return false;
}

#define SQR(x)  ((x) * (x))

/**
 * @param source          Source pixmap to compare with
 * @param sx, sy          Origin of the source rectangle to compare
 * @param swidth, sheight Size of the source rectangle to compare
 * @param tx, ty          Origin of the target rectangle to compare
 * @param averrp          Average distance encountered in all pixel differences (0..1)
 * @param maxerrp         Maximum distance encountered in all pixel differences (0..1)
 * @param nerrp           Number of pixel differences counted
 * @param npixp           Number of total pixels compared (swidth * sheight)
 * @returns               False if no pixel differences where found, else True
 *
 * Compare a rectangle of the @a source pixmap with a rectangle of @a this pixmap.
 * A simple sum difference is calculated per pixel (differences can vary between 0.0 and 1.0)
 * and accumulated for all pixels in the rectangle.
 */
template<class Pixbuf> bool
PixmapT<Pixbuf>::compare (const Pixbuf &source,
                          uint sx, uint sy, int swidth, int sheight,
                          uint tx, uint ty,
                          double *averrp, double *maxerrp,
                          double *nerrp, double *npixp) const
{
  if (averrp) *averrp = 0;
  if (maxerrp) *maxerrp = 0;
  if (nerrp) *nerrp = 0;
  if (npixp) *npixp = 0;
  if (sx >= size_t (source.width()) || sy >= size_t (source.height()) ||
      tx >= size_t (pixbuf_->width()) || ty >= size_t (pixbuf_->height()))
    return false;
  if (swidth < 0)
    swidth = source.width();
  if (sheight < 0)
    sheight = source.height();
  swidth = MIN (MIN (pixbuf_->width() - int (tx), source.width() - int (sx)), swidth);
  sheight = MIN (MIN (pixbuf_->height() - int (ty), source.height() - int (sy)), sheight);
  const uint npix = sheight * swidth;
  uint nerr = 0;
  double erraccu = 0, errmax = 0;
  for (int k = 0; k < sheight; k++)
    {
      const uint32 *r1 = pixbuf_->row (ty + k);
      const uint32 *r2 = source.row (sy + k);
      for (int j = 0; j < swidth; j++)
        if (r1[tx + j] != r2[sx + j])
          {
            const double scale = 1.0 / 510.0; // 510 == 255 * sqrt (4)
            const uint8 *p1 = (uint8*) &r1[tx + j], *p2 = (uint8*) &r2[sx + j];
            double pixerr = sqrt (SQR (p1[0] - p2[0]) + SQR (p1[1] - p2[1]) +
                                  SQR (p1[2] - p2[2]) + SQR (p1[3] - p2[3])) * scale;
            errmax = MAX (errmax, pixerr);
            erraccu += pixerr;
            nerr++;
          }
    }
  if (averrp)
    *averrp = erraccu / npix;
  if (maxerrp)
    *maxerrp = errmax;
  if (nerrp)
    *nerrp = nerr;
  if (npixp)
    *npixp = npix;
  return nerr != 0;
}

template<class Pixbuf> bool
PixmapT<Pixbuf>::load_png (const String &filename, bool tryrepair)
{
  size_t nbytes = 0;
  char *bytes = Path::memread (filename, &nbytes);
  const bool success = anon_load_png (*this, nbytes, bytes) || tryrepair;
  Path::memfree (bytes);
  return success;
}

template<class Pixbuf> bool
PixmapT<Pixbuf>::load_png (size_t nbytes, const char *bytes, bool tryrepair)
{
  if (!bytes)
    return false;
  return anon_load_png (*this, nbytes, bytes) || tryrepair;
}

template<class Pixbuf> void
PixmapT<Pixbuf>::copy (const Pixbuf &source, uint sx, uint sy, int swidth, int sheight, uint tx, uint ty)
{
  if (sx >= uint (source.width()) || sy >= uint (source.height()) ||
      tx >= uint (width()) || ty >= uint (height()))
    return;
  if (swidth < 0)
    swidth = source.width();
  if (sheight < 0)
    sheight = source.height();
  swidth = MIN (MIN (width() - int (tx), source.width() - int (sx)), swidth);
  sheight = MIN (MIN (height() - int (ty), source.height() - int (sy)), sheight);
  for (int j = 0; j < sheight; j++)
    {
      uint32 *r1 = row (ty + j);
      const uint32 *r2 = source.row (sy + j);
      memcpy (r1 + tx, r2 + sx, sizeof (r1[0]) * swidth);
    }
}

static inline vector<String>::const_iterator
find_attribute (const vector<String> &attributes, const String &name)
{
  const size_t l = name.size();
  const char *key = name.c_str();
  for (auto it = attributes.begin(); it != attributes.end(); ++it)
    if (strncmp (it->c_str(), key, l) == 0 && (*it)[l] == '=')
      return it;
  return attributes.end();
}

template<class Pixbuf> void
PixmapT<Pixbuf>::set_attribute (const String &name, const String &value)
{
  vector<String>::const_iterator it = find_attribute (pixbuf_->variables, name);
  if (it == pixbuf_->variables.end())
    pixbuf_->variables.push_back (name + "=" + value);
  else
    pixbuf_->variables[it - pixbuf_->variables.begin()] = name + "=" + value;
}

template<class Pixbuf> String
PixmapT<Pixbuf>::get_attribute (const String &name) const
{
  vector<String>::const_iterator it = find_attribute (pixbuf_->variables, name);
  if (it == pixbuf_->variables.end())
    return "";
  else
    return pixbuf_->variables[it - pixbuf_->variables.begin()].substr (name.size() + 1);
}

template<class Pixbuf> static void
pixmap_border (PixmapT<Pixbuf> pixmap, uint32 pixel)
{
  uint32 *row = pixmap.row (0);
  for (int i = 0; i < pixmap.width(); i++)
    row[i] = pixel;
  row = pixmap.row (pixmap.height() - 1);
  for (int i = 0; i < pixmap.width(); i++)
    row[i] = pixel;
  for (int i = 1; i < pixmap.height() - 1; i++)
    {
      row = pixmap.row (i);
      row[0] = pixel;
      row[pixmap.width() - 1] = pixel;
    }
}

static inline uint8 IMUL (uint8 v, uint8 alpha) { return (v * alpha * 0x0101 + 0x8080) >> 16; }

static inline uint32
premultiply (uint32 pixel)
{
  const uint8 alpha = pixel >> 24;
  uint32 p = alpha << 24;
  p |= IMUL (0xff & (pixel >> 16), alpha) << 16;     // red
  p |= IMUL (0xff & (pixel >> 8),  alpha) << 8;      // green
  p |= IMUL (0xff & pixel,         alpha);           // blue
  return p;
}

template<class Pixbuf> static int /* errno */
fill_pixmap_from_pixstream (PixmapT<Pixbuf> pixmap, bool has_alpha, bool rle_encoded,
                            uint pixdata_width, uint pixdata_height, const uint8 *encoded_pixdata)
{
  if (pixdata_width < 1 || pixdata_height < 1)
    return EINVAL;
  uint bpp = has_alpha ? 4 : 3;
  if (!encoded_pixdata)
    return EINVAL;

  if (rle_encoded)
    {
      const uint8 *rle_buffer = encoded_pixdata;
      uint32 *image_buffer = pixmap.row (0); // depends on alignment=0
      uint32 *image_limit = image_buffer + pixdata_width * pixdata_height;

      while (image_buffer < image_limit)
        {
          uint length = *(rle_buffer++);
          if ((length & ~128) == 0)
            return EINVAL;
          bool check_overrun = false;
          if (length & 128)
            {
              length = length - 128;
              check_overrun = image_buffer + length > image_limit;
              if (check_overrun)
                length = image_limit - image_buffer;
              if (bpp < 4)
                while (length--)
                  *image_buffer++ = premultiply ((0xff << 24) | (rle_buffer[0] << 16) | (rle_buffer[1] << 8) | rle_buffer[2]);
              else
                while (length--)
                  *image_buffer++ = premultiply ((rle_buffer[3] << 24) | (rle_buffer[0] << 16) | (rle_buffer[1] << 8) | rle_buffer[2]);
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
                    *image_buffer++ = premultiply ((0xff << 24) | (rle_buffer[0] << 16) | (rle_buffer[1] << 8) | rle_buffer[2]);
                    rle_buffer += 3;
                  }
              else
                while (length--)
                  {
                    *image_buffer++ = premultiply ((rle_buffer[3] << 24) | (rle_buffer[0] << 16) | (rle_buffer[1] << 8) | rle_buffer[2]);
                    rle_buffer += 4;
                  }
            }
          if (check_overrun)
            return EINVAL;
        }
    }
  else
    for (uint y = 0; y < pixdata_height; y++)
      {
        uint32 *row = pixmap.row (y);
        uint length = pixdata_width;
        if (bpp < 4)
          while (length--)
            {
              *row++ = premultiply ((0xff << 24) | (encoded_pixdata[0] << 16) | (encoded_pixdata[1] << 8) | encoded_pixdata[2]);
              encoded_pixdata += 3;
            }
        else
          while (length--)
            {
              *row++ = premultiply ((encoded_pixdata[3] << 24) | (encoded_pixdata[0] << 16) | (encoded_pixdata[1] << 8) | encoded_pixdata[2]);
              encoded_pixdata += 4;
            }
      }

  return 0;
}

static inline uint32
next_uint32 (const uint8 **streamp)
{
  const uint8 *stream = *streamp;
  uint32 result = (stream[0] << 24) + (stream[1] << 16) + (stream[2] << 8) + stream[3];
  *streamp = stream + 4;
  return result;
}

template<class Pixbuf> bool
PixmapT<Pixbuf>::load_pixstream (const uint8 *pixstream)
{
  errno = EINVAL;
  assert_return (pixstream != NULL, NULL);

  errno = ENODATA;
  const uint8 *s = pixstream;
  if (strncmp ((char*) s, "GdkP", 4) != 0)
    return NULL;
  s += 4;

  errno = ENODATA;
  uint len = next_uint32 (&s);
  if (len < 24)
    return NULL;

  errno = ENODATA;
  uint type = next_uint32 (&s);
  if (type != 0x02010001 &&     /* RLE/8bit/RGB */
      type != 0x01010001 &&     /* RAW/8bit/RGB */
      type != 0x02010002 &&     /* RLE/8bit/RGBA */
      type != 0x01010002)       /* RAW/8bit/RGBA */
    return NULL;

  errno = ENOMEM;
  next_uint32 (&s); /* rowstride */
  const uint pwidth = next_uint32 (&s);
  const uint pheight = next_uint32 (&s);
  if (pwidth < 1 || pheight < 1)
    return NULL;

  errno = ENOMEM;
  if (!try_resize (pwidth, pheight))
    return NULL;
  errno = fill_pixmap_from_pixstream (*this,
                                      (type & 0xff) == 0x02,
                                      (type >> 24) == 0x02,
                                      pwidth, pheight, s);
  return errno == 0;
}

} // Rapicorn

#include <png.h>

namespace {

static void
rgba_2_argb_pre (png_structp png, png_row_infop row_info, png_bytep data)
{
  for (uint i = 0; i < row_info->rowbytes; i += 4)
    {
      const uint8 alpha = data[i + 3];          // RGBA bytes
      uint32 p = alpha << 24;
      p |= IMUL (data[i + 0], alpha) << 16;     // red
      p |= IMUL (data[i + 1], alpha) << 8;      // green
      p |= IMUL (data[i + 2], alpha);           // blue
      *(uint32*) &data[i] = p;                  // store ARGB in native endianess
    }
}

static inline uint8 IDIV (uint8 v, uint8 alpha) { return (0xff * v + (alpha >> 1)) / alpha; }

static void
argb_pre_2_rgba (png_structp png, png_row_infop row_info, png_bytep data)
{
  for (uint i = 0; i < row_info->rowbytes; i += 4)
    {
      const uint32 p = *(uint32*) &data[i];     // ARGB in native endianess
      const uint8 alpha = p >> 24;
      data[i + 3] = alpha;                      // store RGBA bytes
      if (alpha == 0)
        data[i + 0] = data[i + 1] = data[i + 2] = 0;
      else
        {
          data[i + 0] = IDIV (p >> 16, alpha);  // red
          data[i + 1] = IDIV (p >> 8, alpha);   // green
          data[i + 2] = IDIV (p, alpha);        // blue
        }
    }
}

template<class Pixbuf>
struct PngContext {
  PixmapT<Pixbuf> pixmap;
  int             error;
  png_byte      **rows;
  FILE           *fp;
  const char     *fbytes;
  size_t          fsize, foffset;
  PngContext (PixmapT<Pixbuf> pxm) : pixmap (pxm), error (0), rows (NULL), fp (NULL), fbytes (NULL), fsize (0), foffset (0) {}
};

static int /* returns errno; longjmp() may jump out of this function */
pngcontext_configure_4argb (png_structp  png_ptr,
                            png_infop    info_ptr,
                            uint        *widthp,
                            uint        *heightp)
{
  int bit_depth = png_get_bit_depth (png_ptr, info_ptr);
  // check bit-depth to be only 1, 2, 4, 8, 16 to prevent libpng errors
  if (bit_depth < 1 || bit_depth > 16 || (bit_depth & (bit_depth - 1)))
    return EINVAL;
  int color_type = png_get_color_type (png_ptr, info_ptr);
  int interlace_type = png_get_interlace_type (png_ptr, info_ptr);
  // beware, ordering of the following transformation requests matters
  if (color_type == PNG_COLOR_TYPE_PALETTE)
    png_set_palette_to_rgb (png_ptr);                           // request RGB format
  if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
    png_set_gray_1_2_4_to_8 (png_ptr);                          // request 8bit per sample
  if (png_get_valid (png_ptr, info_ptr, PNG_INFO_tRNS))
    png_set_tRNS_to_alpha (png_ptr);                            // request transparency as alpha channel
  if (bit_depth == 16)
    png_set_strip_16 (png_ptr);                                 // request 8bit per sample
  if (bit_depth < 8)
    png_set_packing (png_ptr);                                  // request 8bit per sample
  if (color_type == PNG_COLOR_TYPE_GRAY ||
      color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
    png_set_gray_to_rgb (png_ptr);                              // request RGB format
  if (color_type == PNG_COLOR_TYPE_RGB ||
      color_type == PNG_COLOR_TYPE_GRAY)
    png_set_add_alpha (png_ptr, 0xff, PNG_FILLER_AFTER);        // request RGB as RGBA
  if (interlace_type != PNG_INTERLACE_NONE)
    png_set_interlace_handling (png_ptr);                       // request non-interlaced image
  png_set_read_user_transform_fn (png_ptr, rgba_2_argb_pre);    // premultiplies RGBA bytes to native endian ARGB
  // reflect the transformations
  png_read_update_info (png_ptr, info_ptr);
  png_uint_32 pwidth = 0, pheight = 0;
  int compression_type = 0, filter_type = 0;
  png_get_IHDR (png_ptr, info_ptr, &pwidth, &pheight,
                &bit_depth, &color_type, &interlace_type,
                &compression_type, &filter_type);
  *widthp = pwidth;
  *heightp = pheight;
  // sanity checks
  if (pwidth < 1 || pheight < 1 || pwidth > MAXDIM || pheight > MAXDIM)
    return ENOMEM;
  if (bit_depth != 8 ||
      color_type != PNG_COLOR_TYPE_RGB_ALPHA ||
      png_get_channels (png_ptr, info_ptr) != 4)
    return ENOSYS;
  return 0;
}

template<class Pixbuf> static void
pngcontext_read_bytes (png_structp png_ptr, png_bytep data, png_size_t length)
{
  PngContext<Pixbuf> &pcontext = *(PngContext<Pixbuf>*) png_get_io_ptr (png_ptr);
  const size_t l = pcontext.foffset >= pcontext.fsize ? 0 : MIN (length, pcontext.fsize - pcontext.foffset);
  memcpy (data, pcontext.fbytes + pcontext.foffset, l);
  memset (data + l, 0, length - l); // PNG provides no (!!) way to return length of short reads...
  pcontext.foffset += l;
  if (l == 0)
    png_error (png_ptr, "End Of File");
}

template<class Pixbuf> static void /* longjmp() may jump out of this function */
pngcontext_read_image (PngContext<Pixbuf> &pcontext,
                       png_structp png_ptr,
                       png_infop   info_ptr)
{
  /* setup PNG io */
  png_init_io (png_ptr, pcontext.fp);
  if (pcontext.fbytes)
    png_set_read_fn (png_ptr, &pcontext, pngcontext_read_bytes<Pixbuf>);
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
  if (!pcontext.rows || !pcontext.pixmap.try_resize (width, height))
    {
      pcontext.error = ENOMEM;
      return;
    }
  // pixmap_fill (pcontext.pixmap, 0);          // pixmaps are preinitialized to transparent bg
  pixmap_border (pcontext.pixmap, 0x80ff0000);  /* show red error border for partial images */
  for (uint i = 0; i < height; i++)
    pcontext.rows[i] = (png_byte*) pcontext.pixmap.row (i);
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
                pcontext.pixmap.set_attribute ("comment", output);
                if (is_comment)
                  break;
              }
          }
      }
  pcontext.error = 0;
}

template<class Pixbuf> static void
pngcontext_error (png_structp png_ptr, png_const_charp error_msg)
{
  PngContext<Pixbuf> *pcontext = (PngContext<Pixbuf>*) png_get_error_ptr (png_ptr);
  if (!pcontext->error)
    pcontext->error = EIO;
  longjmp (png_ptr->jmpbuf, 1);
}

static void
pngcontext_nop (png_structp     png_ptr,
                png_const_charp error_msg)
{}

template<class Pixbuf> static bool
anon_load_png (PixmapT<Pixbuf> pixmap, size_t nbytes, const char *bytes)
{
  PngContext<Pixbuf> pcontext (pixmap);
  // open image
  pcontext.fp = NULL; // fopen (filename.c_str(), "rb");
  pcontext.fbytes = bytes;
  pcontext.fsize = nbytes;
  pcontext.foffset = 0;
  if (!pcontext.fp && !pcontext.fbytes)
    return NULL; // errno set by fopen/memread
  // read and check PNG signature
  uint8 sigbuffer[8];
  if ((pcontext.fp && fread (sigbuffer, 1, 8, pcontext.fp) != 8) ||
      (pcontext.fbytes && !memcpy (sigbuffer, pcontext.fbytes, 8)) ||
      (pcontext.fbytes && !(pcontext.foffset = 8)) ||
      png_sig_cmp (sigbuffer, 0, 8) != 0)
    {
      if (pcontext.fp)
        fclose (pcontext.fp);
      errno = ENODATA; // not a PNG
      return NULL;
    }
  /* allocate resources */
  pcontext.error = ENOMEM;
  png_structp png_ptr = png_create_read_struct (PNG_LIBPNG_VER_STRING, &pcontext, pngcontext_error<Pixbuf>, pngcontext_nop);
  png_infop info_ptr = !png_ptr ? NULL : png_create_info_struct (png_ptr);
  if (!info_ptr)
    {
      if (png_ptr)
        png_destroy_read_struct (&png_ptr, NULL, NULL);
      if (pcontext.fp)
        fclose (pcontext.fp);
      errno = pcontext.error;
      return NULL;
    }
  /* save stack for longjmp() in png_loader_error() */
  if (setjmp (png_ptr->jmpbuf) == 0)
    {
      /* read pixel image */
      pcontext.error = EINVAL;
      pngcontext_read_image (pcontext, png_ptr, info_ptr);
    }
  /* cleanup */
  png_destroy_read_struct (&png_ptr, &info_ptr, NULL);
  if (pcontext.fp)
    fclose (pcontext.fp);
  if (pcontext.rows)
    delete[] pcontext.rows;
  errno = pcontext.error; // maybe set even if pcontext.pixmap!=NULL
  return errno == 0;
}

} // Anon

namespace Rapicorn {

template<class Pixbuf> bool
PixmapT<Pixbuf>::save_png (const String &filename) /* assigns errno */
{
  const int w = width();
  const int h = height();
  const int depth = 8;
  // allocate resources
  PngContext<Pixbuf> pcontext (*this);
  pcontext.error = ENOMEM;
  png_structp png_ptr = png_create_write_struct (PNG_LIBPNG_VER_STRING, &pcontext,
                                                 pngcontext_error<Pixbuf>, pngcontext_nop);
  png_infop info_ptr = !png_ptr ? NULL : png_create_info_struct (png_ptr);
  if (!info_ptr)
    {
      if (png_ptr)
        png_destroy_write_struct (&png_ptr, &info_ptr);
      errno = pcontext.error;
      return false;
    }
  /* setup key=value pairs for PNG text section */
  const StringVector &sv = pixbuf_->variables;
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
      /* perform image IO */
      png_init_io (png_ptr, pcontext.fp);
      png_write_info (png_ptr, info_ptr);
      png_set_write_user_transform_fn (png_ptr, argb_pre_2_rgba);
      for (int y = 0; y < h; y++)
        {
          png_bytep row_ptr = (png_bytep) row (y);
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

// Explicitely force instantiation and compilation of these templates
template class PixmapT<Rapicorn::ClnT_Pixbuf>; // Instantiate client-side Pixmap
template class PixmapT<Rapicorn::SrvT_Pixbuf>; // Instantiate server-side Pixmap

} // Rapicorn
