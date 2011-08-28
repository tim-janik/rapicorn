// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
#include <rapicorn.hh>
#include <cairo.h>

#define CHECK_CAIRO_STATUS(status)      do {    \
  cairo_status_t ___s = (status);               \
  if (___s != CAIRO_STATUS_SUCCESS)             \
    Rapicorn::printerr ("%s: %s\n", cairo_status_to_string (___s), #status); \
  } while (0)

namespace {
using namespace Rapicorn;

static void
drawable_redraw (Drawable &drawable, int x, int y, int w, int h)
{
  // boilerplate
  PixelRect pic;
  pic.x = x, pic.y = y, pic.width = w, pic.height = h, pic.rowstride = pic.width;
  pic.argb_pixels.resize (pic.rowstride * pic.height);
  cairo_surface_t *surface = cairo_image_surface_create_for_data ((uint8*) pic.argb_pixels.data(), CAIRO_FORMAT_ARGB32,
                                                                  pic.width, pic.height, pic.rowstride * 4);
  CHECK_CAIRO_STATUS (cairo_surface_status (surface));
  cairo_t *cr = cairo_create (surface);
  cairo_surface_destroy (surface);
  // outline drawing rectangle
  cairo_set_line_width (cr, 1);
  cairo_set_source_rgba (cr, 0, 0, 1, 1);
  cairo_rectangle (cr, 0, 0, pic.width, pic.height);
  cairo_stroke (cr);
  // custom drawing
  const double lthickness = 2.25;
  cairo_set_source_rgba (cr, 0, 0, 0, 1);
  cairo_set_line_width (cr, lthickness);
  cairo_move_to (cr, 15, 15), cairo_line_to (cr, 35, 35);
  cairo_move_to (cr, 35, 35), cairo_line_to (cr, 50, 20);
  cairo_move_to (cr, 50, 20), cairo_line_to (cr, 75, 90);
  cairo_move_to (cr, 75, 90), cairo_line_to (cr, 230, 93);
  cairo_stroke (cr);
  cairo_set_line_width (cr, lthickness * 0.5);
  cairo_move_to (cr, 75, 120), cairo_line_to (cr, 230, 110);
  // render remotely
  cairo_destroy (cr);
  drawable.draw_rect (pic);
}

extern "C" int
main (int   argc,
      char *argv[])
{
  // initialize Rapicorn
  Application app = init_app (String ("Rapicorn/Examples/") + RAPICORN__FILE__, &argc, argv);

  // find and load GUI definitions relative to argv[0]
  app.auto_load ("RapicornExamples", "graphics.xml", argv[0]);

  // create main wind0w
  Wind0w wind0w = app.create_wind0w ("graphics-dialog");

  // hook up drawable test
  Drawable drawable = wind0w.component<Drawable> ("/Drawable#drawable1");
  RAPICORN_ASSERT (drawable._is_null() == false);
  drawable.redraw += slot (&drawable_redraw);

  // show main window
  wind0w.show();

  // run event loops while wind0ws are on screen
  return app.run_and_exit();
}

} // anon
