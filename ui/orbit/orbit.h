/*
 * Orbit — HeroOS App Launcher
 *
 * Press Super+Space to open. Type to search. Enter to launch.
 *
 * Features:
 *   - Full-text fuzzy search across apps, files, settings, commands
 *   - Instant results (< 5 ms response)
 *   - Recent apps and files
 *   - Calculator (type math expressions directly)
 *   - Unit converter (type "10km in miles")
 *   - Dictionary (type "define <word>")
 *   - System commands: shutdown, restart, lock, sleep
 *   - HeroPkg integration: install apps directly from Orbit
 *   - Keyboard-driven: arrow keys to navigate, Enter to launch
 *   - Customisable shortcuts (pinned apps)
 *
 * Visual:
 *   ┌──────────────────────────────────────────┐
 *   │  🔍  Search apps, files, commands...      │
 *   ├──────────────────────────────────────────┤
 *   │  ▸ Terminal    Open a terminal window     │
 *   │    Files       Browse your files          │
 *   │    Quill       Code editor                │
 *   │    Lens        Web browser                │
 *   │    Prism       System settings            │
 *   │    Wave        Music & video              │
 *   │    Pulse       System monitor             │
 *   └──────────────────────────────────────────┘
 *
 * Internal name: "orbit"
 * Display name:  "Orbit"
 */

#ifndef ORBIT_H
#define ORBIT_H

#include <kernel/types.h>

/* ── App entry in the registry ────────────────────────────────────────────── */
typedef struct {
    char     id[64];           /* Internal ID e.g. "terminal", "files" */
    char     display_name[64]; /* Shown to user */
    char     description[256];
    char     icon_path[256];
    char     exec[256];        /* Command to launch */
    char     categories[64];   /* e.g. "dev,system" */
    uint32_t launch_count;     /* For sorting by frequency */
    uint64_t last_used;        /* Unix timestamp */
} orbit_app_t;

/* ── Result item ──────────────────────────────────────────────────────────── */
typedef enum {
    ORBIT_RESULT_APP = 0,
    ORBIT_RESULT_FILE,
    ORBIT_RESULT_COMMAND,
    ORBIT_RESULT_SETTING,
    ORBIT_RESULT_CALCULATION,
} orbit_result_type_t;

typedef struct {
    orbit_result_type_t type;
    char   label[256];
    char   subtitle[256];
    char   action[512];    /* What to do when activated */
    float  score;          /* Match relevance (higher = better) */
} orbit_result_t;

/* ── Launcher state ───────────────────────────────────────────────────────── */
#define ORBIT_MAX_APPS     128
#define ORBIT_MAX_RESULTS   32
#define ORBIT_QUERY_MAX    256

typedef struct {
    orbit_app_t    apps[ORBIT_MAX_APPS];
    uint32_t       app_count;
    char           query[ORBIT_QUERY_MAX];
    orbit_result_t results[ORBIT_MAX_RESULTS];
    uint32_t       result_count;
    uint32_t       selected;
    bool           visible;
} orbit_launcher_t;

/* ── Built-in app registry ────────────────────────────────────────────────── */
static const orbit_app_t orbit_builtin_apps[] = {
    {
        "terminal",  "Terminal",  "Command-line terminal emulator",
        "/usr/share/icons/terminal.png",  "/bin/terminal",  "dev,system",  0, 0
    },
    {
        "files",     "Files",     "Browse and manage your files",
        "/usr/share/icons/files.png",     "/bin/files",     "system",      0, 0
    },
    {
        "quill",     "Quill",     "Code and text editor",
        "/usr/share/icons/quill.png",     "/bin/quill",     "dev",         0, 0
    },
    {
        "lens",      "Lens",      "Web browser",
        "/usr/share/icons/lens.png",      "/bin/lens",      "internet",    0, 0
    },
    {
        "prism",     "Prism",     "System settings",
        "/usr/share/icons/prism.png",     "/bin/prism",     "system",      0, 0
    },
    {
        "wave",      "Wave",      "Music and video player",
        "/usr/share/icons/wave.png",      "/bin/wave",      "media",       0, 0
    },
    {
        "pulse",     "Pulse",     "System monitor & diagnostics",
        "/usr/share/icons/pulse.png",     "/bin/pulse",     "system,dev",  0, 0
    },
    {
        "heroshell", "HeroShell", "Advanced system shell",
        "/usr/share/icons/heroshell.png", "/bin/heroshell", "dev,system",  0, 0
    },
};
#define ORBIT_BUILTIN_COUNT  (sizeof(orbit_builtin_apps) / sizeof(orbit_builtin_apps[0]))

/* ── Public API ───────────────────────────────────────────────────────────── */
void orbit_init(orbit_launcher_t *launcher);
void orbit_show(orbit_launcher_t *launcher);
void orbit_hide(orbit_launcher_t *launcher);
void orbit_search(orbit_launcher_t *launcher, const char *query);
void orbit_launch(orbit_launcher_t *launcher, uint32_t result_index);
void orbit_main(void);

#endif /* ORBIT_H */
