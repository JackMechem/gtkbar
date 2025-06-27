#include "media.h"
#include "gio/gio.h"
#include "glib-object.h"
#include "glib.h"

/*
 * Build, Update, And Clean UIs
 */

// Build UI for media player
GtkWidget *get_media_box(void) {
    mediaBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_add_css_class(mediaBox, "mediaBox");
    TrackInfo *t = now_playing_query();
    if (t) {
        cover_pic = GTK_PICTURE(gtk_picture_new());
        gtk_widget_set_hexpand(GTK_WIDGET(cover_pic), TRUE);
        gtk_widget_set_vexpand(GTK_WIDGET(cover_pic), TRUE);
        gtk_picture_set_content_fit(cover_pic, GTK_CONTENT_FIT_COVER);
        gtk_widget_set_size_request(GTK_WIDGET(cover_pic), -1, 285);
        gtk_box_append(GTK_BOX(mediaBox), GTK_WIDGET(cover_pic));
        load_cover_async(t->art_url);
        GtkWidget *title = gtk_label_new(t->title);
        gtk_widget_add_css_class(title, "title-label");
        gtk_label_set_wrap(GTK_LABEL(title), TRUE);
        gtk_label_set_wrap_mode(GTK_LABEL(title), PANGO_WRAP_WORD_CHAR);
        gtk_label_set_justify(GTK_LABEL(title), GTK_JUSTIFY_CENTER);
        gtk_box_append(GTK_BOX(mediaBox), title);
        GtkWidget *album = gtk_label_new(t->album);
        gtk_widget_add_css_class(album, "album-label");
        gtk_label_set_wrap(GTK_LABEL(album), TRUE);
        gtk_label_set_wrap_mode(GTK_LABEL(album), PANGO_WRAP_WORD_CHAR);
        gtk_label_set_justify(GTK_LABEL(album), GTK_JUSTIFY_CENTER);
        gtk_box_append(GTK_BOX(mediaBox), album);
        GtkWidget *artist = gtk_label_new(t->artist);
        gtk_widget_add_css_class(artist, "artist-label");
        gtk_label_set_wrap(GTK_LABEL(artist), TRUE);
        gtk_label_set_wrap_mode(GTK_LABEL(artist), PANGO_WRAP_WORD_CHAR);
        gtk_label_set_justify(GTK_LABEL(artist), GTK_JUSTIFY_CENTER);
        gtk_box_append(GTK_BOX(mediaBox), artist);
        subscribe_mpris_changes();
        track_length = t->length;
        song_progress = GTK_PROGRESS_BAR(gtk_progress_bar_new());
        track_current_position_label = GTK_LABEL(gtk_label_new("--"));
        track_duration_position_label = GTK_LABEL(gtk_label_new("--"));
        GtkWidget *time_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
        GtkWidget *spacer = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
        gtk_widget_set_hexpand(spacer, TRUE);
        gtk_box_append(GTK_BOX(time_box),
                       GTK_WIDGET(track_current_position_label));
        gtk_box_append(GTK_BOX(time_box), spacer);
        gtk_box_append(GTK_BOX(time_box),
                       GTK_WIDGET(track_duration_position_label));
        gtk_widget_set_name(GTK_WIDGET(song_progress), "media-progress");
        gtk_box_append(GTK_BOX(mediaBox), GTK_WIDGET(song_progress));
        gtk_box_append(GTK_BOX(mediaBox), time_box);
        if (position_timer_id == 0)
            position_timer_id =
                g_timeout_add(500, subscribe_to_position_changes, NULL);
        subscribe_to_position_changes(NULL);
        GtkWidget *ctrls = get_media_controls();
        gtk_box_append(GTK_BOX(mediaBox), ctrls);
    } else {
        g_print("No MPRIS player active\n");
    }
    now_playing_free(t);
    return mediaBox;
}

// Update UI for media player
void update_media_box(void) {
    if (!mediaBox)
        return;
    clear_cover_pic();
    GtkWidget *child = gtk_widget_get_first_child(mediaBox);
    while (child) {
        GtkWidget *next = gtk_widget_get_next_sibling(child);
        g_object_ref_sink(child);
        g_object_unref(child);
        gtk_widget_unparent(child);
        child = next;
    }
    TrackInfo *t = now_playing_query();
    if (!t) {
        g_print("No MPRIS player active\n");
        now_playing_free(t);
        return;
    }
    cover_pic = GTK_PICTURE(gtk_picture_new());
    gtk_widget_set_hexpand(GTK_WIDGET(cover_pic), TRUE);
    gtk_widget_set_vexpand(GTK_WIDGET(cover_pic), TRUE);
    gtk_picture_set_content_fit(cover_pic, GTK_CONTENT_FIT_COVER);
    gtk_widget_set_size_request(GTK_WIDGET(cover_pic), -1, 285);
    gtk_box_append(GTK_BOX(mediaBox), GTK_WIDGET(cover_pic));
    load_cover_async(t->art_url);
    GtkWidget *title = gtk_label_new(t->title);
    gtk_widget_add_css_class(title, "title-label");
    gtk_label_set_wrap(GTK_LABEL(title), TRUE);
    gtk_label_set_wrap_mode(GTK_LABEL(title), PANGO_WRAP_WORD_CHAR);
    gtk_label_set_justify(GTK_LABEL(title), GTK_JUSTIFY_CENTER);
    gtk_box_append(GTK_BOX(mediaBox), title);
    GtkWidget *album = gtk_label_new(t->album);
    gtk_widget_add_css_class(album, "album-label");
    gtk_label_set_wrap(GTK_LABEL(album), TRUE);
    gtk_label_set_wrap_mode(GTK_LABEL(album), PANGO_WRAP_WORD_CHAR);
    gtk_label_set_justify(GTK_LABEL(album), GTK_JUSTIFY_CENTER);
    gtk_box_append(GTK_BOX(mediaBox), album);
    GtkWidget *artist = gtk_label_new(t->artist);
    gtk_widget_add_css_class(artist, "artist-label");
    gtk_label_set_wrap(GTK_LABEL(artist), TRUE);
    gtk_label_set_wrap_mode(GTK_LABEL(artist), PANGO_WRAP_WORD_CHAR);
    gtk_label_set_justify(GTK_LABEL(artist), GTK_JUSTIFY_CENTER);
    gtk_box_append(GTK_BOX(mediaBox), artist);
    subscribe_mpris_changes();
    track_length = t->length;
    song_progress = GTK_PROGRESS_BAR(gtk_progress_bar_new());
    track_current_position_label = GTK_LABEL(gtk_label_new("--"));
    track_duration_position_label = GTK_LABEL(gtk_label_new("--"));
    GtkWidget *time_box2 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    GtkWidget *spacer2 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_hexpand(spacer2, TRUE);
    gtk_box_append(GTK_BOX(time_box2),
                   GTK_WIDGET(track_current_position_label));
    gtk_box_append(GTK_BOX(time_box2), spacer2);
    gtk_box_append(GTK_BOX(time_box2),
                   GTK_WIDGET(track_duration_position_label));
    gtk_widget_set_name(GTK_WIDGET(song_progress), "media-progress");
    gtk_box_append(GTK_BOX(mediaBox), GTK_WIDGET(song_progress));
    gtk_box_append(GTK_BOX(mediaBox), time_box2);
    if (position_timer_id == 0)
        position_timer_id =
            g_timeout_add(500, subscribe_to_position_changes, NULL);
    subscribe_to_position_changes(NULL);
    GtkWidget *ctrls2 = get_media_controls();
    gtk_box_append(GTK_BOX(mediaBox), ctrls2);
    now_playing_free(t);
}

// Build UI for media label
GtkWidget *get_media_label(gpointer user_data) {
    GtkWidget *mediaLabel = user_data;
    GtkWidget *mediaLabelLabel = gtk_label_new("--");
    GtkWidget *icon = gtk_image_new_from_icon_name("audio-x-generic-symbolic");
    TrackInfo *t = now_playing_query();
    if (name_owner_sub_id != 0) {
        g_message("Unscubscribed to name_owner_sub_id");
    }
    if (t) {
        char buffer[128];
        sprintf(buffer, "%s - %s", t->title, t->artist);
        gtk_label_set_text(GTK_LABEL(mediaLabelLabel), buffer);
        gtk_box_append(GTK_BOX(mediaLabel), icon);
        gtk_box_append(GTK_BOX(mediaLabel), mediaLabelLabel);
        subscribe_mpris_changes_label(user_data);
    } else {
        g_print("No MPRIS player active\n");
    }
    subscribe_new_mpris_players(user_data);
    now_playing_free(t);

    return mediaLabel;
}

// Update UI for media label
void update_media_label(gpointer user_data) {
    GtkWidget *mediaLabel = GTK_WIDGET(user_data);
    GtkWidget *mediaLabelLabel = gtk_label_new("");
    GtkWidget *icon = gtk_image_new_from_icon_name("audio-x-generic-symbolic");
    GtkWidget *child = gtk_widget_get_first_child(mediaLabel);
    while (child) {
        GtkWidget *next = gtk_widget_get_next_sibling(child);
        g_object_ref_sink(child);
        g_object_unref(child);
        gtk_widget_unparent(child);
        child = next;
    }
    TrackInfo *t = now_playing_query();
    if (t) {
        char buffer[128];
        sprintf(buffer, "%s - %s", t->title, t->artist);
        gtk_label_set_text(GTK_LABEL(mediaLabelLabel), buffer);
        gtk_box_append(GTK_BOX(mediaLabel), icon);
        gtk_box_append(GTK_BOX(mediaLabel), mediaLabelLabel);
        g_message("updating %s", t->title);
    } else {
        g_print("No MPRIS player active\n");
    }
    now_playing_free(t);
}

// Cleanup UI for media label
void cleanup_media_label(gpointer user_data) {
    GtkWidget *mediaLabel = user_data;
    GtkWidget *child = gtk_widget_get_first_child(mediaLabel);
    while (child) {
        GtkWidget *next = gtk_widget_get_next_sibling(child);
        g_object_ref_sink(child);
        g_object_unref(child);
        gtk_widget_unparent(child);
        child = next;
    }
}

// Build UI for media controls
GtkWidget *get_media_controls(void) {
    GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_widget_set_hexpand(hbox, TRUE);
    GtkWidget *btn_prev =
        gtk_button_new_from_icon_name("media-skip-backward-symbolic");
    g_signal_connect(btn_prev, "clicked", G_CALLBACK(on_prev_clicked), NULL);
    gtk_box_append(GTK_BOX(hbox), btn_prev);
    GtkWidget *playBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_hexpand(playBox, TRUE);
    play_button =
        gtk_button_new_from_icon_name("media-playback-start-symbolic");
    g_signal_connect(play_button, "clicked", G_CALLBACK(on_playpause_clicked),
                     NULL);
    gtk_box_append(GTK_BOX(playBox), play_button);
    gtk_widget_set_halign(playBox, GTK_ALIGN_CENTER);
    gtk_box_append(GTK_BOX(hbox), playBox);
    GtkWidget *btn_next =
        gtk_button_new_from_icon_name("media-skip-forward-symbolic");
    g_signal_connect(btn_next, "clicked", G_CALLBACK(on_next_clicked), NULL);
    gtk_box_append(GTK_BOX(hbox), btn_next);
    subscribe_to_playpause_changes();
    gtk_widget_add_css_class(hbox, "media-controls");
    return hbox;
}

/*
 * UI Callbacks
 */

void on_prev_clicked(GtkButton *_btn, gpointer _) {
    send_mpris_command("Previous");
}
void on_playpause_clicked(GtkButton *_btn, gpointer _) {
    send_mpris_command("PlayPause");
}
void on_next_clicked(GtkButton *_btn, gpointer _) {
    send_mpris_command("Next");
}

/*
 * Cover Photo
 */

void clear_cover_pic(void) {
    if (!cover_pic)
        return;
    GdkPaintable *old = gtk_picture_get_paintable(cover_pic);
    if (old) {
        gtk_picture_set_paintable(cover_pic, NULL);
    }
}

void cover_load_thread(GTask *task, gpointer source_object, gpointer task_data,
                       GCancellable *cancellable) {
    const char *uri = task_data;
    GError *err = NULL;

    GFile *file = g_file_new_for_uri(uri);

    GFileInputStream *stream =
        G_FILE_INPUT_STREAM(g_file_read(file, cancellable, &err));
    if (!stream) {
        g_task_return_error(task, err);
        return;
    }
    g_object_unref(file);

    GdkPixbuf *pixbuf = gdk_pixbuf_new_from_stream_at_scale(
        G_INPUT_STREAM(stream), 256, 256,
        TRUE,
        cancellable, &err);
    g_object_unref(stream);

    if (!pixbuf) {
        g_task_return_error(task, err);
        return;
    }

    GdkTexture *tex = gdk_texture_new_for_pixbuf(pixbuf);
    g_object_unref(pixbuf);

    g_task_return_pointer(task, tex, g_object_unref);
}

void on_cover_loaded(GObject *source_object, GAsyncResult *res,
                     gpointer user_data) {
    clear_cover_pic();
    GTask *task = G_TASK(res);
    GError *err = NULL;
    GtkPicture *pic = GTK_PICTURE(user_data);
    GdkTexture *tex = g_task_propagate_pointer(task, &err);
    if (err) {
        g_warning("Cover load failed: %s", err->message);
        g_clear_error(&err);
    } else {
        gtk_picture_set_paintable(pic, GDK_PAINTABLE(tex));
    }
    if (tex)
        g_object_unref(tex);
    g_object_unref(task);
}

void load_cover_async(const char *uri) {
    if (cover_cancellable) {
        g_cancellable_cancel(cover_cancellable);
        g_clear_object(&cover_cancellable);
    }
    cover_cancellable = g_cancellable_new();
    GTask *t = g_task_new(NULL, cover_cancellable, on_cover_loaded, cover_pic);
    g_task_set_task_data(t, g_strdup(uri), g_free);
    g_task_run_in_thread(t, cover_load_thread);
}

/*
 * Query now playing track information
 */

TrackInfo *now_playing_query(void) {
    GError *err = NULL;
    if (!session_bus) {
        session_bus = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, &err);
        if (!session_bus) {
            g_printerr("[ERROR] g_bus_get_sync: %s\n", err->message);
            g_clear_error(&err);
            return NULL;
        }
        g_object_ref_sink(session_bus);
    }
    GVariant *reply = g_dbus_connection_call_sync(
        session_bus, "org.freedesktop.DBus", "/org/freedesktop/DBus",
        "org.freedesktop.DBus", "ListNames", NULL, G_VARIANT_TYPE("(as)"),
        G_DBUS_CALL_FLAGS_NONE, -1, NULL, &err);
    if (!reply) {
        g_printerr("[ERROR] ListNames failed: %s\n", err->message);
        g_clear_error(&err);
        return NULL;
    }
    GVariant *names_array = g_variant_get_child_value(reply, 0);
    g_variant_unref(reply);
    gchar *player = NULL;
    {
        GVariantIter it;
        const gchar *name;
        g_variant_iter_init(&it, names_array);
        while (!player && g_variant_iter_next(&it, "s", &name)) {
            if (g_str_has_prefix(name, "org.mpris.MediaPlayer2."))
                player = g_strdup(name);
        }
    }
    g_variant_unref(names_array);
    if (!player)
        return NULL;
    if (current_player)
        g_free(current_player);
    current_player = player;
    GVariant *prop = g_dbus_connection_call_sync(
        session_bus, current_player, "/org/mpris/MediaPlayer2",
        "org.freedesktop.DBus.Properties", "Get",
        g_variant_new("(ss)", "org.mpris.MediaPlayer2.Player", "Metadata"),
        G_VARIANT_TYPE("(v)"), G_DBUS_CALL_FLAGS_NONE, -1, NULL, &err);
    if (!prop) {
        g_printerr("[ERROR] Get(Metadata) failed: %s\n", err->message);
        g_clear_error(&err);
        return NULL;
    }
    GVariant *inner = g_variant_get_child_value(prop, 0);
    g_variant_unref(prop);
    GVariant *map = g_variant_get_variant(inner);
    g_variant_unref(inner);
    TrackInfo *info = g_new0(TrackInfo, 1);
    info->service = g_strdup(current_player);
    GVariantIter dict;
    const gchar *key;
    GVariant *val;
    g_variant_iter_init(&dict, map);
    while (g_variant_iter_loop(&dict, "{&sv}", &key, &val)) {
        if (g_strcmp0(key, "xesam:title") == 0)
            info->title = g_variant_dup_string(val, NULL);
        else if (g_strcmp0(key, "xesam:artist") == 0) {
            GVariantIter ai;
            const gchar *a;
            g_variant_iter_init(&ai, val);
            if (g_variant_iter_next(&ai, "s", &a))
                info->artist = g_strdup(a);
        } else if (g_strcmp0(key, "xesam:album") == 0)
            info->album = g_variant_dup_string(val, NULL);
        else if (g_strcmp0(key, "mpris:artUrl") == 0)
            info->art_url = g_variant_dup_string(val, NULL);
        else if (g_strcmp0(key, "mpris:length") == 0)
            info->length = g_variant_get_uint64(val);
    }
    g_variant_unref(map);
    return info;
}

void now_playing_free(TrackInfo *info) {
    if (!info)
        return;
    g_free(info->service);
    g_free(info->title);
    g_free(info->artist);
    g_free(info->album);
    g_free(info->art_url);
    g_free(info);
}

/*
 * Subscribe to MPRIS Changes
 */

void subscribe_mpris_changes(void) {
    if (!session_bus || !current_player || mpris_props_id)
        return;
    mpris_props_id = g_dbus_connection_signal_subscribe(
        session_bus, current_player, "org.freedesktop.DBus.Properties",
        "PropertiesChanged", "/org/mpris/MediaPlayer2",
        "org.mpris.MediaPlayer2.Player", G_DBUS_SIGNAL_FLAGS_NONE,
        on_mpris_properties_changed, NULL, NULL);
}

void subscribe_mpris_changes_label(gpointer user_data) {
    if (!session_bus || !current_player)
        return;
    g_message("subscribe mpris changes");
    label_mpris_props_id = g_dbus_connection_signal_subscribe(
        session_bus, current_player, "org.freedesktop.DBus.Properties",
        "PropertiesChanged", "/org/mpris/MediaPlayer2",
        "org.mpris.MediaPlayer2.Player", G_DBUS_SIGNAL_FLAGS_NONE,
        on_mpris_properties_changed_label, user_data, NULL);
}

void unsubscribe_mpris_changes(void) {
    if (cover_cancellable) {
        g_cancellable_cancel(cover_cancellable);
        g_clear_object(&cover_cancellable);
    }
    // Stop D-Bus signals
    if (mpris_props_id != 0) {
        g_dbus_connection_signal_unsubscribe(session_bus, mpris_props_id);
        mpris_props_id = 0;
    }
    // Stop the position timer
    if (position_timer_id != 0) {
        g_source_remove(position_timer_id);
        position_timer_id = 0;
    }
    if (name_owner_sub_id != 0) {
        g_dbus_connection_signal_unsubscribe(session_bus, name_owner_sub_id);
        name_owner_sub_id = 0;
    }
    if (!cover_pic) {
        g_warning("coverpic null");
    }
    // Clear cover art
    clear_cover_pic();
    // Destroy the media box and all children
    if (session_bus) {
        g_object_unref(session_bus);
        session_bus = NULL;
    }
    if (current_player) {
        g_free(current_player);
        current_player = NULL;
    }
    // Reset UI pointers
    play_button = NULL;
    song_progress = NULL;
    track_current_position_label = NULL;
    track_duration_position_label = NULL;
    track_length = 0;

    if (malloc_trim(0) == 0) {
        g_error("malloc_trim fail");
    }
    mallopt(M_TRIM_THRESHOLD, 0);
    mallopt(M_MMAP_THRESHOLD, 0);
}

void on_mpris_properties_changed(GDBusConnection *conn, const gchar *sender,
                                 const gchar *path, const gchar *iface,
                                 const gchar *signal, GVariant *params,
                                 gpointer user_data) {
    gchar *prop_iface;
    GVariant *changed;
    GVariant *invalid;
    g_variant_get(params, "(s@a{sv}@as)", &prop_iface, &changed, &invalid);
    if (g_strcmp0(prop_iface, "org.mpris.MediaPlayer2.Player") == 0) {
        if (g_variant_lookup_value(changed, "Metadata",
                                   G_VARIANT_TYPE("a{sv}")))
            update_media_box();
        GVariant *st = g_variant_lookup_value(changed, "PlaybackStatus",
                                              G_VARIANT_TYPE_STRING);
        if (st && play_button) {
            const char *status = g_variant_get_string(st, NULL);
            const char *icon = (g_strcmp0(status, "Playing") == 0
                                    ? "media-playback-pause-symbolic"
                                    : "media-playback-start-symbolic");
            gtk_button_set_child(GTK_BUTTON(play_button),
                                 gtk_image_new_from_icon_name(icon));
            g_variant_unref(st);
        }
    }
    g_free(prop_iface);
    g_variant_unref(changed);
    g_variant_unref(invalid);
}

void on_mpris_properties_changed_label(GDBusConnection *conn,
                                       const gchar *sender, const gchar *path,
                                       const gchar *iface, const gchar *signal,
                                       GVariant *params, gpointer user_data) {
    gchar *prop_iface;
    GVariant *changed;
    GVariant *invalid;
    g_variant_get(params, "(s@a{sv}@as)", &prop_iface, &changed, &invalid);
    if (g_strcmp0(prop_iface, "org.mpris.MediaPlayer2.Player") == 0) {
        if (g_variant_lookup_value(changed, "Metadata",
                                   G_VARIANT_TYPE("a{sv}")))
            update_media_label(user_data);
    }
    g_free(prop_iface);
    g_variant_unref(changed);
    g_variant_unref(invalid);
}

/*
 * Subscribe to New MPRIS Players
 */

void subscribe_new_mpris_players(gpointer user_data) {
    name_owner_sub_id = g_dbus_connection_signal_subscribe(
        session_bus, "org.freedesktop.DBus", "org.freedesktop.DBus",
        "NameOwnerChanged", "/org/freedesktop/DBus", NULL,
        G_DBUS_SIGNAL_FLAGS_NONE, on_name_owner_changed, user_data, NULL);
}

void on_name_owner_changed(GDBusConnection *conn, const gchar *sender_name,
                           const gchar *object_path,
                           const gchar *interface_name,
                           const gchar *signal_name, GVariant *parameters,
                           gpointer user_data) {
    const gchar *name;
    const gchar *old_owner;
    const gchar *new_owner;

    g_variant_get(parameters, "(sss)", &name, &old_owner, &new_owner);

    if (g_str_has_prefix(name, "org.mpris.MediaPlayer2.")) {
        if (old_owner[0] == '\0' && new_owner[0] != '\0') {
            g_print("[MPRIS] new player appeared: %s\n", name);
            get_media_label(user_data);
        } else if (old_owner[0] != '\0' && new_owner[0] == '\0') {
            g_print("[MPRIS] player disappeared: %s\n", name);
            cleanup_media_label(user_data);
            get_media_label(user_data);
        }
    }
}

/*
 * Send Command to MPRIS
 */

void send_mpris_command(const char *method_name) {
    if (!session_bus || !current_player)
        return;
    GError *err = NULL;
    g_dbus_connection_call_sync(
        session_bus, current_player, "/org/mpris/MediaPlayer2",
        "org.mpris.MediaPlayer2.Player", method_name, NULL, NULL,
        G_DBUS_CALL_FLAGS_NONE, -1, NULL, &err);
    if (err) {
        g_warning("MPRIS %s failed: %s", method_name, err->message);
        g_clear_error(&err);
    }
}

/*
 * Subscribe to MPRIS Changes and Update Media Controls
 */

void subscribe_to_playpause_changes(void) {
    if (!session_bus || !current_player || !play_button)
        return;
    GError *err = NULL;
    GVariant *reply = g_dbus_connection_call_sync(
        session_bus, current_player, "/org/mpris/MediaPlayer2",
        "org.freedesktop.DBus.Properties", "Get",
        g_variant_new("(ss)", "org.mpris.MediaPlayer2.Player",
                      "PlaybackStatus"),
        G_VARIANT_TYPE("(v)"), G_DBUS_CALL_FLAGS_NONE, -1, NULL, &err);
    if (!reply) {
        g_warning("MPRIS Get(PlaybackStatus) failed: %s", err->message);
        g_clear_error(&err);
        return;
    }
    GVariant *inner = g_variant_get_child_value(reply, 0);
    g_variant_unref(reply);
    GVariant *str_var = g_variant_get_variant(inner);
    g_variant_unref(inner);
    const char *status = g_variant_get_string(str_var, NULL);
    g_variant_unref(str_var);
    const char *icon_name =
        (g_strcmp0(status, "Playing") == 0 ? "media-playback-pause-symbolic"
                                           : "media-playback-start-symbolic");
    gtk_button_set_child(GTK_BUTTON(play_button),
                         gtk_image_new_from_icon_name(icon_name));
}

gboolean subscribe_to_position_changes(gpointer _unused) {
    if (!session_bus || !current_player || track_length <= 0)
        return G_SOURCE_CONTINUE;
    GError *err = NULL;
    GVariant *reply = g_dbus_connection_call_sync(
        session_bus, current_player, "/org/mpris/MediaPlayer2",
        "org.freedesktop.DBus.Properties", "Get",
        g_variant_new("(ss)", "org.mpris.MediaPlayer2.Player", "Position"),
        G_VARIANT_TYPE("(v)"), G_DBUS_CALL_FLAGS_NONE, -1, NULL, &err);
    if (reply) {
        GVariant *inner_v = g_variant_get_child_value(reply, 0);
        g_variant_unref(reply);
        GVariant *pos_var = g_variant_get_variant(inner_v);
        g_variant_unref(inner_v);
        gint64 position = g_variant_get_int64(pos_var);
        g_variant_unref(pos_var);
        double frac = (double)position / (double)track_length;
        gtk_progress_bar_set_fraction(song_progress, frac);
        int s = position / 1000000;
        int ts = track_length / 1000000;
        char currbuf[64], lenbuf[64];
        snprintf(currbuf, sizeof(currbuf), "%d:%02d", s / 60, s % 60);
        snprintf(lenbuf, sizeof(lenbuf), "%d:%02d", ts / 60, ts % 60);
        gtk_label_set_text(track_current_position_label, currbuf);
        gtk_label_set_text(track_duration_position_label, lenbuf);
    } else {
        g_clear_error(&err);
    }
    return G_SOURCE_CONTINUE;
}
