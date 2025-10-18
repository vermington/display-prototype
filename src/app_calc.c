// src/app_calc.c
#include "app_calc.h"
#include "gfx.h"
#include "ssd1306.h"
#include "port.h"

#include <ctype.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

// ---------- Display mode ----------
typedef enum { DISP_HEX = 0, DISP_DEC, DISP_BIN } display_mode_t;

// ---------- Calculator state ----------
typedef enum {
    ST_EXPECT_ANY = 0,   // number = load result; operator = wait for arg
    ST_EXPECT_ARG
} calc_state_t;

typedef enum {
    OP_NONE = 0,
    OP_ADD, OP_SUB, OP_AND, OP_OR, OP_XOR, OP_SHL, OP_SHR,
    OP_INVERT,          // unary, immediate
    OP_SHOW_HEX, OP_SHOW_DEC, OP_SHOW_BIN   // display-change "ops"
} op_t;

// ---------- Small string helpers ----------
static void strtrim(char *s) {
    if (!s) return;
    size_t i = 0; while (s[i] && isspace((unsigned char)s[i])) ++i;
    if (i) memmove(s, s + i, strlen(s + i) + 1);
    size_t n = strlen(s);
    while (n && isspace((unsigned char)s[n - 1])) s[--n] = '\0';
}

static void remove_underscores(char *s) {
    char *w = s, *r = s;
    while (*r) { if (*r != '_') *w++ = *r; ++r; }
    *w = '\0';
}

static bool streq_ci(const char *a, const char *b) {
    for (; *a && *b; ++a, ++b) {
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b)) return false;
    }
    return *a == '\0' && *b == '\0';
}

// ---------- Number parsing ----------
static int parse_hex64(const char *in, uint64_t *out) {
    if (!in || !out) return -1;
    char buf[128]; strncpy(buf, in, sizeof(buf)-1); buf[sizeof(buf)-1] = '\0';
    strtrim(buf); remove_underscores(buf);
    const char *s = buf;
    if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) s += 2;
    if (!*s) return -2;
    int digits = 0;
    for (const char *p = s; *p; ++p) { if (!isxdigit((unsigned char)*p)) return -3; ++digits; }
    if (digits > 16) return -4;
    uint64_t v = 0;
    for (int i = 0; s[i]; ++i) {
        char c = s[i];
        uint8_t n = (c <= '9') ? (uint8_t)(c - '0') :
                    (c <= 'F') ? (uint8_t)(10 + c - 'A') :
                                 (uint8_t)(10 + c - 'a');
        v = (v << 4) | n;
    }
    *out = v; return 0;
}

static int parse_bin64(const char *in, uint64_t *out) {
    if (!in || !out) return -1;
    char buf[256]; strncpy(buf, in, sizeof(buf)-1); buf[sizeof(buf)-1] = '\0';
    strtrim(buf); remove_underscores(buf);
    const char *s = buf;
    if (s[0] == '0' && (s[1] == 'b' || s[1] == 'B')) s += 2;
    if (!*s) return -2;
    int digits = 0;
    uint64_t v = 0;
    while (*s) {
        char c = *s++;
        if (c != '0' && c != '1') return -3;
        if (++digits > 64) return -4;
        v = (v << 1) | (uint64_t)(c - '0');
    }
    *out = v; return 0;
}

static int parse_dec64(const char *in, uint64_t *out) {
    if (!in || !out) return -1;
    char buf[128]; strncpy(buf, in, sizeof(buf)-1); buf[sizeof(buf)-1] = '\0';
    strtrim(buf); remove_underscores(buf);

    if (!*buf) return -2;
    for (const char *p = buf; *p; ++p) if (!isdigit((unsigned char)*p)) return -3;

    errno = 0;
    unsigned long long v = strtoull(buf, NULL, 10);
    if (errno == ERANGE) return -4;
    *out = (uint64_t)v; return 0;
}

static int parse_num64_anybase(const char *token, uint64_t *out) {
    if (!token || !*token) return -1;
    if (token[0]=='0' && (token[1]=='x' || token[1]=='X')) return parse_hex64(token, out);
    if (token[0]=='0' && (token[1]=='b' || token[1]=='B')) return parse_bin64(token, out);
    if (token[0]=='0' && (token[1]=='d' || token[1]=='D')) return parse_dec64(token+2, out);
    return parse_dec64(token, out); // default decimal
}

// ---------- Formatting ----------
static void fmt_hex64_us(uint64_t v, char out[2 + 16 + 3 + 1]) {
    static const char HEX[] = "0123456789ABCDEF";
    char raw[16];
    for (int i = 0; i < 16; ++i) { raw[15 - i] = HEX[(uint8_t)(v & 0xF)]; v >>= 4; }
    char *o = out; *o++='0'; *o++='x';
    for (int i = 0; i < 16; ++i) { *o++ = raw[i]; if (i==3||i==7||i==11) *o++ = '_'; }
    *o = '\0';
}

static void fmt_dec64(uint64_t v, char out[32]) {
    snprintf(out, 32, "%" PRIu64, v);
}

// Show tail 16 bits; prefix "0b" and ellipsis if higher bits present.
static void fmt_bin_tail16(uint64_t v, char out[2 + 3 + 16 + 1]) {
    char *o = out;
    *o++='0'; *o++='b';
    if ((v >> 16) != 0) { *o++='.'; *o++='.'; *o++='.'; }
    for (int i = 15; i >= 0; --i) *o++ = ((v >> i) & 1u) ? '1' : '0';
    *o = '\0';
}

// ---------- Bit grid ----------
static void draw_bitgrid64(ssd1306_t *dev, uint64_t value) {
    const int rows = gfx_text_rows(dev);
    const int grid_row0 = rows - 4; if (grid_row0 < 0) return;

    const int COLS = 16;
    const int CELL = 7;    // px per cell horizontally
    const int BOX  = 5;    // box size
    const int EXTRA_GAP = 1; // extra px after each nibble

    const int grid_w = COLS * CELL + 3 * EXTRA_GAP;
    const int start_x = (int)(dev->width - grid_w) / 2;
    const int margin_x = (CELL - BOX) / 2;
    const int margin_y = (8 - BOX) / 2;

    for (int r = 0; r < 4; ++r) {
        const int y = (grid_row0 + r) * 8 + margin_y;
        for (int c = 0; c < COLS; ++c) {
            const int x = start_x + c * CELL + (c / 4) * EXTRA_GAP + margin_x;
            const int bit_index = 63 - (r * 16) - c; // left→right: MSB..LSB of each 16-bit slice
            const int bit = (int)((value >> bit_index) & 1u);
            if (bit) gfx_fill_rect(dev, x, y, BOX, BOX, 1);
            else     gfx_draw_rect(dev, x, y, BOX, BOX, 1);
        }
    }
}

// ---------- Rendering ----------
static void render_calc(ssd1306_t *dev,
                        const char *input_line,
                        bool        last_token_was_op,
                        bool        show_operation,
                        const char *operation_label,
                        uint64_t    result,
                        display_mode_t disp_mode)
{
    ssd1306_clear(dev);

    const int rows = gfx_text_rows(dev);          // 128x64 → 8 rows
    const int head_row = rows - 5;                // result headline
    const int grid_row0 = rows - 4;

    // Row 0: last token shown ONLY if it was NOT an operation
    if (!last_token_was_op && input_line && *input_line) {
        gfx_print_line(dev, input_line, 0, GFX_ALIGN_LEFT);
    } else {
        gfx_clear_line(dev, 0);
    }

    // Row 1: OPERATION line (blank only when an immediate value was loaded)
    if (show_operation && operation_label && *operation_label) {
        char buf[64];
        snprintf(buf, sizeof(buf), "OPERATION: %s", operation_label);
        gfx_print_line(dev, buf, 1, GFX_ALIGN_LEFT);
    } else {
        gfx_clear_line(dev, 1);
    }

    // Result headline in selected display base
    if (head_row >= 0) {
        switch (disp_mode) {
            case DISP_HEX: {
                char hx[2+16+3+1]; fmt_hex64_us(result, hx);
                gfx_print_line(dev, hx, head_row, GFX_ALIGN_CENTER);
            } break;
            case DISP_DEC: {
                char dc[32]; fmt_dec64(result, dc);
                gfx_print_line(dev, dc, head_row, GFX_ALIGN_CENTER);
            } break;
            case DISP_BIN: {
                char bn[2+3+16+1]; fmt_bin_tail16(result, bn);
                gfx_print_line(dev, bn, head_row, GFX_ALIGN_CENTER);
            } break;
        }
    }

    // Bottom 4 rows: grid
    if (grid_row0 >= 0) {
        for (int r = 0; r < 4; ++r) gfx_clear_line(dev, grid_row0 + r);
        draw_bitgrid64(dev, result);
    }

    ssd1306_update_full(dev);
}

// ---------- Operator parsing ----------
static op_t parse_operator(const char *token) {
    if (!token) return OP_NONE;

    // Symbols first
    if (streq_ci(token, "+"))  return OP_ADD;
    if (streq_ci(token, "-"))  return OP_SUB;
    if (streq_ci(token, "&"))  return OP_AND;
    if (streq_ci(token, "|"))  return OP_OR;
    if (streq_ci(token, "^"))  return OP_XOR;
    if (streq_ci(token, "~"))  return OP_INVERT;
    if (strcmp(token, "<<") == 0) return OP_SHL;
    if (strcmp(token, ">>") == 0) return OP_SHR;

    // Words
    if (streq_ci(token, "add"))      return OP_ADD;
    if (streq_ci(token, "subtract")) return OP_SUB;
    if (streq_ci(token, "and"))      return OP_AND;
    if (streq_ci(token, "or"))       return OP_OR;
    if (streq_ci(token, "xor"))      return OP_XOR;
    if (streq_ci(token, "invert"))   return OP_INVERT;

    if (streq_ci(token, "hex"))      return OP_SHOW_HEX;
    if (streq_ci(token, "dec"))      return OP_SHOW_DEC;
    if (streq_ci(token, "bin"))      return OP_SHOW_BIN;

    return OP_NONE;
}

static const char* op_label(op_t op) {
    switch (op) {
        case OP_ADD: return "add";
        case OP_SUB: return "subtract";
        case OP_AND: return "and";
        case OP_OR:  return "or";
        case OP_XOR: return "xor";
        case OP_SHL: return "<<";
        case OP_SHR: return ">>";
        case OP_INVERT: return "invert";
        case OP_SHOW_HEX: return "display hex";
        case OP_SHOW_DEC: return "display dec";
        case OP_SHOW_BIN: return "display bin";
        default: return "";
    }
}

static bool is_binary_op(op_t op) {
    return (op == OP_ADD || op == OP_SUB || op == OP_AND || op == OP_OR ||
            op == OP_XOR || op == OP_SHL || op == OP_SHR);
}

// ---------- Public entry ----------
void app_run_calc(ssd1306_t *dev) {
    if (!dev) return;

    uint64_t        result = 0;
    calc_state_t    state  = ST_EXPECT_ANY;
    display_mode_t  disp   = DISP_HEX;

    op_t            pending = OP_NONE;

    // OPERATION display policy:
    //  - shown when an operator is pending OR a non-immediate op just executed
    //  - cleared only when an immediate value load occurs
    bool            show_op = false;
    char            op_name[32] = {0};

    // Row 0 policy: show last token only if it was a number (not an op)
    bool            last_was_op = false;

    char input_line[256] = {0};  // what to render on row 0 (as typed)

    // Initial screen
    render_calc(dev, input_line, last_was_op, show_op, op_name, result, disp);

    char line[256];
    for (;;) {
        printf("[result=0x%016" PRIX64 "] Enter number (0x/0b/0d or dec), op (+,-,<<,>>,and,or,xor,invert,hex,dec,bin), 'c' clear, 'q' quit:\n> ",
               result);
        fflush(stdout);

        if (!fgets(line, sizeof(line), stdin)) { putchar('\n'); break; }
        strtrim(line);
        if (!*line) { continue; }

        // Remember input as-typed
        strncpy(input_line, line, sizeof(input_line)-1);
        input_line[sizeof(input_line)-1] = '\0';

        // Quit / Clear
        if ((line[0]=='q'||line[0]=='Q') && line[1]=='\0') break;
        if ((line[0]=='c'||line[0]=='C') && line[1]=='\0') {
            result = 0;
            state  = ST_EXPECT_ANY;
            pending = OP_NONE;
            show_op = false; op_name[0] = '\0';
            last_was_op = false; // immediate load
            render_calc(dev, input_line, last_was_op, show_op, op_name, result, disp);
            continue;
        }

        // Operator?
        op_t op = parse_operator(line);
        if (op != OP_NONE) {
            // These are operations → don't show token on row 0
            last_was_op = true;

            if (op == OP_SHOW_HEX || op == OP_SHOW_DEC || op == OP_SHOW_BIN) {
                disp = (op == OP_SHOW_HEX) ? DISP_HEX : (op == OP_SHOW_DEC ? DISP_DEC : DISP_BIN);
                show_op = true;
                strncpy(op_name, op_label(op), sizeof(op_name)-1);
                op_name[sizeof(op_name)-1] = '\0';
                render_calc(dev, input_line, last_was_op, show_op, op_name, result, disp);
                continue;
            }
            if (op == OP_INVERT) {
                result = ~result;
                show_op = true;
                strncpy(op_name, op_label(op), sizeof(op_name)-1);
                op_name[sizeof(op_name)-1] = '\0';
                state = ST_EXPECT_ANY; pending = OP_NONE;
                render_calc(dev, input_line, last_was_op, show_op, op_name, result, disp);
                continue;
            }
            // Binary operator: set pending and wait for arg
            pending = op;
            show_op = true;
            strncpy(op_name, op_label(op), sizeof(op_name)-1);
            op_name[sizeof(op_name)-1] = '\0';
            state = ST_EXPECT_ARG;
            render_calc(dev, input_line, last_was_op, show_op, op_name, result, disp);
            continue;
        }

        // Number?
        uint64_t arg = 0;
        int nerr = parse_num64_anybase(line, &arg);
        if (nerr != 0) {
            printf(" !! Invalid number. Examples: 0x1A2B, 0b1010_1111, 0d42, 1234\n");
            continue;
        }

        // Now the last token is a value → show it on row 0
        last_was_op = false;

        if (state == ST_EXPECT_ANY) {
            // Immediate load
            result = arg;
            pending = OP_NONE;
            state   = ST_EXPECT_ANY;
            show_op = false; op_name[0] = '\0';   // blank on immediate load
            render_calc(dev, input_line, last_was_op, show_op, op_name, result, disp);
        } else {
            // Apply pending binary op with this arg
            uint8_t sh = (uint8_t)(arg & 63u);  // clamp shift 0..63
            switch (pending) {
                case OP_ADD: result = (uint64_t)(result + arg); break;
                case OP_SUB: result = (uint64_t)(result - arg); break;
                case OP_AND: result = (uint64_t)(result & arg); break;
                case OP_OR:  result = (uint64_t)(result | arg); break;
                case OP_XOR: result = (uint64_t)(result ^ arg); break;
                case OP_SHL: result = (result << sh); break;
                case OP_SHR: result = (result >> sh); break;
                default: break;
            }
            state = ST_EXPECT_ANY;
            pending = OP_NONE;
            // Keep OPERATION visible (do not blank it here)
            render_calc(dev, input_line, last_was_op, show_op, op_name, result, disp);
        }
    }

    port_delay_ms(200);
}