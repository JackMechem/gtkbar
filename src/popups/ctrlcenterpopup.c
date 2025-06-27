#include "ctrlcenterpopup.h"
#include "app.h"
#include "media.h"
#include "networking.h"
#include "volume.h"

// **
// ** UI Helpers
// **
GtkWidget *new_icon_button(char *icon_name) {
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_add_css_class(box, "icon-button");
    GtkWidget *icon = gtk_image_new_from_icon_name(icon_name);
    gtk_box_append(GTK_BOX(box), icon);
    gtk_image_set_pixel_size(GTK_IMAGE(icon), 18);
    return box;
}

GtkWidget *new_bottom_button(char *icon_name, char *text, GtkWidget *label) {
    GtkWidget *overlay = gtk_overlay_new();
    gtk_widget_add_css_class(overlay, "overlay");

    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_widget_add_css_class(box, "button");

    GtkWidget *logo = gtk_image_new_from_icon_name(icon_name);
    if (label != NULL) {
        gtk_label_set_text(GTK_LABEL(label), text);
    } else {
        label = gtk_label_new(text);
    }
    gtk_label_set_width_chars(GTK_LABEL(label), 10);
    gtk_label_set_xalign(GTK_LABEL(label), 0.0);
    gtk_label_set_ellipsize(GTK_LABEL(label), PANGO_ELLIPSIZE_END);
    gtk_widget_set_hexpand(label, FALSE);

    gtk_box_append(GTK_BOX(box), logo);
    gtk_box_append(GTK_BOX(box), label);
    gtk_overlay_set_child(GTK_OVERLAY(overlay), box);

    GtkWidget *overlay_button =
        gtk_button_new_from_icon_name("go-next-symbolic");
    gtk_widget_set_halign(overlay_button, GTK_ALIGN_END);
    gtk_widget_set_valign(overlay_button, GTK_ALIGN_CENTER);
    gtk_overlay_add_overlay(GTK_OVERLAY(overlay), overlay_button);

    gtk_widget_set_cursor_from_name(overlay, NULL);

    GtkEventController *motion = gtk_event_controller_motion_new();
    g_signal_connect(motion, "enter", G_CALLBACK(on_overlay_hover_enter),
                     overlay_button);
    g_signal_connect(motion, "leave", G_CALLBACK(on_overlay_hover_leave),
                     overlay_button);
    gtk_widget_add_controller(overlay, motion);

    return overlay;
}

// **
// ** Hover Events
// **
gboolean on_overlay_hover_enter(GtkEventController *controller,
                                gpointer user_data) {
    gtk_widget_add_css_class(GTK_WIDGET(user_data), "visible");
    GdkSurface *surface =
        gtk_native_get_surface(gtk_widget_get_native(GTK_WIDGET(user_data)));
    GdkCursor *cursor = gdk_cursor_new_from_name("pointer", NULL);
    gdk_surface_set_cursor(surface, cursor);
    return FALSE;
}

gboolean on_overlay_hover_leave(GtkEventController *controller,
                                gpointer user_data) {
    gtk_widget_remove_css_class(GTK_WIDGET(user_data), "visible");
    GdkSurface *surface =
        gtk_native_get_surface(gtk_widget_get_native(GTK_WIDGET(user_data)));
    GdkCursor *cursor = gdk_cursor_new_from_name("default", NULL);
    gdk_surface_set_cursor(surface, cursor);
    return FALSE;
}

// **
// ** Volume Controls
// **
gboolean volume_ref_wrap(gpointer user_data) {
    if (GTK_IS_WIDGET(user_data)) {
        if (prev_volume != get_volume_percentage()) {
            if (scaleChanging == FALSE) {
                volumeChanging = FALSE;
                int volume = get_volume_percentage();
                GtkWidget *volume_scale = user_data;
                gtk_range_set_value(GTK_RANGE(volume_scale), volume);
                prev_volume = volume;
                volumeChanging = TRUE;
            }
        }
        return TRUE;
    } else {
        return FALSE;
    }
}

void on_volume_scale_changed(GtkRange *range, gpointer user_data) {
    scaleChanging = TRUE;
    double value = gtk_range_get_value(range);
    if (volumeChanging == TRUE)
        set_volume((int)value);

    GtkWidget *label = user_data;
    int volume = get_volume_percentage();
    char text[32];
    if (volume >= 0)
        snprintf(text, sizeof(text), "%d%%", volume);
    else
        snprintf(text, sizeof(text), "--");

    gtk_label_set_text(GTK_LABEL(label), text);
    scaleChanging = FALSE;
}

// **
// ** Wi-Fi / Networking
// **
gboolean wifi_ref_wrap(gpointer user_data) {
    if (GTK_IS_WIDGET(user_data)) {
        GtkWidget *label = user_data;
        gtk_label_set_text(GTK_LABEL(label), get_wifi_name());
        return TRUE;
    } else {
        return FALSE;
    }
}

void on_stack_transition(GObject *stack, GParamSpec *pspec,
                         gpointer user_data) {
    gboolean running = gtk_stack_get_transition_running(GTK_STACK(stack));
    if (!running) {
        if (GTK_IS_BOX(user_data)) {
            GtkWidget *box = user_data;
            refresh_wifi_list(box);
            g_signal_handler_disconnect(stack, stacktransition_id);
            stacktransition_id = 0;
        }
    }
}

void on_wifi_clicked(GtkGestureClick *gesture, int n_press, double x, double y,
                     gpointer user_data) {
    WifiCLickedArgs *args = user_data;
    GtkStack *stack = GTK_STACK(args->stack);
    stacktransition_id =
        g_signal_connect(stack, "notify::transition-running",
                         G_CALLBACK(on_stack_transition), args->box);
    gtk_stack_set_visible_child_name(stack, "wifi");
    gtk_widget_set_size_request(args->wifiMenu, -1, 600);
}

void on_back_clicked(GtkGestureClick *gesture, int n_press, double x, double y,
                     gpointer user_data) {
    WifiCLickedArgs *args = user_data;
    GtkStack *stack = GTK_STACK(args->stack);
    gtk_widget_set_size_request(args->wifiMenu, -1, -1);
    gtk_stack_set_visible_child_name(stack, "page1");
    disconnect_handlers();
    clear_wifi_list(args->box);
}

// **
// ** Power Options
// **
void show_power_options(GtkEventController *controller, gpointer user_data) {
    GtkWidget *powerOptions = GTK_WIDGET(user_data);
    if (!GTK_IS_WIDGET(powerOptions))
        return;
    gtk_widget_set_visible(powerOptions, TRUE);
}

void hide_power_options(GtkEventController *controller, gpointer user_data) {
    GtkWidget *powerOptions = GTK_WIDGET(user_data);
    if (!GTK_IS_WIDGET(powerOptions))
        return;
    gtk_widget_set_visible(powerOptions, FALSE);
}

void on_sleep_clicked(GtkGestureClick *gesture, int n_press, double x, double y,
                      gpointer user_data) {
    if (system("systemctl suspend") != 0)
        perror("suspend failed");
}

void on_logout_clicked(GtkGestureClick *gesture, int n_press, double x,
                       double y, gpointer user_data) {
    if (system("loginctl terminate-session $XDG_SESSION_ID") != 0)
        perror("logout failed");
}

void on_restart_clicked(GtkGestureClick *gesture, int n_press, double x,
                        double y, gpointer user_data) {
    if (system("systemctl reboot") != 0)
        perror("reboot failed");
}

void on_power_clicked(GtkGestureClick *gesture, int n_press, double x, double y,
                      gpointer user_data) {
    if (system("systemctl poweroff") != 0)
        perror("shutdown failed");
}

// **
// ** Gesture Navigation
// **
void on_mixer_clicked(GtkGestureClick *gesture, int n_press, double x, double y,
                      gpointer user_data) {
    GtkStack *stack = user_data;
    gtk_stack_set_visible_child_name(stack, "mixer");
}

void on_bt_click(GtkGestureClick *gesture, int n_press, double x, double y,
                 gpointer user_data) {
    WifiCLickedArgs *args = user_data;
    gtk_widget_set_size_request(args->wifiMenu, -1, 500);
    gtk_stack_set_visible_child_name(args->stack, "bt");
}

// **
// ** Main Popup Launcher
// **
void ctrl_center_popup_toggle(GtkWidget *anchor) {
    if (!GTK_IS_WIDGET(anchor))
        return;

    if (popup_window) {
        g_source_remove(volume_timeout_id);
        g_source_remove(wifi_timeout_id);
        disconnect_handlers();
        unsubscribe_mpris_changes();
        wifi_timeout_id = 0;
        volume_timeout_id = 0;
        gtk_window_destroy(popup_window);
        popup_window = NULL;
        return;
    }

    popup_window = GTK_WINDOW(gtk_window_new());

    gtk_layer_init_for_window(GTK_WINDOW(popup_window));
    gtk_layer_set_keyboard_mode(GTK_WINDOW(popup_window),
                                GTK_LAYER_SHELL_KEYBOARD_MODE_ON_DEMAND);

    gtk_window_set_decorated(popup_window, FALSE);
    gtk_window_set_resizable(popup_window, FALSE);
    gtk_window_set_default_size(popup_window, POPUP_WIDTH, POPUP_HEIGHT);

    gtk_layer_set_layer(popup_window, GTK_LAYER_SHELL_LAYER_TOP);
    gtk_layer_auto_exclusive_zone_enable(popup_window);

    gtk_layer_set_anchor(popup_window, GTK_LAYER_SHELL_EDGE_TOP, TRUE);

    int pixels_from_right = get_distance(anchor, POPUP_WIDTH);

    if (pixels_from_right == -1) {
        gtk_layer_set_anchor(popup_window, GTK_LAYER_SHELL_EDGE_RIGHT, TRUE);
        gtk_layer_set_margin(popup_window, GTK_LAYER_SHELL_EDGE_RIGHT, 0);
    } else {
        gtk_layer_set_anchor(popup_window, GTK_LAYER_SHELL_EDGE_LEFT, TRUE);
        gtk_layer_set_margin(popup_window, GTK_LAYER_SHELL_EDGE_LEFT,
                             pixels_from_right);
    }

    GtkWidget *stack = gtk_stack_new();
    gtk_stack_set_transition_type(GTK_STACK(stack),
                                  GTK_STACK_TRANSITION_TYPE_SLIDE_LEFT_RIGHT);
    gtk_stack_set_transition_duration(GTK_STACK(stack), 300);

    GtkWidget *main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_add_css_class(main_box, "ctrlcenter");

    // Battery percentage (as a label)
    // GtkWidget *battery_label = gtk_label_new("🔋 36 %");
    // gtk_box_append(GTK_BOX(main_box), battery_label);

    GtkWidget *top_controls = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_widget_set_halign(top_controls, GTK_ALIGN_END);
    gtk_box_append(GTK_BOX(main_box), top_controls);

    GtkWidget *powerBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    GtkWidget *powerOptionsBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    GtkWidget *powerButton = new_icon_button("system-shutdown-symbolic");
    GtkWidget *sleepButton = new_icon_button("weather-clear-night-symbolic");
    GtkWidget *restartButton = new_icon_button("system-reboot-symbolic");
    GtkWidget *logoutButton = new_icon_button("system-log-out-rtl-symbolic");

    gtk_widget_add_css_class(powerBox, "power-box");

    gtk_box_append(GTK_BOX(powerOptionsBox), sleepButton);
    gtk_box_append(GTK_BOX(powerOptionsBox), logoutButton);
    gtk_box_append(GTK_BOX(powerOptionsBox), restartButton);

    GtkGesture *sleepClick = gtk_gesture_click_new();
    gtk_event_controller_set_propagation_phase(GTK_EVENT_CONTROLLER(sleepClick),
                                               GTK_PHASE_BUBBLE);
    gtk_widget_add_controller(sleepButton, GTK_EVENT_CONTROLLER(sleepClick));

    g_signal_connect(sleepClick, "released", G_CALLBACK(on_sleep_clicked),
                     NULL);

    GtkGesture *logoutClick = gtk_gesture_click_new();
    gtk_event_controller_set_propagation_phase(
        GTK_EVENT_CONTROLLER(logoutClick), GTK_PHASE_BUBBLE);
    gtk_widget_add_controller(logoutButton, GTK_EVENT_CONTROLLER(logoutClick));

    g_signal_connect(logoutClick, "released", G_CALLBACK(on_logout_clicked),
                     NULL);

    GtkGesture *restartClick = gtk_gesture_click_new();
    gtk_event_controller_set_propagation_phase(
        GTK_EVENT_CONTROLLER(restartClick), GTK_PHASE_BUBBLE);
    gtk_widget_add_controller(restartButton,
                              GTK_EVENT_CONTROLLER(restartClick));

    g_signal_connect(restartClick, "released", G_CALLBACK(on_restart_clicked),
                     NULL);

    GtkGesture *powerClick = gtk_gesture_click_new();
    gtk_event_controller_set_propagation_phase(GTK_EVENT_CONTROLLER(powerClick),
                                               GTK_PHASE_BUBBLE);
    gtk_widget_add_controller(powerButton, GTK_EVENT_CONTROLLER(powerClick));

    g_signal_connect(powerClick, "released", G_CALLBACK(on_power_clicked),
                     NULL);

    gtk_box_append(GTK_BOX(powerBox), powerOptionsBox);
    gtk_box_append(GTK_BOX(powerBox), powerButton);

    gtk_widget_set_focusable(powerButton, TRUE);

    GtkEventController *motion = gtk_event_controller_motion_new();
    g_signal_connect(motion, "enter", G_CALLBACK(show_power_options),
                     powerOptionsBox);
    g_signal_connect(motion, "leave", G_CALLBACK(hide_power_options),
                     powerOptionsBox);
    gtk_widget_add_controller(powerBox, motion);

    gtk_box_append(GTK_BOX(top_controls), powerBox);

    gtk_widget_set_visible(powerOptionsBox, FALSE);

    int volume = get_volume_percentage();
    GtkWidget *volumeBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_widget_add_css_class(volumeBox, "volume-box");
    GtkWidget *volumeImage =
        gtk_image_new_from_icon_name("audio-volume-high-symbolic");
    GtkWidget *volume_scale =
        gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, 100, 1);
    char volumetext[32];
    if (volume >= 0)
        snprintf(volumetext, sizeof(volumetext), "%d%%", volume);
    else
        snprintf(volumetext, sizeof(volumetext), "--");
    GtkWidget *volumeLabel = gtk_label_new(volumetext);
    gtk_widget_set_hexpand(volumeBox, TRUE);
    gtk_widget_set_hexpand(volume_scale, TRUE);
    gtk_scale_set_value_pos(GTK_SCALE(volume_scale), volume);
    gtk_range_set_value(GTK_RANGE(volume_scale),
                        (double)get_volume_percentage());

    gtk_widget_set_focusable(volume_scale, TRUE);
    gtk_widget_set_sensitive(volume_scale, TRUE);
    gtk_widget_set_can_target(volume_scale, TRUE);

    g_signal_connect(volume_scale, "value-changed",
                     G_CALLBACK(on_volume_scale_changed), volumeLabel);

    gtk_box_append(GTK_BOX(volumeBox), volumeImage);
    gtk_box_append(GTK_BOX(volumeBox), volume_scale);
    gtk_box_append(GTK_BOX(volumeBox), volumeLabel);
    gtk_box_append(GTK_BOX(main_box), volumeBox);

    GtkWidget *mediaBox = get_media_box();

    gtk_box_append(GTK_BOX(main_box), mediaBox);

    GtkWidget *rows = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_add_css_class(rows, "bottom-rows");
    GtkWidget *row1 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_append(GTK_BOX(rows), row1);
    gtk_widget_set_hexpand(row1, TRUE);

    GtkWidget *wifiLabel = gtk_label_new("");
    GtkWidget *wifiBox =
        new_bottom_button("network-wireless-signal-excellent-symbolic",
                          get_wifi_name(), wifiLabel);
    gtk_widget_set_hexpand(wifiBox, TRUE);
    gtk_box_append(GTK_BOX(row1), wifiBox);

    GtkWidget *mixerLabel = gtk_label_new("");
    GtkWidget *mixerButton = new_bottom_button("audio-x-generic-symbolic",
                                               "Audio Mixer", mixerLabel);

    gtk_widget_set_hexpand(mixerButton, TRUE);
    gtk_box_append(GTK_BOX(row1), mixerButton);

    GtkWidget *row2 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);

    GtkWidget *btlabel = gtk_label_new("");
    GtkWidget *btbox =
        new_bottom_button("bluetooth-symbolic", "Bluetooth", btlabel);
    gtk_widget_set_hexpand(btbox, TRUE);
    gtk_box_append(GTK_BOX(row2), btbox);

    GtkWidget *bluetoothLabel2 = gtk_label_new("");
    GtkWidget *bluetoothButton2 =
        new_bottom_button("bluetooth-symbolic", "Last Option", bluetoothLabel2);

    gtk_widget_set_hexpand(bluetoothButton2, TRUE);
    gtk_box_append(GTK_BOX(row2), bluetoothButton2);
    gtk_box_append(GTK_BOX(rows), row2);

    gtk_box_append(GTK_BOX(main_box), rows);

    GtkWidget *wifiMenu = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_add_css_class(wifiMenu, "ctrlcenter");
    GtkWidget *topButtons = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    GtkWidget *backButton = new_icon_button("go-previous-symbolic");
    gtk_widget_set_vexpand(topButtons, FALSE);
    gtk_box_append(GTK_BOX(topButtons), backButton);
    gtk_box_append(GTK_BOX(wifiMenu), topButtons);

    GtkWidget *scrolled = gtk_scrolled_window_new();
    gtk_widget_set_vexpand(scrolled, TRUE);
    gtk_widget_add_css_class(scrolled, "wifi-devices-scroll");
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_add_css_class(box, "wifi-devices-scroll-box");
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled), box);

    gtk_box_append(GTK_BOX(wifiMenu), scrolled);

    GtkWidget *btMenu = btBox(GTK_STACK(stack));
    gtk_stack_add_named(GTK_STACK(stack), main_box, "page1");
    gtk_stack_add_named(GTK_STACK(stack), wifiMenu, "wifi");
    gtk_stack_add_named(GTK_STACK(stack),
                        (GtkWidget *)mixerBox(GTK_STACK(stack)), "mixer");
    gtk_stack_add_named(GTK_STACK(stack), btMenu, "bt");
    gtk_stack_set_visible_child_name(GTK_STACK(stack), "page1");

    GtkGesture *click = gtk_gesture_click_new();
    gtk_event_controller_set_propagation_phase(GTK_EVENT_CONTROLLER(click),
                                               GTK_PHASE_BUBBLE);
    gtk_widget_add_controller(wifiBox, GTK_EVENT_CONTROLLER(click));

    WifiCLickedArgs *wifiClickedArgs = malloc(sizeof(WifiCLickedArgs));
    wifiClickedArgs->wifiMenu = wifiMenu;
    wifiClickedArgs->stack = GTK_STACK(stack);
    wifiClickedArgs->box = box;

    g_signal_connect(click, "released", G_CALLBACK(on_wifi_clicked),
                     wifiClickedArgs);

    GtkGesture *mixerClick = gtk_gesture_click_new();
    gtk_event_controller_set_propagation_phase(GTK_EVENT_CONTROLLER(mixerClick),
                                               GTK_PHASE_BUBBLE);
    gtk_widget_add_controller(mixerButton, GTK_EVENT_CONTROLLER(mixerClick));

    g_signal_connect(mixerClick, "released", G_CALLBACK(on_mixer_clicked),
                     stack);

    GtkGesture *btClick = gtk_gesture_click_new();
    gtk_event_controller_set_propagation_phase(GTK_EVENT_CONTROLLER(mixerClick),
                                               GTK_PHASE_BUBBLE);
    gtk_widget_add_controller(btbox, GTK_EVENT_CONTROLLER(btClick));

    WifiCLickedArgs *btClickArgs = malloc(sizeof(WifiCLickedArgs));
    btClickArgs->stack = GTK_STACK(stack);
    btClickArgs->wifiMenu = btMenu;

    g_signal_connect(btClick, "released", G_CALLBACK(on_bt_click), btClickArgs);

    GtkGesture *backClick = gtk_gesture_click_new();
    gtk_event_controller_set_propagation_phase(GTK_EVENT_CONTROLLER(backClick),
                                               GTK_PHASE_BUBBLE);
    gtk_widget_add_controller(backButton, GTK_EVENT_CONTROLLER(backClick));

    g_signal_connect(backClick, "released", G_CALLBACK(on_back_clicked),
                     wifiClickedArgs);

    gtk_widget_add_css_class(GTK_WIDGET(popup_window), "popup-window");
    gtk_window_set_child(popup_window, stack);

    gtk_window_present(popup_window);
    wifi_timeout_id = g_timeout_add(1000, wifi_ref_wrap, wifiLabel);
    volume_timeout_id = g_timeout_add(100, volume_ref_wrap, volume_scale);
}
