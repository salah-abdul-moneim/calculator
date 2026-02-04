#include "style_manager.h"

#include <gio/gio.h>
#include <string.h>

struct StyleManager {
    GtkCssProvider *provider;
    GtkSettings *gtk_settings; // unowned
    GSettings *iface_settings; // owned
    char *dark_path;
    char *light_path;
    gboolean dark;
    gboolean attached;
};

static char *exe_dir(void) {
    GError *err = NULL;
    char *exe = g_file_read_link("/proc/self/exe", &err);
    if (!exe) {
        if (err) g_error_free(err);
        return g_strdup(".");
    }
    char *dir = g_path_get_dirname(exe);
    g_free(exe);
    return dir;
}

static char *resolve_path(const char *path) {
    if (!path) return NULL;
    if (g_path_is_absolute(path)) return g_strdup(path);
    char *dir = exe_dir();
    char *full = g_build_filename(dir, path, NULL);
    g_free(dir);
    return full;
}

static gboolean detect_dark(StyleManager *m) {
    // GNOME preference used by Adwaita (and GNOME apps).
    if (m->iface_settings) {
        char *scheme = g_settings_get_string(m->iface_settings, "color-scheme");
        if (scheme) {
            gboolean prefer_dark = (g_strcmp0(scheme, "prefer-dark") == 0);
            g_free(scheme);
            return prefer_dark;
        }
    }

    // Fallback: theme name contains "dark".
    if (m->gtk_settings) {
        char *theme = NULL;
        g_object_get(m->gtk_settings, "gtk-theme-name", &theme, NULL);
        if (theme) {
            char *lower = g_ascii_strdown(theme, -1);
            gboolean prefer_dark = (strstr(lower, "dark") != NULL);
            g_free(lower);
            g_free(theme);
            return prefer_dark;
        }
    }

    return TRUE;
}

static void apply_css_file(StyleManager *m, gboolean dark) {
    const char *path = dark ? m->dark_path : m->light_path;
    if (!path || !path[0]) return;

    char *full = resolve_path(path);
    if (!full) return;

    GFile *file = g_file_new_for_path(full);
    gtk_css_provider_load_from_file(m->provider, file);
    g_object_unref(file);
    g_free(full);

    m->dark = dark;
}

static void sync(StyleManager *m) {
    gboolean want_dark = detect_dark(m);
    if (want_dark != m->dark) {
        apply_css_file(m, want_dark);
    }
}

static void on_gtk_settings_notify(GObject *obj, GParamSpec *pspec, gpointer user_data) {
    (void)obj;
    (void)pspec;
    StyleManager *m = (StyleManager *)user_data;
    if (!m) return;
    sync(m);
}

static void on_iface_color_scheme_changed(GSettings *settings, gchar *key, gpointer user_data) {
    (void)settings;
    (void)key;
    StyleManager *m = (StyleManager *)user_data;
    if (!m) return;
    sync(m);
}

StyleManager *style_manager_new(const char *dark_css_path, const char *light_css_path) {
    StyleManager *m = g_new0(StyleManager, 1);
    m->provider = gtk_css_provider_new();
    m->dark_path = g_strdup(dark_css_path);
    m->light_path = g_strdup(light_css_path);
    m->dark = TRUE;
    return m;
}

void style_manager_attach(StyleManager *m) {
    if (!m || m->attached) return;
    m->attached = TRUE;

    GdkDisplay *display = gdk_display_get_default();
    if (display) {
        gtk_style_context_add_provider_for_display(
            display, GTK_STYLE_PROVIDER(m->provider),
            GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    }

    m->gtk_settings = gtk_settings_get_default();
    if (m->gtk_settings) {
        g_signal_connect(m->gtk_settings, "notify::gtk-theme-name",
                         G_CALLBACK(on_gtk_settings_notify), m);
    }

    m->iface_settings = g_settings_new("org.gnome.desktop.interface");
    if (m->iface_settings) {
        g_signal_connect(m->iface_settings, "changed::color-scheme",
                         G_CALLBACK(on_iface_color_scheme_changed), m);
    }

    // Force initial load
    m->dark = !detect_dark(m);
    sync(m);
}

void style_manager_free(StyleManager *m) {
    if (!m) return;

    if (m->gtk_settings) {
        g_signal_handlers_disconnect_by_data(m->gtk_settings, m);
    }
    if (m->iface_settings) {
        g_signal_handlers_disconnect_by_data(m->iface_settings, m);
        g_object_unref(m->iface_settings);
    }

    GdkDisplay *display = gdk_display_get_default();
    if (display) {
        gtk_style_context_remove_provider_for_display(display, GTK_STYLE_PROVIDER(m->provider));
    }

    g_object_unref(m->provider);
    g_free(m->dark_path);
    g_free(m->light_path);
    g_free(m);
}

