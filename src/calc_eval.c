#include "calc_eval.h"

#include <math.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

typedef enum {
    TOK_NUM,
    TOK_OP
} TokenType;

typedef struct {
    TokenType type;
    double value;
    char op;
} Token;

static gboolean is_func_op(char op) {
    return (op == 'S' || op == 'C' || op == 'T' || op == 'Q' ||
            op == 'L' || op == 'N' || op == 'G' || op == 'A' ||
            op == 'E' || op == 'P' || op == 'I' || op == 'J' || op == 'K');
}

static int op_precedence(char op) {
    switch (op) {
        case '!': return 5;
        case 'S':
        case 'C':
        case 'T':
        case 'Q':
        case 'L':
        case 'N':
        case 'G':
        case 'A':
        case 'E':
        case 'P':
        case 'I':
        case 'J':
        case 'K': return 5; // functions
        case '^': return 4;
        case 'u': return 3; // unary minus
        case '*':
        case '/':
        case '%': return 2;
        case '+':
        case '-': return 1;
        default: return 0;
    }
}

static gboolean op_right_assoc(char op) {
    return (op == '^' || op == 'u');
}

static gboolean is_operator_char(char c) {
    return (c == '+' || c == '-' || c == '*' || c == '/' || c == '^' || c == '%' || c == '!');
}

static double to_radians(double v, gboolean degrees) {
    return degrees ? (v * (G_PI / 180.0)) : v;
}

static int normalize_degrees(int deg) {
    int d = deg % 360;
    if (d < 0) d += 360;
    return d;
}

static gboolean special_trig_func(const char *func, int deg, char *out, size_t out_cap, double *out_val,
                                  char *err, size_t err_cap) {
    int d = normalize_degrees(deg);

    if (strcmp(func, "sin") == 0) {
        switch (d) {
            case 0:   *out_val = 0.0; g_snprintf(out, out_cap, "0"); return TRUE;
            case 30:  *out_val = 0.5; g_snprintf(out, out_cap, "1/2"); return TRUE;
            case 45:  *out_val = sqrt(2.0)/2.0; g_snprintf(out, out_cap, "sqrt(2)/2"); return TRUE;
            case 60:  *out_val = sqrt(3.0)/2.0; g_snprintf(out, out_cap, "sqrt(3)/2"); return TRUE;
            case 90:  *out_val = 1.0; g_snprintf(out, out_cap, "1"); return TRUE;
            case 120: *out_val = sqrt(3.0)/2.0; g_snprintf(out, out_cap, "sqrt(3)/2"); return TRUE;
            case 135: *out_val = sqrt(2.0)/2.0; g_snprintf(out, out_cap, "sqrt(2)/2"); return TRUE;
            case 150: *out_val = 0.5; g_snprintf(out, out_cap, "1/2"); return TRUE;
            case 180: *out_val = 0.0; g_snprintf(out, out_cap, "0"); return TRUE;
            case 210: *out_val = -0.5; g_snprintf(out, out_cap, "-1/2"); return TRUE;
            case 225: *out_val = -sqrt(2.0)/2.0; g_snprintf(out, out_cap, "-sqrt(2)/2"); return TRUE;
            case 240: *out_val = -sqrt(3.0)/2.0; g_snprintf(out, out_cap, "-sqrt(3)/2"); return TRUE;
            case 270: *out_val = -1.0; g_snprintf(out, out_cap, "-1"); return TRUE;
            case 300: *out_val = -sqrt(3.0)/2.0; g_snprintf(out, out_cap, "-sqrt(3)/2"); return TRUE;
            case 315: *out_val = -sqrt(2.0)/2.0; g_snprintf(out, out_cap, "-sqrt(2)/2"); return TRUE;
            case 330: *out_val = -0.5; g_snprintf(out, out_cap, "-1/2"); return TRUE;
        }
    } else if (strcmp(func, "cos") == 0) {
        switch (d) {
            case 0:   *out_val = 1.0; g_snprintf(out, out_cap, "1"); return TRUE;
            case 30:  *out_val = sqrt(3.0)/2.0; g_snprintf(out, out_cap, "sqrt(3)/2"); return TRUE;
            case 45:  *out_val = sqrt(2.0)/2.0; g_snprintf(out, out_cap, "sqrt(2)/2"); return TRUE;
            case 60:  *out_val = 0.5; g_snprintf(out, out_cap, "1/2"); return TRUE;
            case 90:  *out_val = 0.0; g_snprintf(out, out_cap, "0"); return TRUE;
            case 120: *out_val = -0.5; g_snprintf(out, out_cap, "-1/2"); return TRUE;
            case 135: *out_val = -sqrt(2.0)/2.0; g_snprintf(out, out_cap, "-sqrt(2)/2"); return TRUE;
            case 150: *out_val = -sqrt(3.0)/2.0; g_snprintf(out, out_cap, "-sqrt(3)/2"); return TRUE;
            case 180: *out_val = -1.0; g_snprintf(out, out_cap, "-1"); return TRUE;
            case 210: *out_val = -sqrt(3.0)/2.0; g_snprintf(out, out_cap, "-sqrt(3)/2"); return TRUE;
            case 225: *out_val = -sqrt(2.0)/2.0; g_snprintf(out, out_cap, "-sqrt(2)/2"); return TRUE;
            case 240: *out_val = -0.5; g_snprintf(out, out_cap, "-1/2"); return TRUE;
            case 270: *out_val = 0.0; g_snprintf(out, out_cap, "0"); return TRUE;
            case 300: *out_val = 0.5; g_snprintf(out, out_cap, "1/2"); return TRUE;
            case 315: *out_val = sqrt(2.0)/2.0; g_snprintf(out, out_cap, "sqrt(2)/2"); return TRUE;
            case 330: *out_val = sqrt(3.0)/2.0; g_snprintf(out, out_cap, "sqrt(3)/2"); return TRUE;
        }
    } else if (strcmp(func, "tan") == 0) {
        switch (d) {
            case 0:   *out_val = 0.0; g_snprintf(out, out_cap, "0"); return TRUE;
            case 30:  *out_val = sqrt(3.0)/3.0; g_snprintf(out, out_cap, "sqrt(3)/3"); return TRUE;
            case 45:  *out_val = 1.0; g_snprintf(out, out_cap, "1"); return TRUE;
            case 60:  *out_val = sqrt(3.0); g_snprintf(out, out_cap, "sqrt(3)"); return TRUE;
            case 90:
            case 270:
                g_snprintf(err, err_cap, "tan undefined");
                return FALSE;
            case 120: *out_val = -sqrt(3.0); g_snprintf(out, out_cap, "-sqrt(3)"); return TRUE;
            case 135: *out_val = -1.0; g_snprintf(out, out_cap, "-1"); return TRUE;
            case 150: *out_val = -sqrt(3.0)/3.0; g_snprintf(out, out_cap, "-sqrt(3)/3"); return TRUE;
            case 180: *out_val = 0.0; g_snprintf(out, out_cap, "0"); return TRUE;
            case 210: *out_val = sqrt(3.0)/3.0; g_snprintf(out, out_cap, "sqrt(3)/3"); return TRUE;
            case 225: *out_val = 1.0; g_snprintf(out, out_cap, "1"); return TRUE;
            case 240: *out_val = sqrt(3.0); g_snprintf(out, out_cap, "sqrt(3)"); return TRUE;
            case 300: *out_val = -sqrt(3.0); g_snprintf(out, out_cap, "-sqrt(3)"); return TRUE;
            case 315: *out_val = -1.0; g_snprintf(out, out_cap, "-1"); return TRUE;
            case 330: *out_val = -sqrt(3.0)/3.0; g_snprintf(out, out_cap, "-sqrt(3)/3"); return TRUE;
        }
    } else if (strcmp(func, "csc") == 0) {
        switch (d) {
            case 0:
            case 180:
                g_snprintf(err, err_cap, "csc undefined");
                return FALSE;
            case 30:  *out_val = 2.0; g_snprintf(out, out_cap, "2"); return TRUE;
            case 45:  *out_val = sqrt(2.0); g_snprintf(out, out_cap, "sqrt(2)"); return TRUE;
            case 60:  *out_val = 2.0/sqrt(3.0); g_snprintf(out, out_cap, "2/sqrt(3)"); return TRUE;
            case 90:  *out_val = 1.0; g_snprintf(out, out_cap, "1"); return TRUE;
            case 120: *out_val = 2.0/sqrt(3.0); g_snprintf(out, out_cap, "2/sqrt(3)"); return TRUE;
            case 135: *out_val = sqrt(2.0); g_snprintf(out, out_cap, "sqrt(2)"); return TRUE;
            case 150: *out_val = 2.0; g_snprintf(out, out_cap, "2"); return TRUE;
            case 210: *out_val = -2.0; g_snprintf(out, out_cap, "-2"); return TRUE;
            case 225: *out_val = -sqrt(2.0); g_snprintf(out, out_cap, "-sqrt(2)"); return TRUE;
            case 240: *out_val = -2.0/sqrt(3.0); g_snprintf(out, out_cap, "-2/sqrt(3)"); return TRUE;
            case 270: *out_val = -1.0; g_snprintf(out, out_cap, "-1"); return TRUE;
            case 300: *out_val = -2.0/sqrt(3.0); g_snprintf(out, out_cap, "-2/sqrt(3)"); return TRUE;
            case 315: *out_val = -sqrt(2.0); g_snprintf(out, out_cap, "-sqrt(2)"); return TRUE;
            case 330: *out_val = -2.0; g_snprintf(out, out_cap, "-2"); return TRUE;
        }
    } else if (strcmp(func, "sec") == 0) {
        switch (d) {
            case 90:
            case 270:
                g_snprintf(err, err_cap, "sec undefined");
                return FALSE;
            case 0:   *out_val = 1.0; g_snprintf(out, out_cap, "1"); return TRUE;
            case 30:  *out_val = 2.0/sqrt(3.0); g_snprintf(out, out_cap, "2/sqrt(3)"); return TRUE;
            case 45:  *out_val = sqrt(2.0); g_snprintf(out, out_cap, "sqrt(2)"); return TRUE;
            case 60:  *out_val = 2.0; g_snprintf(out, out_cap, "2"); return TRUE;
            case 120: *out_val = -2.0; g_snprintf(out, out_cap, "-2"); return TRUE;
            case 135: *out_val = -sqrt(2.0); g_snprintf(out, out_cap, "-sqrt(2)"); return TRUE;
            case 150: *out_val = -2.0/sqrt(3.0); g_snprintf(out, out_cap, "-2/sqrt(3)"); return TRUE;
            case 180: *out_val = -1.0; g_snprintf(out, out_cap, "-1"); return TRUE;
            case 210: *out_val = -2.0/sqrt(3.0); g_snprintf(out, out_cap, "-2/sqrt(3)"); return TRUE;
            case 225: *out_val = -sqrt(2.0); g_snprintf(out, out_cap, "-sqrt(2)"); return TRUE;
            case 240: *out_val = -2.0; g_snprintf(out, out_cap, "-2"); return TRUE;
            case 300: *out_val = 2.0; g_snprintf(out, out_cap, "2"); return TRUE;
            case 315: *out_val = sqrt(2.0); g_snprintf(out, out_cap, "sqrt(2)"); return TRUE;
            case 330: *out_val = 2.0/sqrt(3.0); g_snprintf(out, out_cap, "2/sqrt(3)"); return TRUE;
        }
    } else if (strcmp(func, "cot") == 0) {
        switch (d) {
            case 0:
            case 180:
                g_snprintf(err, err_cap, "cot undefined");
                return FALSE;
            case 30:  *out_val = sqrt(3.0); g_snprintf(out, out_cap, "sqrt(3)"); return TRUE;
            case 45:  *out_val = 1.0; g_snprintf(out, out_cap, "1"); return TRUE;
            case 60:  *out_val = sqrt(3.0)/3.0; g_snprintf(out, out_cap, "sqrt(3)/3"); return TRUE;
            case 90:  *out_val = 0.0; g_snprintf(out, out_cap, "0"); return TRUE;
            case 120: *out_val = -sqrt(3.0)/3.0; g_snprintf(out, out_cap, "-sqrt(3)/3"); return TRUE;
            case 135: *out_val = -1.0; g_snprintf(out, out_cap, "-1"); return TRUE;
            case 150: *out_val = -sqrt(3.0); g_snprintf(out, out_cap, "-sqrt(3)"); return TRUE;
            case 210: *out_val = sqrt(3.0); g_snprintf(out, out_cap, "sqrt(3)"); return TRUE;
            case 225: *out_val = 1.0; g_snprintf(out, out_cap, "1"); return TRUE;
            case 240: *out_val = sqrt(3.0)/3.0; g_snprintf(out, out_cap, "sqrt(3)/3"); return TRUE;
            case 270: *out_val = 0.0; g_snprintf(out, out_cap, "0"); return TRUE;
            case 300: *out_val = -sqrt(3.0)/3.0; g_snprintf(out, out_cap, "-sqrt(3)/3"); return TRUE;
            case 315: *out_val = -1.0; g_snprintf(out, out_cap, "-1"); return TRUE;
            case 330: *out_val = -sqrt(3.0); g_snprintf(out, out_cap, "-sqrt(3)"); return TRUE;
        }
    }

    return FALSE;
}

static gboolean add_token(Token *out, size_t *count, size_t cap, Token tok, char *err, size_t err_cap) {
    if (*count >= cap) {
        g_snprintf(err, err_cap, "expression too long");
        return FALSE;
    }
    out[(*count)++] = tok;
    return TRUE;
}

static gboolean shunting_yard(const char *expr, Token *output, size_t *out_count, char *err, size_t err_cap) {
    char op_stack[256];
    int op_top = -1;
    *out_count = 0;

    enum { PREV_NONE, PREV_NUM, PREV_OP, PREV_LPAREN, PREV_RPAREN } prev = PREV_NONE;

    const char *p = expr;
    while (*p) {
        if (isspace((unsigned char)*p)) { p++; continue; }

        if (isalpha((unsigned char)*p)) {
            char ident[8] = {0};
            size_t n = 0;
            while (isalpha((unsigned char)*p) && n < sizeof(ident) - 1) {
                ident[n++] = *p++;
            }
            char op = 0;
            if (strcmp(ident, "sin") == 0) op = 'S';
            else if (strcmp(ident, "cos") == 0) op = 'C';
            else if (strcmp(ident, "tan") == 0) op = 'T';
            else if (strcmp(ident, "sqrt") == 0) op = 'Q';
            else if (strcmp(ident, "log") == 0) op = 'L';
            else if (strcmp(ident, "ln") == 0) op = 'N';
            else if (strcmp(ident, "log2") == 0) op = 'G';
            else if (strcmp(ident, "abs") == 0) op = 'A';
            else if (strcmp(ident, "exp") == 0) op = 'E';
            else if (strcmp(ident, "pow") == 0) op = 'P';
            else if (strcmp(ident, "csc") == 0) op = 'I';
            else if (strcmp(ident, "sec") == 0) op = 'J';
            else if (strcmp(ident, "cot") == 0) op = 'K';
            else {
                g_snprintf(err, err_cap, "unknown function: %s", ident);
                return FALSE;
            }
            op_stack[++op_top] = op;
            prev = PREV_OP;
            continue;
        }

        if (isdigit((unsigned char)*p) || *p == '.') {
            char *endptr = NULL;
            double val = strtod(p, &endptr);
            if (endptr == p) { g_snprintf(err, err_cap, "invalid number"); return FALSE; }
            Token t = { .type = TOK_NUM, .value = val, .op = 0 };
            if (!add_token(output, out_count, 512, t, err, err_cap)) return FALSE;
            p = endptr;
            prev = PREV_NUM;
            continue;
        }

        if (*p == '(') { op_stack[++op_top] = '('; p++; prev = PREV_LPAREN; continue; }

        if (*p == ')') {
            gboolean found = FALSE;
            while (op_top >= 0) {
                char op = op_stack[op_top--];
                if (op == '(') { found = TRUE; break; }
                Token t = { .type = TOK_OP, .op = op };
                if (!add_token(output, out_count, 512, t, err, err_cap)) return FALSE;
            }
            if (!found) { g_snprintf(err, err_cap, "mismatched parentheses"); return FALSE; }
            if (op_top >= 0 && is_func_op(op_stack[op_top])) {
                char op = op_stack[op_top--];
                Token t = { .type = TOK_OP, .op = op };
                if (!add_token(output, out_count, 512, t, err, err_cap)) return FALSE;
            }
            p++; prev = PREV_RPAREN; continue;
        }

        if (*p == ',') {
            while (op_top >= 0 && op_stack[op_top] != '(') {
                char op = op_stack[op_top--];
                Token t = { .type = TOK_OP, .op = op };
                if (!add_token(output, out_count, 512, t, err, err_cap)) return FALSE;
            }
            if (op_top < 0) { g_snprintf(err, err_cap, "misplaced comma"); return FALSE; }
            p++; prev = PREV_OP; continue;
        }

        if (is_operator_char(*p)) {
            char op = *p;
            if (op == '-' && (prev == PREV_NONE || prev == PREV_OP || prev == PREV_LPAREN)) op = 'u';

            if (op == '!') {
                if (!(prev == PREV_NUM || prev == PREV_RPAREN)) { g_snprintf(err, err_cap, "factorial needs a value"); return FALSE; }
            } else {
                if (!(prev == PREV_NUM || prev == PREV_RPAREN) && op != 'u') { g_snprintf(err, err_cap, "operator missing value"); return FALSE; }
            }

            while (op_top >= 0) {
                char top = op_stack[op_top];
                if (top == '(') break;
                int p1 = op_precedence(op);
                int p2 = op_precedence(top);
                if ((!op_right_assoc(op) && p1 <= p2) || (op_right_assoc(op) && p1 < p2)) {
                    op_stack[op_top--] = 0;
                    Token t = { .type = TOK_OP, .op = top };
                    if (!add_token(output, out_count, 512, t, err, err_cap)) return FALSE;
                } else break;
            }

            op_stack[++op_top] = op;
            p++;
            prev = (op == '!') ? PREV_NUM : PREV_OP;
            continue;
        }

        g_snprintf(err, err_cap, "invalid character: %c", *p);
        return FALSE;
    }

    while (op_top >= 0) {
        char op = op_stack[op_top--];
        if (op == '(') { g_snprintf(err, err_cap, "mismatched parentheses"); return FALSE; }
        Token t = { .type = TOK_OP, .op = op };
        if (!add_token(output, out_count, 512, t, err, err_cap)) return FALSE;
    }
    return TRUE;
}

static gboolean eval_rpn(const Token *rpn, size_t count, gboolean degrees, double *out, char *err, size_t err_cap) {
    double stack[512];
    int top = -1;

    for (size_t i = 0; i < count; i++) {
        if (rpn[i].type == TOK_NUM) { stack[++top] = rpn[i].value; continue; }

        char op = rpn[i].op;
        if (op == 'u') {
            if (top < 0) { g_snprintf(err, err_cap, "invalid expression"); return FALSE; }
            stack[top] = -stack[top];
            continue;
        }

        if (op == '!') {
            if (top < 0) { g_snprintf(err, err_cap, "invalid expression"); return FALSE; }
            double v = stack[top];
            double r = round(v);
            if (v < 0 || fabs(v - r) > 1e-9) { g_snprintf(err, err_cap, "factorial requires a non-negative integer"); return FALSE; }
            if (r > 170) { g_snprintf(err, err_cap, "factorial overflow"); return FALSE; }
            double acc = 1.0;
            for (int k = 2; k <= (int)r; k++) acc *= (double)k;
            stack[top] = acc;
            continue;
        }

        if (is_func_op(op)) {
            if (top < 0) { g_snprintf(err, err_cap, "invalid expression"); return FALSE; }
            double a = stack[top];
            switch (op) {
                case 'S': stack[top] = sin(to_radians(a, degrees)); break;
                case 'C': stack[top] = cos(to_radians(a, degrees)); break;
                case 'T': stack[top] = tan(to_radians(a, degrees)); break;
                case 'Q':
                    if (a < 0.0) { g_snprintf(err, err_cap, "sqrt domain error"); return FALSE; }
                    stack[top] = sqrt(a);
                    break;
                case 'L':
                    if (a <= 0.0) { g_snprintf(err, err_cap, "log domain error"); return FALSE; }
                    stack[top] = log10(a);
                    break;
                case 'N':
                    if (a <= 0.0) { g_snprintf(err, err_cap, "ln domain error"); return FALSE; }
                    stack[top] = log(a);
                    break;
                case 'G':
                    if (a <= 0.0) { g_snprintf(err, err_cap, "log2 domain error"); return FALSE; }
                    stack[top] = log2(a);
                    break;
                case 'A':
                    stack[top] = fabs(a);
                    break;
                case 'E':
                    stack[top] = exp(a);
                    break;
                case 'I': {
                    double s = sin(to_radians(a, degrees));
                    if (s == 0.0) { g_snprintf(err, err_cap, "csc domain error"); return FALSE; }
                    stack[top] = 1.0 / s;
                    break;
                }
                case 'J': {
                    double c = cos(to_radians(a, degrees));
                    if (c == 0.0) { g_snprintf(err, err_cap, "sec domain error"); return FALSE; }
                    stack[top] = 1.0 / c;
                    break;
                }
                case 'K': {
                    double t = tan(to_radians(a, degrees));
                    if (t == 0.0) { g_snprintf(err, err_cap, "cot domain error"); return FALSE; }
                    stack[top] = 1.0 / t;
                    break;
                }
                default:
                    g_snprintf(err, err_cap, "unknown operator");
                    return FALSE;
            }
            continue;
        }

        if (top < 1) { g_snprintf(err, err_cap, "invalid expression"); return FALSE; }
        double b = stack[top--];
        double a = stack[top];

        switch (op) {
            case '+': stack[top] = a + b; break;
            case '-': stack[top] = a - b; break;
            case '*': stack[top] = a * b; break;
            case '/':
                if (b == 0.0) { g_snprintf(err, err_cap, "division by zero"); return FALSE; }
                stack[top] = a / b;
                break;
            case '%':
                if (b == 0.0) { g_snprintf(err, err_cap, "division by zero"); return FALSE; }
                stack[top] = fmod(a, b);
                break;
            case '^':
            case 'P':
                stack[top] = pow(a, b);
                break;
            default:
                g_snprintf(err, err_cap, "unknown operator");
                return FALSE;
        }
    }

    if (top != 0) { g_snprintf(err, err_cap, "invalid expression"); return FALSE; }
    *out = stack[top];
    return TRUE;
}

gboolean calc_eval(const char *expr, gboolean degrees, double *result, char *err, size_t err_cap) {
    Token rpn[512];
    size_t count = 0;

    if (!shunting_yard(expr, rpn, &count, err, err_cap)) return FALSE;
    if (!eval_rpn(rpn, count, degrees, result, err, err_cap)) return FALSE;
    return TRUE;
}

gboolean calc_try_special_trig(const char *expr, int *out_deg, char *display_out, size_t out_cap,
                               double *out_val, char *err, size_t err_cap) {
    // Recognize: <func>(<int-deg>) with optional whitespace.
    char name[8] = {0};
    const char *p = expr;
    while (isspace((unsigned char)*p)) p++;
    size_t n = 0;
    while (isalpha((unsigned char)*p) && n < sizeof(name) - 1) name[n++] = *p++;
    while (isspace((unsigned char)*p)) p++;
    if (n == 0 || *p != '(') return FALSE;
    p++;
    while (isspace((unsigned char)*p)) p++;
    char *endptr = NULL;
    double angle = strtod(p, &endptr);
    if (endptr == p) return FALSE;
    p = endptr;
    while (isspace((unsigned char)*p)) p++;
    if (*p != ')') return FALSE;
    p++;
    while (isspace((unsigned char)*p)) p++;
    if (*p != '\0') return FALSE;

    int deg = (int)llround(angle);
    if (fabs(angle - (double)deg) > 1e-9) return FALSE;
    if (out_deg) *out_deg = deg;
    return special_trig_func(name, deg, display_out, out_cap, out_val, err, err_cap);
}
