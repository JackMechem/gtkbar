#ifndef MEDIA_POPUP_H
#define MEDIA_POPUP_H

#include "media.h"
#include <gtk-4.0/gtk/gtk.h>
#include <gtk4-layer-shell/gtk4-layer-shell.h>

static GtkWidget *mediaPopup = NULL;
static guint timeout_id = 0;
static guint mpris_popup_props_id = 0;
static guint mpris_new_id = 0;

static PlayerInfo *player_info;


gboolean hide_media_popup(gpointer data);

void listen_media_changes();

void show_media_popup(GDBusConnection *conn, const gchar *sender,
                      const gchar *path, const gchar *iface,
                      const gchar *signal, GVariant *params,
                      gpointer user_data);

#endif
