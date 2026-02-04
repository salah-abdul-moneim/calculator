#pragma once

#include <glib.h>
#include <stddef.h>

// Evaluates an expression. If degrees is TRUE, trig functions use degrees.
// Returns TRUE on success; otherwise returns FALSE and writes a short error into err.
gboolean calc_eval(const char *expr, gboolean degrees, double *result, char *err, size_t err_cap);

// Tries to produce a symbolic (Casio-like) result for simple trig expressions in degrees.
// Example: "cos(45)" -> "sqrt(2)/2". Returns TRUE if handled.
// On success, writes both display_out and out_val.
gboolean calc_try_special_trig(const char *expr, int *out_deg, char *display_out, size_t out_cap,
                               double *out_val, char *err, size_t err_cap);

