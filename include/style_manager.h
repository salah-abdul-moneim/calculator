#pragma once

#include <gtk/gtk.h>

typedef struct StyleManager StyleManager;

// Create a manager that loads these CSS files and follows system light/dark preference.
// Relative paths are resolved relative to the executable directory.
StyleManager *style_manager_new(const char *dark_css_path, const char *light_css_path);

void style_manager_attach(StyleManager *m);
void style_manager_free(StyleManager *m);

