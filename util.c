#include "app.h"

char *int_to_cstr(int in_int) {
	char *str = malloc(16); // enough for any 32-bit int including sign
	if (!str)
		return NULL;

	snprintf(str, 16, "%d", in_int);
	return str;
}

GtkWidget *find_button_by_label(GtkWidget *box, const char *target_label) {
	GtkWidget *child = gtk_widget_get_first_child(box);
	while (child != NULL) {
		if (GTK_IS_BUTTON(child)) {
			const char *label = gtk_button_get_label(GTK_BUTTON(child));
			if (label && strcmp(label, target_label) == 0) {
				return child; // found it!
			}
		}
		child = gtk_widget_get_next_sibling(child);
	}
	return NULL; // not found
}

bool string_arrays_equal(char **a, int a_len, char **b, int b_len) {
	if (!a || !b)
		return false;
	if (a_len != b_len)
		return false;

	for (int i = 0; i < a_len; i++) {
		if (!a[i] || !b[i])
			return false;
		if (strcmp(a[i], b[i]) != 0)
			return false;
	}
	return true;
}
