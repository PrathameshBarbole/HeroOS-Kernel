/*
 * Lens — HeroOS Web Browser
 *
 * A fast, privacy-focused web browser with a minimal, premium UI.
 *
 * Features:
 *   - Tabbed browsing with tab groups
 *   - Address + search bar with inline suggestions
 *   - Site isolation (each tab runs in its own sandbox namespace)
 *   - Ad & tracker blocking built-in (no extension needed)
 *   - Reading mode — strip clutter, focus on content
 *   - Picture-in-picture video player
 *   - Download manager with progress
 *   - Password manager integration (stored encrypted in /secure)
 *   - Dark mode for all websites
 *   - Local history + bookmarks sync via HeroServe
 *   - Dev Tools panel (Inspector, Console, Network, Performance)
 *   - WebAssembly support
 *
 * Internal name: "lens"
 * Display name:  "Lens"
 */

#ifndef LENS_H
#define LENS_H

#include <kernel/types.h>

#define LENS_URL_MAX         2048
#define LENS_TITLE_MAX        256
#define LENS_MAX_TABS          64
#define LENS_MAX_HISTORY     1000
#define LENS_MAX_BOOKMARKS    500

/* ── Tab state ────────────────────────────────────────────────────────────── */
typedef enum {
    LENS_TAB_LOADING = 0,
    LENS_TAB_COMPLETE,
    LENS_TAB_ERROR,
} lens_tab_state_t;

typedef struct {
    char             url[LENS_URL_MAX];
    char             title[LENS_TITLE_MAX];
    lens_tab_state_t state;
    bool             can_go_back;
    bool             can_go_forward;
    bool             muted;
    bool             pinned;
    /* Navigation history */
    char  nav_history[64][LENS_URL_MAX];
    int   nav_pos;
    int   nav_len;
} lens_tab_t;

/* ── Bookmark ─────────────────────────────────────────────────────────────── */
typedef struct {
    char url[LENS_URL_MAX];
    char title[LENS_TITLE_MAX];
    char folder[64];
    uint64_t added_at;
} lens_bookmark_t;

/* ── History entry ────────────────────────────────────────────────────────── */
typedef struct {
    char     url[LENS_URL_MAX];
    char     title[LENS_TITLE_MAX];
    uint64_t visited_at;
    uint32_t visit_count;
} lens_history_entry_t;

/* ── Browser settings ─────────────────────────────────────────────────────── */
typedef struct {
    char     homepage[LENS_URL_MAX];
    bool     block_ads;
    bool     block_trackers;
    bool     https_only;
    bool     dark_mode_all;
    bool     enable_cookies;
    bool     enable_js;
    bool     save_passwords;
    uint32_t max_connections_per_host;
} lens_settings_t;

/* ── Browser state ────────────────────────────────────────────────────────── */
typedef struct {
    lens_tab_t          tabs[LENS_MAX_TABS];
    uint32_t            tab_count;
    uint32_t            active_tab;
    lens_bookmark_t     bookmarks[LENS_MAX_BOOKMARKS];
    uint32_t            bookmark_count;
    lens_history_entry_t history[LENS_MAX_HISTORY];
    uint32_t            history_count;
    lens_settings_t     settings;
    bool                sidebar_open;
    bool                reading_mode;
} lens_browser_t;

/* ── Public API ───────────────────────────────────────────────────────────── */
void lens_init(void);
void lens_main(void);
void lens_navigate(lens_tab_t *tab, const char *url);
void lens_go_back(lens_tab_t *tab);
void lens_go_forward(lens_tab_t *tab);
void lens_reload(lens_tab_t *tab);
int  lens_open_tab(lens_browser_t *browser, const char *url);
void lens_close_tab(lens_browser_t *browser, uint32_t idx);
void lens_add_bookmark(lens_browser_t *browser, const char *url, const char *title);

/* Default home page */
#define LENS_DEFAULT_HOMEPAGE  "heroos://newtab"

#endif /* LENS_H */
