#include "app.h"

char **current_workspace_names;
int current_workspace_count;
char *current_active_workspace;
GtkWidget *button_box;

void on_button_clicked(GtkButton *button, gpointer user_data) {
	const char *workspace = (const char *)user_data;

	char cmd[64];
	snprintf(cmd, sizeof(cmd), "hyprctl dispatch workspace %s", workspace);
	system(cmd);
}

int get_hyprland_workspace_count() {
	FILE *fp = popen("hyprctl workspaces -j | jq length", "r");
	if (!fp)
		return -1;

	int count = -1;
	fscanf(fp, "%d", &count);
	pclose(fp);
	return count;
}
// Comparison function for qsort: compares as integers
int compare_workspace_names(const void *a, const void *b) {
	const char *str_a = *(const char **)a;
	const char *str_b = *(const char **)b;
	int num_a = atoi(str_a);
	int num_b = atoi(str_b);
	return num_a - num_b;
}

char **get_hyprland_workspace_names(int *count_out) {
	FILE *fp = popen("hyprctl workspaces -j", "r");
	if (!fp)
		return NULL;

	char buffer[8192] = {0};
	fread(buffer, 1, sizeof(buffer) - 1, fp);
	pclose(fp);

	cJSON *root = cJSON_Parse(buffer);
	if (!root || !cJSON_IsArray(root)) {
		cJSON_Delete(root);
		return NULL;
	}

	int count = cJSON_GetArraySize(root);
	char **names = malloc(sizeof(char *) * count);

	for (int i = 0; i < count; i++) {
		cJSON *item = cJSON_GetArrayItem(root, i);
		cJSON *name = cJSON_GetObjectItem(item, "name");
		if (cJSON_IsString(name)) {
			names[i] = strdup(name->valuestring); // copy the string
		} else {
			names[i] = strdup("unknown");
		}
	}

	cJSON_Delete(root);
	*count_out = count;
	qsort(names, count, sizeof(char *), compare_workspace_names);
	return names;
}

void set_active_workspace(GtkWidget *box, const char *workspace_name) {
	GtkWidget *child = gtk_widget_get_first_child(box);

	if (current_active_workspace)
		free(current_active_workspace); // free old one

	current_active_workspace = strdup(workspace_name);

	while (child != NULL) {
		GtkWidget *next = gtk_widget_get_next_sibling(child);

		gtk_widget_remove_css_class(child, "active-workspace");

		child = next;
	}

	GtkWidget *button = find_button_by_label(box, workspace_name);
	gtk_widget_set_vexpand(button, FALSE);
	gtk_widget_add_css_class(button, "active-workspace");
}

char *get_active_workspace_name() {
	FILE *fp = popen("hyprctl activeworkspace -j", "r");
	if (!fp)
		return NULL;

	char buffer[4096] = {0};
	fread(buffer, 1, sizeof(buffer) - 1, fp);
	pclose(fp);

	cJSON *root = cJSON_Parse(buffer);
	if (!root) {
		fprintf(stderr, "Failed to parse JSON: %s\n", cJSON_GetErrorPtr());
		return NULL;
	}

	cJSON *name = cJSON_GetObjectItem(root, "name");
	if (!cJSON_IsString(name)) {
		cJSON_Delete(root);
		return NULL;
	}

	char *result = strdup(name->valuestring);
	cJSON_Delete(root);
	return result;
}

void refresh_workspaces(GtkWidget *workspace_box) {

	GtkWidget *child = gtk_widget_get_first_child(workspace_box);
	while (child != NULL) {
		GtkWidget *next = gtk_widget_get_next_sibling(child);
		gtk_widget_unparent(child); // removes it from the box
		child = next;
	}

	// Then re-add new buttons

	int workspaces = get_hyprland_workspace_count();
	current_workspace_count = workspaces;
	int count = 0;
	char **workspace_names = get_hyprland_workspace_names(&count);

	current_workspace_names = workspace_names;

	for (int i = 0; i < workspaces; i++) {
		GtkWidget *button = gtk_button_new_with_label(workspace_names[i]);

		gtk_widget_set_vexpand(button, FALSE);
		gtk_widget_set_halign(button, GTK_ALIGN_CENTER);

		gtk_widget_add_css_class(button, "workspace");

		g_signal_connect(button, "clicked", G_CALLBACK(on_button_clicked),
						 workspace_names[i]);

		gtk_box_append(GTK_BOX(workspace_box), button);
	}
}
gboolean workspace_refresh_wrapper(gpointer user_workspace_box) {
	GtkWidget *workspace_box = GTK_WIDGET(user_workspace_box);
	int workspace_count = get_hyprland_workspace_count();
	int count;
	char **workspace_names = get_hyprland_workspace_names(&count);
	// printf("Current Workspace Count: %d\n", current_workspace_count);
	// printf("New Workspace Count: %d\n", workspace_count);

	if (!string_arrays_equal(current_workspace_names, current_workspace_count,
							 workspace_names, workspace_count)) {
		refresh_workspaces(workspace_box);
	}

	char *active_name = get_active_workspace_name();

	// printf("active workspace %s\n", active_name);
	// printf("current active workspace %s\n", current_active_workspace);

	if (active_name && (!current_active_workspace ||
						strcmp(current_active_workspace, active_name) != 0)) {
		set_active_workspace(workspace_box, active_name);

		free(current_active_workspace);
		current_active_workspace = strdup(active_name);
	}

	return TRUE; // TRUE = keep repeating; FALSE = run once
}
