#ifndef CALENDARPOPUP_H
#define CALENDARPOPUP_H

#include <gtk/gtk.h>
#include <gtk4-layer-shell.h>

// Call this function to toggle the calendar popup.
// Pass in a reference widget if you want it positioned near it (future
// expansion).
void calendar_popup_toggle(GtkWidget *anchor);

#endif
