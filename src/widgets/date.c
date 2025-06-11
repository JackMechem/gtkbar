#include "app.h"
#include "calendarpopup.h"

gboolean time_refresh_wrapper(gpointer user_time_box) {
	GtkLabel *time_box = GTK_LABEL(user_time_box);

	if (time_box) {
		time_t rawtime;
		struct tm *timeinfo;

		time(&rawtime); // Get current time in seconds since epoch
		timeinfo = localtime(&rawtime); // Convert to local time struct
		char buffer[64];
		strftime(buffer, sizeof(buffer), "%a %b %d  %I:%M:%S %p", timeinfo);
		gtk_label_set_label(GTK_LABEL(time_box), buffer);
	}
	return TRUE;
}
