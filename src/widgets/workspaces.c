#include "app.h"
#include "cJSON.h"
#include "glib.h"
#include "gtk/gtk.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

int hyprland_ipc1_query(const char *msg, char *out_buf, size_t out_buf_len) {
	const char *sock_id = getenv("HYPRLAND_INSTANCE_SIGNATURE");
	const char *xdg_runtime_dir = getenv("XDG_RUNTIME_DIR");
	if (!sock_id) {
		printf("socket failed");
		return -1;
	}

	char socket_path[256];
	snprintf(socket_path, sizeof(socket_path), "%s/hypr/%s/.socket.sock",
			 xdg_runtime_dir, sock_id);

	int sock = socket(AF_UNIX, SOCK_STREAM, 0);
	if (sock < 0) {
		printf("socket failed");
		return -1;
	}

	struct sockaddr_un addr = {0};
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path) - 1);

	if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
		close(sock);
		printf("socket failed");
		return -1;
	}

	if (write(sock, msg, strlen(msg)) < 0) {
		printf("socket failed");
		close(sock);
		return -1;
	}

	ssize_t len = read(sock, out_buf, out_buf_len - 1);
	close(sock);

	if (len <= 0) {
		printf("socket failed");
		return -1;
	}
	out_buf[len] = '\0';
	return 0;
}

int connect_to_hyprland_socket() {
	const char *xdg_runtime_dir = getenv("XDG_RUNTIME_DIR");
	const char *hyprland_instance = getenv("HYPRLAND_INSTANCE_SIGNATURE");

	if (!xdg_runtime_dir || !hyprland_instance) {
		fprintf(stderr, "Hyprland environment variables not set.\n");
		return -1;
	}

	char socket_path[512];
	snprintf(socket_path, sizeof(socket_path), "%s/hypr/%s/.socket2.sock",
			 xdg_runtime_dir, hyprland_instance);

	int sock = socket(AF_UNIX, SOCK_STREAM, 0);
	if (sock < 0) {
		perror("socket");
		return -1;
	}

	struct sockaddr_un addr = {0};
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path) - 1);

	if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		perror("connect");
		close(sock);
		return -1;
	}

	return sock;
}

typedef struct {
	WorkspaceState *workspace_state;
	char *workspace_name;
} WorkspaceStateAndNameArgs;

gboolean set_active_workspace_p(gpointer data) {
	WorkspaceStateAndNameArgs *args = data;

	GtkWidget *child =
		gtk_widget_get_first_child(args->workspace_state->workspace_box);

	while (child != NULL) {
		GtkWidget *next = gtk_widget_get_next_sibling(child);
		gtk_widget_remove_css_class(child, "active-workspace");
		child = next;
	}

	GtkWidget *button = find_button_by_label(
		args->workspace_state->workspace_box, args->workspace_name);
	if (!GTK_IS_WIDGET(button)) {
		return FALSE;
	}
	gtk_widget_set_vexpand(button, FALSE);
	gtk_widget_add_css_class(button, "active-workspace");
	return FALSE;
}
gboolean refresh_workspaces_p(gpointer data) {
	WorkspaceState *workspace_state = data;
	GtkWidget *child =
		gtk_widget_get_first_child(workspace_state->workspace_box);
	while (child != NULL) {
		GtkWidget *next = gtk_widget_get_next_sibling(child);
		gtk_widget_unparent(child); // removes it from the box
		child = next;
	}

	// Then re-add new buttons

	int workspaces = get_hyprland_workspace_count(workspace_state->bar);
	char **workspace_names = get_hyprland_workspace_names(workspace_state->bar);

	if (1 > workspaces) {
		printf("No workspaces\n");
		return FALSE;
	}

	for (int i = 0; i < workspaces; i++) {
		if (strcmp(workspace_names[i], "special:magic")) {
			GtkWidget *button = gtk_button_new_with_label(workspace_names[i]);

			gtk_widget_set_vexpand(button, FALSE);
			gtk_widget_set_halign(button, GTK_ALIGN_CENTER);

			gtk_widget_remove_css_class(button, "open-workspace");
			gtk_widget_add_css_class(button, "workspace");

			g_signal_connect(button, "clicked", G_CALLBACK(on_button_clicked),
							 workspace_names[i]);

			gtk_box_append(GTK_BOX(workspace_state->workspace_box), button);
		}
	}

	int open_workspace_count =
		get_hyprland_open_workspace_count(workspace_state->bar);
	char **open_workspace_names =
		get_hyprland_open_workspace_names(workspace_state->bar);

	for (int i = 0; i < open_workspace_count; i++) {

		GtkWidget *button = find_button_by_label(workspace_state->workspace_box,
												 open_workspace_names[i]);
		if (!GTK_IS_WIDGET(button)) {
			continue;
		}
		gtk_widget_remove_css_class(button, "workspace");
		gtk_widget_set_vexpand(button, FALSE);
		gtk_widget_add_css_class(button, "open-workspace");
	}

	return FALSE;
}
void listen_for_workspace_events(gpointer data) {
	WorkspaceState *workspace_state = data;
	int sock = connect_to_hyprland_socket(); // your socket connect logic
	if (sock < 0)
		return;

	send(sock, "j/clients", 9,
		 0); // optional: some protocols expect a handshake

	char focused_monitor[64] = "";
	char buffer[4096];
	char foc_workspace[256] = "";
	while (1) {
		ssize_t len = recv(sock, buffer, sizeof(buffer) - 1, 0);
		if (len <= 0) {
			break;
		}
		buffer[len] = '\0';

		char *bufline = strtok(buffer, "\n");
		while (bufline != NULL) {
			if (strncmp(bufline, "workspace>>", 11) == 0) {
				char *comma = strchr(bufline, '>');
				if (comma && comma[1] && comma[2] != '\0') {
					strcpy(foc_workspace, comma + 2);
				}
				WorkspaceStateAndNameArgs *args =
					calloc(1, sizeof(WorkspaceStateAndNameArgs));
				args->workspace_name = foc_workspace;
				args->workspace_state = workspace_state;
				g_idle_add((GSourceFunc)set_active_workspace_p, args);
			} else if (strncmp(bufline, "createworkspace>>", 17) == 0) {
				char *comma = strchr(bufline, '>');
				if (comma && comma[1] && comma[2] != '\0') {
					strcpy(foc_workspace, comma + 2);
				}
				WorkspaceStateAndNameArgs *args =
					calloc(1, sizeof(WorkspaceStateAndNameArgs));
				args->workspace_name = foc_workspace;
				args->workspace_state = workspace_state;
				g_idle_add((GSourceFunc)set_active_workspace_p, args);
				g_idle_add((GSourceFunc)refresh_workspaces_p, workspace_state);
			} else if (strncmp(bufline, "destroyworkspace>>", 18) == 0) {
				g_idle_add((GSourceFunc)refresh_workspaces_p, workspace_state);
				WorkspaceStateAndNameArgs *args =
					calloc(1, sizeof(WorkspaceStateAndNameArgs));
				args->workspace_name = foc_workspace;
				args->workspace_state = workspace_state;
				g_idle_add((GSourceFunc)set_active_workspace_p, args);
			} else if (strncmp(bufline, "focusedmon>>", 12) == 0) {
				snprintf(focused_monitor, sizeof(focused_monitor), "%s",
						 buffer + 12);

				char *line = strtok(buffer, "\n");
				while (line != NULL) {
					if (strncmp(line, "focusedmon>>", 12) == 0) {
						char *comma = strchr(line, ',');
						if (comma && comma[1] != '\0') {
							strcpy(foc_workspace, comma + 1);
						}
						break; // only care about the first matching line
					}
					line = strtok(NULL, "\n");
				}
				WorkspaceStateAndNameArgs *args =
					calloc(1, sizeof(WorkspaceStateAndNameArgs));
				args->workspace_name = foc_workspace;
				args->workspace_state = workspace_state;

				g_idle_add((GSourceFunc)set_active_workspace_p, args);
			}
			bufline = strtok(NULL, "\n");
		}
	}

	close(sock);
}

void on_button_clicked(GtkButton *button, gpointer user_data) {
	const char *workspace = (const char *)user_data;

	char cmd[64];
	snprintf(cmd, sizeof(cmd), "hyprctl dispatch workspace %s", workspace);
	system(cmd);
}

int get_hyprland_workspace_count(char *bar) {
	char buffer[8192] = {0};

	if (hyprland_ipc1_query("j/workspacerules", buffer, sizeof(buffer)) != 0)
		return -1;

	cJSON *root = cJSON_Parse(buffer);
	if (!root || !cJSON_IsArray(root)) {
		cJSON_Delete(root);
		return -1;
	}
	int count = 0;
	for (int i = 0; i < cJSON_GetArraySize(root); i++) {
		cJSON *item = cJSON_GetArrayItem(root, i);
		cJSON *name = cJSON_GetObjectItem(item, "workspaceString");
		cJSON *monitor = cJSON_GetObjectItem(item, "monitor");

		if (cJSON_IsString(name) && cJSON_IsString(monitor) == 1) {
			if (monitor->valuestring != NULL) {
				if (strcmp(monitor->valuestring, bar) == 0) {
					count += 1;
				}
			}
		}
	}

	cJSON_Delete(root);
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

char **get_hyprland_workspace_names(char *bar) {
	char buffer[8192] = {0};

	if (hyprland_ipc1_query("j/workspacerules", buffer, sizeof(buffer)) != 0)
		return NULL;

	cJSON *root = cJSON_Parse(buffer);
	if (!root || !cJSON_IsArray(root)) {
		cJSON_Delete(root);
		return NULL;
	}

	char **names = malloc(sizeof(char *) * (get_hyprland_workspace_count(bar)));
	if (!names) {
		cJSON_Delete(root);
		return NULL;
	}

	int name_index = 0;

	for (int i = 0; i < cJSON_GetArraySize(root); i++) {
		cJSON *item = cJSON_GetArrayItem(root, i);
		cJSON *name = cJSON_GetObjectItem(item, "workspaceString");
		cJSON *monitor = cJSON_GetObjectItem(item, "monitor");

		if (cJSON_IsString(name) && cJSON_IsString(monitor) == 1) {
			if (monitor->valuestring && bar != NULL) {
				if (strcmp(monitor->valuestring, bar) == 0) {
					names[name_index++] = strdup(name->valuestring);
				}
			}
		} else {
			names[name_index++] = strdup("unknown");
		}
	}

	cJSON_Delete(root);
	return names;
}
int get_hyprland_open_workspace_count(char *bar) {
	char buffer[8192] = {0};

	if (hyprland_ipc1_query("j/workspaces", buffer, sizeof(buffer)) != 0)
		return -1;

	cJSON *root = cJSON_Parse(buffer);
	if (!root || !cJSON_IsArray(root)) {
		cJSON_Delete(root);
		return -1;
	}

	int count = cJSON_GetArraySize(root);
	cJSON_Delete(root);
	return count;
}
char **get_hyprland_open_workspace_names(char *bar) {
	char buffer[8192] = {0};

	if (hyprland_ipc1_query("j/workspaces", buffer, sizeof(buffer)) != 0)
		return NULL;

	cJSON *root = cJSON_Parse(buffer);
	if (!root || !cJSON_IsArray(root)) {
		cJSON_Delete(root);
		return NULL;
	}

	int count = cJSON_GetArraySize(root);

	char **names = malloc(sizeof(char *) * count);
	if (!names) {
		cJSON_Delete(root);
		return NULL;
	}

	for (int i = 0; i < cJSON_GetArraySize(root); i++) {
		cJSON *item = cJSON_GetArrayItem(root, i);
		cJSON *name = cJSON_GetObjectItem(item, "name");

		if (cJSON_IsString(name)) {
			names[i] = strdup(name->valuestring);
		} else {
			names[i] = strdup("unknown");
		}
	}

	cJSON_Delete(root);
	return names;
}

void set_active_workspace(GtkWidget *box, const char *workspace_name,
						  WorkspaceState *workspace_state) {
	GtkWidget *child = gtk_widget_get_first_child(box);

	workspace_state->current_active_workspace = get_active_workspace_name();

	GtkWidget *button = find_button_by_label(box, workspace_name);

	if (!GTK_IS_WIDGET(button)) {
		return;
	}

	while (child != NULL) {
		GtkWidget *next = gtk_widget_get_next_sibling(child);

		gtk_widget_remove_css_class(child, "active-workspace");

		child = next;
	}

	gtk_widget_set_vexpand(button, FALSE);
	gtk_widget_add_css_class(button, "active-workspace");
}

char *get_active_workspace_name() {
	char buffer[4096] = {0};

	if (hyprland_ipc1_query("j/activeworkspace", buffer, sizeof(buffer)) != 0)
		return NULL;

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

void refresh_workspaces(WorkspaceState *workspace_state) {

	GtkWidget *child =
		gtk_widget_get_first_child(workspace_state->workspace_box);
	while (child != NULL) {
		GtkWidget *next = gtk_widget_get_next_sibling(child);
		gtk_widget_unparent(child); // removes it from the box
		child = next;
	}

	// Then re-add new buttons

	int workspaces = get_hyprland_workspace_count(workspace_state->bar);
	char **workspace_names = get_hyprland_workspace_names(workspace_state->bar);

	for (int i = 0; i < workspaces; i++) {
		if (strcmp(workspace_names[i], "special:magic")) {
			GtkWidget *button = gtk_button_new_with_label(workspace_names[i]);

			gtk_widget_set_vexpand(button, FALSE);
			gtk_widget_set_halign(button, GTK_ALIGN_CENTER);

			gtk_widget_add_css_class(button, "workspace");

			g_signal_connect(button, "clicked", G_CALLBACK(on_button_clicked),
							 workspace_names[i]);

			gtk_box_append(GTK_BOX(workspace_state->workspace_box), button);
		}
	}
}
gboolean workspace_refresh_wrapper(gpointer data) {
	// int workspace_count = get_hyprland_workspace_count();
	//  char **workspace_names = get_hyprland_workspace_names();
	//   printf("Current Workspace Count: %d\n", current_workspace_count);
	//   printf("New Workspace Count: %d\n", workspace_count);
	WorkspaceState *ctx = (WorkspaceState *)data;
	if (!ctx || !ctx->workspace_box) {
		fprintf(stderr, "NULL context or workspace_box\n");
		return FALSE;
	}
	// printf("Bar: %s\n", ctx->bar);

	//	if (!string_arrays_equal(ctx->current_workspace_names,
	//							 ctx->current_workspace_count, workspace_names,
	//							 workspace_count)) {
	//		refresh_workspaces(ctx);
	//	} else {
	//		// printf("equal\n");
	//	}

	char *active_name = get_active_workspace_name();

	// printf("active workspace %s\n", active_name);
	// printf("current active workspace %s\n", current_active_workspace);

	if (active_name &&
		(!ctx->current_active_workspace ||
		 strcmp(ctx->current_active_workspace, active_name) != 0)) {
		set_active_workspace(GTK_WIDGET(ctx->workspace_box), active_name, ctx);

		free(ctx->current_active_workspace);
		ctx->current_active_workspace = strdup(active_name);
	}

	return TRUE; // TRUE = keep repeating; FALSE = run once
}
