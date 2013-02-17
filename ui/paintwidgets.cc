// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#include "paintwidgets.hh"
#include "factory.hh"
#include "painter.hh"

#define CHECK_CAIRO_STATUS(status)      do {    \
  cairo_status_t ___s = (status);               \
  if (___s != CAIRO_STATUS_SUCCESS)             \
    DEBUG ("%s: %s", cairo_status_to_string (___s), #status);   \
  } while (0)

namespace Rapicorn {

static DataKey<SizePolicyType> size_policy_key;

const PropertyList&
Arrow::_property_list()
{
  static Property *properties[] = {
    MakeProperty (Arrow, arrow_dir,   _("Arrow Direction"), _("The direction the arrow points to"), "rw"),
    MakeProperty (Arrow, size_policy, _("Size Policy"),     _("Policy which determines coupling of width and height"), "rw"),
  };
  static const PropertyList property_list (properties, WidgetImpl::_property_list());
  return property_list;
}

class ArrowImpl : public virtual WidgetImpl, public virtual Arrow {
  DirType dir_;
public:
  explicit ArrowImpl() :
    dir_ (DIR_RIGHT)
  {}
  ~ArrowImpl()
  {}
  virtual void    arrow_dir (DirType dir)       { dir_ = dir; expose(); }
  virtual DirType arrow_dir () const            { return dir_; }
  virtual void
  size_policy (SizePolicyType spol)
  {
    if (!spol)
      delete_data (&size_policy_key);
    else
      set_data (&size_policy_key, spol);
  }
  virtual SizePolicyType
  size_policy () const
  {
    SizePolicyType spol = get_data (&size_policy_key);
    return spol;
  }
protected:
  virtual void
  size_request (Requisition &requisition)
  {
    requisition.width = 3;
    requisition.height = 3;
  }
  virtual void
  size_allocate (Allocation area, bool changed)
  {
    SizePolicyType spol = size_policy();
    if (spol == SIZE_POLICY_WIDTH_FROM_HEIGHT)
      tune_requisition (area.height, -1);
    else if (spol == SIZE_POLICY_HEIGHT_FROM_WIDTH)
      tune_requisition (-1, area.width);
  }
  virtual void
  render (RenderContext &rcontext, const Rect &rect)
  {
    IRect ia = allocation();
    int x = ia.x, y = ia.y, width = ia.width, height = ia.height;
    if (width >= 2 && height >= 2)
      {
        cairo_t *cr = cairo_context (rcontext, rect);
        CPainter painter (cr);
        painter.draw_dir_arrow (x, y, width, height, foreground(), dir_);
      }
  }
};
static const WidgetFactory<ArrowImpl> arrow_factory ("Rapicorn::Factory::Arrow");

void
DotGrid::dot_type (FrameType ft)
{
  normal_dot (ft);
  impressed_dot (ft);
}

const PropertyList&
DotGrid::_property_list()
{
  static Property *properties[] = {
    MakeProperty (DotGrid, normal_dot, _("Normal Dot"), _("The kind of dot-frame to draw in normal state"), "rw"),
    MakeProperty (DotGrid, impressed_dot, _("Impresed Dot"), _("The kind of dot-frame to draw in impressed state"), "rw"),
    MakeProperty (DotGrid, dot_type, _("Dot Type"), _("The kind of dot-frame to draw in all states"), "w"),
    MakeProperty (DotGrid, n_hdots, _("H-Dot #"), _("The number of horizontal dots to be drawn"), 0u, 99999u, 3u, "rw"),
    MakeProperty (DotGrid, n_vdots, _("V-Dot #"), _("The number of vertical dots to be drawn"), 0u, 99999u, 3u, "rw"),
    MakeProperty (DotGrid, right_padding_dots, _("Right Padding Dots"), _("Amount of padding in dots to add at the child's right side"), 0, 65535, 3, "rw"),
    MakeProperty (DotGrid, top_padding_dots, _("Top Padding Dots"), _("Amount of padding in dots to add at the child's top side"), 0, 65535, 3, "rw"),
    MakeProperty (DotGrid, left_padding_dots, _("Left Padding Dots"), _("Amount of padding in dots to add at the child's left side"), 0, 65535, 3, "rw"),
    MakeProperty (DotGrid, bottom_padding_dots, _("Bottom Padding Dots"), _("Amount of padding in dots to add at the child's bottom side"), 0, 65535, 3, "rw"),
  };
  static const PropertyList property_list (properties, WidgetImpl::_property_list());
  return property_list;
}

class DotGridImpl : public virtual WidgetImpl, public virtual DotGrid {
  FrameType normal_dot_, impressed_dot_;
  uint      n_hdots_, n_vdots_;
  uint16    right_padding_dots_, top_padding_dots_, left_padding_dots_, bottom_padding_dots_;
public:
  explicit DotGridImpl() :
    normal_dot_ (FRAME_IN),
    impressed_dot_ (FRAME_IN),
    n_hdots_ (1), n_vdots_ (1),
    right_padding_dots_ (0), top_padding_dots_ (0),
    left_padding_dots_ (0), bottom_padding_dots_ (0)
  {}
  ~DotGridImpl()
  {}
  virtual void      impressed_dot (FrameType ft)        { impressed_dot_ = ft; expose(); }
  virtual FrameType impressed_dot () const              { return impressed_dot_; }
  virtual void      normal_dot    (FrameType ft)        { normal_dot_ = ft; expose(); }
  virtual FrameType normal_dot    () const              { return normal_dot_; }
  FrameType         current_dot   () const              { return branch_impressed() ? impressed_dot() : normal_dot(); }
  virtual void      n_hdots       (uint   num)          { n_hdots_ = num; expose(); }
  virtual uint      n_hdots       () const              { return n_hdots_; }
  virtual void      n_vdots       (uint   num)          { n_vdots_ = num; expose(); }
  virtual uint      n_vdots       () const              { return n_vdots_; }
  virtual uint      right_padding_dots () const         { return right_padding_dots_; }
  virtual void      right_padding_dots (uint c)         { right_padding_dots_ = c; expose(); }
  virtual uint      top_padding_dots  () const          { return top_padding_dots_; }
  virtual void      top_padding_dots  (uint c)          { top_padding_dots_ = c; expose(); }
  virtual uint      left_padding_dots  () const         { return left_padding_dots_; }
  virtual void      left_padding_dots  (uint c)         { left_padding_dots_ = c; expose(); }
  virtual uint      bottom_padding_dots  () const       { return bottom_padding_dots_; }
  virtual void      bottom_padding_dots  (uint c)       { bottom_padding_dots_ = c; expose(); }
  virtual void
  size_request (Requisition &requisition)
  {
    uint ythick = 1, xthick = 1;
    requisition.width = n_hdots_ * (xthick + xthick) + MAX (n_hdots_ - 1, 0) * xthick;
    requisition.height = n_vdots_ * (ythick + ythick) + MAX (n_vdots_ - 1, 0) * ythick;
    requisition.width += (right_padding_dots_ + left_padding_dots_) * 3 * xthick;
    requisition.height += (top_padding_dots_ + bottom_padding_dots_) * 3 * ythick;
  }
  virtual void
  size_allocate (Allocation area, bool changed)
  {}
  virtual void
  render (RenderContext &rcontext, const Rect &rect)
  {
    int ythick = 1, xthick = 1, n_hdots = n_hdots_, n_vdots = n_vdots_;
    IRect ia = allocation();
    int x = ia.x, y = ia.y, width = ia.width, height = ia.height;
    int rq_width = n_hdots_ * (xthick + xthick) + MAX (n_hdots - 1, 0) * xthick;
    int rq_height = n_vdots_ * (ythick + ythick) + MAX (n_vdots - 1, 0) * ythick;
    /* split up extra width */
    uint hpadding = right_padding_dots_ + left_padding_dots_;
    double halign = hpadding ? left_padding_dots_ * 1.0 / hpadding : 0.5;
    if (rq_width < width)
      x += ifloor ((width - rq_width) * halign);
    /* split up extra height */
    uint vpadding = top_padding_dots_ + bottom_padding_dots_;
    double valign = vpadding ? bottom_padding_dots_ * 1.0 / vpadding : 0.5;
    if (rq_height < height)
      y += ifloor ((height - rq_height) * valign);
    /* draw dots */
    if (width >= 2 * xthick && height >= 2 * ythick && n_hdots && n_vdots)
      {
        /* limit n_hdots */
        if (rq_width > width)
          {
            int w = width - 2 * xthick;         /* dot1 */
            n_hdots = 1 + w / (3 * xthick);
          }
        /* limit n_vdots */
        if (rq_height > height)
          {
            int h = height - 2 * ythick;        /* dot1 */
            n_vdots = 1 + h / (3 * ythick);
          }
        cairo_t *cr = cairo_context (rcontext, rect);
        CPainter rp (cr);
        for (int j = 0; j < n_vdots; j++)
          {
            int xtmp = 0;
            for (int i = 0; i < n_hdots; i++)
              {
                rp.draw_shaded_rect (x + xtmp, y + 2 * ythick - 1, dark_shadow(),
                                     x + xtmp + 2 * xthick - 1, y, light_glint());
                xtmp += 3 * xthick;
              }
            y += 3 * ythick;
          }
      }
  }
};
static const WidgetFactory<DotGridImpl> dot_grid_factory ("Rapicorn::Factory::DotGrid");

// == DrawableImpl ==
DrawableImpl::DrawableImpl() :
  x_ (0), y_ (0)
{}

const PropertyList&
DrawableImpl::_property_list ()
{
  return RAPICORN_AIDA_PROPERTY_CHAIN (WidgetImpl::_property_list(), DrawableIface::_property_list());
}

void
DrawableImpl::size_request (Requisition &requisition)
{
  requisition.width = 320;
  requisition.height = 200;
}

void
DrawableImpl::size_allocate (Allocation area, bool changed)
{
  pixbuf_ = Pixbuf();
  x_ = 0;
  y_ = 0;
  sig_redraw.emit (area.x, area.y, area.width, area.height);
}

void
DrawableImpl::draw_rect (int x, int y, const Pixbuf &pixbuf)
{
  const Allocation &area = allocation();
  const size_t rowstride = pixbuf.width();
  if (x >= area.x && y >= area.y &&
      x + pixbuf.width() <= area.x + area.width &&
      y + pixbuf.height() <= area.y + area.height &&
      rowstride * pixbuf.height() <= pixbuf.pixels.size())
    {
      x_ = x;
      y_ = y;
      pixbuf_ = pixbuf;
    }
  else if (pixbuf_.width() > 0)
    {
      pixbuf_ = Pixbuf();
      x_ = 0;
      y_ = 0;
    }
  expose();
}

void
DrawableImpl::render (RenderContext &rcontext, const Rect &rect)
{
  const uint size = 10;
  const Allocation &area = allocation();
  cairo_t *cr = cairo_context (rcontext, rect);
  // checkerboard pattern
  if (true)
    {
      cairo_save (cr);
      cairo_surface_t *ps = cairo_surface_create_similar (cairo_get_target (cr), CAIRO_CONTENT_COLOR, 2 * size, 2 * size);
      cairo_t *pc = cairo_create (ps);
      cairo_set_source_rgb (pc, 1.0, 1.0, 1.0);
      cairo_rectangle (pc,    0,    0, size, size);
      cairo_rectangle (pc, size, size, size, size);
      cairo_fill (pc);
      cairo_set_source_rgb (pc, 0.9, 0.9, 0.9);
      cairo_rectangle (pc,    0, size, size, size);
      cairo_rectangle (pc, size,    0, size, size);
      cairo_fill (pc);
      cairo_destroy (pc);
      cairo_pattern_t *pat = cairo_pattern_create_for_surface (ps);
      cairo_surface_destroy (ps);
      cairo_pattern_set_extend (pat, CAIRO_EXTEND_REPEAT);
      // render pattern
      cairo_rectangle (cr, area.x, area.y, area.width, area.height);
      cairo_clip (cr);
      cairo_translate (cr, area.x, area.y + area.height);
      cairo_set_source (cr, pat);
      cairo_paint (cr);
      cairo_pattern_destroy (pat);
      cairo_restore (cr);
    }
  // handle user draw
  if (pixbuf_.width() > 0 && pixbuf_.height() > 0)
    {
      const int rowstride = pixbuf_.width();
      cairo_surface_t *surface = cairo_image_surface_create_for_data ((uint8*) pixbuf_.pixels.data(), CAIRO_FORMAT_ARGB32,
                                                                      pixbuf_.width(), pixbuf_.height(), rowstride * 4);
      CHECK_CAIRO_STATUS (cairo_surface_status (surface));
      cairo_set_source_surface (cr, surface, x_, y_);
      cairo_paint (cr);
      cairo_surface_destroy (surface);
    }
}

static const WidgetFactory<DrawableImpl> drawable_factory ("Rapicorn::Factory::Drawable");

} // Rapicorn
