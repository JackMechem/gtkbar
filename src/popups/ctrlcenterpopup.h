#ifndef CTRLCENTERPOPUP_H
#define CTRLCENTERPOPUP_H

#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

#include <glib-object.h>
#include <glib.h>

#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <gtk/gtkshortcut.h>
#include <gtk4-layer-shell.h>

#include <NetworkManager.h>
#include <graphene-gobject.h>
#include <nm-utils.h>

#include "bt.h"
#include "mixer.h"
#include "popupposition.h"

// Globals
static GtkWindow *popup_window = NULL;
static const int POPUP_WIDTH = 350;
static const int POPUP_HEIGHT = 100;
static guint volume_timeout_id = 0;
static guint stacktransition_id = 0;
static gpointer devInst;
static int prev_volume = -1;
static gboolean volumeChanging = FALSE;
static gboolean scaleChanging = FALSE;

// Core Popup
void ctrl_center_popup_toggle(GtkWidget *anchor);

// UI Helpers
GtkWidget *new_icon_button(char *icon_name);
GtkWidget *new_bottom_button(char *icon_name, char *text, GtkWidget *label);

// Hover Events
gboolean on_overlay_hover_enter(GtkEventController *controller,
                                gpointer user_data);
gboolean on_overlay_hover_leave(GtkEventController *controller,
                                gpointer user_data);

// Volume Controls
gboolean volume_ref_wrap(gpointer user_data);
void on_volume_scale_changed(GtkRange *range, gpointer user_data);

// Wi-Fi / Network Handling
gboolean wifi_ref_wrap(gpointer user_data);
void on_wifi_clicked(GtkGestureClick *gesture, int n_press, double x, double y,
                     gpointer user_data);
void on_back_clicked(GtkGestureClick *gesture, int n_press, double x, double y,
                     gpointer user_data);
void on_stack_transition(GObject *stack, GParamSpec *pspec, gpointer user_data);

// Power Options
void show_power_options(GtkEventController *controller, gpointer user_data);
void hide_power_options(GtkEventController *controller, gpointer user_data);
void on_sleep_clicked(GtkGestureClick *gesture, int n_press, double x, double y,
                      gpointer user_data);
void on_logout_clicked(GtkGestureClick *gesture, int n_press, double x,
                       double y, gpointer user_data);
void on_restart_clicked(GtkGestureClick *gesture, int n_press, double x,
                        double y, gpointer user_data);
void on_power_clicked(GtkGestureClick *gesture, int n_press, double x, double y,
                      gpointer user_data);

// Dialogs
void on_response(GtkDialog *dialog, int response_id, gpointer user_data);
void show_confirm_dialog(GtkWindow *parent);

#endif // CTRLCENTERPOPUP_H
