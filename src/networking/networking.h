#ifndef NETWORKING_H
#define NETWORKING_H
#include "NetworkManager.h"
#include "app.h"
#include "gio/gio.h"
#include "glib.h"
#include "gtk/gtk.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static guint wifi_timeout_id = 0;
static guint wifinetworks_timeout_id = 0;
static GCancellable *add_connection_cancellable = NULL;
static GCancellable *deactivate_connection_cancellable = NULL;

typedef struct {
	GHashTable *seen_ssids;
	GtkBox *box;
} AccessPointCbArgs;
typedef struct {
	GtkWidget *networkBox;
	char *ssid;
	gboolean menuOpen;
} OnConnectStartArgs;
static char *passwordText;
typedef struct {
	char *ssid_str;
	GtkWidget *wifiMenu;
} DisconnectForgetProps;
typedef struct {
	GtkStack *stack;
	GtkWidget *wifiMenu;
	GtkWidget *box;
} WifiCLickedArgs;

gboolean refresh_wifi_list(GtkWidget *box);

void disconnect_cb(GObject *client, GAsyncResult *res, gpointer user_data);

void on_active_connections_changed(NMClient *client, GParamSpec *pspec,
								   gpointer user_data);

void disconnect_from_wifi(const char *ssid, GtkWidget *wifiMenu);

void on_disconnect_clicked(GtkGestureClick *gesture, int n_press, double x,
						   double y, gpointer user_data);

void on_entry_changed(GtkEditable *editable, gpointer user_data);

int connect_to_wifi_via_nmcli(const char *ssid, const char *password);

GtkWidget *add_wifi_networks(gpointer user_data);

void disconnect_handlers();

gboolean is_saved_network(const char *target_ssid);

void clear_wifi_list(GtkWidget *box);

static void access_points_ready(GObject *source_object, GAsyncResult *res,
								gpointer user_data);
GtkWidget *new_wifi_entry(char *ssid_str, GtkWidget *wifiMenu);
void on_connect_start(GtkGestureClick *gesture, int n_press, double x, double y,
					  gpointer user_data);

gboolean forget_connection_by_ssid(const char *ssid);

void on_forget_pressed(GtkGestureClick *gesture, int n_press, double x,
					   double y, gpointer user_data);

void new_access_point_cb(NMDeviceWifi *device, NMAccessPoint *ap,
						 gpointer user_data);

void on_connect_button_clicked(GtkButton *button, gpointer user_data);

#endif
