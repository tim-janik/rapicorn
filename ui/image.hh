// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __RAPICORN_IMAGE_HH__
#define __RAPICORN_IMAGE_HH__

#include <ui/widget.hh>
#include "../rcore/rsvg/svg.hh"

namespace Rapicorn {

class ImageImpl : public virtual WidgetImpl, public virtual ImageIface {
  String        image_url_, stock_id_;
  Pixmap        pixmap_;
  Svg::FileP    svgf_;
  Svg::ElementP svge_;
  struct PixView {
    int xoffset, yoffset, pwidth, pheight;
    double xscale, yscale, scale;
  };
protected:
  void                  reset           ();
  void                  load_pixmap     ();
  void                  stock           (const String &stock_id);
  String                stock           () const;
  PixView               adjust_view     ();
  void                  render_svg      (RenderContext &rcontext, const Rect &render_rect);
  virtual void          size_request    (Requisition &requisition);
  virtual void          size_allocate   (Allocation area, bool changed);
  virtual void          render          (RenderContext &rcontext, const Rect &rect);
public:
  virtual void          pixbuf  (const Pixbuf &pixbuf);
  virtual Pixbuf        pixbuf  () const;
  virtual void          source  (const String &uri);
  virtual String        source  () const;
};

} // Rapicorn

#endif  // __RAPICORN_IMAGE_HH__
