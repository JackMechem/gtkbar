#ifndef MEDIA_H
#define MEDIA_H

#include <gio/gio.h>
#include <malloc.h>
#include <gtk/gtk.h>

// **
// ** Globals
// **

// Struct for storing track information
typedef struct {
    char *service;
    char *title;
    char *artist;
    char *album;
    char *art_url;
    gint64 length;
} TrackInfo;

typedef struct {
    GDBusConnection *ses_bus;
    char *cur_player;
} PlayerInfo;

// Async and timeout ids
static guint name_owner_sub_id = 0;
static guint position_timer_id = 0;
static guint mpris_props_id = 0;
static guint label_mpris_props_id = 0;

// DBus connection and player name
static GDBusConnection *session_bus = NULL;
static char *current_player = NULL;

// UI globals
static GtkWidget *mediaBox = NULL;
static GtkWidget *play_button = NULL;
static GtkProgressBar *song_progress = NULL;
static GtkLabel *track_current_position_label = NULL;
static GtkLabel *track_duration_position_label = NULL;
static GtkPicture *cover_pic = NULL;
static GCancellable *cover_cancellable = NULL;

// Track length
static gint64 track_length = 0;

// **
// ** Definitions
// **

// Build, update, and clean UIs
GtkWidget *get_media_box(void);
void update_media_box(void);
GtkWidget *get_media_label(gpointer user_data);
void update_media_label(gpointer user_data);
void cleanup_media_label(gpointer user_data);
GtkWidget *get_media_controls();

// UI callbacks
void on_prev_clicked(GtkButton *_btn, gpointer _);
void on_playpause_clicked(GtkButton *_btn, gpointer _);
void on_next_clicked(GtkButton *_btn, gpointer _);

// Cover photo
void clear_cover_pic(void);
void cover_load_thread(GTask *task, gpointer source_object, gpointer task_data,
                       GCancellable *cancellable);
void on_cover_loaded(GObject *source_object, GAsyncResult *res,
                     gpointer user_data);
void load_cover_async(const char *uri);

// Query now playing track information
TrackInfo *now_playing_query(void);
PlayerInfo *get_player_info();
void now_playing_free(TrackInfo *info);

// Subscribe to mpris changes
void subscribe_mpris_changes();
void subscribe_mpris_changes_label(gpointer user_data);
void unsubscribe_mpris_changes(void);
void on_mpris_properties_changed(GDBusConnection *conn, const gchar *sender,
                                 const gchar *path, const gchar *iface,
                                 const gchar *signal, GVariant *params,
                                 gpointer user_data);
void on_mpris_properties_changed_label(GDBusConnection *conn,
                                       const gchar *sender, const gchar *path,
                                       const gchar *iface, const gchar *signal,
                                       GVariant *params, gpointer user_data);

// Subscribe to new mpris players
void subscribe_new_mpris_players(gpointer user_data);
void on_name_owner_changed(GDBusConnection *conn,
                                  const gchar *sender_name,
                                  const gchar *object_path,
                                  const gchar *interface_name,
                                  const gchar *signal_name,
                                  GVariant *parameters, gpointer user_data);

// Send command to mpris
void send_mpris_command(const char *method_name);

// Subscribe to mpris changes and update media controls
void subscribe_to_playpause_changes(void);
gboolean subscribe_to_position_changes(gpointer _unused);

#endif
