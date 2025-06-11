#include "wifi.h"
#include <NetworkManager.h>

char *get_wifi_name() {
	GError *error = NULL;
	NMClient *client = nm_client_new(NULL, &error);
	if (!client) {
		if (error)
			g_error_free(error);
		return g_strdup("not connected");
	}

	const GPtrArray *devices = nm_client_get_devices(client);
	for (guint i = 0; i < devices->len; ++i) {
		NMDevice *device = g_ptr_array_index(devices, i);

		if (NM_IS_DEVICE_WIFI(device)) {
			NMAccessPoint *ap =
				nm_device_wifi_get_active_access_point(NM_DEVICE_WIFI(device));
			if (ap) {
				GBytes *ssid_bytes = nm_access_point_get_ssid(ap);
				if (ssid_bytes) {
					gsize len;
					const guint8 *data = g_bytes_get_data(ssid_bytes, &len);
					char *ssid = g_strndup((const char *)data, len);
					g_object_unref(client);
					return ssid; // caller must free
				}
			}
		}
	}

	g_object_unref(client);
	return g_strdup("Disconnected");
}

gboolean wifi_refresh_wrapper(gpointer user_wifi_label) {
	GtkLabel *wifi_label = GTK_LABEL(user_wifi_label);

	gtk_label_set_label(wifi_label, get_wifi_name());

	return TRUE;
}
