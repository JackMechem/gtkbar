#include "mediaPopup.h"
#include "gdk/gdk.h"
#include "gio/gio.h"
#include "glib-object.h"
#include "glib.h"
#include "gtk/gtk.h"
#include "gtk/gtkshortcut.h"
#include "media.h"
#include "pango/pango-layout.h"
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gio/gio.h>

gboolean hide_media_popup(gpointer data) {
    gtk_widget_set_visible(mediaPopup, FALSE);
    gtk_window_destroy(GTK_WINDOW(mediaPopup));
    mediaPopup = NULL;
    timeout_id = 0;
    return G_SOURCE_REMOVE;
}

// Callback for subscribe_new_mpris_players.
// Reconstructs media label
void name_owner_changed(GDBusConnection *conn, const gchar *sender_name,
                        const gchar *object_path, const gchar *interface_name,
                        const gchar *signal_name, GVariant *parameters,
                        gpointer user_data) {
    const gchar *name;
    const gchar *old_owner;
    const gchar *new_owner;

    g_variant_get(parameters, "(sss)", &name, &old_owner, &new_owner);

    if (g_str_has_prefix(name, "org.mpris.MediaPlayer2.")) {
        if (old_owner[0] == '\0' && new_owner[0] != '\0') {
            listen_media_changes();
        } else if (old_owner[0] != '\0' && new_owner[0] == '\0') {
            listen_media_changes();
        }
    }
}

// Runs on_name_owner_changed whenever there is an mpris player created or
// removed.
void sub_new_mpris_players(gpointer user_data) {
    mpris_new_id = g_dbus_connection_signal_subscribe(
        player_info->ses_bus, "org.freedesktop.DBus", "org.freedesktop.DBus",
        "NameOwnerChanged", "/org/freedesktop/DBus", NULL,
        G_DBUS_SIGNAL_FLAGS_NONE, name_owner_changed, NULL, NULL);
}

void listen_media_changes() {
    TrackInfo *t = now_playing_query();
    player_info = get_player_info();
    if (mpris_new_id == 0) {
        sub_new_mpris_players(NULL);
    }
    if (t) {
        if (!player_info->ses_bus || !player_info->cur_player) {
            return;
        }
        if (mpris_popup_props_id != 0) {
            g_dbus_connection_signal_unsubscribe(player_info->ses_bus,
                                                 mpris_popup_props_id);
        }
        mpris_popup_props_id = g_dbus_connection_signal_subscribe(
            player_info->ses_bus, player_info->cur_player,
            "org.freedesktop.DBus.Properties", "PropertiesChanged",
            "/org/mpris/MediaPlayer2", "org.mpris.MediaPlayer2.Player",
            G_DBUS_SIGNAL_FLAGS_NONE, show_media_popup, NULL, NULL);
    }
    now_playing_free(t);
}
GCancellable *imageCan;
GCancellable *imageCan2;
static void on_image_loaded(GObject *source, GAsyncResult *res,
                            gpointer user_data) {
    GFileInputStream *stream = g_file_read_finish(G_FILE(source), res, NULL);
    if (!stream)
        return;

    GdkPixbuf *pixbuf = gdk_pixbuf_new_from_stream_at_scale(
        G_INPUT_STREAM(stream), 128, 128, TRUE, imageCan2, NULL);
    GdkTexture *texture = gdk_texture_new_for_pixbuf(pixbuf);
    if (!texture) {
        g_object_unref(stream);
        return;
    }

    if (!user_data || !GTK_IS_IMAGE(user_data)) {
        return;
    }

    GtkImage *image = GTK_IMAGE(user_data);
    if (!image) {
        return;
    }
    gtk_image_set_from_paintable(image, GDK_PAINTABLE(texture));

    g_object_unref(texture);
    g_object_unref(stream);
}

GtkWidget *create_overlay() {
    mediaPopup = gtk_window_new();
    gtk_window_set_decorated(GTK_WINDOW(mediaPopup), FALSE);
    gtk_window_set_resizable(GTK_WINDOW(mediaPopup), FALSE);
    gtk_window_set_title(GTK_WINDOW(mediaPopup), "Media Overlay");
    gtk_window_set_default_size(GTK_WINDOW(mediaPopup), -1, -1);

    gtk_layer_init_for_window(GTK_WINDOW(mediaPopup));
    gtk_layer_set_layer(GTK_WINDOW(mediaPopup), GTK_LAYER_SHELL_LAYER_OVERLAY);
    gtk_layer_set_anchor(GTK_WINDOW(mediaPopup), GTK_LAYER_SHELL_EDGE_TOP,
                         TRUE);
    gtk_layer_set_anchor(GTK_WINDOW(mediaPopup), GTK_LAYER_SHELL_EDGE_RIGHT,
                         TRUE);

    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_path(provider, "../style.css");
    gtk_style_context_add_provider_for_display(
        gdk_display_get_default(), GTK_STYLE_PROVIDER(provider),
        GTK_STYLE_PROVIDER_PRIORITY_USER);

    gtk_widget_add_css_class(GTK_WIDGET(mediaPopup), "media-window");

    gtk_window_set_hide_on_close(GTK_WINDOW(mediaPopup), TRUE);
    gtk_window_set_modal(GTK_WINDOW(mediaPopup), FALSE);
    gtk_window_present(GTK_WINDOW(mediaPopup));

    TrackInfo *t = now_playing_query();

    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 15);
    gtk_window_set_child(GTK_WINDOW(mediaPopup), box);
    gtk_widget_add_css_class(box, "media-popup");

    // Prepare an image widget as a placeholder
    GtkWidget *image_frame = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_size_request(image_frame, 128, 128);
    gtk_widget_add_css_class(image_frame, "cover-frame");
    gtk_widget_set_overflow(image_frame, GTK_OVERFLOW_HIDDEN);

    GtkWidget *image = gtk_image_new();
    gtk_widget_set_size_request(image, 128, 128);
    gtk_widget_add_css_class(image, "cover-art");

    gtk_box_append(GTK_BOX(image_frame), image);
    gtk_box_append(GTK_BOX(box), image_frame);

    // Load from remote URL
    GFile *file = g_file_new_for_uri(t->art_url);
    g_file_read_async(file, G_PRIORITY_DEFAULT, imageCan, on_image_loaded,
                      image);
    g_object_unref(file);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);

    GtkWidget *title = gtk_label_new(t->title);
    gtk_widget_add_css_class(title, "title-label");
    gtk_label_set_wrap(GTK_LABEL(title), TRUE);
    gtk_label_set_wrap_mode(GTK_LABEL(title), PANGO_WRAP_WORD_CHAR);
    gtk_label_set_max_width_chars(GTK_LABEL(title), 20);
    gtk_label_set_lines(GTK_LABEL(title), 2); // GTK4 only
    gtk_label_set_ellipsize(GTK_LABEL(title), PANGO_ELLIPSIZE_END);
    gtk_label_set_xalign(GTK_LABEL(title), 0.0);

    GtkWidget *album = gtk_label_new(t->album);
    gtk_widget_add_css_class(album, "album-label");
    gtk_label_set_wrap(GTK_LABEL(album), TRUE);
    gtk_label_set_wrap_mode(GTK_LABEL(album), PANGO_WRAP_WORD_CHAR);
    gtk_label_set_max_width_chars(GTK_LABEL(album), 15);
    gtk_label_set_lines(GTK_LABEL(album), 2);
    gtk_label_set_ellipsize(GTK_LABEL(album), PANGO_ELLIPSIZE_END);
    gtk_label_set_xalign(GTK_LABEL(album), 0.0);
    gtk_widget_set_vexpand(album, TRUE);

    GtkWidget *artist = gtk_label_new(t->artist);
    gtk_widget_add_css_class(artist, "artist-label");
    gtk_label_set_wrap(GTK_LABEL(artist), TRUE);
    gtk_label_set_wrap_mode(GTK_LABEL(artist), PANGO_WRAP_WORD_CHAR);
    gtk_label_set_max_width_chars(GTK_LABEL(artist), 15);
    gtk_label_set_lines(GTK_LABEL(artist), 2);
    gtk_label_set_ellipsize(GTK_LABEL(artist), PANGO_ELLIPSIZE_END);
    gtk_label_set_xalign(GTK_LABEL(artist), 0.0);
    gtk_widget_set_vexpand(vbox, TRUE);
    gtk_widget_set_valign(vbox, GTK_ALIGN_CENTER);
    gtk_box_append(GTK_BOX(vbox), title);
    gtk_box_append(GTK_BOX(vbox), album);
    gtk_box_append(GTK_BOX(vbox), artist);
    gtk_box_append(GTK_BOX(box), vbox);

    now_playing_free(t);
    return GTK_WIDGET(mediaPopup);
}

void show_media_popup(GDBusConnection *conn, const gchar *sender,
                      const gchar *path, const gchar *iface,
                      const gchar *signal, GVariant *params,
                      gpointer user_data) {
    gchar *prop_iface;
    GVariant *changed;
    GVariant *invalid;
    // g_message("popup triggered");
    g_variant_get(params, "(s@a{sv}@as)", &prop_iface, &changed, &invalid);
    if (g_strcmp0(prop_iface, "org.mpris.MediaPlayer2.Player") == 0) {
        if (g_variant_lookup_value(changed, "Metadata",
                                   G_VARIANT_TYPE("a{sv}"))) {
            if (!g_cancellable_is_cancelled(imageCan)) {
                g_cancellable_cancel(imageCan2);
                g_cancellable_cancel(imageCan);
                // g_message("canceled");
            }
            if (mediaPopup) {
                gtk_widget_set_visible(mediaPopup, FALSE);
            }
            mediaPopup = create_overlay();
        }

        // Reset timer
        if (timeout_id > 0)
            g_source_remove(timeout_id);
        timeout_id = g_timeout_add(3000, hide_media_popup, NULL);
    }
    g_free(prop_iface);
    g_variant_unref(changed);
    g_variant_unref(invalid);
}
