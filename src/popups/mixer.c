#include "mixer.h"
#include "ctrlcenterpopup.h"
#include "gtk/gtk.h"
#include "gtk/gtkshortcut.h"
#include "volume.h"

void on_back_button_clicked(GtkGestureClick *gesture, int n_press, double x,
							double y, gpointer user_data) {
	GtkStack *stack = GTK_STACK(user_data);
	gtk_stack_set_visible_child_name(stack, "page1");
}

GtkWidget *mixerBox(GtkStack *stack) {
	GtkWidget *mainBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
	gtk_widget_add_css_class(mainBox, "ctrlcenter");

	GtkWidget *topBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);

	GtkWidget *backButton = new_icon_button("go-previous-symbolic");

	GtkGesture *backClick = gtk_gesture_click_new();
	gtk_event_controller_set_propagation_phase(GTK_EVENT_CONTROLLER(backClick),
											   GTK_PHASE_BUBBLE);
	gtk_widget_add_controller(backButton, GTK_EVENT_CONTROLLER(backClick));

	g_signal_connect(backClick, "released", G_CALLBACK(on_back_button_clicked),
					 stack);

	gtk_box_append(GTK_BOX(topBox), backButton);

	gtk_box_append(GTK_BOX(mainBox), topBox);

	GtkWidget *deviceDrop = create_output_device_dropdown();
	gtk_widget_add_css_class(deviceDrop, "device-drop");
	GtkWidget *appVols = create_app_volume_box();
	gtk_widget_add_css_class(appVols, "app-mixer");

	gtk_box_append(GTK_BOX(mainBox), deviceDrop);
	gtk_box_append(GTK_BOX(mainBox), appVols);

	return mainBox;
}
