/* Rapicorn Examples
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
#include <rapicorn/rapicorn.hh>
#include "gtkrootwidget.hh"
#include <gtk/gtk.h>

namespace {
using namespace Rapicorn;
using Rapicorn::uint;

static void
construct_gui (GtkWindow  *window,
               const char *path)
{
  try {
    Factory.parse_resource ("tour.xml", "Test");
  } catch (...) {
    try {
      String file = String (path) + DIR_SEPARATOR + "tour.xml";
      Factory.parse_resource (file.c_str(), "Test");
    } catch (...) {
      String file = String (path) + DIR_SEPARATOR + ".." + DIR_SEPARATOR + "tour.xml";
      Factory.parse_resource (file.c_str(), "Test", nothrow);
    }
  }

  /* create root item */
  Item &item = Factory.create_gadget ("root");
  Root &root = item.interface<Root&>();
  root.ref_sink();

  /* create dialog */
  Item &dialog = Factory.create_gadget ("tour-dialog");
  root.add (dialog);

  /* complete gtk window */
  GtkWidget *rwidget = Gtk::root_widget_from_root (root);
  gtk_container_add (GTK_CONTAINER (window), rwidget);
}

extern "C" int
main (int   argc,
      char *argv[])
{
  printf ("EXAMPLE: %s:\n", basename (argv[0]));
  rapicorn_gettext_init ("Rapicorn", NULL); // FIXME: gettext path
  gtk_init (&argc, &argv);

  GtkWidget *window = gtk_widget_new (GTK_TYPE_WINDOW, NULL);
  gtk_window_set_default_size (GTK_WINDOW (window), 640, 480);
  Gtk::gtk_window_set_min_size (GTK_WINDOW (window), 20, 20);
  g_signal_connect (window, "delete-event", G_CALLBACK (gtk_widget_hide_on_delete), NULL);
  g_signal_connect (window, "hide", G_CALLBACK (gtk_main_quit), NULL);

  construct_gui (GTK_WINDOW (window), dirname (argv[0]).c_str());

  gtk_widget_show (window);

  gtk_main();
  gtk_widget_destroy (window);
  return 0;
}

} // anon
