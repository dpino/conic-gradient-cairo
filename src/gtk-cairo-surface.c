#include <stdio.h>
#include <stdint.h>
#include <gtk/gtk.h>

#include <math.h>

#define PAINT_GRADIENT 1

#define WINDOW_WIDTH  800
#define WINDOW_HEIGHT 800

static uint16_t window_width = WINDOW_WIDTH;
static uint16_t window_height = WINDOW_HEIGHT;

/* Surface to store current scribbles */
static cairo_surface_t *surface = NULL;

static void activate (GApplication *app, gpointer user_data);
static void close_window (void);
static gboolean draw_cb (GtkWidget *widget, cairo_t *cr, gpointer data);
static gboolean configure_event_cb (GtkWidget *widget, GdkEventConfigure *event, gpointer data);
static void clear_surface (void);

int main(int argc, char *argv[])
{
  GtkApplication *app;
  int status;

  app = gtk_application_new("org.gnome.example", G_APPLICATION_FLAGS_NONE);
  g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
  status = g_application_run(G_APPLICATION(app), argc, argv);
  g_object_unref(app);

  return 0;
}

static void activate (GApplication *app,
                      gpointer      user_data)
{
  GtkWidget *window;
  GtkWidget *frame;
  GtkWidget *drawing_area;
  const uint16_t w = window_width, h = window_height;

  window = gtk_application_window_new (GTK_APPLICATION (app));
  gtk_window_set_title (GTK_WINDOW (window), "Drawing Area");
  gtk_window_set_default_size (GTK_WINDOW (window), w, h);

  g_signal_connect (window, "destroy", G_CALLBACK (close_window), NULL);

  gtk_container_set_border_width (GTK_CONTAINER (window), 8);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_container_add (GTK_CONTAINER (window), frame);

  drawing_area = gtk_drawing_area_new ();

  /* Set a minimum size. */
  gtk_widget_set_size_request (drawing_area, w, h);

  gtk_container_add (GTK_CONTAINER (frame), drawing_area);

  /* Signals used to handle the backing surface */
  g_signal_connect (drawing_area, "draw",
                    G_CALLBACK (draw_cb), NULL);
  g_signal_connect (drawing_area,"configure-event",
                    G_CALLBACK (configure_event_cb), NULL);

  gtk_widget_set_events (drawing_area, gtk_widget_get_events (drawing_area));

  gtk_widget_show_all (window);
}

static void
close_window (void)
{
  if (surface)
    cairo_surface_destroy(surface);
}

static void
paint_rectangle (cairo_t *cr, double w, double h)
{
  cairo_set_line_width (cr, 0.5);
  cairo_move_to (cr, 0, 0);
  cairo_line_to (cr, w, 0);
  cairo_line_to (cr, w, h);
  cairo_line_to (cr, 0, h);
  cairo_stroke(cr);
}

/* Redraw the screen from the surface. Note that the ::draw
 * signal receives a ready-to-be-used cairo_t that is already
 * clipped to only draw the exposed areas of the widget
 */
static gboolean
draw_cb (GtkWidget *widget,
         cairo_t   *cr,
         gpointer   data)
{
  const uint16_t w = window_width, h = window_height;

  paint_rectangle (cr, w, h);
  cairo_stroke(cr);

  return FALSE;
}


/* Create a new surface of the appropriate size to store our scribbles */
static gboolean
configure_event_cb (GtkWidget         *widget,
                    GdkEventConfigure *event,
                    gpointer           data)
{
  if (surface)
    cairo_surface_destroy (surface);

  surface = gdk_window_create_similar_surface (gtk_widget_get_window (widget),
                                               CAIRO_CONTENT_COLOR,
                                               gtk_widget_get_allocated_width (widget),
                                               gtk_widget_get_allocated_height (widget));

  /* Initialize the surface to white */
  clear_surface ();

  /* We've handled the configure event, no need for further processing. */
  return TRUE;
}

static void
clear_surface (void)
{
  cairo_t *cr;

  cr = cairo_create (surface);

  cairo_set_source_rgb (cr, 1, 1, 1);
  cairo_paint (cr);

  cairo_destroy (cr);
}
