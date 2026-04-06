/*
 * Files — HeroOS File Manager
 * App entry + panel logic skeleton
 */

#include "files.h"
#include <kernel/printk.h>
#include <kernel/proc/sched.h>
#include <kernel/mm/kheap.h>
#include <lib/string.h>

/* ── Navigate to a directory ────────────────────────────────────────────── */

int files_navigate(files_panel_t *panel, const char *path) {
    if (!panel || !path) return -1;

    vfs_node_t *dir = vfs_open(path, O_RDONLY | O_DIRECTORY);
    if (!dir) { pr_warn("Files: cannot open '%s'\n", path); return -1; }

    /* Save history */
    if (panel->history_pos < 63) {
        panel->history_pos++;
        strncpy(panel->history[panel->history_pos], path, VFS_PATH_MAX - 1);
        panel->history_len = panel->history_pos + 1;
    }

    strncpy(panel->cwd, path, VFS_PATH_MAX - 1);
    files_refresh(panel);
    vfs_close(dir);
    return 0;
}

int files_navigate_up(files_panel_t *panel) {
    char parent[VFS_PATH_MAX];
    vfs_path_dirname(panel->cwd, parent, VFS_PATH_MAX);
    return files_navigate(panel, parent);
}

int files_navigate_back(files_panel_t *panel) {
    if (panel->history_pos <= 0) return -1;
    panel->history_pos--;
    strncpy(panel->cwd, panel->history[panel->history_pos], VFS_PATH_MAX - 1);
    return files_refresh(panel);
}

int files_navigate_forward(files_panel_t *panel) {
    if (panel->history_pos + 1 >= panel->history_len) return -1;
    panel->history_pos++;
    strncpy(panel->cwd, panel->history[panel->history_pos], VFS_PATH_MAX - 1);
    return files_refresh(panel);
}

/* ── Refresh directory listing ───────────────────────────────────────────── */

int files_refresh(files_panel_t *panel) {
    vfs_node_t *dir = vfs_open(panel->cwd, O_RDONLY | O_DIRECTORY);
    if (!dir) return -1;

    panel->entry_count = 0;

    vfs_dirent_t dirent;
    uint32_t idx = 0;
    while (panel->entry_count < FILES_MAX_ENTRIES) {
        if (!vfs_readdir(dir, idx, &dirent)) break;
        files_entry_t *e = &panel->entries[panel->entry_count];
        strncpy(e->name, dirent.name, FILES_NAME_MAX - 1);
        /* Build full path */
        if (strcmp(panel->cwd, "/") == 0)
            snprintk(e->path, VFS_PATH_MAX, "/%s", dirent.name);
        else
            snprintk(e->path, VFS_PATH_MAX, "%s/%s", panel->cwd, dirent.name);
        e->type    = dirent.type;
        e->hidden  = (dirent.name[0] == '.');
        e->selected = false;
        if (!panel->show_hidden && e->hidden) { idx++; continue; }
        panel->entry_count++;
        idx++;
    }
    vfs_close(dir);
    return 0;
}

/* ── View / sort ─────────────────────────────────────────────────────────── */

void files_set_view(files_panel_t *panel, files_view_t view) {
    panel->view = view;
}

void files_set_sort(files_panel_t *panel, files_sort_t sort, bool ascending) {
    panel->sort     = sort;
    panel->sort_asc = ascending;
    /* TODO: sort panel->entries array */
}

/* ── File operations (stub — VFS-backed) ─────────────────────────────────── */

int files_copy(const char **srcs, uint32_t count, const char *dest_dir) {
    (void)srcs; (void)count; (void)dest_dir;
    pr_info("Files: copy %u items → %s\n", count, dest_dir);
    return 0;   /* TODO: implement recursive copy via VFS */
}

int files_move(const char **srcs, uint32_t count, const char *dest_dir) {
    (void)count; (void)dest_dir;
    for (uint32_t i = 0; i < count; i++) {
        char dest[VFS_PATH_MAX];
        char base[VFS_PATH_MAX];
        vfs_path_basename(srcs[i], base, VFS_PATH_MAX);
        snprintk(dest, VFS_PATH_MAX, "%s/%s", dest_dir, base);
        vfs_rename(srcs[i], dest);
    }
    return 0;
}

int files_delete(const char **paths, uint32_t count) {
    for (uint32_t i = 0; i < count; i++)
        vfs_unlink(paths[i]);
    return 0;
}

int files_rename(const char *src, const char *new_name) {
    char dir[VFS_PATH_MAX], dest[VFS_PATH_MAX];
    vfs_path_dirname(src, dir, VFS_PATH_MAX);
    snprintk(dest, VFS_PATH_MAX, "%s/%s", dir, new_name);
    return vfs_rename(src, dest);
}

int files_mkdir(const char *parent, const char *name) {
    char path[VFS_PATH_MAX];
    snprintk(path, VFS_PATH_MAX, "%s/%s", parent, name);
    return vfs_mkdir(path, 0755);
}

int files_search(files_panel_t *panel, const char *query) {
    /* Simple substring search over loaded entries */
    uint32_t matches = 0;
    for (uint32_t i = 0; i < panel->entry_count; i++) {
        panel->entries[i].selected = (strstr(panel->entries[i].name, query) != NULL);
        if (panel->entries[i].selected) matches++;
    }
    panel->selected_count = matches;
    return (int)matches;
}

/* ── Bookmarks ────────────────────────────────────────────────────────────── */

void files_add_bookmark(files_panel_t *panel, const char *label, const char *path) {
    if (panel->bookmark_count >= FILES_MAX_BOOKMARKS) return;
    files_bookmark_t *b = &panel->bookmarks[panel->bookmark_count++];
    strncpy(b->label, label, 63);
    strncpy(b->path, path, VFS_PATH_MAX - 1);
}

void files_remove_bookmark(files_panel_t *panel, uint32_t index) {
    if (index >= panel->bookmark_count) return;
    for (uint32_t i = index; i < panel->bookmark_count - 1; i++)
        panel->bookmarks[i] = panel->bookmarks[i + 1];
    panel->bookmark_count--;
}

/* ── Init ────────────────────────────────────────────────────────────────── */

void files_init(void) {
    pr_info("Files: file manager initialised\n");
}

/* ── App entry point ─────────────────────────────────────────────────────── */

void files_main(void) {
    pr_info("Files: starting\n");

    files_panel_t *panel = (files_panel_t *)kcalloc(1, sizeof(files_panel_t));
    if (!panel) return;

    /* Install default bookmarks */
    for (uint32_t i = 0; i < FILES_DEFAULT_BOOKMARK_COUNT; i++) {
        files_add_bookmark(panel,
                           files_default_bookmarks[i].label,
                           files_default_bookmarks[i].path);
    }

    panel->view     = FILES_VIEW_GRID;
    panel->sort     = FILES_SORT_NAME;
    panel->sort_asc = true;

    files_navigate(panel, "/home");

    /* App event loop — will be driven by Aura WM events in Phase 5 */
    for (;;) sched_sleep(100);
}
