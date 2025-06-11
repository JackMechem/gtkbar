#include "calendarpopup.h"
#include "gtk/gtk.h"
#include "popupposition.h"

#include <graphene-gobject.h>
static GtkWindow *popup_window = NULL;
static const int POPUP_WIDTH = 360;
static const int POPUP_HEIGHT = 250;

void calendar_popup_toggle(GtkWidget *anchor) {
	if (!GTK_IS_WIDGET(anchor))
		return;

	if (popup_window) {
		gtk_window_destroy(popup_window);
		popup_window = NULL;
		return;
	}

	popup_window = GTK_WINDOW(gtk_window_new());
	gtk_window_set_decorated(popup_window, FALSE);
	gtk_window_set_resizable(popup_window, FALSE);
	gtk_window_set_default_size(popup_window, POPUP_WIDTH, POPUP_HEIGHT);

	gtk_layer_init_for_window(popup_window);
	gtk_layer_set_layer(popup_window, GTK_LAYER_SHELL_LAYER_OVERLAY);
	gtk_layer_auto_exclusive_zone_enable(popup_window);

	gtk_layer_set_anchor(popup_window, GTK_LAYER_SHELL_EDGE_TOP, TRUE);

	int pixels_from_right = get_distance(anchor, POPUP_WIDTH);

	if (pixels_from_right == -1) {
		gtk_layer_set_anchor(popup_window, GTK_LAYER_SHELL_EDGE_RIGHT, TRUE);
		gtk_layer_set_margin(popup_window, GTK_LAYER_SHELL_EDGE_RIGHT, 0);
	} else {
		gtk_layer_set_anchor(popup_window, GTK_LAYER_SHELL_EDGE_LEFT, TRUE);
		gtk_layer_set_margin(popup_window, GTK_LAYER_SHELL_EDGE_LEFT,
							 pixels_from_right);
	}

	GtkWidget *calendar = gtk_calendar_new();
	gtk_widget_add_css_class(GTK_WIDGET(popup_window), "popup-window");
	gtk_window_set_child(popup_window, calendar);

	gtk_window_present(popup_window);
}
