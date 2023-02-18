#include <stdio.h>
#include <gtk/gtk.h>

#include <math.h>

#define PAINT_GRADIENT 1

typedef struct color_t {
  short unsigned int r, g, b, a;
} color_t;

typedef struct point_t {
    double x, y;
} point_t;

static double rad(double deg) {
	return deg * (M_PI / 180);
}

color_t aqua    = {r: 100, g: 255, b: 255, a: 255};
color_t black   = {r: 0,   g: 0,   b: 0, a: 255};
color_t blue    = {r: 0,   g: 0,   b: 255, a: 255};
color_t gold    = {r: 255, g: 215, b: 0, a: 255};
color_t orange  = {r: 255, g: 128, b: 0, a: 255};
color_t green   = {r: 0,   g: 128, b: 0, a: 255};
color_t lime    = {r: 0,   g: 255, b: 0, a: 255};
color_t magenta = {r: 255, g: 0,   b: 255, a: 255};
color_t middle  = {r: 100, g: 40,  b: 20, a: 255};
color_t pink    = {r: 255, g: 20,  b: 90, a: 255};
color_t red     = {r: 255, g: 0,   b: 0, a: 255};
color_t yellow  = {r: 255, g: 255, b: 0, a: 255};

static color_t
interpolate (color_t from, color_t to)
{
  color_t ret = {
    r: from.r + (to.r - from.r) * 0.5,
    g: from.g + (to.g - from.g) * 0.5,
    b: from.b + (to.b - from.b) * 0.5,
    a: from.a + (to.a - from.a) * 0.5,
  };
  return ret;
}

/* Surface to store current scribbles */
static cairo_surface_t *surface = NULL;

static void activate (GApplication *app, gpointer user_data);
static void close_window (void);
static gboolean draw_cb (GtkWidget *widget, cairo_t *cr, gpointer data);
static gboolean configure_event_cb (GtkWidget *widget, GdkEventConfigure *event, gpointer data);
static void clear_surface (void);

static void conic_sector (cairo_pattern_t *pattern, double cx, double cy, double size, 
						  double degStart, double degEnd, color_t from, color_t to);
static void conic_gradient(cairo_t *cr, int width, int height, const color_t *colorStops, size_t nColorStops);
static void create_gouraud_gradient(cairo_pattern_t *pattern);
static void create_coons_gradient(cairo_pattern_t *pattern);
static void create_conic_sector_gradient(cairo_pattern_t *pattern);
static void create_conic_sector_gradient2(cairo_pattern_t *pattern);

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
  int w = 800, h = 800;

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

static void mesh_pattern_set_corner_color_rgb (cairo_pattern_t *pattern, int id, color_t c)
{
    double r, g, b, a;

    r = c.r / 255.0f;
    g = c.g / 255.0f;
    b = c.b / 255.0f;
    a = c.a / 255.0f;
    // printf("%.2f, %.2f, %.2f, %.2f\n", r, g, b, a);
    cairo_mesh_pattern_set_corner_color_rgba (pattern, id, r, g, b, 1.0);
}

static void print_point(point_t p)
{
	printf("x: %.4f; y: %.4f\n", p.x, p.y);
}

static void
paint_surface (cairo_t *cr, double w, double h)
{
  	cairo_move_to (cr, 0, 0);
  	cairo_line_to (cr, w, 0);
    cairo_line_to (cr, w, h);
    cairo_line_to (cr, 0, h);
    cairo_fill (cr);
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
	int w = 800, h = 800;

	// color_t colorStops[] = { red, yellow, lime, aqua, blue, magenta, red };
	// color_t colorStops[] = { red, gold, lime, black };
	color_t colorStops[] = { green,black,orange,red};

    cairo_pattern_t * pattern = cairo_pattern_create_mesh ();
	create_conic_sector_gradient(pattern);
	// create_coons_gradient(pattern);
	// create_gouraud_gradient(pattern);

    cairo_set_source(cr, pattern);

	paint_surface (cr, w, h);
	cairo_stroke(cr);

	return FALSE;
}

static void conic_sector (cairo_pattern_t *pattern, double cx, double cy, double size, 
						  double degStart, double degEnd, color_t from, color_t to)
{
	const int degOffset = 90;
	double r = sqrt(2) * size / 2;
    degStart = rad(degStart - degOffset);
    degEnd = rad(degEnd - degOffset);

    double f = 4 * tan((degEnd - degStart) / 4) / 3;
    point_t p0 = {
		x: cx + (r * cos(degStart)),
        y: cy + (r * sin(degStart))
	};
    point_t p1 = {
    	x: cx + (r * cos(degStart)) - f * (r * sin(degStart)),
       	y: cy + (r * sin(degStart)) + f * (r * cos(degStart))
	};
    point_t p2 = {
      	x: cx + (r * cos(degEnd)) + f * (r * sin(degEnd)),
    	y: cy + (r * sin(degEnd)) - f * (r * cos(degEnd))
	};
    point_t p3 = {
		x: cx + (r * cos(degEnd)),
        y: cy + (r * sin(degEnd))
	};

    cairo_mesh_pattern_begin_patch (pattern);

    cairo_mesh_pattern_move_to (pattern, cx, cy);
    cairo_mesh_pattern_line_to (pattern, p0.x, p0.y);
    cairo_mesh_pattern_curve_to (pattern, p1.x, p1.y, p2.x, p2.y, p3.x, p3.y);
    cairo_mesh_pattern_line_to (pattern, cx, cy);

    mesh_pattern_set_corner_color_rgb (pattern, 0, from);
    mesh_pattern_set_corner_color_rgb (pattern, 1, from);
    mesh_pattern_set_corner_color_rgb (pattern, 2, to);
    mesh_pattern_set_corner_color_rgb (pattern, 3, to);

    cairo_mesh_pattern_end_patch (pattern);
}

static void conic_gradient(cairo_t *cr, int width, int height, const color_t *colorStops, size_t nColorStops)
{
    cairo_pattern_t * pattern = cairo_pattern_create_mesh ();

	color_t from, to;
	double cx = width / 2, cy = height / 2;
	double size = fmax(width, height);

	if (nColorStops == 2) {
		from = colorStops[0], to = colorStops[1];
    	color_t middle = interpolate(from, to);
		conic_sector (pattern, cx, cy, size, 0, 180, from, middle);
		conic_sector (pattern, cx, cy, size, 180, 360, middle, to);
	} else {
		double degStride = 360 / (nColorStops - 1);
		degStride = 90;
		double degStart = 0, degEnd = degStart + degStride;
		double r = 1 / 4;
		double c = 0.551915024494;
		for (size_t i = 0; i < nColorStops; i++) {
			from = colorStops[i], to = colorStops[i + 1];
            double f = 4 * tan((degEnd - degStart) / 4) / 3;
            // printf("f: %.4f\n", f);
			double xo = cx + (r * cos(degStart)) + c * (r * sin(degStart));
    	    double yo = cy + (r * sin(degStart)) - c * (r * cos(degStart));
			conic_sector (pattern, xo, yo, size, degStart, degEnd, from, from);
			degStart = degEnd, degEnd = degStart + degStride;
		}
	}
    cairo_set_source(cr, pattern);
}

static void
create_gouraud_gradient(cairo_pattern_t *pattern)
{
    cairo_mesh_pattern_begin_patch (pattern);

    cairo_mesh_pattern_move_to (pattern, 0, 0);
    cairo_mesh_pattern_line_to (pattern, 200, 200);
    cairo_mesh_pattern_line_to (pattern, 0, 200);

    cairo_mesh_pattern_set_corner_color_rgb (pattern, 0, 1, 0, 0); // red
    cairo_mesh_pattern_set_corner_color_rgb (pattern, 1, 0, 1, 0); // green
    cairo_mesh_pattern_set_corner_color_rgb (pattern, 2, 0, 0, 1); // blue

    cairo_mesh_pattern_end_patch (pattern);
}

static void
create_coons_gradient(cairo_pattern_t *pattern)
{
	cairo_mesh_pattern_begin_patch (pattern);

	cairo_mesh_pattern_move_to (pattern, 45, 12);

	cairo_mesh_pattern_curve_to(pattern, 69, 24, 173, -15, 115, 50);
	cairo_mesh_pattern_curve_to(pattern, 127, 66, 174, 47, 148, 104);
	cairo_mesh_pattern_curve_to(pattern, 65, 58, 70, 69, 18, 103);
	cairo_mesh_pattern_curve_to(pattern, 42, 43, 63, 45, 45, 12);

    cairo_mesh_pattern_set_corner_color_rgb (pattern, 0, 1, 0, 0); // red
    cairo_mesh_pattern_set_corner_color_rgb (pattern, 1, 0, 1, 0); // green
    cairo_mesh_pattern_set_corner_color_rgb (pattern, 2, 0, 0, 1); // blue
    cairo_mesh_pattern_set_corner_color_rgb (pattern, 3, 1, 1, 0); // yellow

	cairo_mesh_pattern_end_patch (pattern);
}

static void
create_conic_sector_gradient(cairo_pattern_t *pattern)
{
    cairo_mesh_pattern_begin_patch(pattern);

    cairo_mesh_pattern_move_to(pattern, 0, 200);
    cairo_mesh_pattern_line_to(pattern, 0, 0);
    cairo_mesh_pattern_curve_to(pattern, 110.4, 0, 200, 110.4, 200, 200);

    cairo_mesh_pattern_set_corner_color_rgb (pattern, 0, 1, 0, 0); // red
    cairo_mesh_pattern_set_corner_color_rgb (pattern, 1, 0, 1, 0); // green
    cairo_mesh_pattern_set_corner_color_rgb (pattern, 2, 0, 0, 1); // blue
    cairo_mesh_pattern_set_corner_color_rgb (pattern, 3, 1, 1, 0); // yellow

    cairo_mesh_pattern_end_patch(pattern);
}

static void
create_conic_sector_gradient2(cairo_pattern_t *pattern)
{
    cairo_mesh_pattern_begin_patch(pattern);

    cairo_mesh_pattern_move_to(pattern, 0, 200);
    cairo_mesh_pattern_line_to(pattern, 0, 0);
    cairo_mesh_pattern_curve_to(pattern, 110.4, 0, 200, 110.4, 200, 200);

    cairo_mesh_pattern_set_corner_color_rgb (pattern, 0, 1, 0, 0); // red
    cairo_mesh_pattern_set_corner_color_rgb (pattern, 1, 1, 0, 0); // red
    cairo_mesh_pattern_set_corner_color_rgb (pattern, 2, 1, 1, 0); // yellow
    cairo_mesh_pattern_set_corner_color_rgb (pattern, 3, 1, 1, 0); // yellow

    cairo_mesh_pattern_end_patch(pattern);
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
