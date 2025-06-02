#include "app.h"

char *get_wifi_name() {
	static char result[256] = {0};

	FILE *fp = popen("nmcli -t -f TYPE,STATE,CONNECTION device | grep "
					 "':connected' | head -n1",
					 "r");
	if (!fp)
		return "Network Unavailable";

	char buffer[256];
	if (fgets(buffer, sizeof(buffer), fp) != NULL) {
		// Example output: wifi:connected:MySSID
		//                 ethernet:connected:Wired connection 1
		char *type = strtok(buffer, ":");
		strtok(NULL, ":"); // skip 'connected'
		char *name = strtok(NULL, "\n");

		if (type && name) {
			if (strcmp(type, "ethernet") == 0)
				snprintf(result, sizeof(result), "Ethernet");
			else if (strcmp(type, "wifi") == 0)
				snprintf(result, sizeof(result), "%s", name);
			else
				snprintf(result, sizeof(result), "%s", name);
		} else {
			snprintf(result, sizeof(result), "No Connection");
		}
	} else {
		snprintf(result, sizeof(result), "No Connection");
	}

	pclose(fp);
	return result;
}

gboolean wifi_refresh_wrapper(gpointer user_wifi_label) {
	GtkLabel *wifi_label = GTK_LABEL(user_wifi_label);

	gtk_label_set_label(wifi_label, get_wifi_name());

	return TRUE;
}
