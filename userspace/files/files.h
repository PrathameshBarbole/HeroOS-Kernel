/*
 * Files — HeroOS File Manager
 *
 * A clean, fast file explorer with a premium look.
 *
 * Features:
 *   - Dual-pane or single-pane layout
 *   - Breadcrumb navigation bar
 *   - File/folder operations: cut, copy, paste, rename, delete, archive
 *   - Drag-and-drop support
 *   - File preview panel (image thumbnails, text preview)
 *   - Context menus with keyboard shortcuts
 *   - Search with real-time filtering
 *   - Bookmarks / quick-access sidebar
 *   - Themeable via Prism settings
 *   - Integrated terminal pane (opens Terminal in current dir)
 *   - Developer extras: show hidden files, permissions, file sizes
 *
 * Internal name used in the kernel/IPC: "files"
 * Display name shown to the user:       "Files"
 */

#ifndef FILES_H
#define FILES_H

#include <kernel/types.h>
#include <kernel/fs/vfs.h>

/* ── View modes ─────────────────────────────────────────────────────────── */
typedef enum {
    FILES_VIEW_GRID  = 0,   /* Large icons with labels */
    FILES_VIEW_LIST  = 1,   /* Detailed list with columns */
    FILES_VIEW_COLS  = 2,   /* macOS-style column view */
    FILES_VIEW_TILES = 3,   /* Medium tiles */
} files_view_t;

/* ── Sort order ─────────────────────────────────────────────────────────── */
typedef enum {
    FILES_SORT_NAME    = 0,
    FILES_SORT_SIZE    = 1,
    FILES_SORT_DATE    = 2,
    FILES_SORT_TYPE    = 3,
} files_sort_t;

/* ── File entry (display model) ─────────────────────────────────────────── */
#define FILES_NAME_MAX  256

typedef struct {
    char     name[FILES_NAME_MAX];
    char     path[VFS_PATH_MAX];
    uint64_t size;
    uint32_t type;            /* VFS node type */
    uint64_t mtime;
    bool     hidden;
    bool     selected;
} files_entry_t;

/* ── Bookmark ────────────────────────────────────────────────────────────── */
typedef struct {
    char label[64];
    char path[VFS_PATH_MAX];
} files_bookmark_t;

/* ── Panel state ─────────────────────────────────────────────────────────── */
#define FILES_MAX_ENTRIES   4096
#define FILES_MAX_BOOKMARKS   32

typedef struct {
    char              cwd[VFS_PATH_MAX];
    files_entry_t     entries[FILES_MAX_ENTRIES];
    uint32_t          entry_count;
    uint32_t          selected_count;
    files_view_t      view;
    files_sort_t      sort;
    bool              sort_asc;
    bool              show_hidden;
    files_bookmark_t  bookmarks[FILES_MAX_BOOKMARKS];
    uint32_t          bookmark_count;
    /* Navigation history */
    char  history[64][VFS_PATH_MAX];
    int   history_pos;
    int   history_len;
} files_panel_t;

/* ── Public API ──────────────────────────────────────────────────────────── */
void files_init(void);
int  files_navigate(files_panel_t *panel, const char *path);
int  files_navigate_up(files_panel_t *panel);
int  files_navigate_back(files_panel_t *panel);
int  files_navigate_forward(files_panel_t *panel);
int  files_refresh(files_panel_t *panel);
void files_set_view(files_panel_t *panel, files_view_t view);
void files_set_sort(files_panel_t *panel, files_sort_t sort, bool ascending);

int  files_copy(const char **srcs, uint32_t count, const char *dest_dir);
int  files_move(const char **srcs, uint32_t count, const char *dest_dir);
int  files_delete(const char **paths, uint32_t count);
int  files_rename(const char *src, const char *new_name);
int  files_mkdir(const char *parent, const char *name);
int  files_search(files_panel_t *panel, const char *query);

void files_add_bookmark(files_panel_t *panel, const char *label, const char *path);
void files_remove_bookmark(files_panel_t *panel, uint32_t index);

void files_main(void);   /* App entry point */

/* ── Default bookmarks ────────────────────────────────────────────────────── */
static const files_bookmark_t files_default_bookmarks[] = {
    { "Home",      "/home"      },
    { "Desktop",   "/home/desktop" },
    { "Documents", "/home/documents" },
    { "Downloads", "/home/downloads" },
    { "Projects",  "/home/projects"  },
    { "Music",     "/home/music"     },
    { "Pictures",  "/home/pictures"  },
    { "Videos",    "/home/videos"    },
    { "System",    "/"              },
    { "tmp",       "/tmp"           },
};
#define FILES_DEFAULT_BOOKMARK_COUNT \
    (sizeof(files_default_bookmarks) / sizeof(files_default_bookmarks[0]))

#endif /* FILES_H */
