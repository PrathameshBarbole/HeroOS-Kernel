/*
 * Echo — HeroOS Notification System
 *
 * Delivers system and app notifications with a clean, non-intrusive UI.
 *
 * Design:
 *   - Notifications slide in from the top-right corner
 *   - Auto-dismiss after configurable timeout (default 5 s)
 *   - Notification centre panel (click the bell icon in Aura topbar)
 *   - Priority levels: info, success, warning, critical
 *   - Action buttons (e.g., "Dismiss", "Open App", "Retry")
 *   - Do Not Disturb mode
 *   - Sound playback on critical notifications
 *   - History: last 100 notifications kept in memory
 *
 * Internal name: "echo"
 * Display name:  "Echo" (not user-visible; powers all notifications)
 */

#ifndef ECHO_H
#define ECHO_H

#include <kernel/types.h>

/* ── Priority ─────────────────────────────────────────────────────────────── */
typedef enum {
    ECHO_INFO     = 0,   /* Soft, auto-dismiss */
    ECHO_SUCCESS  = 1,   /* Green checkmark */
    ECHO_WARNING  = 2,   /* Yellow caution */
    ECHO_CRITICAL = 3,   /* Red, stays until dismissed */
} echo_priority_t;

/* ── Action button ────────────────────────────────────────────────────────── */
#define ECHO_MAX_ACTIONS  4

typedef struct {
    char label[64];
    char action[256];   /* Command or IPC message */
} echo_action_t;

/* ── Notification ─────────────────────────────────────────────────────────── */
#define ECHO_MAX_NOTIFS  100

typedef struct {
    uint32_t         id;
    char             app_id[64];
    char             app_name[64];
    char             title[256];
    char             body[1024];
    echo_priority_t  priority;
    echo_action_t    actions[ECHO_MAX_ACTIONS];
    uint32_t         action_count;
    uint64_t         timestamp;
    uint32_t         timeout_ms;    /* 0 = never auto-dismiss */
    bool             dismissed;
    bool             read;
    char             icon_path[256];
} echo_notification_t;

/* ── Notification centre ──────────────────────────────────────────────────── */
typedef struct {
    echo_notification_t notifs[ECHO_MAX_NOTIFS];
    uint32_t            count;
    uint32_t            unread;
    bool                do_not_disturb;
    bool                panel_open;
    uint32_t            next_id;
} echo_centre_t;

/* ── Public API ───────────────────────────────────────────────────────────── */
void echo_init(void);
void echo_main(void);

uint32_t echo_notify(const char *app_id, const char *title, const char *body,
                     echo_priority_t priority, uint32_t timeout_ms);
void echo_dismiss(uint32_t id);
void echo_dismiss_all(void);
void echo_set_dnd(bool enabled);
void echo_mark_read(uint32_t id);
void echo_add_action(uint32_t notif_id, const char *label, const char *action);

/* Convenience wrappers */
#define echo_info(app, title, body)     echo_notify(app, title, body, ECHO_INFO,     5000)
#define echo_success(app, title, body)  echo_notify(app, title, body, ECHO_SUCCESS,  4000)
#define echo_warning(app, title, body)  echo_notify(app, title, body, ECHO_WARNING,  8000)
#define echo_critical(app, title, body) echo_notify(app, title, body, ECHO_CRITICAL, 0)

#endif /* ECHO_H */
