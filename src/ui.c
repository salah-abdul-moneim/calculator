#include "ui.h"

#include "calc_eval.h"
#include "style_manager.h"

#include <math.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

typedef struct {
    GtkWidget *entry;
    GtkWidget *extra_grid;
    gboolean extra_visible;
    gboolean degrees;
    GtkWidget *mode_button;
    gboolean has_result;
    double last_result;
    int compact_height;
    gboolean compact_height_set;
    GtkWindow *window;
    GtkWidget *root_window;
    GtkWidget *headerbar;
    GtkWidget *content_box;
    GtkWidget *btn_max;
    StyleManager *style; // not owned (global)
} AppState;

#define COMPACT_WIDTH 360
#define COMPACT_HEIGHT 480
#define EXTRA_SHOW_WIDTH 560

static StyleManager *g_style = NULL;
static int g_style_refs = 0;

static void style_global_ref(void) {
    if (!g_style) {
        g_style = style_manager_new("assets/dark.css", "assets/light.css");
        style_manager_attach(g_style);
        g_style_refs = 0;
    }
    g_style_refs++;
}

static void style_global_unref(void) {
    if (g_style_refs > 0) g_style_refs--;
    if (g_style && g_style_refs == 0) {
        style_manager_free(g_style);
        g_style = NULL;
    }
}

static void set_entry_text(GtkEntry *entry, const char *text) {
    gtk_editable_set_text(GTK_EDITABLE(entry), text ? text : "");
}

static void handle_backspace(GtkEntry *entry) {
    const char *text = gtk_editable_get_text(GTK_EDITABLE(entry));
    if (!text || text[0] == '\0') return;
    glong len = g_utf8_strlen(text, -1);
    if (len <= 0) return;
    const char *cut = g_utf8_offset_to_pointer(text, len - 1);
    gchar *new_text = g_strndup(text, cut - text);
    set_entry_text(entry, new_text);
    g_free(new_text);
}

static void format_result(double value, char *out, size_t out_cap) {
    g_snprintf(out, out_cap, "%.12g", value);
}

static void on_button_clicked(GtkButton *button, gpointer user_data) {
    AppState *state = (AppState *)user_data;
    const char *label = gtk_button_get_label(button);
    GtkEntry *entry = GTK_ENTRY(state->entry);

    if (g_strcmp0(label, "C") == 0) {
        set_entry_text(entry, "");
        return;
    }
    if (g_strcmp0(label, "deg") == 0 || g_strcmp0(label, "rad") == 0) {
        state->degrees = !state->degrees;
        gtk_button_set_label(button, state->degrees ? "deg" : "rad");
        return;
    }
    if (g_strcmp0(label, "CE") == 0) { // "⌫"
        handle_backspace(entry);
        return;
    }
    if (g_strcmp0(label, "=") == 0) {
        const char *expr = gtk_editable_get_text(GTK_EDITABLE(entry));
        char err[128] = {0};
        double result = 0.0;

        if (state->degrees) {
            int deg = 0;
            char display_out[128] = {0};
            if (calc_try_special_trig(expr, &deg, display_out, sizeof(display_out),
                                      &result, err, sizeof(err))) {
                set_entry_text(entry, display_out);
                state->last_result = result;
                state->has_result = TRUE;
                return;
            }
            if (err[0]) {
                gchar *out = g_strdup_printf("Error: %s", err);
                set_entry_text(entry, out);
                g_free(out);
                state->has_result = FALSE;
                return;
            }
        }

        if (calc_eval(expr, state->degrees, &result, err, sizeof(err))) {
            char out[128];
            format_result(result, out, sizeof(out));
            set_entry_text(entry, out);
            state->last_result = result;
            state->has_result = TRUE;
        } else {
            gchar *out = g_strdup_printf("Error: %s", err);
            set_entry_text(entry, out);
            g_free(out);
            state->has_result = FALSE;
        }
        return;
    }

    const char *insert = label;
    if (g_strcmp0(label, "sin") == 0) insert = "sin(";
    else if (g_strcmp0(label, "cos") == 0) insert = "cos(";
    else if (g_strcmp0(label, "tan") == 0) insert = "tan(";
    else if (g_strcmp0(label, "√") == 0) insert = "sqrt(";
    else if (g_strcmp0(label, "log") == 0) insert = "log(";
    else if (g_strcmp0(label, "ln") == 0) insert = "ln(";
    else if (g_strcmp0(label, "log2") == 0) insert = "log2(";
    else if (g_strcmp0(label, "abs") == 0) insert = "abs(";
    else if (g_strcmp0(label, "exp") == 0) insert = "exp(";
    else if (g_strcmp0(label, "pow") == 0) insert = "pow(";
    else if (g_strcmp0(label, "csc") == 0) insert = "csc(";
    else if (g_strcmp0(label, "sec") == 0) insert = "sec(";
    else if (g_strcmp0(label, "cot") == 0) insert = "cot(";
    else if (g_strcmp0(label, "π") == 0) insert = "3.141592653589793";
    else if (g_strcmp0(label, "e") == 0) insert = "2.718281828459045";

    const char *current = gtk_editable_get_text(GTK_EDITABLE(entry));
    gchar *new_text = g_strconcat(current, insert, NULL);
    set_entry_text(entry, new_text);
    g_free(new_text);
}

static void on_entry_activate(GtkEntry *entry, gpointer user_data) {
    (void)entry;
    AppState *state = (AppState *)user_data;
    GtkButton *fake = GTK_BUTTON(gtk_button_new_with_label("="));
    on_button_clicked(fake, state);
    g_object_unref(fake);
}

static void on_window_close(GtkButton *button, gpointer user_data) {
    (void)button;
    GtkWindow *win = GTK_WINDOW(user_data);
    gtk_window_close(win);
}

static void on_window_minimize(GtkButton *button, gpointer user_data) {
    (void)button;
    GtkWindow *win = GTK_WINDOW(user_data);
    gtk_window_minimize(win);
}

static void on_window_maximize(GtkButton *button, gpointer user_data) {
    (void)button;
    GtkWindow *win = GTK_WINDOW(user_data);
    if (gtk_window_is_maximized(win)) {
        gtk_window_unmaximize(win);
    } else {
        gtk_window_maximize(win);
    }
}

static void on_window_destroy(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    AppState *state = (AppState *)user_data;
    if (!state) return;
    style_global_unref();
    g_free(state);
}

static GtkWidget *make_button(const char *label, AppState *state) {
    GtkWidget *btn = gtk_button_new_with_label(label);
    gtk_widget_set_hexpand(btn, TRUE);
    gtk_widget_set_vexpand(btn, TRUE);
    gtk_widget_add_css_class(btn, "btn");
    g_signal_connect(btn, "clicked", G_CALLBACK(on_button_clicked), state);
    return btn;
}

static void add_btn_class(GtkWidget *btn, const char *klass) {
    if (btn && klass) {
        gtk_widget_add_css_class(btn, klass);
    }
}

static gboolean on_grid_tick(GtkWidget *widget, GdkFrameClock *frame_clock, gpointer user_data) {
    (void)frame_clock;
    AppState *state = (AppState *)user_data;
    if (!state || !state->extra_grid) return G_SOURCE_CONTINUE;
    int width = gtk_widget_get_width(widget);
    if (state->window) {
        width = gtk_widget_get_width(GTK_WIDGET(state->window));
    }
    gboolean show = width >= EXTRA_SHOW_WIDTH;
    if (show != state->extra_visible) {
        gtk_widget_set_visible(state->extra_grid, show);
        state->extra_visible = show;
    }
    return G_SOURCE_CONTINUE;
}

static gboolean on_window_tick(GtkWidget *widget, GdkFrameClock *frame_clock, gpointer user_data) {
    (void)frame_clock;
    (void)user_data;
    GtkWindow *win = GTK_WINDOW(widget);
    if (gtk_window_is_maximized(win) || gtk_window_is_fullscreen(win)) {
        return G_SOURCE_CONTINUE;
    }
    int w = gtk_widget_get_width(widget);
    int h = gtk_widget_get_height(widget);
    if (w != COMPACT_WIDTH || h != COMPACT_HEIGHT) {
        gtk_window_set_default_size(win, COMPACT_WIDTH, COMPACT_HEIGHT);
        gtk_widget_set_size_request(widget, COMPACT_WIDTH, COMPACT_HEIGHT);
        gtk_widget_queue_resize(widget);
    }
    return G_SOURCE_CONTINUE;
}

static void on_window_state_changed(GtkWindow *win, GParamSpec *pspec, gpointer user_data) {
    (void)pspec;
    AppState *state = (AppState *)user_data;
    if (!state) return;
    gboolean maxed = gtk_window_is_maximized(win) || gtk_window_is_fullscreen(win);
    if (state->root_window) {
        if (maxed) gtk_widget_add_css_class(state->root_window, "maxed");
        else gtk_widget_remove_css_class(state->root_window, "maxed");
    }
    if (state->headerbar) {
        if (maxed) gtk_widget_add_css_class(state->headerbar, "maxed");
        else gtk_widget_remove_css_class(state->headerbar, "maxed");
    }
    if (state->content_box) {
        if (maxed) gtk_widget_add_css_class(state->content_box, "maxed");
        else gtk_widget_remove_css_class(state->content_box, "maxed");
    }
    if (state->btn_max) {
        gtk_button_set_icon_name(GTK_BUTTON(state->btn_max),
                                 maxed ? "window-restore-symbolic" : "window-maximize-symbolic");
    }
}

void ui_activate(GtkApplication *app, gpointer user_data) {
    (void)user_data;

    style_global_ref();

    AppState *state = g_new0(AppState, 1);
    state->style = g_style;

    GtkWidget *window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "Calculator");
    gtk_window_set_default_size(GTK_WINDOW(window), COMPACT_WIDTH, COMPACT_HEIGHT);
    gtk_window_set_resizable(GTK_WINDOW(window), TRUE);
    gtk_window_set_decorated(GTK_WINDOW(window), FALSE);
    gtk_widget_set_size_request(GTK_WIDGET(window), COMPACT_WIDTH, COMPACT_HEIGHT);
    gtk_widget_add_css_class(window, "calc-window");
    state->window = GTK_WINDOW(window);
    state->root_window = window;
    gtk_widget_add_tick_callback(window, on_window_tick, NULL, NULL);
    g_signal_connect(window, "notify::maximized", G_CALLBACK(on_window_state_changed), state);
    g_signal_connect(window, "notify::fullscreen", G_CALLBACK(on_window_state_changed), state);
    g_signal_connect(window, "destroy", G_CALLBACK(on_window_destroy), state);

    GtkWidget *titlebar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_widget_add_css_class(titlebar, "headerbar");
    gtk_widget_add_css_class(titlebar, "titlebar-box");
    state->headerbar = titlebar;

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_widget_set_margin_top(vbox, 0);
    gtk_widget_set_margin_bottom(vbox, 12);
    gtk_widget_set_margin_start(vbox, 0);
    gtk_widget_set_margin_end(vbox, 0);
    gtk_widget_add_css_class(vbox, "content");
    gtk_widget_set_overflow(vbox, GTK_OVERFLOW_HIDDEN);
    gtk_window_set_child(GTK_WINDOW(window), vbox);
    state->content_box = vbox;

    GtkWidget *title = gtk_label_new("Calculator");
    gtk_widget_add_css_class(title, "header-title");
    gtk_widget_set_hexpand(title, TRUE);
    gtk_widget_set_halign(title, GTK_ALIGN_START);

    GtkWidget *actions = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_widget_add_css_class(actions, "titlebar-actions");
    gtk_box_set_spacing(GTK_BOX(actions), 6);
    gtk_widget_set_halign(actions, GTK_ALIGN_END);
    gtk_widget_set_valign(actions, GTK_ALIGN_CENTER);

    GtkWidget *btn_min = gtk_button_new_from_icon_name("window-minimize-symbolic");
    gtk_widget_add_css_class(btn_min, "titlebar-btn");
    gtk_widget_add_css_class(btn_min, "btn-min");
    gtk_widget_set_size_request(btn_min, 30, 30);
    g_signal_connect(btn_min, "clicked", G_CALLBACK(on_window_minimize), window);

    GtkWidget *btn_max = gtk_button_new_from_icon_name("window-maximize-symbolic");
    gtk_widget_add_css_class(btn_max, "titlebar-btn");
    gtk_widget_add_css_class(btn_max, "btn-max");
    gtk_widget_set_size_request(btn_max, 30, 30);
    g_signal_connect(btn_max, "clicked", G_CALLBACK(on_window_maximize), window);
    state->btn_max = btn_max;

    GtkWidget *btn_close = gtk_button_new_from_icon_name("window-close-symbolic");
    gtk_widget_add_css_class(btn_close, "titlebar-btn");
    gtk_widget_add_css_class(btn_close, "close");
    gtk_widget_add_css_class(btn_close, "btn-close");
    gtk_widget_set_size_request(btn_close, 30, 30);
    g_signal_connect(btn_close, "clicked", G_CALLBACK(on_window_close), window);

    gtk_box_append(GTK_BOX(actions), btn_min);
    gtk_box_append(GTK_BOX(actions), btn_max);
    gtk_box_append(GTK_BOX(actions), btn_close);

    gtk_box_append(GTK_BOX(titlebar), title);
    gtk_box_append(GTK_BOX(titlebar), actions);

    GtkWidget *handle = gtk_window_handle_new();
    gtk_window_handle_set_child(GTK_WINDOW_HANDLE(handle), titlebar);
    gtk_box_append(GTK_BOX(vbox), handle);

    GtkWidget *content = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_widget_set_margin_top(content, 12);
    gtk_widget_set_margin_start(content, 12);
    gtk_widget_set_margin_end(content, 12);
    gtk_box_append(GTK_BOX(vbox), content);

    GtkWidget *entry = gtk_entry_new();
    gtk_entry_set_alignment(GTK_ENTRY(entry), 1.0);
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry), "0");
    gtk_entry_set_input_purpose(GTK_ENTRY(entry), GTK_INPUT_PURPOSE_NUMBER);
    gtk_editable_set_editable(GTK_EDITABLE(entry), FALSE);
    gtk_widget_set_can_focus(entry, FALSE);
    gtk_widget_set_hexpand(entry, TRUE);
    gtk_widget_add_css_class(entry, "display");
    gtk_box_append(GTK_BOX(content), entry);

    g_signal_connect(entry, "activate", G_CALLBACK(on_entry_activate), state);

    GtkWidget *grid_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_widget_set_hexpand(grid_row, TRUE);
    gtk_widget_set_vexpand(grid_row, TRUE);
    gtk_box_append(GTK_BOX(content), grid_row);
    gtk_widget_add_tick_callback(grid_row, on_grid_tick, state, NULL);

    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 6);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 6);
    gtk_widget_set_hexpand(grid, TRUE);
    gtk_widget_set_vexpand(grid, TRUE);
    gtk_widget_add_css_class(grid, "grid");
    gtk_box_append(GTK_BOX(grid_row), grid);

    GtkWidget *extra = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(extra), 6);
    gtk_grid_set_column_spacing(GTK_GRID(extra), 6);
    gtk_widget_set_hexpand(extra, TRUE);
    gtk_widget_set_vexpand(extra, TRUE);
    gtk_widget_add_css_class(extra, "grid");
    gtk_widget_set_visible(extra, FALSE);
    gtk_box_append(GTK_BOX(grid_row), extra);

    state->entry = entry;
    state->extra_grid = extra;
    state->extra_visible = FALSE;
    state->degrees = FALSE;
    state->mode_button = NULL;
    state->has_result = FALSE;
    state->last_result = 0.0;
    state->compact_height = COMPACT_HEIGHT;
    state->compact_height_set = TRUE;
    state->window = GTK_WINDOW(window);

    GtkWidget *btn;

    // Row 1
    btn = make_button("C", state);
    add_btn_class(btn, "btn-clear");
    gtk_grid_attach(GTK_GRID(grid), btn, 0, 0, 1, 1);
    btn = make_button("CE", state); // "⌫"
    add_btn_class(btn, "btn-fn");
    gtk_grid_attach(GTK_GRID(grid), btn, 1, 0, 1, 1);
    btn = make_button("(", state);
    add_btn_class(btn, "btn-op");
    gtk_grid_attach(GTK_GRID(grid), btn, 2, 0, 1, 1);
    btn = make_button(")", state);
    add_btn_class(btn, "btn-op");
    gtk_grid_attach(GTK_GRID(grid), btn, 3, 0, 1, 1);

    // Row 2
    btn = make_button("^", state);
    add_btn_class(btn, "btn-op");
    gtk_grid_attach(GTK_GRID(grid), btn, 0, 1, 1, 1);
    btn = make_button("%", state);
    add_btn_class(btn, "btn-op");
    gtk_grid_attach(GTK_GRID(grid), btn, 1, 1, 1, 1);
    btn = make_button("!", state);
    add_btn_class(btn, "btn-op");
    gtk_grid_attach(GTK_GRID(grid), btn, 2, 1, 1, 1);
    btn = make_button("/", state);
    add_btn_class(btn, "btn-op");
    gtk_grid_attach(GTK_GRID(grid), btn, 3, 1, 1, 1);

    // Row 3
    btn = make_button("7", state);
    gtk_grid_attach(GTK_GRID(grid), btn, 0, 2, 1, 1);
    btn = make_button("8", state);
    gtk_grid_attach(GTK_GRID(grid), btn, 1, 2, 1, 1);
    btn = make_button("9", state);
    gtk_grid_attach(GTK_GRID(grid), btn, 2, 2, 1, 1);
    btn = make_button("*", state);
    add_btn_class(btn, "btn-op");
    gtk_grid_attach(GTK_GRID(grid), btn, 3, 2, 1, 1);

    // Row 4
    btn = make_button("4", state);
    gtk_grid_attach(GTK_GRID(grid), btn, 0, 3, 1, 1);
    btn = make_button("5", state);
    gtk_grid_attach(GTK_GRID(grid), btn, 1, 3, 1, 1);
    btn = make_button("6", state);
    gtk_grid_attach(GTK_GRID(grid), btn, 2, 3, 1, 1);
    btn = make_button("-", state);
    add_btn_class(btn, "btn-op");
    gtk_grid_attach(GTK_GRID(grid), btn, 3, 3, 1, 1);

    // Row 5
    btn = make_button("1", state);
    gtk_grid_attach(GTK_GRID(grid), btn, 0, 4, 1, 1);
    btn = make_button("2", state);
    gtk_grid_attach(GTK_GRID(grid), btn, 1, 4, 1, 1);
    btn = make_button("3", state);
    gtk_grid_attach(GTK_GRID(grid), btn, 2, 4, 1, 1);
    btn = make_button("+", state);
    add_btn_class(btn, "btn-op");
    gtk_grid_attach(GTK_GRID(grid), btn, 3, 4, 1, 1);

    // Row 6
    btn = make_button("0", state);
    gtk_grid_attach(GTK_GRID(grid), btn, 0, 5, 2, 1);
    btn = make_button(".", state);
    gtk_grid_attach(GTK_GRID(grid), btn, 2, 5, 1, 1);
    btn = make_button("=", state);
    add_btn_class(btn, "btn-eq");
    gtk_grid_attach(GTK_GRID(grid), btn, 3, 5, 1, 1);

    // Extra functions (shown on wider window)
    btn = make_button("sin", state);
    add_btn_class(btn, "btn-op");
    add_btn_class(btn, "btn-extra");
    gtk_grid_attach(GTK_GRID(extra), btn, 0, 0, 1, 1);
    btn = make_button("cos", state);
    add_btn_class(btn, "btn-op");
    add_btn_class(btn, "btn-extra");
    gtk_grid_attach(GTK_GRID(extra), btn, 1, 0, 1, 1);
    btn = make_button("tan", state);
    add_btn_class(btn, "btn-op");
    add_btn_class(btn, "btn-extra");
    gtk_grid_attach(GTK_GRID(extra), btn, 2, 0, 1, 1);
    btn = make_button("√", state);
    add_btn_class(btn, "btn-op");
    add_btn_class(btn, "btn-extra");
    gtk_grid_attach(GTK_GRID(extra), btn, 3, 0, 1, 1);
    btn = make_button("log", state);
    add_btn_class(btn, "btn-op");
    add_btn_class(btn, "btn-extra");
    gtk_grid_attach(GTK_GRID(extra), btn, 0, 1, 1, 1);
    btn = make_button("ln", state);
    add_btn_class(btn, "btn-op");
    add_btn_class(btn, "btn-extra");
    gtk_grid_attach(GTK_GRID(extra), btn, 1, 1, 1, 1);
    btn = make_button("π", state);
    add_btn_class(btn, "btn-op");
    add_btn_class(btn, "btn-extra");
    gtk_grid_attach(GTK_GRID(extra), btn, 2, 1, 1, 1);
    btn = make_button("e", state);
    add_btn_class(btn, "btn-op");
    add_btn_class(btn, "btn-extra");
    gtk_grid_attach(GTK_GRID(extra), btn, 3, 1, 1, 1);
    btn = make_button("log2", state);
    add_btn_class(btn, "btn-op");
    add_btn_class(btn, "btn-extra");
    gtk_grid_attach(GTK_GRID(extra), btn, 0, 2, 1, 1);
    btn = make_button("abs", state);
    add_btn_class(btn, "btn-op");
    add_btn_class(btn, "btn-extra");
    gtk_grid_attach(GTK_GRID(extra), btn, 1, 2, 1, 1);
    btn = make_button("exp", state);
    add_btn_class(btn, "btn-op");
    add_btn_class(btn, "btn-extra");
    gtk_grid_attach(GTK_GRID(extra), btn, 2, 2, 1, 1);
    btn = make_button("pow", state);
    add_btn_class(btn, "btn-op");
    add_btn_class(btn, "btn-extra");
    gtk_grid_attach(GTK_GRID(extra), btn, 3, 2, 1, 1);
    btn = make_button("csc", state);
    add_btn_class(btn, "btn-op");
    add_btn_class(btn, "btn-extra");
    gtk_grid_attach(GTK_GRID(extra), btn, 0, 3, 1, 1);
    btn = make_button("sec", state);
    add_btn_class(btn, "btn-op");
    add_btn_class(btn, "btn-extra");
    gtk_grid_attach(GTK_GRID(extra), btn, 1, 3, 1, 1);
    btn = make_button("cot", state);
    add_btn_class(btn, "btn-op");
    add_btn_class(btn, "btn-extra");
    gtk_grid_attach(GTK_GRID(extra), btn, 2, 3, 1, 1);
    btn = make_button(",", state);
    add_btn_class(btn, "btn-op");
    add_btn_class(btn, "btn-extra");
    gtk_grid_attach(GTK_GRID(extra), btn, 3, 3, 1, 1);
    btn = make_button("deg", state);
    add_btn_class(btn, "btn-fn");
    gtk_grid_attach(GTK_GRID(extra), btn, 0, 4, 4, 1);

    state->mode_button = btn;

    gtk_window_present(GTK_WINDOW(window));
}
