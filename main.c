#include "app.h"
#include "calendarpopup.h"
#include "gtk/gtk.h"
#include "gtk/gtkcssprovider.h"
#include "volumepopup.h"
#include <string.h>

static void on_box_clicked(GtkGestureClick *gesture, int n_press, double x,
						   double y, gpointer user_data) {
	GtkWidget *box = GTK_WIDGET(user_data);
	calendar_popup_toggle(box);
}

int main(int argc, char *argv[]) {
	gtk_init();

	GtkWidget *window = gtk_window_new();

	gtk_window_set_title(GTK_WINDOW(window), "Overlay");
	// Get monitor geometry from the first monitor
	GdkDisplay *display = gdk_display_get_default();
	GListModel *monitors = gdk_display_get_monitors(display);
	GdkMonitor *monitor =
		GDK_MONITOR(g_list_model_get_item(monitors, 0)); // monitor #0

	GdkRectangle geometry;
	gdk_monitor_get_geometry(monitor, &geometry);
	g_object_unref(monitor); // free GObject ref

	// Set full width, auto height
	gtk_window_set_default_size(GTK_WINDOW(window), geometry.width, -1);
	gtk_window_set_decorated(GTK_WINDOW(window), FALSE);

	gtk_layer_init_for_window(GTK_WINDOW(window));
	gtk_layer_set_layer(GTK_WINDOW(window), GTK_LAYER_SHELL_LAYER_OVERLAY);
	gtk_layer_set_anchor(GTK_WINDOW(window), GTK_LAYER_SHELL_EDGE_TOP, TRUE);
	gtk_layer_set_anchor(GTK_WINDOW(window), GTK_LAYER_SHELL_EDGE_LEFT, TRUE);
	gtk_layer_set_anchor(GTK_WINDOW(window), GTK_LAYER_SHELL_EDGE_RIGHT, TRUE);

	gtk_layer_auto_exclusive_zone_enable(GTK_WINDOW(window));

	GtkCssProvider *provider = gtk_css_provider_new();
	typedef struct {
		char *css_path;
	} sargs;
	sargs args;
	args.css_path = "/usr/share/gtkbar/style.css";
	if (argc > 1) {
		for (int i = 0; i < argc; i++) {
			if (strcmp(argv[i], "--css-path")) {
				strcpy(args.css_path, argv[i + 1]);
				args.css_path = argv[i + 1];
				i++;
			}
		}
	}
	gtk_style_context_add_provider_for_display(
		gdk_display_get_default(), GTK_STYLE_PROVIDER(provider),
		GTK_STYLE_PROVIDER_PRIORITY_USER);

	char full_path[PATH_MAX];

	if (realpath(args.css_path, full_path) != NULL) {
		printf("Full path: %s\n", full_path);
		gtk_css_provider_load_from_path(provider, args.css_path);
	} else {
		perror("realpath");
	}
	GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
	gtk_widget_add_css_class(box, "window");
	gtk_widget_set_hexpand(box, TRUE);
	GtkWidget *workspace_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	GtkWidget *right_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	GtkWidget *center_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 15);
	gtk_widget_set_vexpand(workspace_box, FALSE);
	gtk_widget_set_hexpand(right_box, FALSE);
	gtk_widget_set_hexpand(center_box, TRUE);
	gtk_widget_set_halign(center_box, GTK_ALIGN_CENTER);
	gtk_widget_set_halign(right_box, GTK_ALIGN_END);

	GtkWidget *volumeBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
	gtk_widget_add_css_class(volumeBox, "widget");
	GtkWidget *volumeIcon =
		gtk_image_new_from_icon_name("audio-volume-high-symbolic");
	int volume_precentage = get_battery_percentage();
	char volume_label_text[32];
	snprintf(volume_label_text, sizeof(volume_label_text), "%d%%",
			 volume_precentage);
	GtkWidget *volumeLabel = gtk_label_new(volume_label_text);

	gtk_box_append(GTK_BOX(volumeBox), volumeIcon);
	gtk_box_append(GTK_BOX(volumeBox), volumeLabel);
	gtk_box_append(GTK_BOX(right_box), volumeBox);

	GtkWidget *wifiBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
	GtkWidget *wifiIcon = gtk_image_new_from_icon_name(
		"network-wireless-signal-excellent-symbolic");
	GtkWidget *wifiLabel = gtk_label_new(get_wifi_name());
	gtk_widget_add_css_class(wifiBox, "widget");
	gtk_box_append(GTK_BOX(wifiBox), wifiIcon);
	gtk_box_append(GTK_BOX(wifiBox), wifiLabel);

	gtk_box_append(GTK_BOX(right_box), wifiBox);

	GtkWidget *batteryBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
	gtk_widget_add_css_class(batteryBox, "widget");
	GtkWidget *batteryIcon = gtk_image_new_from_icon_name("battery-symbolic");
	int battery_precentage = get_battery_percentage();
	char label_text[32];
	snprintf(label_text, sizeof(label_text), "%d%%", battery_precentage);
	GtkWidget *batteryLabel = gtk_label_new(label_text);

	gtk_box_append(GTK_BOX(batteryBox), batteryIcon);
	gtk_box_append(GTK_BOX(batteryBox), batteryLabel);

	gtk_box_append(GTK_BOX(right_box), batteryBox);

	time_t rawtime;
	struct tm *timeinfo;

	time(&rawtime);					// Get current time in seconds since epoch
	timeinfo = localtime(&rawtime); // Convert to local time struct
	char buffer[64];
	strftime(buffer, sizeof(buffer), "%a %b %d  %I:%M:%S %p", timeinfo);

	GtkWidget *timeBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
	gtk_widget_set_focusable(timeBox, TRUE);
	gtk_widget_add_css_class(timeBox, "widget");
	GtkWidget *calIcon = gtk_image_new_from_icon_name("clock-symbolic");
	GtkWidget *time_label = gtk_label_new(buffer);

	// Add gesture controller

	GtkGesture *click = gtk_gesture_click_new();
	gtk_widget_add_controller(timeBox, GTK_EVENT_CONTROLLER(click));

	g_signal_connect(click, "released", G_CALLBACK(on_box_clicked), timeBox);

	gtk_box_append(GTK_BOX(timeBox), calIcon);
	gtk_box_append(GTK_BOX(timeBox), time_label);
	gtk_box_append(GTK_BOX(right_box), timeBox);

	gtk_widget_set_margin_top(box, 0);
	gtk_widget_set_margin_bottom(box, 0);
	gtk_widget_set_margin_start(box, 20);
	gtk_widget_set_margin_end(box, 20);

	refresh_workspaces(workspace_box);
	char *active_workspace = get_active_workspace_name();
	if (active_workspace) {
		set_active_workspace(workspace_box, active_workspace);
	}

	gtk_box_append(GTK_BOX(box), workspace_box);
	gtk_box_append(GTK_BOX(box), center_box);
	gtk_box_append(GTK_BOX(box), right_box);

	gtk_window_set_child(GTK_WINDOW(window), box);

	gtk_window_present(GTK_WINDOW(window)); // Show the window properly

	g_timeout_add(100, workspace_refresh_wrapper, workspace_box);
	g_timeout_add(1000, time_refresh_wrapper, time_label);
	g_timeout_add(1000, wifi_refresh_wrapper, wifiLabel);
	g_timeout_add(1000, battery_refresh_wrapper, batteryLabel);
	g_timeout_add(100, volume_refresh_wrapper, volumeLabel);
	g_timeout_add(100, check_volume_change, NULL);

	GMainLoop *loop = g_main_loop_new(NULL, FALSE);
	g_main_loop_run(loop);

	return 0;
}
