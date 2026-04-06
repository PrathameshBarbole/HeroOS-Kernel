/*
 * HeroServe — HeroOS Zero-Config HTTP Server
 *
 * Design goals:
 *   - Instant startup (no config needed, just `hero serve`)
 *   - Serve static files from current directory
 *   - Optional reverse proxy for local dev servers
 *   - Built-in HTTPS via auto-generated self-signed cert
 *   - Live reload support (WebSocket-based)
 *   - Directory listing with nice UI
 *
 * Usage:
 *   hero serve             — serve CWD on port 8080
 *   hero serve <port>      — serve CWD on given port
 *   hero serve <dir>       — serve given directory on port 8080
 *   hero serve --https     — serve with HTTPS
 *
 * Implementation:
 *   Runs as a kernel thread in Phase 1 (no network stack yet).
 *   Will move to full user-space daemon once TCP/IP is implemented.
 */

#ifndef HEROSERVE_H
#define HEROSERVE_H

#include <kernel/types.h>

#define HEROSERVE_DEFAULT_PORT    8080
#define HEROSERVE_MAX_CONNECTIONS 256
#define HEROSERVE_BUF_SIZE        65536

/* HTTP method */
typedef enum {
    HTTP_GET     = 0,
    HTTP_POST    = 1,
    HTTP_PUT     = 2,
    HTTP_DELETE  = 3,
    HTTP_HEAD    = 4,
    HTTP_OPTIONS = 5,
} http_method_t;

/* HTTP status codes */
#define HTTP_200_OK             200
#define HTTP_201_CREATED        201
#define HTTP_204_NO_CONTENT     204
#define HTTP_301_MOVED          301
#define HTTP_304_NOT_MODIFIED   304
#define HTTP_400_BAD_REQUEST    400
#define HTTP_403_FORBIDDEN      403
#define HTTP_404_NOT_FOUND      404
#define HTTP_500_SERVER_ERROR   500

/* Request */
typedef struct {
    http_method_t method;
    char          path[1024];
    char          query[512];
    char         *body;
    size_t        body_len;
    bool          keep_alive;
} http_request_t;

/* Response */
typedef struct {
    uint16_t  status;
    char      content_type[128];
    char     *body;
    size_t    body_len;
    bool      send_file;
    char      file_path[1024];
} http_response_t;

/* Route handler */
typedef int (*route_handler_t)(http_request_t *req, http_response_t *res);

typedef struct {
    http_method_t method;
    char          path[256];
    route_handler_t handler;
} route_t;

/* HeroServe config */
typedef struct {
    uint16_t    port;
    const char *root_dir;
    bool        enable_https;
    bool        enable_cors;
    bool        directory_listing;
    bool        live_reload;
} heroserve_config_t;

/* Public API */
int  heroserve_init(heroserve_config_t *cfg);
int  heroserve_start(void);
void heroserve_stop(void);
void heroserve_add_route(http_method_t method, const char *path, route_handler_t handler);

/* Default config */
extern heroserve_config_t heroserve_defaults;

#endif /* HEROSERVE_H */
