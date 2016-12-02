// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#include "paintwidgets.hh"
#include "factory.hh"
#include "painter.hh"
#include "rcore/cairoutils.hh"

namespace Rapicorn {

// == ArrowImpl ==
ArrowImpl::ArrowImpl() :
  dir_ (Direction::RIGHT)
{}

ArrowImpl::~ArrowImpl()
{}

Direction
ArrowImpl::arrow_dir () const
{
  return dir_;
}

void
ArrowImpl::arrow_dir (Direction dir)
{
  dir_ = dir;
  invalidate_content();
  changed ("arrow_dir");
}

static DataKey<SizePolicy> size_policy_key;

SizePolicy
ArrowImpl::size_policy () const
{
  SizePolicy spol = get_data (&size_policy_key);
  return spol;
}

void
ArrowImpl::size_policy (SizePolicy spol)
{
  if (spol == 0)
    delete_data (&size_policy_key);
  else
    set_data (&size_policy_key, spol);
  invalidate_size();
  changed ("size_policy");
}

void
ArrowImpl::size_request (Requisition &requisition)
{
  requisition.width = 3;
  requisition.height = 3;
}

void
ArrowImpl::size_allocate (Allocation area)
{
  SizePolicy spol = size_policy();
  if (spol == SizePolicy::WIDTH_FROM_HEIGHT)
    tune_requisition (area.height, -1);
  else if (spol == SizePolicy::HEIGHT_FROM_WIDTH)
    tune_requisition (-1, area.width);
}

void
ArrowImpl::render (RenderContext &rcontext)
{
  IRect ia = allocation();
  int x = ia.x, y = ia.y, width = ia.width, height = ia.height;
  if (width >= 2 && height >= 2)
    {
      cairo_t *cr = cairo_context (rcontext);
      CPainter painter (cr);
      painter.draw_dir_arrow (x, y, width, height, foreground(), dir_);
    }
}

static const WidgetFactory<ArrowImpl> arrow_factory ("Rapicorn::Arrow");

// == DotGrid ==
DotGridImpl::DotGridImpl() :
  normal_dot_ (DrawFrame::IN),
  active_dot_ (DrawFrame::IN),
  n_hdots_ (1), n_vdots_ (1),
  right_padding_dots_ (0), top_padding_dots_ (0),
  left_padding_dots_ (0), bottom_padding_dots_ (0)
{}

DotGridImpl::~DotGridImpl()
{}

DrawFrame
DotGridImpl::dot_type () const
{
  RAPICORN_ASSERT_UNREACHED();
}

void
DotGridImpl::dot_type (DrawFrame ft)
{
  normal_dot (ft);
  active_dot (ft);
}

DrawFrame
DotGridImpl::current_dot ()
{
  return ancestry_active() ? active_dot() : normal_dot();
}

static inline int
u31 (int v)
{
  return CLAMP (v, 0, INT_MAX);
}

void
DotGridImpl::active_dot (DrawFrame ft)
{
  active_dot_ = ft;
  invalidate_content();
  changed ("active_dot");
}

DrawFrame
DotGridImpl::active_dot () const
{
  return active_dot_;
}

void
DotGridImpl::normal_dot (DrawFrame ft)
{
  normal_dot_ = ft;
  invalidate_content();
  changed ("normal_dot");
}

DrawFrame
DotGridImpl::normal_dot () const
{
  return normal_dot_;
}

void
DotGridImpl::n_hdots (int num)
{
  n_hdots_ = u31 (num);
  invalidate_content();
  changed ("n_hdots");
}

int
DotGridImpl::n_hdots () const
{
  return n_hdots_;
}

void
DotGridImpl::n_vdots (int num)
{
  n_vdots_ = u31 (num);
  invalidate_content();
  changed ("n_vdots");
}

int
DotGridImpl::n_vdots () const
{
  return n_vdots_;
}

int
DotGridImpl::right_padding_dots () const
{
  return right_padding_dots_;
}

void
DotGridImpl::right_padding_dots (int c)
{
  right_padding_dots_ = u31 (c);
  invalidate_content();
  changed ("right_padding_dots");
}

int
DotGridImpl::top_padding_dots () const
{
  return top_padding_dots_;
}

void
DotGridImpl::top_padding_dots (int c)
{
  top_padding_dots_ = u31 (c);
  invalidate_content();
  changed ("top_padding_dots");
}

int
DotGridImpl::left_padding_dots () const
{
  return left_padding_dots_;
}

void
DotGridImpl::left_padding_dots (int c)
{
  left_padding_dots_ = u31 (c);
  invalidate_content();
  changed ("left_padding_dots");
}

int
DotGridImpl::bottom_padding_dots () const
{
  return bottom_padding_dots_;
}

void
DotGridImpl::bottom_padding_dots (int c)
{
  bottom_padding_dots_ = u31 (c);
  invalidate_content();
  changed ("bottom_padding_dots");
}

void
DotGridImpl::size_request (Requisition &requisition)
{
  const uint ythick = 1, xthick = 1;
  requisition.width = n_hdots_ * (xthick + xthick) + MAX (n_hdots_ - 1, 0) * xthick;
  requisition.height = n_vdots_ * (ythick + ythick) + MAX (n_vdots_ - 1, 0) * ythick;
  requisition.width += (right_padding_dots_ + left_padding_dots_) * 3 * xthick;
  requisition.height += (top_padding_dots_ + bottom_padding_dots_) * 3 * ythick;
}

void
DotGridImpl::size_allocate (Allocation area)
{}

void
DotGridImpl::render (RenderContext &rcontext)
{
  const int ythick = 1, xthick = 1;
  int n_hdots = n_hdots_, n_vdots = n_vdots_;
  const IRect ia = allocation();
  int x = ia.x, y = ia.y, width = ia.width, height = ia.height;
  const int rq_width = n_hdots_ * (xthick + xthick) + MAX (n_hdots - 1, 0) * xthick;
  const int rq_height = n_vdots_ * (ythick + ythick) + MAX (n_vdots - 1, 0) * ythick;
  /* split up extra width */
  const uint hpadding = right_padding_dots_ + left_padding_dots_;
  const double halign = hpadding ? left_padding_dots_ * 1.0 / hpadding : 0.5;
  if (rq_width < width)
    x += ifloor ((width - rq_width) * halign);
  /* split up extra height */
  const uint vpadding = top_padding_dots_ + bottom_padding_dots_;
  const double valign = vpadding ? bottom_padding_dots_ * 1.0 / vpadding : 0.5;
  if (rq_height < height)
    y += ifloor ((height - rq_height) * valign);
  /* draw dots */
  if (width >= 2 * xthick && height >= 2 * ythick && n_hdots && n_vdots)
    {
      /* limit n_hdots */
      if (rq_width > width)
        {
          const int w = width - 2 * xthick;     // dot1
          n_hdots = 1 + w / (3 * xthick);
        }
      /* limit n_vdots */
      if (rq_height > height)
        {
          const int h = height - 2 * ythick;    // dot1
          n_vdots = 1 + h / (3 * ythick);
        }
      cairo_t *cr = cairo_context (rcontext);
      CPainter rp (cr);
      for (int j = 0; j < n_vdots; j++)
        {
          int xtmp = 0;
          for (int i = 0; i < n_hdots; i++)
            {
              rp.draw_shaded_rect (x + xtmp, y + 2 * ythick - 1, dark_color(),
                                   x + xtmp + 2 * xthick - 1, y, light_color());
              xtmp += 3 * xthick;
            }
          y += 3 * ythick;
        }
    }
}

static const WidgetFactory<DotGridImpl> dot_grid_factory ("Rapicorn::DotGrid");

// == DrawableImpl ==
DrawableImpl::DrawableImpl() :
  x_ (0), y_ (0)
{}

void
DrawableImpl::size_request (Requisition &requisition)
{
  requisition.width = 320;
  requisition.height = 200;
}

void
DrawableImpl::size_allocate (Allocation area)
{
  if (false) // clear out, can lead to flicker
    {
      pixbuf_ = Pixbuf();
      x_ = 0;
      y_ = 0;
    }
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
  invalidate_content();
}

void
DrawableImpl::render (RenderContext &rcontext)
{
  const uint size = 10;
  const Allocation &area = allocation();
  cairo_t *cr = cairo_context (rcontext);
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
      CAIRO_CHECK_STATUS (cairo_surface_status (surface));
      cairo_set_source_surface (cr, surface, x_, y_);
      cairo_paint (cr);
      cairo_surface_destroy (surface);
    }
}

static const WidgetFactory<DrawableImpl> drawable_factory ("Rapicorn::Drawable");

} // Rapicorn
