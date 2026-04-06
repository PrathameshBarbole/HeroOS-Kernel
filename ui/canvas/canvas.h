/*
 * Canvas — HeroOS Display Compositor
 *
 * A lightweight Wayland-inspired compositor that manages windows,
 * GPU acceleration, and the visual layer of HeroOS.
 *
 * Design:
 *   - Direct framebuffer compositing (no X11 dependency)
 *   - Scene graph: window surfaces → effects → final frame
 *   - GPU acceleration via framebuffer blitting (software fallback)
 *   - Smooth 60 fps animations with vsync
 *   - Effects: blur, shadow, rounded corners, transparency
 *   - Input routing: keyboard focus, mouse events, touch
 *   - Multi-monitor support
 *
 * Internal name: "canvas"
 * Display name:  "Canvas" (runs as a system service, not user-visible)
 *
 * Architecture:
 *
 *   ┌──────────────────────────────────────────────────┐
 *   │              HeroOS App Processes                 │
 *   │  Files  Quill  Terminal  Lens  Wave  Prism  ...  │
 *   └────────────────────┬─────────────────────────────┘
 *              IPC (shared framebuffer surfaces)
 *   ┌────────────────────▼─────────────────────────────┐
 *   │              Canvas Compositor                    │
 *   │   Scene Graph → Blend → Effects → Framebuffer    │
 *   └────────────────────┬─────────────────────────────┘
 *              DRM/KMS or direct framebuffer write
 *   ┌────────────────────▼─────────────────────────────┐
 *   │              Display Hardware                     │
 *   └──────────────────────────────────────────────────┘
 */

#ifndef CANVAS_H
#define CANVAS_H

#include <kernel/types.h>

/* ── Surface (one per window/layer) ────────────────────────────────────────── */
#define CANVAS_MAX_SURFACES  128

typedef enum {
    CANVAS_SURFACE_NORMAL = 0,
    CANVAS_SURFACE_POPUP,
    CANVAS_SURFACE_OVERLAY,    /* Always on top (notifications, etc.) */
    CANVAS_SURFACE_WALLPAPER,  /* Always at bottom */
    CANVAS_SURFACE_CURSOR,
} canvas_surface_type_t;

typedef struct {
    uint32_t  id;
    char      app_id[64];      /* Which app owns this surface */
    char      title[256];
    int32_t   x, y;            /* Position on screen */
    uint32_t  width, height;
    uint32_t *pixels;          /* ARGB pixel buffer (shared memory) */
    float     opacity;         /* 0.0–1.0 */
    float     blur_radius;     /* Background blur strength */
    uint32_t  shadow_radius;
    uint32_t  shadow_color;    /* ARGB */
    uint32_t  corner_radius;
    bool      visible;
    bool      focused;
    bool      minimised;
    bool      maximised;
    bool      fullscreen;
    canvas_surface_type_t type;
} canvas_surface_t;

/* ── Compositor state ─────────────────────────────────────────────────────── */
typedef struct {
    canvas_surface_t  surfaces[CANVAS_MAX_SURFACES];
    uint32_t          surface_count;
    canvas_surface_t *focused;
    uint32_t          screen_width;
    uint32_t          screen_height;
    uint32_t          refresh_rate;   /* Target fps */
    uint64_t          frame_count;
    uint64_t          last_frame_us;
    /* Wallpaper */
    uint32_t         *wallpaper;
    uint32_t          wallpaper_w, wallpaper_h;
} canvas_compositor_t;

/* ── Public API ───────────────────────────────────────────────────────────── */
void canvas_init(uint32_t screen_w, uint32_t screen_h);
void canvas_main(void);

canvas_surface_t *canvas_create_surface(const char *app_id, uint32_t w, uint32_t h,
                                         canvas_surface_type_t type);
void canvas_destroy_surface(canvas_surface_t *surface);
void canvas_commit(canvas_surface_t *surface);    /* Surface ready to composite */

void canvas_move_surface(canvas_surface_t *s, int32_t x, int32_t y);
void canvas_resize_surface(canvas_surface_t *s, uint32_t w, uint32_t h);
void canvas_focus(canvas_surface_t *s);
void canvas_raise(canvas_surface_t *s);
void canvas_lower(canvas_surface_t *s);
void canvas_minimise(canvas_surface_t *s);
void canvas_maximise(canvas_surface_t *s);
void canvas_fullscreen(canvas_surface_t *s, bool enable);

void canvas_composite_frame(canvas_compositor_t *c);
void canvas_set_wallpaper(const char *path);

/* ── Animation helpers ────────────────────────────────────────────────────── */
typedef float (*ease_fn_t)(float t);   /* t in [0,1] → value in [0,1] */
float ease_linear(float t);
float ease_in_out_cubic(float t);
float ease_out_elastic(float t);
float ease_out_bounce(float t);

#endif /* CANVAS_H */
