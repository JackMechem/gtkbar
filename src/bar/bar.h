#include "calendarpopup.h"
#include "ctrlcenterpopup.h"
#include "gdk/gdk.h"
#include "gtk/gtk.h"
#include "volumepopup.h"

void on_callendar_button_clicked(GtkGestureClick *gesture, int n_press,
								 double x, double y, gpointer user_data);

void on_ctrl_box_clicked(GtkGestureClick *gesture, int n_press, double x,
						 double y, gpointer user_data);

int bar(int *argc, char *(*argv[]), GtkWidget *window, GdkMonitor *monitor);
