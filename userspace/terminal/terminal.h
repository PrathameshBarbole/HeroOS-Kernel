/*
 * Terminal — HeroOS Terminal Emulator
 *
 * A fast, GPU-accelerated terminal emulator with a premium feel.
 *
 * Features:
 *   - Runs HeroShell (or any shell) via PTY
 *   - Full VT100/ANSI escape code support
 *   - Tabs + split panes (vertical / horizontal)
 *   - Smooth scrollback with configurable buffer size (100k lines default)
 *   - Custom fonts (HeroMono — embedded bitmap + optional TTF)
 *   - Ligature support for coding fonts
 *   - Per-tab colour profiles
 *   - Keyboard shortcuts: new tab (Ctrl+T), split (Ctrl+\), close (Ctrl+W)
 *   - URL detection (clickable links)
 *   - Search within output (Ctrl+F)
 *   - SSH integration: one-click remote connection
 *   - Transparency and blur background effect (when Canvas compositor is active)
 *
 * Internal name: "terminal"
 * Display name:  "Terminal"
 */

#ifndef TERMINAL_H
#define TERMINAL_H

#include <kernel/types.h>

/* ── ANSI escape state machine ──────────────────────────────────────────── */
typedef enum {
    TERM_STATE_NORMAL = 0,
    TERM_STATE_ESCAPE,        /* Got ESC */
    TERM_STATE_CSI,           /* Got ESC [ */
    TERM_STATE_OSC,           /* Got ESC ] */
} term_esc_state_t;

/* ── Cell attributes ─────────────────────────────────────────────────────── */
#define TERM_ATTR_BOLD       BIT(0)
#define TERM_ATTR_DIM        BIT(1)
#define TERM_ATTR_ITALIC     BIT(2)
#define TERM_ATTR_UNDERLINE  BIT(3)
#define TERM_ATTR_BLINK      BIT(4)
#define TERM_ATTR_REVERSE    BIT(5)
#define TERM_ATTR_STRIKE     BIT(6)

/* ── Terminal cell ────────────────────────────────────────────────────────── */
typedef struct {
    uint32_t codepoint;   /* Unicode code point */
    uint32_t fg;          /* Foreground colour (ARGB) */
    uint32_t bg;          /* Background colour (ARGB) */
    uint8_t  attrs;       /* Attribute flags */
} term_cell_t;

/* ── Terminal dimensions ─────────────────────────────────────────────────── */
#define TERM_DEFAULT_COLS    220
#define TERM_DEFAULT_ROWS     50
#define TERM_SCROLLBACK      100000   /* Lines */
#define TERM_MAX_TABS          16

/* ── PTY (pseudo-terminal) ───────────────────────────────────────────────── */
typedef struct {
    int      master_fd;
    int      slave_fd;
    uint32_t cols, rows;
} pty_t;

/* ── Tab ─────────────────────────────────────────────────────────────────── */
typedef struct {
    char     title[128];
    pty_t    pty;
    term_cell_t *cells;   /* cols × rows grid */
    uint32_t cols, rows;
    uint32_t cursor_col, cursor_row;
    term_esc_state_t esc_state;
    uint8_t  esc_params[16];
    int      esc_param_count;
    bool     active;
} term_tab_t;

/* ── Terminal window ─────────────────────────────────────────────────────── */
typedef struct {
    term_tab_t tabs[TERM_MAX_TABS];
    uint32_t   tab_count;
    uint32_t   active_tab;
    bool       show_tabbar;
    float      opacity;         /* 0.0–1.0 background transparency */
} terminal_t;

/* ── Colour themes ───────────────────────────────────────────────────────── */
typedef struct {
    const char *name;
    uint32_t    background;
    uint32_t    foreground;
    uint32_t    cursor;
    uint32_t    selection;
    uint32_t    colours[16];   /* Standard 16 ANSI colours */
} term_theme_t;

/* Built-in themes */
extern const term_theme_t term_theme_hero_dark;    /* Default: HeroOS Dark */
extern const term_theme_t term_theme_hero_light;   /* Light variant */
extern const term_theme_t term_theme_midnight;     /* Deep blue */
extern const term_theme_t term_theme_aurora;       /* Green on dark */

/* ── Public API ──────────────────────────────────────────────────────────── */
void terminal_init(void);
void terminal_main(void);
terminal_t *terminal_create(uint32_t cols, uint32_t rows);
int  terminal_open_tab(terminal_t *term, const char *shell);
void terminal_close_tab(terminal_t *term, uint32_t idx);
void terminal_write(term_tab_t *tab, const char *data, size_t len);
void terminal_resize(term_tab_t *tab, uint32_t cols, uint32_t rows);
void terminal_set_theme(terminal_t *term, const term_theme_t *theme);

#endif /* TERMINAL_H */
