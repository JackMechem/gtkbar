#ifndef APP_H
#define APP_H

#include "gdk/gdk.h"
#include "glib.h"
#include <cjson/cJSON.h>
#include <gtk/gtk.h>
#include <gtk4-layer-shell/gtk4-layer-shell.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

char *int_to_cstr(int in_int);

GtkWidget *find_button_by_label(GtkWidget *box, const char *target_label);

bool string_arrays_equal(char **a, int a_len, char **b, int b_len);

void on_button_clicked(GtkButton *button, gpointer user_data);

int get_hyprland_workspace_count();

int compare_workspace_names(const void *a, const void *b);

char **get_hyprland_workspace_names(int *count_out);

char *get_active_workspace_name();

void refresh_workspaces(GtkWidget *workspace_box);

int read_value(const char *path);

int get_battery_percentage();

char *int_to_cstr(int in_int);

GtkWidget *find_button_by_label(GtkWidget *box, const char *target_label);

void set_active_workspace(GtkWidget *box, const char *workspace_name);

bool string_arrays_equal(char **a, int a_len, char **b, int b_len);

char *get_wifi_name();

gboolean time_refresh_wrapper(gpointer user_time_box);

gboolean wifi_refresh_wrapper(gpointer user_wifi_label);

gboolean battery_refresh_wrapper(gpointer user_battery_label);

gboolean workspace_refresh_wrapper(gpointer user_workspace_box);

gboolean volume_refresh_wrapper(gpointer user_label);

int get_volume_percentage();

GtkWidget *create_volume_label();

#endif
