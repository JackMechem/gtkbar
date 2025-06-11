#include "volume.h"
#include "app.h"
#include "gtk/gtk.h"
#include "volumepopup.h"
#include <ctype.h> // for isalpha()
#include <glib.h>
#include <pulse/context.h>
#include <pulse/introspect.h>
#include <pulse/pulseaudio.h>
#include <stdio.h>
#include <string.h> // for strrchr()

static pa_mainloop *mainloop = NULL;
static pa_context *context = NULL;
static int volume_percent = -1;
static int target_volume_percent;

//------------------------------------------------------------------------------
// Existing volume‐get/set callbacks & helpers
//------------------------------------------------------------------------------

void set_volume_cb(pa_context *c, int success, void *userdata) {
	if (!success)
		fprintf(stderr, "Failed to set volume\n");
	pa_mainloop_quit(mainloop, 0);
}

void sink_info_cb(pa_context *c, const pa_sink_info *i, int eol,
				  void *userdata) {
	if (eol || !i) {
		pa_mainloop_quit(mainloop, 0);
		return;
	}
	if (i->volume.channels > 0) {
		volume_percent = pa_cvolume_avg(&i->volume) * 100 / PA_VOLUME_NORM;
	}
	pa_mainloop_quit(mainloop, 0);
}

void sink_info_set_cb(pa_context *c, const pa_sink_info *i, int eol,
					  void *userdata) {
	if (eol || !i)
		return;
	pa_cvolume vol;
	float factor = target_volume_percent / 100.0f;
	pa_cvolume_set(&vol, i->channel_map.channels,
				   (pa_volume_t)(PA_VOLUME_NORM * factor));
	pa_context_set_sink_volume_by_index(c, i->index, &vol, set_volume_cb, NULL);
}

void context_state_cb(pa_context *c, void *userdata) {
	switch (pa_context_get_state(c)) {
	case PA_CONTEXT_READY:
		pa_operation_unref(pa_context_get_sink_info_by_name(
			c, "@DEFAULT_SINK@", sink_info_cb, NULL));
		break;
	case PA_CONTEXT_FAILED:
	case PA_CONTEXT_TERMINATED:
		pa_mainloop_quit(mainloop, 1);
		break;
	default:
		break;
	}
}

void context_state_set_cb(pa_context *c, void *userdata) {
	switch (pa_context_get_state(c)) {
	case PA_CONTEXT_READY:
		pa_operation_unref(pa_context_get_sink_info_by_name(
			c, "@DEFAULT_SINK@", sink_info_set_cb, NULL));
		break;
	case PA_CONTEXT_FAILED:
	case PA_CONTEXT_TERMINATED:
		pa_mainloop_quit(mainloop, 1);
		break;
	default:
		break;
	}
}

int get_volume_percentage() {
	mainloop = pa_mainloop_new();
	context = pa_context_new(pa_mainloop_get_api(mainloop), "volume-check");

	pa_context_set_state_callback(context, context_state_cb, NULL);
	pa_context_connect(context, NULL, PA_CONTEXT_NOFLAGS, NULL);

	pa_mainloop_run(mainloop, NULL);

	pa_context_disconnect(context);
	pa_context_unref(context);
	pa_mainloop_free(mainloop);

	return volume_percent;
}

void set_volume(int value) {
	printf("running\n");
	if (value < 0)
		value = 0;
	else if (value > 100)
		value = 100;
	target_volume_percent = value;

	mainloop = pa_mainloop_new();
	context = pa_context_new(pa_mainloop_get_api(mainloop), "Set Volume");

	pa_context_connect(context, NULL, PA_CONTEXT_NOFLAGS, NULL);
	pa_context_set_state_callback(context, context_state_set_cb, NULL);

	pa_mainloop_run(mainloop, NULL);

	pa_context_disconnect(context);
	pa_context_unref(context);
	pa_mainloop_free(mainloop);
}

gboolean volume_refresh_wrapper(gpointer user_label) {
	GtkLabel *label = GTK_LABEL(user_label);
	int volume = get_volume_percentage();

	char text[32];
	if (volume >= 0)
		snprintf(text, sizeof(text), "%d%%", volume);
	else
		snprintf(text, sizeof(text), "--");

	gtk_label_set_text(label, text);
	return TRUE; // keep the timer running
}

//------------------------------------------------------------------------------
// 1) List all sinks: get_output_devices()
//------------------------------------------------------------------------------

static GPtrArray *device_list = NULL;

static void sink_list_cb(pa_context *c, const pa_sink_info *i, int eol,
						 void *userdata) {
	(void)userdata;
	if (eol) {
		// all done
		pa_mainloop_quit(mainloop, 0);
		return;
	}
	if (i && i->name) {
		g_ptr_array_add(device_list, g_strdup(i->name));
	}
}

static void context_state_list_cb(pa_context *c, void *userdata) {
	switch (pa_context_get_state(c)) {
	case PA_CONTEXT_READY: {
		pa_operation *op =
			pa_context_get_sink_info_list(c, sink_list_cb, userdata);
		pa_operation_unref(op);
	} break;

	case PA_CONTEXT_FAILED:
		// real error — bail out
		fprintf(stderr, "PA context failed: %s\n",
				pa_strerror(pa_context_errno(c)));
		pa_mainloop_quit(mainloop, 1);
		break;

	default:
		// CONNECTING, AUTHORIZING, SETTING_NAME, TERMINATED, etc: ignore
		break;
	}
}

GPtrArray *get_output_devices(void) {
	device_list = g_ptr_array_new_with_free_func(g_free);

	mainloop = pa_mainloop_new();
	context = pa_context_new(pa_mainloop_get_api(mainloop), "list-sinks");

	int exit_code;
	int err;

	pa_context_set_state_callback(context, context_state_list_cb, NULL);

	// point explicitly at PipeWire's socket
	char server[128];
	snprintf(server, sizeof(server), "unix:/run/user/%d/pulse/native",
			 getuid());
	pa_context_connect(context, server, PA_CONTEXT_NOAUTOSPAWN, NULL);

	err = pa_mainloop_run(mainloop, &exit_code);

	pa_context_disconnect(context);
	pa_context_unref(context);
	pa_mainloop_free(mainloop);

	if (err < 0) {
		fprintf(stderr, "[get_output_devices] mainloop error: %s\n",
				pa_strerror(pa_context_errno(context)));
		g_ptr_array_free(device_list, TRUE);
		return NULL;
	}
	if (exit_code != 0) {
		fprintf(stderr, "[get_output_devices] exit code %d (no devices?)\n",
				exit_code);
		g_ptr_array_free(device_list, TRUE);
		return NULL;
	}
	printf("device list count: %d\n", device_list->len);
	return device_list;
}
//------------------------------------------------------------------------------
// 2) Set default sink: set_default_output_device()
//------------------------------------------------------------------------------

static void set_default_sink_cb(pa_context *c, int success, void *userdata) {
	char *name = userdata;
	if (!success)
		fprintf(stderr, "Failed to set default sink to '%s'\n", name);
	pa_mainloop_quit(mainloop, 0);
	g_free(name);
}

static void context_state_set_default_cb(pa_context *c, void *userdata) {
	pa_context_state_t st = pa_context_get_state(c);

	switch (st) {
	case PA_CONTEXT_READY: {
		pa_operation *op = pa_context_set_default_sink(
			c, (const char *)userdata,
			set_default_sink_cb, // will free userdata on success
			userdata);
		pa_operation_unref(op);
	} break;

	case PA_CONTEXT_FAILED:
		// error path: free userdata here
		fprintf(stderr, "PA context failed: %s\n",
				pa_strerror(pa_context_errno(c)));
		g_free(userdata);
		pa_mainloop_quit(mainloop, 1);
		break;

	default:
		// ignore CONNECTING, AUTHORIZING, SETTING_NAME, TERMINATED, etc.
		break;
	}
}

void set_default_output_device(const char *name) {
	char *choice = g_strdup(name);

	mainloop = pa_mainloop_new();
	context = pa_context_new(pa_mainloop_get_api(mainloop), "set-default-sink");
	pa_context_set_state_callback(context, context_state_set_default_cb,
								  choice);
	pa_context_connect(context, NULL, PA_CONTEXT_NOFLAGS, NULL);

	pa_mainloop_run(mainloop, NULL);

	pa_context_disconnect(context);
	pa_context_unref(context);
	pa_mainloop_free(mainloop);
}

static char *current_default = NULL;

static void default_sink_cb(pa_context *c, const pa_sink_info *i, int eol,
							void *userdata) {
	(void)userdata;
	if (eol || !i) {
		pa_mainloop_quit(mainloop, 0);
		return;
	}
	// grab the name
	current_default = g_strdup(i->name);
	pa_mainloop_quit(mainloop, 0);
}

static void context_state_default_cb(pa_context *c, void *userdata) {
	if (pa_context_get_state(c) == PA_CONTEXT_READY) {
		pa_operation *op = pa_context_get_sink_info_by_name(
			c, "@DEFAULT_SINK@", default_sink_cb, userdata);
		pa_operation_unref(op);
	}
}

static char *get_current_default_device(void) {
	current_default = NULL;

	mainloop = pa_mainloop_new();
	context = pa_context_new(pa_mainloop_get_api(mainloop), "get-default-sink");

	pa_context_set_state_callback(context, context_state_default_cb, NULL);

	// point at PipeWire socket (no autospawn)
	char server[128];
	snprintf(server, sizeof(server), "unix:/run/user/%d/pulse/native",
			 getuid());
	pa_context_connect(context, server, PA_CONTEXT_NOAUTOSPAWN, NULL);

	pa_mainloop_run(mainloop, NULL);

	pa_context_disconnect(context);
	pa_context_unref(context);
	pa_mainloop_free(mainloop);

	// current_default is either strdup'ed name or NULL
	return current_default;
}

static char *friendly_name(const char *sink_name) {
	// 1) skip everything up to the last dot
	const char *p = strrchr(sink_name, '.');
	p = p ? p + 1 : sink_name;

	// 2) allocate a buffer
	size_t len = strlen(p);
	char *label = g_malloc(len + 1);

	// 3) uppercase first letter of each word, convert [-_] → space
	gboolean capitalize = TRUE;
	for (size_t i = 0; i < len; ++i) {
		char c = p[i];
		if (c == '-' || c == '_') {
			label[i] = ' ';
			capitalize = TRUE;
		} else {
			if (capitalize && g_ascii_isalpha(c)) {
				label[i] = g_ascii_toupper(c);
			} else {
				label[i] = c;
			}
			capitalize = FALSE;
		}
	}
	label[len] = '\0';
	return label;
}

// Updated “changed” callback: pull the real sink-name out of the ID
static void on_device_changed(GtkComboBoxText *combo, gpointer user_data) {
	(void)user_data;
	const gchar *sink_id = gtk_combo_box_get_active_id(GTK_COMBO_BOX(combo));
	if (!sink_id)
		return;

	g_print("[UI] user picked: %s\n", sink_id);
	set_default_output_device(sink_id);
}

// Reworked factory: append(id, label) instead of append_text(label)
GtkWidget *create_output_device_dropdown(void) {
	// 1) fetch raw sink names
	GPtrArray *devices = get_output_devices();
	char *def = get_current_default_device();
	g_print("[UI] default sink: %s\n", def ? def : "(none)");

	// 2) build a GtkComboBoxText
	GtkWidget *combo = gtk_combo_box_text_new();
	gchar *chosen_id = NULL;

	if (devices && devices->len > 0) {
		for (guint i = 0; i < devices->len; ++i) {
			const char *sink_name = devices->pdata[i];
			char *label = friendly_name(sink_name);

			// store real sink_name as the “id”, display the friendly label
			gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(combo), sink_name,
									  label);
			if (def && strcmp(def, sink_name) == 0)
				chosen_id = (gchar *)sink_name; // pick this one later

			g_free(label);
		}

		// select either the real default or fallback to first entry
		if (chosen_id)
			gtk_combo_box_set_active_id(GTK_COMBO_BOX(combo), chosen_id);
		else
			gtk_combo_box_set_active(GTK_COMBO_BOX(combo), 0);
	} else {
		// no devices at all
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), "No devices");
		gtk_combo_box_set_active(GTK_COMBO_BOX(combo), 0);
	}

	// cleanup
	if (devices)
		g_ptr_array_free(devices, TRUE);
	g_free(def);

	// wire up
	g_signal_connect(combo, "changed", G_CALLBACK(on_device_changed), NULL);

	return combo;
}

// ─── 1) Data struct for each app stream
// ───────────────────────────────────────
typedef struct {
	uint32_t index; // sink-input index
	char *name;		// application name
	int volume;		// initial volume %
} AppSinkInput;

// ─── 2) Enumerate sink-inputs synchronously ──────────────────────────────────
static GPtrArray *app_list = NULL;

// called for each sink-input
static void sink_input_list_cb(pa_context *c, const pa_sink_input_info *i,
							   int eol, void *userdata) {
	(void)c;
	(void)userdata;

	if (eol) {
		pa_mainloop_quit(mainloop, 0);
		return;
	}

	// gather index, app-name (fallback if missing), and current volume%
	AppSinkInput *asi = g_malloc0(sizeof(*asi));
	asi->index = i->index;

	const char *app = pa_proplist_gets(i->proplist, PA_PROP_APPLICATION_NAME);
	asi->name = g_strdup(app ? app : "Unknown");

	asi->volume = pa_cvolume_avg(&i->volume) * 100 / PA_VOLUME_NORM;

	g_ptr_array_add(app_list, asi);
}

// called on state-changes
static void context_state_input_list_cb(pa_context *c, void *userdata) {
	switch (pa_context_get_state(c)) {
	case PA_CONTEXT_READY: {
		pa_operation *op = pa_context_get_sink_input_info_list(
			c, sink_input_list_cb, userdata);
		pa_operation_unref(op);
	} break;

	case PA_CONTEXT_FAILED:
	case PA_CONTEXT_TERMINATED:
		pa_mainloop_quit(mainloop, 1);
		break;

	default:
		break;
	}
}

// returns a GPtrArray of AppSinkInput* (caller frees each->name, each, then the
// array)
static GPtrArray *get_app_sink_inputs(void) {
	// 1) prepare array
	app_list = g_ptr_array_new();

	// 2) pump PulseAudio
	mainloop = pa_mainloop_new();
	context = pa_context_new(pa_mainloop_get_api(mainloop), "list-sink-inputs");
	pa_context_set_state_callback(context, context_state_input_list_cb, NULL);
	pa_context_connect(context, NULL, PA_CONTEXT_NOFLAGS, NULL);
	pa_mainloop_run(mainloop, NULL);

	// 3) teardown
	pa_context_disconnect(context);
	pa_context_unref(context);
	pa_mainloop_free(mainloop);

	return app_list;
}

// ─── 3) Helpers to set an individual app’s volume ────────────────────────────
static uint32_t app_target_idx;
static int app_target_percent;

static void set_app_volume_cb(pa_context *c, int success, void *userdata) {
	(void)c;
	(void)userdata;
	if (!success)
		fprintf(stderr, "Failed to set app volume\n");
	pa_mainloop_quit(mainloop, 0);
}

static void context_state_app_cb(pa_context *c, void *userdata) {
	switch (pa_context_get_state(c)) {
	case PA_CONTEXT_READY: {
		pa_cvolume vol;
		float factor = app_target_percent / 100.0f;
		// stereo hard-coded here; Pulse will cope with more channels
		pa_cvolume_set(&vol, 2, (pa_volume_t)(PA_VOLUME_NORM * factor));

		pa_operation *op = pa_context_set_sink_input_volume(
			c, app_target_idx, &vol, set_app_volume_cb, NULL);
		pa_operation_unref(op);
	} break;

	case PA_CONTEXT_FAILED:
		g_warning("PA context failed: %s", pa_strerror(pa_context_errno(c)));
		pa_mainloop_quit(mainloop, 1);
		break;

	default:
		/* ignore CONNECTING, AUTHORIZING, SETTING_NAME, TERMINATED, etc. */
		break;
	}
}
// small helper to hold per-slider data
typedef struct {
	uint32_t index;
	GtkWidget *percent_label;
} VolumeControl;

// updated callback: update label + fire the PA volume change
static void on_app_volume_changed(GtkRange *range, gpointer user_data) {
	VolumeControl *vc = user_data;
	int vol = (int)gtk_range_get_value(range);

	// 1) Update the percent label
	char buf[8];
	snprintf(buf, sizeof(buf), "%d%%", vol);
	gtk_label_set_text(GTK_LABEL(vc->percent_label), buf);

	// 2) Store for the callback
	app_target_idx = vc->index;
	app_target_percent = vol;

	// 3) Spin up the **global** mainloop/context
	mainloop = pa_mainloop_new();
	context = pa_context_new(pa_mainloop_get_api(mainloop), "set-app-volume");
	pa_context_set_state_callback(context, context_state_app_cb, NULL);
	pa_context_connect(context, NULL, PA_CONTEXT_NOFLAGS, NULL);

	// 4) Run until our callback quits it
	pa_mainloop_run(mainloop, NULL);

	// 5) Tear it down
	pa_context_disconnect(context);
	pa_context_unref(context);
	pa_mainloop_free(mainloop);
}

GtkWidget *create_app_volume_box(void) {
	GPtrArray *inputs = get_app_sink_inputs();
	GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);

	if (inputs->len == 0) {
		gtk_box_append(GTK_BOX(vbox), gtk_label_new("No active streams"));
	} else {
		for (guint i = 0; i < inputs->len; ++i) {
			AppSinkInput *asi = inputs->pdata[i];

			// 1) make the per-row container
			GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);

			// 2) app name
			char *display_name = g_strdup(asi->name);
			if (display_name[0] != '\0')
				display_name[0] = g_ascii_toupper(display_name[0]);

			GtkWidget *name_lbl = gtk_label_new(display_name);
			g_free(display_name);
			gtk_widget_set_hexpand(name_lbl, FALSE);
			gtk_label_set_width_chars(GTK_LABEL(name_lbl), 8);
			gtk_label_set_xalign(GTK_LABEL(name_lbl), 0.0);
			// 3) slider
			GtkWidget *scale =
				gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, 100, 1);
			gtk_range_set_value(GTK_RANGE(scale), asi->volume);
			gtk_widget_set_hexpand(scale, TRUE);
			gtk_widget_set_halign(scale, GTK_ALIGN_FILL);

			// 4) percent label
			char buf[8];
			snprintf(buf, sizeof(buf), "%d%%", asi->volume);
			GtkWidget *pct_lbl = gtk_label_new(buf);

			// 5) wire them together
			VolumeControl *vc = g_new0(VolumeControl, 1);
			vc->index = asi->index;
			vc->percent_label = pct_lbl;

			g_signal_connect(scale, "value-changed",
							 G_CALLBACK(on_app_volume_changed), vc);

			// 6) pack into the row
			gtk_box_append(GTK_BOX(hbox), name_lbl);
			gtk_box_append(GTK_BOX(hbox), scale);
			gtk_box_append(GTK_BOX(hbox), pct_lbl);
			gtk_box_append(GTK_BOX(vbox), hbox);

			// cleanup temporary data
			g_free(asi->name);
			g_free(asi);
		}
	}

	g_ptr_array_free(inputs, TRUE);
	return vbox;
}
