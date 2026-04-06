/*
 * Aura — HeroOS Desktop Environment
 *
 * A clean, premium desktop with tiling + floating window management.
 *
 * Visual language:
 *   - Dark background (#12121F) with cyan/purple accents
 *   - Frosted glass panel effects
 *   - Smooth 60 fps window animations
 *   - Per-app rounded corners (12px default)
 *   - Subtle drop shadows
 *
 * Layout:
 *   ┌──────────────────────────────────────────────────────────────────────┐
 *   │  [⬡ HeroOS]  Files  Terminal  Quill  Lens  ...    [Orbit] [  12:34] │  ← Top bar
 *   ├──────────────────────────────────────────────────────────────────────┤
 *   │                                                                      │
 *   │                        Desktop / Wallpaper                           │
 *   │                                                                      │
 *   │   ┌──────────────────────┐   ┌─────────────┐                        │
 *   │   │  Terminal            │   │  Files      │   ← Floating windows   │
 *   │   │  hero$ _             │   │  /home      │                        │
 *   │   └──────────────────────┘   └─────────────┘                        │
 *   │                                                                      │
 *   ├──────────────────────────────────────────────────────────────────────┤
 *   │  [▣ Desk 1] [▣ Desk 2]  [Terminal] [Files] [Quill]  ⊞  CPU ▇ MEM ▆ │  ← Taskbar
 *   └──────────────────────────────────────────────────────────────────────┘
 *
 * Keyboard shortcuts:
 *   Super + Enter         Open Terminal
 *   Super + E             Open Files
 *   Super + B             Open Lens (browser)
 *   Super + Space         Open Orbit (app launcher)
 *   Super + Q             Close window
 *   Super + F             Toggle fullscreen
 *   Super + M             Toggle maximise
 *   Super + ←/→           Snap window to left/right half
 *   Super + 1–9           Switch workspace
 *   Super + Tab           Cycle windows
 *   Super + Shift + Tab   Cycle windows backwards
 *
 * Internal name: "aura"
 * Display name:  "Aura"
 */

#ifndef AURA_H
#define AURA_H

#include <kernel/types.h>
#include "canvas.h"

/* ── Workspaces ───────────────────────────────────────────────────────────── */
#define AURA_MAX_WORKSPACES   9
#define AURA_MAX_WINDOWS     64

typedef enum {
    AURA_LAYOUT_FLOATING = 0,   /* Free-form, user moves windows */
    AURA_LAYOUT_TILING,         /* Master-stack tiling */
    AURA_LAYOUT_FULLSCREEN,     /* One window occupies full screen */
    AURA_LAYOUT_GRID,           /* Equal-size grid */
} aura_layout_t;

typedef struct aura_window {
    canvas_surface_t *surface;
    char              app_id[64];
    char              title[256];
    int32_t           x, y;
    uint32_t          width, height;
    bool              floating;
    bool              sticky;       /* Appears on all workspaces */
    int32_t           workspace;
    struct aura_window *prev;
    struct aura_window *next;
} aura_window_t;

typedef struct {
    int          index;
    char         name[32];
    aura_layout_t layout;
    aura_window_t *windows;
    uint32_t      window_count;
    aura_window_t *focused;
} aura_workspace_t;

/* ── Panel / Taskbar items ────────────────────────────────────────────────── */
typedef struct {
    bool     visible;
    uint32_t height;
    uint32_t bg_color;
    float    opacity;
    bool     blur;
} aura_panel_t;

/* ── Desktop state ────────────────────────────────────────────────────────── */
typedef struct {
    aura_workspace_t  workspaces[AURA_MAX_WORKSPACES];
    int               active_workspace;
    aura_panel_t      topbar;
    aura_panel_t      taskbar;
    uint32_t          screen_w, screen_h;
    char              wallpaper_path[4096];
    /* Compositor reference */
    canvas_compositor_t *compositor;
} aura_desktop_t;

/* ── Public API ───────────────────────────────────────────────────────────── */
void aura_init(uint32_t screen_w, uint32_t screen_h);
void aura_main(void);

void aura_add_window(aura_desktop_t *desk, aura_window_t *win);
void aura_remove_window(aura_desktop_t *desk, aura_window_t *win);
void aura_focus_window(aura_desktop_t *desk, aura_window_t *win);
void aura_close_window(aura_window_t *win);

void aura_switch_workspace(aura_desktop_t *desk, int index);
void aura_move_window_to_workspace(aura_window_t *win, int workspace);

void aura_tile_windows(aura_workspace_t *ws, uint32_t screen_w, uint32_t screen_h);
void aura_snap_left(aura_window_t *win, uint32_t screen_w, uint32_t screen_h);
void aura_snap_right(aura_window_t *win, uint32_t screen_w, uint32_t screen_h);
void aura_snap_maximise(aura_window_t *win, uint32_t screen_w, uint32_t screen_h);

void aura_launch_app(const char *app_id);
void aura_render_frame(aura_desktop_t *desk);

#endif /* AURA_H */
