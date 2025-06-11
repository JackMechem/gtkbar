#include "popupposition.h"

int get_distance(GtkWidget *widget, int popupWidth) {
	if (!gtk_widget_get_realized(widget))
		return -1;

	GtkRoot *root = gtk_widget_get_root(widget);
	if (!GTK_IS_WIDGET(root))
		return -1;

	// Get widget width

	// Compute widget's top-left corner relative to the root
	graphene_point_t local = {0, 0};
	graphene_point_t root_pos;
	if (!gtk_widget_compute_point(widget, GTK_WIDGET(root), &local, &root_pos))
		return -1;

	// Find which monitor the root window is on
	GdkSurface *surface =
		gtk_native_get_surface(gtk_widget_get_native(GTK_WIDGET(root)));
	GdkDisplay *display = gdk_surface_get_display(surface);
	GdkMonitor *monitor = gdk_display_get_monitor_at_surface(display, surface);

	if (!monitor)
		return -1;

	// Get monitor width (only care about width, ignore x-offset!)
	GdkRectangle geometry;
	gdk_monitor_get_geometry(monitor, &geometry);

	int screen_right = geometry.width;
	int widget_left = root_pos.x;

	if (screen_right - widget_left - popupWidth < 0) {
		return -1;
	} else {
		return widget_left - 15;
	}
}
