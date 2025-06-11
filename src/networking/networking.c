#include "networking.h"
#include "gio/gio.h"
#include "glib.h"
#include "gtk/gtk.h"
#include "gtk/gtkshortcut.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

NMDeviceWifi *deviceInstance;

typedef struct {
	char *ssid;
	GtkWidget *wifiMenuBox;
} ConnectArgs;

gpointer connect_thread_func(gpointer user_data) {
	ConnectArgs *args = user_data;
	connect_to_wifi_via_nmcli(args->ssid, passwordText);
	g_idle_add((GSourceFunc)refresh_wifi_list, args->wifiMenuBox);
	return NULL;
}

void on_connect_start(GtkGestureClick *gesture, int n_press, double x, double y,
					  gpointer user_data) {
	OnConnectStartArgs *args = user_data;

	GtkWidget *networkBox = args->networkBox;
	if (args->menuOpen == TRUE) {
		gtk_widget_unparent(gtk_widget_get_last_child(networkBox));
		args->menuOpen = FALSE;
		return;
	}
	args->menuOpen = TRUE;
	if (is_saved_network(strdup(args->ssid))) {
		ConnectArgs *connectArgs = malloc(sizeof(ConnectArgs));
		connectArgs->ssid = strdup(args->ssid);
		connectArgs->wifiMenuBox = gtk_widget_get_parent(networkBox);

		GtkWidget *box = gtk_widget_get_first_child(networkBox);

		GtkWidget *spinner = gtk_spinner_new();
		gtk_widget_unparent(gtk_widget_get_last_child(box));
		gtk_box_append(GTK_BOX(box), spinner);
		gtk_spinner_start(GTK_SPINNER(spinner));

		GThread *thr =
			g_thread_new("connect-thread", connect_thread_func, connectArgs);
		g_thread_unref(thr);
		return;
	}

	GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
	GtkWidget *entryBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	GtkWidget *entryLabel = gtk_label_new("Password:");
	GtkWidget *entry = gtk_entry_new();

	gtk_widget_set_hexpand(entry, TRUE);
	gtk_widget_set_hexpand(entryBox, TRUE);
	gtk_editable_set_editable(GTK_EDITABLE(entry), TRUE);
	gtk_label_set_xalign(GTK_LABEL(entryLabel), 0.0);
	gtk_widget_set_focusable(entry, TRUE);
	GtkWidget *button =
		gtk_button_new_from_icon_name("network-transmit-receive-symbolic");

	// Add entry and button to box
	gtk_box_append(GTK_BOX(entryBox), entryLabel);
	gtk_box_append(GTK_BOX(entryBox), entry);
	gtk_box_append(GTK_BOX(box), entryBox);
	gtk_box_append(GTK_BOX(box), button);

	gtk_box_append(GTK_BOX(networkBox), box);

	// Text change signal
	g_signal_connect(entry, "changed", G_CALLBACK(on_entry_changed), NULL);

	// Button click signal
	g_signal_connect(button, "clicked", G_CALLBACK(on_connect_button_clicked),
					 strdup(args->ssid));
}

void on_connect_button_clicked(GtkButton *button, gpointer user_data) {
	char *ssid = user_data;
	GtkWidget *wifiMenuBox = gtk_widget_get_parent(
		gtk_widget_get_parent(gtk_widget_get_parent(GTK_WIDGET(button))));

	GtkWidget *spinner = gtk_spinner_new();
	gtk_button_set_child(GTK_BUTTON(button), spinner);
	gtk_spinner_start(GTK_SPINNER(spinner));

	ConnectArgs *connectArgs = malloc(sizeof(ConnectArgs));

	connectArgs->ssid = strdup(ssid);
	connectArgs->wifiMenuBox = wifiMenuBox;

	GThread *thr =
		g_thread_new("connect-thread", connect_thread_func, connectArgs);
	g_thread_unref(thr);
}

gboolean forget_connection_by_ssid(const char *ssid) {

	if (!ssid || !*ssid) {
		fprintf(stderr, "connect_to_wifi_via_nmcli: SSID must not be empty\n");
		return -EINVAL;
	}

	char cmd[512];
	snprintf(cmd, sizeof(cmd), "nmcli connection delete '%s'", ssid);

	// popen so we can capture & print what nmcli says
	FILE *fp = popen(cmd, "r");
	if (!fp) {
		perror("popen");
		return -errno;
	}

	char buf[256];
	while (fgets(buf, sizeof(buf), fp)) {
		fputs(buf, stdout);
	}

	int rc = pclose(fp);
	if (rc != 0) {
		// nmcli returns non-zero on failure
		fprintf(stderr, "nmcli exited with %d\n", rc);
		return -EIO;
	}

	// success!
	return 0;
}

gpointer forget_thr_func(gpointer user_data) {

	DisconnectForgetProps *args = user_data;

	// If you need to update the UI, schedule it on the main loop:
	forget_connection_by_ssid(strdup(args->ssid_str));
	g_idle_add((GSourceFunc)refresh_wifi_list, args->wifiMenu);

	return NULL; // or return some result pointer
}

void on_forget_pressed(GtkGestureClick *gesture, int n_press, double x,
					   double y, gpointer user_data) {
	DisconnectForgetProps *props = user_data;

	GtkWidget *box =
		gtk_event_controller_get_widget(GTK_EVENT_CONTROLLER(gesture));
	GtkWidget *spinner = gtk_spinner_new();
	gtk_widget_unparent(gtk_widget_get_first_child(box));
	gtk_box_append(GTK_BOX(box), spinner);
	gtk_spinner_start(GTK_SPINNER(spinner));

	GThread *thr = g_thread_new("forget-thread", forget_thr_func, props);
	g_thread_unref(thr);
}

void new_access_point_cb(NMDeviceWifi *device, NMAccessPoint *ap,
						 gpointer user_data) {
	deviceInstance = device;
	if (!user_data)
		return;
	AccessPointCbArgs *accessPointCbArgs = user_data;
	if (!accessPointCbArgs->box) {
		return;
	}
	GtkBox *box = accessPointCbArgs->box;
	gsize len;
	GBytes *ssid_bytes = nm_access_point_get_ssid(ap);
	if (!ssid_bytes) {
		return;
	}
	const guint8 *ssid_data = g_bytes_get_data(ssid_bytes, &len);
	char *ssid_str = nm_utils_ssid_to_utf8(ssid_data, len);

	if (!ssid_str ||
		g_hash_table_contains(accessPointCbArgs->seen_ssids, ssid_str))
		return;

	GtkWidget *networkBox = new_wifi_entry(ssid_str, GTK_WIDGET(box));
	gtk_box_append(box, networkBox);
	g_hash_table_add(accessPointCbArgs->seen_ssids, g_strdup(ssid_str));
	g_message("New access point detected: %s", ssid_str);

	g_free(ssid_str);
}

GtkWidget *new_wifi_entry(char *ssid_str, GtkWidget *wifiMenu) {
	GtkWidget *networkBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
	GtkWidget *networkChildBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	GtkWidget *label = gtk_label_new(ssid_str);

	gtk_box_append(GTK_BOX(networkBox), networkChildBox);
	gtk_box_append(GTK_BOX(networkChildBox), label);
	gtk_widget_set_hexpand(label, TRUE);
	gtk_label_set_xalign(GTK_LABEL(label), 0.0);
	if (strcmp(ssid_str, get_wifi_name()) == 0) {
		gtk_widget_add_css_class(networkBox, "active-network");

		GtkWidget *disconnect_button =
			gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
		GtkWidget *disconnect_icon =
			gtk_image_new_from_icon_name("network-wireless-disabled-symbolic");
		gtk_box_append(GTK_BOX(disconnect_button), disconnect_icon);
		gtk_widget_add_css_class(disconnect_button, "disconnect-button");

		gtk_box_append(GTK_BOX(networkChildBox), disconnect_button);

		GtkGesture *disconnect = gtk_gesture_click_new();
		gtk_event_controller_set_propagation_phase(
			GTK_EVENT_CONTROLLER(disconnect), GTK_PHASE_BUBBLE);
		gtk_widget_add_controller(disconnect_button,
								  GTK_EVENT_CONTROLLER(disconnect));

		DisconnectForgetProps *disconnectProps =
			malloc(sizeof(DisconnectForgetProps));
		disconnectProps->wifiMenu = wifiMenu;
		disconnectProps->ssid_str = strdup(ssid_str);

		g_signal_connect(disconnect, "released",
						 G_CALLBACK(on_disconnect_clicked), disconnectProps);
	} else {

		GtkGesture *connectStartClick = gtk_gesture_click_new();
		gtk_event_controller_set_propagation_phase(
			GTK_EVENT_CONTROLLER(connectStartClick), GTK_PHASE_BUBBLE);
		gtk_widget_add_controller(label,
								  GTK_EVENT_CONTROLLER(connectStartClick));

		OnConnectStartArgs *onConnectStartArgs =
			malloc(sizeof(OnConnectStartArgs));
		onConnectStartArgs->networkBox = networkBox;
		onConnectStartArgs->ssid = strdup(ssid_str);
		onConnectStartArgs->menuOpen = FALSE;

		g_signal_connect(connectStartClick, "released",
						 G_CALLBACK(on_connect_start), onConnectStartArgs);

		if (is_saved_network(ssid_str)) {
			GtkWidget *forgetButton =
				gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
			GtkWidget *forgetIcon = gtk_image_new_from_icon_name(
				"network-wireless-no-route-symbolic");
			gtk_box_append(GTK_BOX(forgetButton), forgetIcon);
			gtk_widget_add_css_class(forgetButton, "disconnect-button");

			gtk_box_append(GTK_BOX(networkChildBox), forgetButton);

			GtkGesture *forget = gtk_gesture_click_new();
			gtk_event_controller_set_propagation_phase(
				GTK_EVENT_CONTROLLER(forget), GTK_PHASE_BUBBLE);
			gtk_widget_add_controller(forgetButton,
									  GTK_EVENT_CONTROLLER(forget));

			DisconnectForgetProps *forgetProps =
				malloc(sizeof(DisconnectForgetProps));
			forgetProps->ssid_str = strdup(ssid_str);
			forgetProps->wifiMenu = wifiMenu;

			g_signal_connect(forget, "released", G_CALLBACK(on_forget_pressed),
							 forgetProps);
		}
	}
	return networkBox;
}

static void access_points_ready(GObject *source_object, GAsyncResult *res,
								gpointer user_data) {
	if (!user_data) {
		return;
	}
	if (!source_object) {
		return;
	}
	if (!res) {
		return;
	}
	if (g_cancellable_is_cancelled(add_connection_cancellable)) {
		return;
	}

	GtkBox *box = GTK_BOX(user_data);

	GHashTable *seen_ssids = g_hash_table_new(g_str_hash, g_str_equal);
	deviceInstance = NM_DEVICE_WIFI(source_object);

	AccessPointCbArgs *accessPointCbArgs = calloc(sizeof(AccessPointCbArgs), 1);

	accessPointCbArgs->box = box;
	accessPointCbArgs->seen_ssids = seen_ssids;

	if (!nm_device_wifi_request_scan_finish(deviceInstance, res, NULL)) {
		g_printerr("Failed to complete Wi-Fi scan\n");
		return;
	}

	const GPtrArray *aps = nm_device_wifi_get_access_points(deviceInstance);
	if (aps->len < 1) {
		return;
	}
	for (guint i = 0; i < aps->len; ++i) {
		NMAccessPoint *ap = g_ptr_array_index(aps, i);
		GBytes *ssid_bytes = nm_access_point_get_ssid(ap);
		if (!ssid_bytes)
			continue;

		gsize len;
		const guint8 *ssid_data = g_bytes_get_data(ssid_bytes, &len);
		char *ssid_str = nm_utils_ssid_to_utf8(ssid_data, len);

		if (!ssid_str || g_hash_table_contains(seen_ssids, ssid_str))
			continue;

		g_hash_table_add(seen_ssids, g_strdup(ssid_str));

		if (box) {

			GtkWidget *networkBox = new_wifi_entry(ssid_str, GTK_WIDGET(box));
			gtk_box_append(box, networkBox);
			if (strcmp(ssid_str, get_wifi_name()) == 0) {
				gtk_widget_add_css_class(networkBox, "active-network");
			}
			g_message("Found network: %s", ssid_str);
		}
		g_free(ssid_str);
	}
	wifinetworks_timeout_id =
		g_signal_connect(deviceInstance, "access-point-added",
						 G_CALLBACK(new_access_point_cb), accessPointCbArgs);
}

GtkWidget *add_wifi_networks(gpointer user_data) {
	GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	if (!user_data) {
		return box;
	}
	if (!GTK_IS_BOX(user_data)) {
		return box;
	}
	box = user_data;
	NMClient *client = nm_client_new(NULL, NULL);
	if (!client) {
		g_printerr("Failed to create NMClient\n");
		return box;
	}
	if (g_cancellable_is_cancelled(add_connection_cancellable)) {
		return box;
	}

	const GPtrArray *devices = nm_client_get_devices(client);
	for (guint i = 0; i < devices->len; i++) {
		NMDevice *dev = devices->pdata[i];
		if (NM_IS_DEVICE_WIFI(dev)) {
			GtkWidget *statusBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
			GtkWidget *label = gtk_label_new("Searching for wifi");
			GtkWidget *spinner = gtk_spinner_new();
			gtk_widget_set_margin_start(statusBox, 10);
			gtk_widget_set_margin_end(statusBox, 10);
			gtk_box_append(GTK_BOX(statusBox), label);
			gtk_box_append(GTK_BOX(statusBox), spinner);
			gtk_spinner_start(GTK_SPINNER(spinner));
			gtk_box_append(GTK_BOX(box), statusBox);
			add_connection_cancellable = g_cancellable_new();
			if (GTK_IS_WIDGET(box)) {
				nm_device_wifi_request_scan_async(NM_DEVICE_WIFI(dev),
												  add_connection_cancellable,
												  access_points_ready, box);
			}
			break;
		}
	}

	return box;
}

void disconnect_handlers() {
	if (wifinetworks_timeout_id != 0) {
		g_signal_handler_disconnect(deviceInstance, wifinetworks_timeout_id);
		wifinetworks_timeout_id = 0;
	}
	if (g_cancellable_is_cancelled(add_connection_cancellable) == FALSE) {
		g_cancellable_cancel(add_connection_cancellable);
		add_connection_cancellable = NULL;
	}
}
void clear_wifi_list(GtkWidget *box) {
	GtkWidget *child = gtk_widget_get_first_child(box);
	GtkWidget *next = gtk_widget_get_first_child(box);

	while (next != NULL) {
		next = gtk_widget_get_next_sibling(child);

		gtk_widget_unparent(child);

		child = next;
	}
}

gboolean refresh_wifi_list(GtkWidget *box) {

	GtkWidget *child = gtk_widget_get_first_child(box);
	GtkWidget *next = gtk_widget_get_first_child(box);

	while (next != NULL) {
		next = gtk_widget_get_next_sibling(child);

		gtk_widget_unparent(child);

		child = next;
	}
	disconnect_handlers();

	add_wifi_networks(box);
	return FALSE;
}

void disconnect_cb(GObject *client, GAsyncResult *res, gpointer user_data) {
	GtkWidget *box = user_data;
	usleep(1000000); // ill fix this at some point
	refresh_wifi_list(box);

	g_message("Disconnect attempt complete");
}
void on_active_connections_changed(NMClient *client, GParamSpec *pspec,
								   gpointer user_data) {
	GtkWidget *wifi_menu = GTK_WIDGET(user_data);

	/* NMClient just updated its list: rebuild your UI now */
	refresh_wifi_list(wifi_menu);

	/* if you only wanted to do it once, disconnect the handler: */
	g_signal_handlers_disconnect_by_func(client, on_active_connections_changed,
										 user_data);
}

void disconnect_from_wifi(const char *ssid, GtkWidget *wifiMenu) {
	NMClient *client = nm_client_new(NULL, NULL);
	if (!client) {
		g_printerr("Failed to create NMClient\n");
		return;
	}

	// g_signal_connect(client, "notify::active-connections",
	// 				 G_CALLBACK(on_active_connections_changed), wifiMenu);

	const GPtrArray *devices = nm_client_get_devices(client);
	for (guint i = 0; i < devices->len; i++) {
		NMDevice *device = g_ptr_array_index(devices, i);
		if (!NM_IS_DEVICE_WIFI(device))
			continue;

		NMActiveConnection *active = nm_device_get_active_connection(device);
		if (!active)
			continue;

		NMRemoteConnection *remote =
			nm_active_connection_get_connection(active);
		if (!remote)
			continue;

		NMSettingWireless *s_wifi =
			nm_connection_get_setting_wireless(NM_CONNECTION(remote));
		if (!s_wifi)
			continue;

		const GBytes *ssid_bytes = nm_setting_wireless_get_ssid(s_wifi);
		if (!ssid_bytes)
			continue;

		gsize len;
		const guint8 *ssid_data = g_bytes_get_data((GBytes *)ssid_bytes, &len);
		char *conn_ssid = nm_utils_ssid_to_utf8(ssid_data, len);

		if (g_strcmp0(conn_ssid, ssid) == 0) {
			g_print("Disconnecting from %s...\n", conn_ssid);
			deactivate_connection_cancellable = g_cancellable_new();
			nm_client_deactivate_connection_async(
				client, active, deactivate_connection_cancellable,
				disconnect_cb, wifiMenu);
			g_free(conn_ssid);
			break;
		}

		g_free(conn_ssid);
	}

	g_object_unref(client);
}

void on_disconnect_clicked(GtkGestureClick *gesture, int n_press, double x,
						   double y, gpointer user_data) {
	GtkWidget *box =
		gtk_event_controller_get_widget(GTK_EVENT_CONTROLLER(gesture));
	GtkWidget *spinner = gtk_spinner_new();
	gtk_widget_unparent(gtk_widget_get_first_child(box));
	gtk_box_append(GTK_BOX(box), spinner);
	gtk_spinner_start(GTK_SPINNER(spinner));

	DisconnectForgetProps *props = user_data;
	disconnect_from_wifi(props->ssid_str, props->wifiMenu);
}

void on_entry_changed(GtkEditable *editable, gpointer user_data) {
	g_free(passwordText);
	passwordText = strdup(gtk_editable_get_text(editable));
}

int connect_to_wifi_via_nmcli(const char *ssid, const char *password) {
	if (!ssid || !*ssid) {
		fprintf(stderr, "connect_to_wifi_via_nmcli: SSID must not be empty\n");
		return -EINVAL;
	}

	// build the command line
	char cmd[512];
	if (password && *password) {
		snprintf(
			cmd, sizeof(cmd),
			"nmcli -t -f GENERAL.STATE device wifi connect '%s' password '%s'",
			ssid, password);
	} else {
		snprintf(cmd, sizeof(cmd),
				 "nmcli -t -f GENERAL.STATE device wifi connect '%s'", ssid);
	}

	// popen so we can capture & print what nmcli says
	FILE *fp = popen(cmd, "r");
	if (!fp) {
		perror("popen");
		return -errno;
	}

	char buf[256];
	while (fgets(buf, sizeof(buf), fp)) {
		fputs(buf, stdout);
	}

	int rc = pclose(fp);
	if (rc == 256) {

		snprintf(cmd, sizeof(cmd),
				 "nmcli -t -f GENERAL.STATE device wifi connect '%s'", ssid);

		// popen so we can capture & print what nmcli says
		FILE *fp = popen(cmd, "r");
		if (!fp) {
			perror("popen");
			return -errno;
		}

		char buf[256];
		while (fgets(buf, sizeof(buf), fp)) {
			fputs(buf, stdout);
		}

		int rc2 = pclose(fp);
		if (rc2 != 0) {
			// nmcli returns non-zero on failure
			fprintf(stderr, "nmcli exited with %d\n", rc);
			return -EIO;
		}

	} else if (rc != 0) {
		// nmcli returns non-zero on failure
		fprintf(stderr, "nmcli exited with %d\n", rc);
		return -EIO;
	}

	// success!
	return 0;
}

gboolean is_saved_network(const char *target_ssid) {
	g_return_val_if_fail(target_ssid != NULL, FALSE);

	GError *error = NULL;
	NMClient *client = nm_client_new(NULL, &error);
	if (!client) {
		g_warning("NMClient failed: %s", error->message);
		g_clear_error(&error);
		return FALSE;
	}

	/* Grab all saved (remote) connections */
	const GPtrArray *remotes = nm_client_get_connections(client);
	for (guint i = 0; i < remotes->len; i++) {
		NMRemoteConnection *rc = g_ptr_array_index(remotes, i);
		NMConnection *conn = NM_CONNECTION(rc);

		/* Pull out the wireless setting, if any */
		NMSettingWireless *sw = nm_connection_get_setting_wireless(conn);
		if (!sw)
			continue;

		/* SSID is stored as a GBytes blob */
		GBytes *ssid_blob = nm_setting_wireless_get_ssid(sw);
		if (!ssid_blob)
			continue;

		/* Decode to a NUL-terminated UTF-8 string */
		gsize len = 0;
		const guint8 *raw = g_bytes_get_data(ssid_blob, &len);
		gchar *ssid = nm_utils_ssid_to_utf8(raw, len);
		if (ssid) {
			if (g_strcmp0(ssid, target_ssid) == 0) {
				g_free(ssid);
				g_object_unref(client);
				return TRUE;
			}
			g_free(ssid);
		}
	}

	g_object_unref(client);
	return FALSE;
}
