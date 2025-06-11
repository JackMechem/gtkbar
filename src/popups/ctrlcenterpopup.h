#ifndef CTRLCENTERPOPUP_H
#define CTRLCENTERPOPUP_H

#include "glib-object.h"
#include "glib.h"
#include "gtk/gtk.h"
#include "popupposition.h"
#include "volume.h"
#include <NetworkManager.h>

#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <gtk4-layer-shell.h>
#include <nm-utils.h>

#include <graphene-gobject.h>

void ctrl_center_popup_toggle(GtkWidget *anchor);

GtkWidget *new_icon_button(char *icon_name);

gboolean on_overlay_hover_enter(GtkEventController *controller,
								gpointer user_data);

gboolean on_overlay_hover_leave(GtkEventController *controller,
								gpointer user_data);

gboolean wifi_ref_wrap(gpointer user_data);

gboolean wifi_ref_wrap(gpointer user_data);

void on_click_begin(GtkGestureClick *gesture, double start_x, double start_y,
					gpointer user_data);

void on_click_end(GtkGestureClick *gesture, double offset_x, double offset_y,
				  gpointer user_data);

gboolean volume_ref_wrap(gpointer user_data);

void on_volume_scale_changed(GtkRange *range, gpointer user_data);

void on_response(GtkDialog *dialog, int response_id, gpointer user_data);

void show_confirm_dialog(GtkWindow *parent);

void show_power_options(GtkEventController *controller, gpointer user_data);

void hide_power_options(GtkEventController *controller, gpointer user_data);

void on_wifi_clicked(GtkGestureClick *gesture, int n_press, double x, double y,
					 gpointer user_data);

void on_back_clicked(GtkGestureClick *gesture, int n_press, double x, double y,
					 gpointer user_data);

#endif
