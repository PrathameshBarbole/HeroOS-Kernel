/*
 * Wave — HeroOS Media Player
 *
 * Clean, minimal media player for audio and video.
 *
 * Features:
 *   - Plays audio: MP3, FLAC, AAC, OGG, WAV, OPUS
 *   - Plays video: MP4/H.264, WebM/VP9, MKV
 *   - Playlists with drag-and-drop reordering
 *   - Waveform visualiser
 *   - Gapless playback
 *   - Lyrics display (LRC file support)
 *   - Mini player mode (compact floating widget)
 *   - Media key support (play/pause/next/prev)
 *   - Sleep timer
 *   - Integrates with Echo notifications (now-playing card)
 *
 * Internal name: "wave"
 * Display name:  "Wave"
 */

#ifndef WAVE_H
#define WAVE_H

#include <kernel/types.h>

/* ── Media formats ────────────────────────────────────────────────────────── */
typedef enum {
    WAVE_FORMAT_UNKNOWN = 0,
    WAVE_FORMAT_WAV,
    WAVE_FORMAT_MP3,
    WAVE_FORMAT_FLAC,
    WAVE_FORMAT_OGG,
    WAVE_FORMAT_AAC,
    WAVE_FORMAT_OPUS,
    WAVE_FORMAT_MP4,
    WAVE_FORMAT_WEBM,
    WAVE_FORMAT_MKV,
} wave_format_t;

/* ── Track metadata ───────────────────────────────────────────────────────── */
typedef struct {
    char title[256];
    char artist[128];
    char album[128];
    char genre[64];
    uint32_t year;
    uint32_t track_number;
    uint32_t duration_ms;
    char path[4096];
    wave_format_t format;
    bool has_artwork;
} wave_track_t;

/* ── Playlist ─────────────────────────────────────────────────────────────── */
#define WAVE_MAX_TRACKS  10000

typedef struct {
    char          name[128];
    wave_track_t *tracks[WAVE_MAX_TRACKS];
    uint32_t      track_count;
    uint32_t      current;
    bool          shuffle;
    bool          repeat_all;
    bool          repeat_one;
} wave_playlist_t;

/* ── Player state ─────────────────────────────────────────────────────────── */
typedef enum {
    WAVE_STOPPED = 0,
    WAVE_PLAYING,
    WAVE_PAUSED,
} wave_state_t;

typedef struct {
    wave_state_t    state;
    wave_playlist_t playlist;
    uint32_t        position_ms;    /* Current playback position */
    int             volume;         /* 0–100 */
    bool            muted;
    float           playback_speed; /* 0.5–2.0x */
} wave_player_t;

/* ── Public API ───────────────────────────────────────────────────────────── */
void wave_init(void);
void wave_main(void);
int  wave_open(wave_player_t *player, const char *path);
void wave_play(wave_player_t *player);
void wave_pause(wave_player_t *player);
void wave_stop(wave_player_t *player);
void wave_next(wave_player_t *player);
void wave_prev(wave_player_t *player);
void wave_seek(wave_player_t *player, uint32_t position_ms);
void wave_set_volume(wave_player_t *player, int volume);

wave_format_t wave_detect_format(const char *path);

#endif /* WAVE_H */
