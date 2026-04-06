/*
 * Prism — HeroOS System Settings
 *
 * The one place to configure everything — beautiful, organised, powerful.
 *
 * Panels:
 *   ┌──────────────────────────────────────────────────────────────────┐
 *   │  Prism · Settings                                                │
 *   ├─────────────┬────────────────────────────────────────────────────┤
 *   │ Appearance  │  Theme       ● Dark  ○ Light  ○ Auto               │
 *   │ Display     │  Accent      [cyan ▼]  ████████                    │
 *   │ Sound       │  Font        [HeroMono 13 ▼]                       │
 *   │ Network     │  Transparency  ━━━━━●━━━━  60%                     │
 *   │ Keyboard    │  Animations    ● On  ○ Off                         │
 *   │ Mouse       │                                                    │
 *   │ Battery     │                                                    │
 *   │ Security    │                                                    │
 *   │ Developer   │                                                    │
 *   │ About       │                                                    │
 *   └─────────────┴────────────────────────────────────────────────────┘
 *
 * Internal name: "prism"
 * Display name:  "Prism"
 */

#ifndef PRISM_H
#define PRISM_H

#include <kernel/types.h>

/* ── Appearance ───────────────────────────────────────────────────────────── */
typedef enum {
    PRISM_THEME_DARK  = 0,
    PRISM_THEME_LIGHT = 1,
    PRISM_THEME_AUTO  = 2,   /* Follow system time */
} prism_theme_t;

typedef struct {
    prism_theme_t theme;
    uint32_t      accent_color;       /* ARGB */
    char          font_name[64];
    uint32_t      font_size;
    float         ui_scale;           /* 1.0 = 100%, 1.5 = 150% HiDPI */
    float         transparency;       /* 0.0–1.0 (window bg blur amount) */
    bool          animations_enabled;
    bool          rounded_corners;
    uint32_t      corner_radius;      /* Pixels */
    bool          show_desktop_icons;
} prism_appearance_t;

/* ── Display ──────────────────────────────────────────────────────────────── */
typedef struct {
    uint32_t  width, height;
    uint32_t  refresh_rate;
    uint8_t   bpp;
    int       brightness;     /* 0–100 */
    bool      night_light;
    uint32_t  night_light_temp;   /* Kelvin: 3000–6500 */
    bool      hdr_enabled;
} prism_display_t;

/* ── Sound ────────────────────────────────────────────────────────────────── */
typedef struct {
    int   master_volume;   /* 0–100 */
    int   system_sounds;   /* 0–100 */
    bool  muted;
} prism_sound_t;

/* ── Network ──────────────────────────────────────────────────────────────── */
typedef struct {
    bool         wifi_enabled;
    bool         ethernet_enabled;
    char         hostname[64];
    bool         firewall_enabled;
    bool         vpn_enabled;
    char         vpn_server[256];
    bool         proxy_enabled;
    char         proxy_host[256];
    uint16_t     proxy_port;
} prism_network_t;

/* ── Security ─────────────────────────────────────────────────────────────── */
typedef struct {
    bool  screen_lock;
    int   lock_timeout_secs;
    bool  disk_encryption;
    bool  secure_boot;
    bool  capabilities_enabled;
    bool  sandbox_apps;
} prism_security_t;

/* ── Developer ────────────────────────────────────────────────────────────── */
typedef struct {
    bool  show_fps_overlay;
    bool  show_memory_overlay;
    bool  enable_ssh_server;
    uint16_t ssh_port;
    bool  enable_heroserve_autostart;
    uint16_t heroserve_default_port;
    bool  verbose_logging;
} prism_developer_t;

/* ── Power / Battery ──────────────────────────────────────────────────────── */
typedef struct {
    bool  power_save_on_battery;
    int   screen_timeout_secs;
    int   sleep_timeout_secs;
    bool  show_battery_percentage;
} prism_power_t;

/* ── Full settings model ──────────────────────────────────────────────────── */
typedef struct {
    prism_appearance_t appearance;
    prism_display_t    display;
    prism_sound_t      sound;
    prism_network_t    network;
    prism_security_t   security;
    prism_developer_t  developer;
    prism_power_t      power;
} prism_settings_t;

/* ── Public API ───────────────────────────────────────────────────────────── */
void prism_init(void);
void prism_main(void);
int  prism_load(const char *path, prism_settings_t *out);
int  prism_save(const char *path, const prism_settings_t *settings);
void prism_apply_appearance(const prism_appearance_t *a);
const prism_settings_t *prism_defaults(void);

/* Config file location */
#define PRISM_CONFIG_PATH  "/etc/prism.conf"

#endif /* PRISM_H */
