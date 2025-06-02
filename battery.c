#include "app.h"

int read_value(const char *path) {
	FILE *fp = fopen(path, "r");
	if (!fp)
		return -1;

	int val;
	if (fscanf(fp, "%d", &val) != 1) {
		fclose(fp);
		return -1;
	}

	fclose(fp);
	return val;
}

int get_battery_percentage() {
	const char *base_path = "/sys/class/power_supply";
	DIR *dir = opendir(base_path);
	if (!dir)
		return -1;

	struct dirent *entry;
	int64_t total_now = 0;
	int64_t total_full = 0;

	while ((entry = readdir(dir)) != NULL) {
		if (strncmp(entry->d_name, "BAT", 3) != 0)
			continue;

		char path_now[512], path_full[512];
		int now = -1, full = -1;

		// Try energy_* first
		snprintf(path_now, sizeof(path_now), "%s/%s/energy_now", base_path,
				 entry->d_name);
		snprintf(path_full, sizeof(path_full), "%s/%s/energy_full", base_path,
				 entry->d_name);
		now = read_value(path_now);
		full = read_value(path_full);

		// Fallback to charge_* if needed
		if (now < 0 || full <= 0) {
			snprintf(path_now, sizeof(path_now), "%s/%s/charge_now", base_path,
					 entry->d_name);
			snprintf(path_full, sizeof(path_full), "%s/%s/charge_full",
					 base_path, entry->d_name);
			now = read_value(path_now);
			full = read_value(path_full);
		}

		if (now < 0 || full <= 0 || now > full)
			continue;

		total_now += now;
		total_full += full;
	}

	closedir(dir);

	if (total_full <= 0)
		return -1;

	int percent = (int)((total_now * 100) / total_full);
	if (percent < 0)
		percent = 0;
	if (percent > 100)
		percent = 100;

	return percent;
}

gboolean battery_refresh_wrapper(gpointer user_battery_label) {
	GtkLabel *battery_label = GTK_LABEL(user_battery_label);

	int battery_precentage = get_battery_percentage();
	char label_text[32];
	snprintf(label_text, sizeof(label_text), "%d%%", battery_precentage);
	gtk_label_set_label(battery_label, label_text);
	return TRUE;
}
