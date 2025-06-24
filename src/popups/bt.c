/* src/popups/bt.c */

#include "ctrlcenterpopup.h"
#include "gtk/gtkshortcut.h"
#include "mixer.h"
#include "volume.h"

#include <gdk/gdk.h>
#include <gio/gio.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <stdlib.h>

typedef struct {
	gchar *path;
	gchar *name;
	gboolean paired;
	gboolean connected;
} BLEDevice;

/* Subscription IDs for cleanup */
static guint iface_added_id = 0;
static guint iface_removed_id = 0;

/* Globals */
static GDBusConnection *bt_conn = NULL;
static gchar *adapter_path = NULL;
static GtkListBox *bt_list = NULL;
static gboolean scanning = FALSE;

/* Forward declarations */
static gchar *get_adapter_path(void);
static GPtrArray *get_bt_devices(void);
static void rebuild_device_list(void);
static void destroy_bt_manager_box_cb(GtkWidget *widget, gpointer user_data);

static void on_bt_device_button_clicked(GtkButton *button, gpointer user_data);
static void on_scan_button_clicked(GtkButton *button, gpointer user_data);
static void on_interfaces_changed(GDBusConnection *conn, const gchar *sender,
								  const gchar *object_path,
								  const gchar *interface_name,
								  const gchar *signal_name, GVariant *params,
								  gpointer user_data);
static void on_device_right_click(GtkGestureClick *gesture, int n_press,
								  double x, double y, gpointer hbox_widget);
static void on_bt_back_button_clicked(GtkGestureClick *gesture, int n_press,
									  double x, double y, gpointer user_data);

typedef struct {
	GtkStack *stack;
	GtkWidget *box;
} BtBackArgs;

// ─── Find first Adapter1 ─────────────────────────────────────────────────────
static gchar *get_adapter_path(void) {
	GError *err = NULL;
	bt_conn = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, &err);
	if (!bt_conn)
		return NULL;

	GVariant *reply = g_dbus_connection_call_sync(
		bt_conn, "org.bluez", "/", "org.freedesktop.DBus.ObjectManager",
		"GetManagedObjects", NULL, NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL,
		&err);

	gchar *path = NULL;
	if (reply) {
		GVariantIter *iter;
		g_variant_get(reply, "(a{oa{sa{sv}}})", &iter);
		GVariant *map;
		gchar *obj;
		while (!path &&
			   g_variant_iter_loop(iter, "{&o@a{sa{sv}}}", &obj, &map)) {
			if (g_variant_lookup_value(map, "org.bluez.Adapter1", NULL))
				path = g_strdup(obj);
		}
		g_variant_iter_free(iter);
		g_variant_unref(reply);
	}

	return path;
}

// ─── Enumerate Device1 objects ───────────────────────────────────────────────
static GPtrArray *get_bt_devices(void) {
	GError *err = NULL;
	GVariant *reply = g_dbus_connection_call_sync(
		bt_conn, "org.bluez", "/", "org.freedesktop.DBus.ObjectManager",
		"GetManagedObjects", NULL, NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL,
		&err);

	GHashTable *seen = g_hash_table_new(g_str_hash, g_str_equal);
	GPtrArray *arr = g_ptr_array_new_with_free_func(g_free);

	if (reply) {
		GVariantIter *iter;
		g_variant_get(reply, "(a{oa{sa{sv}}})", &iter);
		GVariant *map;
		gchar *obj;
		while (g_variant_iter_loop(iter, "{&o@a{sa{sv}}}", &obj, &map)) {
			if (g_hash_table_contains(seen, obj))
				continue;
			g_hash_table_add(seen, g_strdup(obj));

			GVariant *dev =
				g_variant_lookup_value(map, "org.bluez.Device1", NULL);
			if (dev) {
				GVariant *vn = g_variant_lookup_value(dev, "Name", NULL);
				GVariant *vp = g_variant_lookup_value(dev, "Paired", NULL);
				GVariant *vc = g_variant_lookup_value(dev, "Connected", NULL);

				if (vn) {
					BLEDevice *d = g_new0(BLEDevice, 1);
					d->path = g_strdup(obj);
					d->name = g_variant_dup_string(vn, NULL);
					d->paired = vp ? g_variant_get_boolean(vp) : FALSE;
					d->connected = vc ? g_variant_get_boolean(vc) : FALSE;
					g_ptr_array_add(arr, d);
				}
			}
		}
		g_variant_iter_free(iter);
		g_variant_unref(reply);
	}

	g_hash_table_destroy(seen);
	return arr;
}

// ─── Rebuild device list UI ──────────────────────────────────────────────────
static void rebuild_device_list(void) {
	gtk_list_box_remove_all(bt_list);

	GPtrArray *devs = get_bt_devices();
	for (guint i = 0; i < devs->len; ++i) {
		BLEDevice *d = devs->pdata[i];

		GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);

		// vertical box for status + name
		GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);

		// status label
		const char *status_txt = d->connected ? "Connected"
								 : d->paired  ? "Paired"
											  : "Not paired";
		if (d->connected) {
			gtk_widget_add_css_class(hbox, "connected");
		}
		GtkWidget *status_lbl = gtk_label_new(status_txt);
		gtk_widget_add_css_class(status_lbl, "status-label");

		// device-name label
		GtkWidget *name_lbl = gtk_label_new(d->name);
		gtk_label_set_max_width_chars(GTK_LABEL(name_lbl), 10);
		gtk_label_set_ellipsize(GTK_LABEL(name_lbl), PANGO_ELLIPSIZE_END);

		gtk_label_set_xalign(GTK_LABEL(name_lbl), 0.0);
		gtk_label_set_xalign(GTK_LABEL(status_lbl), 0.0);

		gtk_widget_set_valign(hbox, GTK_ALIGN_CENTER);
		gtk_widget_set_valign(vbox, GTK_ALIGN_CENTER);

		// pack status above name
		gtk_box_append(GTK_BOX(vbox), name_lbl);

		gtk_widget_set_hexpand(vbox, TRUE);

		// action button
		const gchar *btn_lbl =
			!d->paired ? "Pair" : (d->connected ? "Disconnect" : "Connect");
		GtkWidget *btn = gtk_button_new_with_label(btn_lbl);

		// stash metadata
		g_object_set_data_full(G_OBJECT(hbox), "device-path", g_strdup(d->path),
							   g_free);
		g_object_set_data(G_OBJECT(hbox), "device-paired",
						  GINT_TO_POINTER(d->paired));
		g_object_set_data(G_OBJECT(hbox), "device-connected",
						  GINT_TO_POINTER(d->connected));
		g_object_set_data_full(G_OBJECT(btn), "device-path", g_strdup(d->path),
							   g_free);

		g_signal_connect(btn, "clicked",
						 G_CALLBACK(on_bt_device_button_clicked), hbox);

		// right-click context
		GtkGesture *gst = gtk_gesture_click_new();
		gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(gst),
									  GDK_BUTTON_SECONDARY);
		gtk_widget_add_controller(hbox, GTK_EVENT_CONTROLLER(gst));
		g_signal_connect(gst, "pressed", G_CALLBACK(on_device_right_click),
						 hbox);

		// assemble row
		gtk_box_append(GTK_BOX(hbox), vbox);
		gtk_box_append(GTK_BOX(hbox), btn);
		gtk_list_box_append(bt_list, hbox);

		// cleanup
		g_free(d->path);
		g_free(d->name);
		g_free(d);
	}
}

// ─── Context menu on right-click ─────────────────────────────────────────────
static void on_device_right_click(GtkGestureClick *gesture, int n_press,
								  double x, double y, gpointer hbox_widget) {
	GtkWidget *hbox = GTK_WIDGET(hbox_widget);

	gboolean paired =
		GPOINTER_TO_INT(g_object_get_data(G_OBJECT(hbox), "device-paired"));
	gboolean connected =
		GPOINTER_TO_INT(g_object_get_data(G_OBJECT(hbox), "device-connected"));
	const gchar *path = g_object_get_data(G_OBJECT(hbox), "device-path");

	GtkPopover *popover = GTK_POPOVER(gtk_popover_new());
	gtk_box_append(GTK_BOX(hbox), GTK_WIDGET(popover));
	gtk_widget_set_valign(GTK_WIDGET(popover), GTK_ALIGN_START);
	GtkWidget *menu = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

#define ADD_ITEM(label, action)                                                \
	do {                                                                       \
		GtkWidget *it = gtk_button_new_with_label(label);                      \
		g_object_set_data_full(G_OBJECT(it), "action", g_strdup(action),       \
							   g_free);                                        \
		g_object_set_data_full(G_OBJECT(it), "device-path", g_strdup(path),    \
							   g_free);                                        \
		g_signal_connect(it, "clicked",                                        \
						 G_CALLBACK(on_bt_device_button_clicked), hbox);       \
		gtk_box_append(GTK_BOX(menu), it);                                     \
	} while (0)

	if (!paired) {
		ADD_ITEM("Pair", "Pair");
	} else {
		if (!connected)
			ADD_ITEM("Connect", "Connect");
		else
			ADD_ITEM("Disconnect", "Disconnect");
		ADD_ITEM("Remove", "Remove");
	}
#undef ADD_ITEM

	gtk_popover_set_child(popover, menu);
	GdkRectangle rect = {(int)x, (int)y, 1, 1};
	gtk_popover_set_pointing_to(popover, &rect);
	gtk_popover_popup(popover);
}

// ─── Handle device actions ───────────────────────────────────────────────────
static void on_bt_device_button_clicked(GtkButton *button, gpointer user_data) {
	const gchar *action = g_object_get_data(G_OBJECT(button), "action");
	if (!action)
		action = gtk_button_get_label(button);

	const gchar *path = g_object_get_data(G_OBJECT(button), "device-path");
	if (!path && user_data)
		path = g_object_get_data(G_OBJECT(user_data), "device-path");
	if (!path)
		return;

	GError *err = NULL;
	if (g_strcmp0(action, "Remove") == 0) {
		GDBusProxy *adp = g_dbus_proxy_new_sync(
			bt_conn, G_DBUS_PROXY_FLAGS_NONE, NULL, "org.bluez", adapter_path,
			"org.bluez.Adapter1", NULL, &err);
		if (adp) {
			GVariant *args = g_variant_new("(o)", path);
			g_dbus_proxy_call_sync(adp, "RemoveDevice", args,
								   G_DBUS_CALL_FLAGS_NONE, -1, NULL, &err);
			g_clear_object(&adp);
		}
	} else {
		GDBusProxy *dev = g_dbus_proxy_new_sync(
			bt_conn, G_DBUS_PROXY_FLAGS_NONE, NULL, "org.bluez", path,
			"org.bluez.Device1", NULL, &err);
		if (dev) {
			g_dbus_proxy_call_sync(dev, action, NULL, G_DBUS_CALL_FLAGS_NONE,
								   -1, NULL, &err);
			g_clear_object(&dev);
		}
	}

	if (err) {
		g_warning("Action '%s' failed on '%s': %s", action, path, err->message);
		g_clear_error(&err);
	}
	rebuild_device_list();
}

// ─── Clean up on destroy ─────────────────────────────────────────────────────
static void destroy_bt_manager_box_cb(GtkWidget *widget, gpointer user_data) {
	if (scanning) {
		GError *err = NULL;
		GDBusProxy *adp = g_dbus_proxy_new_sync(
			bt_conn, G_DBUS_PROXY_FLAGS_NONE, NULL, "org.bluez", adapter_path,
			"org.bluez.Adapter1", NULL, &err);
		if (adp) {
			g_dbus_proxy_call_sync(adp, "StopDiscovery", NULL,
								   G_DBUS_CALL_FLAGS_NONE, -1, NULL, &err);
		}
		scanning = FALSE;
	}

	if (iface_added_id) {
		g_dbus_connection_signal_unsubscribe(bt_conn, iface_added_id);
		iface_added_id = 0;
	}
	if (iface_removed_id) {
		g_dbus_connection_signal_unsubscribe(bt_conn, iface_removed_id);
		iface_removed_id = 0;
	}

	g_free(adapter_path);
	adapter_path = NULL;
}

// ─── Watch for BlueZ signals ────────────────────────────────────────────────
static void on_interfaces_changed(GDBusConnection *conn, const gchar *sender,
								  const gchar *object_path,
								  const gchar *interface_name,
								  const gchar *signal_name, GVariant *params,
								  gpointer user_data) {
	(void)conn;
	(void)sender;
	(void)object_path;
	(void)interface_name;
	(void)signal_name;
	(void)params;
	(void)user_data;
	rebuild_device_list();
}

// ─── Scan handler ────────────────────────────────────────────────────────────
static void on_scan_button_clicked(GtkButton *button, gpointer user_data) {
	(void)button;
	(void)user_data;
	GError *err = NULL;
	GDBusProxy *adp = g_dbus_proxy_new_sync(bt_conn, G_DBUS_PROXY_FLAGS_NONE,
											NULL, "org.bluez", adapter_path,
											"org.bluez.Adapter1", NULL, &err);
	if (adp) {
		g_dbus_proxy_call_sync(adp, "StartDiscovery", NULL,
							   G_DBUS_CALL_FLAGS_NONE, -1, NULL, &err);
		scanning = TRUE;
	}
	if (err) {
		g_warning("Scan failed: %s", err->message);
		g_clear_error(&err);
	}
}

// ─── Back button handler ─────────────────────────────────────────────────────
static void on_bt_back_button_clicked(GtkGestureClick *gesture, int n_press,
									  double x, double y, gpointer user_data) {
	BtBackArgs *args = user_data;
	gtk_widget_set_size_request(GTK_WIDGET(args->box), -1, -1);
	gtk_stack_set_visible_child_name(args->stack, "page1");
}

// ─── Build scrolling Bluetooth manager box ───────────────────────────────────
GtkWidget *create_bt_manager_box(void) {
	bt_conn = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, NULL);
	adapter_path = get_adapter_path();

	iface_added_id = g_dbus_connection_signal_subscribe(
		bt_conn, "org.bluez", "org.freedesktop.DBus.ObjectManager",
		"InterfacesAdded", NULL, NULL, 0, on_interfaces_changed, NULL, NULL);
	iface_removed_id = g_dbus_connection_signal_subscribe(
		bt_conn, "org.bluez", "org.freedesktop.DBus.ObjectManager",
		"InterfacesRemoved", NULL, NULL, 0, on_interfaces_changed, NULL, NULL);

	GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
	GtkWidget *scan_btn = gtk_button_new_with_label("Scan");
	g_signal_connect(scan_btn, "clicked", G_CALLBACK(on_scan_button_clicked),
					 NULL);
	gtk_box_append(GTK_BOX(vbox), scan_btn);

	bt_list = GTK_LIST_BOX(gtk_list_box_new());
	GtkWidget *scrollerParent = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	GtkWidget *scroller = gtk_scrolled_window_new();
	gtk_box_append(GTK_BOX(scrollerParent), scroller);
	gtk_widget_add_css_class(scrollerParent, "scroller");
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroller),
								   GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroller),
								  GTK_WIDGET(bt_list));
	gtk_widget_set_vexpand(scroller, TRUE);
	gtk_box_append(GTK_BOX(vbox), scrollerParent);

	g_signal_connect(vbox, "destroy", G_CALLBACK(destroy_bt_manager_box_cb),
					 NULL);

	rebuild_device_list();
	return vbox;
}

// ─── Entry point for the BT popup ────────────────────────────────────────────
GtkWidget *btBox(GtkStack *stack) {
	GtkWidget *mainBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
	gtk_widget_add_css_class(mainBox, "ctrlcenter");

	GtkWidget *topBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
	GtkWidget *backButton = new_icon_button("go-previous-symbolic");
	GtkGesture *backClick = gtk_gesture_click_new();
	gtk_event_controller_set_propagation_phase(GTK_EVENT_CONTROLLER(backClick),
											   GTK_PHASE_BUBBLE);
	gtk_widget_add_controller(backButton, GTK_EVENT_CONTROLLER(backClick));

	BtBackArgs *btBackArgs = g_new(BtBackArgs, 1);
	btBackArgs->stack = stack;
	btBackArgs->box = mainBox;

	g_signal_connect(backClick, "released",
					 G_CALLBACK(on_bt_back_button_clicked), btBackArgs);

	gtk_box_append(GTK_BOX(topBox), backButton);
	gtk_box_append(GTK_BOX(mainBox), topBox);
	GtkWidget *btManagerBox = create_bt_manager_box();
	gtk_widget_add_css_class(btManagerBox, "bt-box");
	gtk_box_append(GTK_BOX(mainBox), btManagerBox);
	return mainBox;
}
