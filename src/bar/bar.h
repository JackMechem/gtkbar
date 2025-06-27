#include <gdk/gdk.h>
#include <gtk-4.0/gtk/gtk.h>


// UI Callbacks

void on_callendar_button_clicked(GtkGestureClick *gesture, int n_press,
                                 double x, double y, gpointer user_data);

void on_ctrl_box_clicked(GtkGestureClick *gesture, int n_press, double x,
                         double y, gpointer user_data);


// Main Bar Function

int bar(int *argc, char *(*argv[]), GtkWidget *window, GdkMonitor *monitor);
