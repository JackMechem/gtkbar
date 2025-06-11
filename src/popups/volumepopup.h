#ifndef VOLUMEPOPUP_H
#define VOLUMEPOPUP_H

#include "app.h"
#include <gtk/gtk.h>
#include <gtk4-layer-shell/gtk4-layer-shell.h>

void create_volume_popup();
void show_volume_popup(int volume);
gboolean check_volume_change(gpointer user_data);

#endif
