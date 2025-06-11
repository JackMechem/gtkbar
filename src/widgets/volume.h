#ifndef VOLUME_H
#define VOLUME_H

#include <glib.h>
#include <gtk/gtk.h>

void set_volume(int value);
GPtrArray *get_output_devices(void);
void set_default_output_device(const char *name);
GtkWidget *create_output_device_dropdown(void);
GtkWidget *create_app_volume_box(void);

#endif
