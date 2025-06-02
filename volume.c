#include "app.h"
#include "glib.h"
#include "volumepopup.h"
#include <stdio.h>

int get_volume_percentage() {
	FILE *fp = popen("pamixer --get-volume", "r");
	if (!fp)
		return -1;

	int volume = -1;
	fscanf(fp, "%d", &volume);
	pclose(fp);
	return volume;
}

gboolean volume_refresh_wrapper(gpointer user_label) {
	GtkLabel *label = GTK_LABEL(user_label);
	int volume = get_volume_percentage();

	char text[32];
	if (volume >= 0)
		snprintf(text, sizeof(text), "%d%%", volume);
	else
		snprintf(text, sizeof(text), "--");

	gtk_label_set_text(label, text);
	return TRUE; // Keep the timer running
}
