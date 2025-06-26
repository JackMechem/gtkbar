#include <gio/gio.h>
#include <glib.h>

#include <glib.h>
#include <gtk/gtk.h>

/// Holds the “now playing” metadata.
typedef struct {
    /// D-Bus service name, e.g. "org.mpris.MediaPlayer2.spotify"
    char *service;
    char *title;
    char *artist;
    char *album;
    /// URI to album art (e.g. "file://…" or "https://…")
    char *art_url;
    gint64 length;
} TrackInfo;

/// Query the first available MPRIS player and pull its Metadata.
/// Returns a newly-allocated TrackInfo, or NULL if no player was found
/// (or on failure).  You must call now_playing_free() on it.
TrackInfo *now_playing_query(void);

/// Free a TrackInfo returned by now_playing_query().
void now_playing_free(TrackInfo *info);

GtkWidget *register_media_controls();

GtkWidget *get_media_box();
GtkWidget *get_media_label(gpointer user_data);
void update_media_label(gpointer user_data);
void subscribe_mpris_changes(void);
void subscribe_mpris_changes_label(gpointer user_data);
void unsubscribe_mpris_changes();
