/* Rapicorn
 * Copyright (C) 2005-2006 Tim Janik
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
#ifndef __RAPICORN_GTK_ROOT_WIDGET_HH__
#define __RAPICORN_GTK_ROOT_WIDGET_HH__

#include <rapicorn/root.hh>

#if     RAPICORN_WITH_GTK
#include <gtk/gtkwindow.h>
namespace Rapicorn {
namespace Gtk {

/* --- type macros --- */
#define BIRNET_CANVAS_GTK_TYPE_ROOT_WIDGET              (root_widget_get_type ())
#define BIRNET_CANVAS_GTK_ROOT_WIDGET(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), BIRNET_CANVAS_GTK_TYPE_ROOT_WIDGET, RootWidget))
#define BIRNET_CANVAS_GTK_ROOT_WIDGET_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), BIRNET_CANVAS_GTK_TYPE_ROOT_WIDGET, RootWidgetClass))
#define BIRNET_CANVAS_GTK_IS_ROOT_WIDGET(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), BIRNET_CANVAS_GTK_TYPE_ROOT_WIDGET))
#define BIRNET_CANVAS_GTK_IS_ROOT_WIDGET_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), BIRNET_CANVAS_GTK_TYPE_ROOT_WIDGET))
#define BIRNET_CANVAS_GTK_ROOT_WIDGET_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BIRNET_CANVAS_GTK_TYPE_ROOT_WIDGET, RootWidgetClass))

/* --- RootWidget --- */
typedef struct {
  GtkContainer    container;
  Root           *root;
  double          last_x, last_y;
  GdkModifierType last_modifier;
} RootWidget;
typedef struct {
  GtkContainerClass container_class;
} RootWidgetClass;

GType           root_widget_get_type    (void);
GtkWidget*      root_widget_from_root   (Root &root);

/* --- Gdk utils --- */
void                    gtk_window_set_min_size    (GtkWindow *window, gint min_width, gint min_height);
void                    gdk_window_set_gravity     (GdkWindow *window, GdkGravity gravity);
void                    gdk_window_set_win_gravity (GdkWindow *window, GdkGravity gravity);
void                    gdk_window_set_bit_gravity (GdkWindow *window, GdkGravity gravity);
#ifndef GDK_GRAVITY_NONE
#define GDK_GRAVITY_NONE        (GdkGravity (0))
#endif

} // Gtk
} // Rapicorn
#endif  /* RAPICORN_WITH_GTK */

#endif  /* __RAPICORN_GTK_ROOT_WIDGET_HH__ */
