#include "bar.h"

void on_callendar_button_clicked(GtkGestureClick *gesture, int n_press,
								 double x, double y, gpointer user_data) {
	GtkWidget *box = GTK_WIDGET(user_data);
	calendar_popup_toggle(box);
}

void on_ctrl_box_clicked(GtkGestureClick *gesture, int n_press, double x,
						 double y, gpointer user_data) {
	GtkWidget *box = GTK_WIDGET(user_data);
	ctrl_center_popup_toggle(box);
}

int bar(int *argc, char *(*argv[]), GtkWidget *window, GdkMonitor *monitor) {

	GtkCssProvider *provider = gtk_css_provider_new();
	typedef struct {
		char *css_path;
	} sargs;
	sargs args;
	args.css_path = strdup("/usr/share/gtkbar/style.css");
	if (*argc > 1) {
		for (int i = 0; i < *argc; i++) {
			if (strcmp((*argv)[i], "--css-path") == 0) {
				if ((*argv)[i + 1]) {
					args.css_path = (*argv)[i + 1];
					g_message("CSS Path: %s", args.css_path);
					i++;
				}
			}
		}
	}
	gtk_style_context_add_provider_for_display(
		gdk_display_get_default(), GTK_STYLE_PROVIDER(provider),
		GTK_STYLE_PROVIDER_PRIORITY_USER);

	gtk_css_provider_load_from_path(provider, args.css_path);

	gtk_layer_set_keyboard_mode(GTK_WINDOW(window),
								GTK_LAYER_SHELL_KEYBOARD_MODE_ON_DEMAND);

	GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
	gtk_widget_add_css_class(window, "window");
	gtk_widget_set_hexpand(box, TRUE);
	GtkWidget *workspace_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	GtkWidget *right_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	GtkWidget *center_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 15);
	gtk_widget_set_vexpand(workspace_box, FALSE);
	gtk_widget_set_hexpand(workspace_box, FALSE);
	gtk_widget_add_css_class(workspace_box, "workspace-box");
	gtk_widget_set_hexpand(right_box, FALSE);
	gtk_widget_set_hexpand(center_box, TRUE);
	gtk_widget_set_halign(center_box, GTK_ALIGN_CENTER);
	gtk_widget_set_halign(right_box, GTK_ALIGN_END);

	GtkWidget *controllCenterBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
	gtk_widget_add_css_class(controllCenterBox, "widget");

	GtkWidget *wifiBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
	GtkWidget *wifiIcon = gtk_image_new_from_icon_name(
		"network-wireless-signal-excellent-symbolic");
	gtk_box_append(GTK_BOX(wifiBox), wifiIcon);

	GtkWidget *volumeBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
	GtkWidget *volumeIcon =
		gtk_image_new_from_icon_name("audio-volume-high-symbolic");
	int volume_precentage = get_battery_percentage();
	char volume_label_text[32];
	snprintf(volume_label_text, sizeof(volume_label_text), "%d%%",
			 volume_precentage);
	GtkWidget *volumeLabel = gtk_label_new(volume_label_text);

	gtk_box_append(GTK_BOX(volumeBox), volumeIcon);
	gtk_box_append(GTK_BOX(volumeBox), volumeLabel);
	gtk_box_append(GTK_BOX(controllCenterBox), wifiBox);
	gtk_box_append(GTK_BOX(controllCenterBox), volumeBox);
	gtk_box_append(GTK_BOX(right_box), controllCenterBox);

	int battery_precentage = get_battery_percentage();
	if (battery_precentage > -1) {
		GtkWidget *batteryBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
		GtkWidget *batteryIcon =
			gtk_image_new_from_icon_name("battery-symbolic");
		char label_text[32];
		snprintf(label_text, sizeof(label_text), "%d%%", battery_precentage);
		GtkWidget *batteryLabel = gtk_label_new(label_text);

		gtk_box_append(GTK_BOX(batteryBox), batteryIcon);
		gtk_box_append(GTK_BOX(batteryBox), batteryLabel);

		gtk_box_append(GTK_BOX(right_box), batteryBox);
		g_timeout_add(1000, battery_refresh_wrapper, batteryLabel);
	}

	time_t rawtime;
	struct tm *timeinfo;

	time(&rawtime);					// Get current time in seconds since epoch
	timeinfo = localtime(&rawtime); // Convert to local time struct
	char buffer[64];
	strftime(buffer, sizeof(buffer), "%a %b %d  %I:%M:%S %p", timeinfo);

	GtkWidget *timeBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
	gtk_widget_set_focusable(timeBox, TRUE);
	gtk_widget_add_css_class(timeBox, "widget");
	GtkWidget *time_label = gtk_label_new(buffer);
	gtk_label_set_width_chars(GTK_LABEL(time_label), 18);
	gtk_label_set_xalign(GTK_LABEL(time_label), 0.5);

	GtkGesture *timeBoxClick = gtk_gesture_click_new();
	gtk_widget_add_controller(timeBox, GTK_EVENT_CONTROLLER(timeBoxClick));
	g_signal_connect(timeBoxClick, "released",
					 G_CALLBACK(on_callendar_button_clicked), timeBox);

	GtkGesture *controllCenterClick = gtk_gesture_click_new();
	gtk_widget_add_controller(controllCenterBox,
							  GTK_EVENT_CONTROLLER(controllCenterClick));
	g_signal_connect(controllCenterClick, "released",
					 G_CALLBACK(on_ctrl_box_clicked), controllCenterBox);
	gtk_box_append(GTK_BOX(timeBox), time_label);
	gtk_box_append(GTK_BOX(right_box), timeBox);

	gtk_widget_set_margin_top(box, 0);
	gtk_widget_set_margin_bottom(box, 0);
	gtk_widget_set_margin_start(box, 0);
	gtk_widget_set_margin_end(box, 0);

	WorkspaceState *workspace_state = malloc(sizeof(WorkspaceState));

	workspace_state->workspace_box = workspace_box;
	workspace_state->current_active_workspace = get_active_workspace_name();
	workspace_state->bar = strdup(gdk_monitor_get_connector(monitor));
	char *bar_str;
	bar_str = workspace_state->bar;
	g_message("Connector: %s", workspace_state->bar);

	workspace_state->current_workspace_names =
		get_hyprland_workspace_names(bar_str);
	workspace_state->current_workspace_count =
		get_hyprland_workspace_count(bar_str);

	refresh_workspaces_p(workspace_state);
	char *active_workspace = get_active_workspace_name();
	if (active_workspace) {
		set_active_workspace(workspace_box, active_workspace, workspace_state);
	}

	gtk_box_append(GTK_BOX(box), workspace_box);
	gtk_box_append(GTK_BOX(box), center_box);
	gtk_box_append(GTK_BOX(box), right_box);

	gtk_window_set_child(GTK_WINDOW(window), box);

	pthread_t event_thread;
	pthread_create(&event_thread, NULL, (void *)listen_for_workspace_events,
				   workspace_state);

	g_timeout_add(1000, time_refresh_wrapper, time_label);
	g_timeout_add(100, volume_refresh_wrapper, volumeLabel);
	g_timeout_add(100, check_volume_change, NULL);

	return 0;
}
