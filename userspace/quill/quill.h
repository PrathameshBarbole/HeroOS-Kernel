/*
 * Quill — HeroOS Text Editor
 *
 * A fast, modern code editor with a clean, minimal interface.
 *
 * Features:
 *   - Syntax highlighting for 30+ languages (C, Python, JS, Go, Rust, …)
 *   - Language Server Protocol (LSP) integration for autocomplete + diagnostics
 *   - Multiple cursors and column selection
 *   - Fuzzy file finder (Ctrl+P)
 *   - Command palette (Ctrl+Shift+P)
 *   - Git blame / diff gutter integration
 *   - Minimap
 *   - Tabs and split view
 *   - Vim keybindings (optional mode)
 *   - Extensible via Quill plugins (QScript — lightweight JS-like scripting)
 *   - HeroServe integration: open project from editor, instant serve
 *
 * Internal name: "quill"
 * Display name:  "Quill"
 */

#ifndef QUILL_H
#define QUILL_H

#include <kernel/types.h>

/* ── Buffer / document ───────────────────────────────────────────────────── */
#define QUILL_PATH_MAX     4096
#define QUILL_MAX_TABS       64

typedef struct quill_line {
    char    *text;
    size_t   len;
    size_t   capacity;
    struct quill_line *prev;
    struct quill_line *next;
} quill_line_t;

typedef struct {
    quill_line_t *head;     /* First line */
    quill_line_t *tail;     /* Last line */
    uint64_t      line_count;
    uint64_t      char_count;
    char          path[QUILL_PATH_MAX];
    bool          modified;
    bool          read_only;
} quill_buffer_t;

/* ── Cursor ───────────────────────────────────────────────────────────────── */
typedef struct {
    uint64_t line;    /* 0-based */
    uint64_t col;     /* 0-based */
    uint64_t preferred_col;   /* For up/down navigation */
} quill_cursor_t;

/* ── Selection ───────────────────────────────────────────────────────────── */
typedef struct {
    quill_cursor_t start;
    quill_cursor_t end;
    bool active;
} quill_selection_t;

/* ── Viewport ─────────────────────────────────────────────────────────────── */
typedef struct {
    uint64_t first_line;    /* Top visible line */
    uint64_t first_col;     /* Left visible column (horizontal scroll) */
    uint32_t visible_lines;
    uint32_t visible_cols;
} quill_viewport_t;

/* ── Language / syntax ───────────────────────────────────────────────────── */
typedef enum {
    QUILL_LANG_PLAIN = 0,
    QUILL_LANG_C,
    QUILL_LANG_CPP,
    QUILL_LANG_PYTHON,
    QUILL_LANG_JAVASCRIPT,
    QUILL_LANG_TYPESCRIPT,
    QUILL_LANG_GO,
    QUILL_LANG_RUST,
    QUILL_LANG_MARKDOWN,
    QUILL_LANG_JSON,
    QUILL_LANG_YAML,
    QUILL_LANG_TOML,
    QUILL_LANG_BASH,
    QUILL_LANG_ASM,
    QUILL_LANG_COUNT,
} quill_lang_t;

/* ── Tab ──────────────────────────────────────────────────────────────────── */
typedef struct {
    quill_buffer_t    buffer;
    quill_cursor_t    cursors[32];   /* Multi-cursor */
    uint32_t          cursor_count;
    quill_selection_t selection;
    quill_viewport_t  viewport;
    quill_lang_t      language;
    uint32_t          tab_size;      /* Spaces per tab (default 4) */
    bool              use_spaces;
} quill_tab_t;

/* ── Editor state ─────────────────────────────────────────────────────────── */
typedef struct {
    quill_tab_t  tabs[QUILL_MAX_TABS];
    uint32_t     tab_count;
    uint32_t     active_tab;
    bool         vim_mode;
    bool         minimap_visible;
    bool         sidebar_visible;   /* File tree */
    bool         statusbar_visible;
} quill_editor_t;

/* ── Public API ───────────────────────────────────────────────────────────── */
void quill_init(void);
void quill_main(void);

quill_buffer_t *quill_open_file(const char *path);
int  quill_save_file(quill_buffer_t *buf);
int  quill_save_file_as(quill_buffer_t *buf, const char *path);
void quill_close_buffer(quill_buffer_t *buf);

void quill_insert_char(quill_tab_t *tab, uint32_t codepoint);
void quill_delete_char(quill_tab_t *tab, bool forward);
void quill_insert_newline(quill_tab_t *tab);
void quill_move_cursor(quill_tab_t *tab, int64_t dline, int64_t dcol);

int  quill_search(quill_tab_t *tab, const char *query, bool regex,
                  bool case_sensitive);
int  quill_replace(quill_tab_t *tab, const char *find, const char *replace_str,
                   bool all);

quill_lang_t quill_detect_language(const char *path);

#endif /* QUILL_H */
