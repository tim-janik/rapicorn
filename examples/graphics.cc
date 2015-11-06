// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
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
drawable_redraw (DrawableH &drawable, int x, int y, int w, int h)
{
  if (!w || !h)
    return;
  // boilerplate
  Pixbuf pixbuf;
  int px = x, py = y;
  pixbuf.resize (w, h);
  cairo_surface_t *surface = cairo_image_surface_create_for_data ((uint8*) pixbuf.pixels.data(), CAIRO_FORMAT_ARGB32,
                                                                  pixbuf.width(), pixbuf.height(), pixbuf.width() * 4);
  CHECK_CAIRO_STATUS (cairo_surface_status (surface));
  cairo_t *cr = cairo_create (surface);
  cairo_surface_destroy (surface);
  // outline drawing rectangle
  cairo_set_line_width (cr, 1);
  cairo_set_source_rgba (cr, 0, 0, 1, 1);
  cairo_rectangle (cr, 0, 0, pixbuf.width(), pixbuf.height());
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
  drawable.draw_rect (px, py, pixbuf);
}

extern "C" int
main (int   argc,
      char *argv[])
{
  // initialize Rapicorn
  ApplicationH app = init_app (__PRETTY_FILE__, &argc, argv);

  // find and load GUI definitions relative to argv[0]
  app.auto_load ("graphics.xml", argv[0]);

  // create main window
  WindowH window = app.create_window ("graphics-dialog");

  // hook up drawable test
  DrawableH drawable = window.component<DrawableH> ("Drawable#drawable1");
  RAPICORN_ASSERT (drawable != NULL);
  drawable.sig_redraw() += [&drawable] (int x, int y, int w, int h) { drawable_redraw (drawable, x, y, w, h); };

  // show main window
  window.show();

  // run event loops while windows are on screen
  return app.run_and_exit();
}

} // anon
