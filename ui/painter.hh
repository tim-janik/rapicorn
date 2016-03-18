// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#ifndef __RAPICORN_PAINTER_HH__
#define __RAPICORN_PAINTER_HH__

#include <ui/primitives.hh>

namespace Rapicorn {

// == Prototypes ==
namespace Svg {
class File;
typedef std::shared_ptr<File> FileP;
} // Svg


/// Cairo painting helper class.
class CPainter {
protected:
  cairo_t *cr;
public:
  explicit      CPainter                (cairo_t *context);
  virtual      ~CPainter                ();
  void          draw_filled_rect        (int x, int y, int width, int height, Color fill_color);
  void          draw_shaded_rect        (int xc0, int yc0, Color color0, int xc1, int yc1, Color color1);
  void          draw_center_shade_rect  (int xc0, int yc0, Color color0, int xc1, int yc1, Color color1);
  void          draw_border             (int x, int y, int width, int height, Color border, const vector<double> &dashes = vector<double>(), double dash_offset = 0.5);
  void          draw_shadow             (int x, int y, int width, int height,
                                         Color outer_upper_left, Color inner_upper_left,
                                         Color inner_lower_right, Color outer_lower_right);
  void          draw_dir_arrow          (double x, double y, double width, double height, Color c, Direction dir);
};

// == Cairo Utilities ==
cairo_surface_t*        cairo_surface_from_pixmap       (Pixmap pixmap);

/// Image painting and transformation.
class ImagePainter {
public:  struct ImageBackend;
private:
  typedef std::shared_ptr<ImageBackend> ImageBackendP;
  ImageBackendP image_backend_;
  void          svg_file_setup  (Svg::FileP svgfile, const String &fragment);
public:
  explicit      ImagePainter    ();
  explicit      ImagePainter    (Pixmap pixmap);                              ///< Construct from pixmap object.
  explicit      ImagePainter    (const String &resource_identifier);          ///< Construct from image resource path.
  explicit      ImagePainter    (Svg::FileP svgfile, const String &fragment); ///< Construct from SVG image fragment.
  virtual      ~ImagePainter    ();     ///< Delete an ImagePainter.
  virtual StringVector list     (const String &prefix = "");                  ///< List the element IDs in an SVG File.
  Requisition   image_size      ();     ///< Retrieve original width and height of the image.
  Rect          fill_area       ();     ///< Retrieve fill area of the image, equals width and height if unknown.
  /// Render image into cairo context transformed into @a image_rect, clipped by @a render_rect.
  void          draw_image      (cairo_t *cairo_context, const Rect &render_rect, const Rect &image_rect);
  ImagePainter& operator=       (const ImagePainter &ip);       ///< Assign an ImagePainter.
  explicit      operator bool   () const; ///< Returns wether image_size() yields 0 for either dimension.
};

} // Rapicorn

#endif  /* __RAPICORN_PAINTER_HH__ */
