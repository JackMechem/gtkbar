#include "volumepopup.h"
#include "app.h"
#include "glib.h"
#include "gtk/gtk.h"
#include "gtk/gtkshortcut.h"
#include <stdio.h>
#include <stdlib.h>

static GtkWidget *popup = NULL;
static GtkWidget *level_bar = NULL;
static guint timeout_id = 0;
static GtkWidget *volumeIcon;
static GtkWidget *volumeLabel;

gboolean hide_popup(gpointer data) {
	gtk_widget_hide(popup);
	timeout_id = 0;
	return G_SOURCE_REMOVE;
}

GtkWidget *create_volume_overlay() {
	popup = gtk_window_new();
	gtk_window_set_decorated(GTK_WINDOW(popup), FALSE);
	gtk_window_set_resizable(GTK_WINDOW(popup), FALSE);
	gtk_window_set_title(GTK_WINDOW(popup), "Volume Overlay");
	gtk_window_set_default_size(GTK_WINDOW(popup), -1, -1);

	gtk_layer_init_for_window(GTK_WINDOW(popup));
	gtk_layer_set_layer(GTK_WINDOW(popup), GTK_LAYER_SHELL_LAYER_OVERLAY);
	gtk_layer_set_anchor(GTK_WINDOW(popup), GTK_LAYER_SHELL_EDGE_TOP, TRUE);

	GtkCssProvider *provider = gtk_css_provider_new();
	gtk_css_provider_load_from_path(provider, "../style.css");
	gtk_style_context_add_provider_for_display(
		gdk_display_get_default(), GTK_STYLE_PROVIDER(provider),
		GTK_STYLE_PROVIDER_PRIORITY_USER);

	gtk_widget_add_css_class(GTK_WIDGET(popup), "volume-window");

	gtk_window_set_hide_on_close(GTK_WINDOW(popup), TRUE);
	gtk_window_set_modal(GTK_WINDOW(popup), FALSE);
	gtk_window_present(GTK_WINDOW(popup));

	GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
	gtk_window_set_child(GTK_WINDOW(popup), box);
	gtk_widget_add_css_class(box, "volume-popup");
	int volume = get_volume_percentage();

	volumeIcon = gtk_image_new();
	if (volume == 0) {
		gtk_widget_remove_css_class(GTK_WIDGET(volumeIcon), "volume-overboost");
		gtk_image_set_from_icon_name(GTK_IMAGE(volumeIcon),
									 "audio-volume-muted-symbolic");
	} else if (volume < 30) {

		gtk_widget_remove_css_class(GTK_WIDGET(volumeIcon), "volume-overboost");
		gtk_image_set_from_icon_name(GTK_IMAGE(volumeIcon),
									 "audio-volume-low-symbolic");
	} else if (volume < 70) {
		gtk_widget_remove_css_class(GTK_WIDGET(volumeIcon), "volume-overboost");
		gtk_image_set_from_icon_name(GTK_IMAGE(volumeIcon),
									 "audio-volume-medium-symbolic");
	} else if (volume > 100) {
		gtk_image_set_from_icon_name(GTK_IMAGE(volumeIcon),
									 "audio-volume-high-symbolic");
		gtk_widget_add_css_class(GTK_WIDGET(volumeIcon), "volume-overboost");
	} else {
		gtk_widget_remove_css_class(GTK_WIDGET(volumeIcon), "volume-overboost");
		gtk_image_set_from_icon_name(GTK_IMAGE(volumeIcon),
									 "audio-volume-high-symbolic");
	}
	gtk_box_append(GTK_BOX(box), volumeIcon);

	level_bar = gtk_level_bar_new();
	gtk_level_bar_set_min_value(GTK_LEVEL_BAR(level_bar), 0);
	gtk_level_bar_set_max_value(GTK_LEVEL_BAR(level_bar), 100);
	if (volume <= 100) {
		gtk_level_bar_set_value(GTK_LEVEL_BAR(level_bar), volume);
	} else {
		gtk_level_bar_set_value(GTK_LEVEL_BAR(level_bar), volume - 100);
	}
	gtk_widget_set_size_request(level_bar, 160, 10);
	gtk_box_append(GTK_BOX(box), level_bar);
	gtk_widget_add_css_class(level_bar, "volume-popup-level-bar");

	char *volume_str = malloc(8);
	snprintf(volume_str, sizeof(volume_str), "%d%%", volume);
	volumeLabel = gtk_label_new(volume_str);
	gtk_box_append(GTK_BOX(box), volumeLabel);
	free(volume_str);

	return GTK_WIDGET(popup);
}
void show_volume_popup(int volume) {
	if (!popup) {
		GtkWidget *popup = create_volume_overlay();
		gtk_widget_set_visible(popup, TRUE);
	}

	// Update the volume bar
	if (volume <= 100) {
		gtk_level_bar_set_value(GTK_LEVEL_BAR(level_bar), volume);
	} else {
		gtk_level_bar_set_value(GTK_LEVEL_BAR(level_bar), volume - 100);
	}

	if (volume == 0) {
		gtk_widget_remove_css_class(GTK_WIDGET(volumeIcon), "volume-overboost");
		gtk_image_set_from_icon_name(GTK_IMAGE(volumeIcon),
									 "audio-volume-muted-symbolic");
	} else if (volume < 30) {

		gtk_widget_remove_css_class(GTK_WIDGET(volumeIcon), "volume-overboost");
		gtk_image_set_from_icon_name(GTK_IMAGE(volumeIcon),
									 "audio-volume-low-symbolic");
	} else if (volume < 70) {
		gtk_widget_remove_css_class(GTK_WIDGET(volumeIcon), "volume-overboost");
		gtk_image_set_from_icon_name(GTK_IMAGE(volumeIcon),
									 "audio-volume-medium-symbolic");
	} else if (volume > 100) {
		gtk_image_set_from_icon_name(GTK_IMAGE(volumeIcon),
									 "audio-volume-high-symbolic");
		gtk_widget_add_css_class(GTK_WIDGET(volumeIcon), "volume-overboost");
	} else {
		gtk_widget_remove_css_class(GTK_WIDGET(volumeIcon), "volume-overboost");
		gtk_image_set_from_icon_name(GTK_IMAGE(volumeIcon),
									 "audio-volume-high-symbolic");
	}
	gtk_widget_set_visible(popup, TRUE);

	char *volume_str = malloc(8);
	snprintf(volume_str, sizeof(volume_str), "%d%%", volume);
	gtk_label_set_text(GTK_LABEL(volumeLabel), volume_str);
	free(volume_str);

	// Reset timer
	if (timeout_id > 0)
		g_source_remove(timeout_id);
	timeout_id = g_timeout_add(1500, hide_popup, NULL);
}

gboolean check_volume_change(gpointer user_data) {
	static int last_volume = -1;
	int current = get_volume_percentage();

	if (current != last_volume && current >= 0) {
		show_volume_popup(current);
		last_volume = current;
	}

	return TRUE; // keep checking
}
