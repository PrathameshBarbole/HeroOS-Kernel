/*
 * HeroPkg — HeroOS Package Manager
 *
 * Design goals:
 *   - Ultra-fast install/update (parallel downloads)
 *   - Curated developer-centric repository
 *   - Reproducible builds (locked versions)
 *   - Zero-friction dev tool setup
 *
 * Usage:
 *   heropkg install <package>
 *   heropkg remove  <package>
 *   heropkg update
 *   heropkg list
 *   heropkg search  <query>
 *   heropkg info    <package>
 */

#ifndef HEROPKG_H
#define HEROPKG_H

#include <kernel/types.h>

#define PKG_NAME_MAX    64
#define PKG_VERSION_MAX 32
#define PKG_DESC_MAX    256
#define PKG_MAX_DEPS    16
#define PKG_REPO_URL    "https://pkg.heroos.dev/v1"

typedef enum {
    PKG_NOT_INSTALLED = 0,
    PKG_INSTALLED,
    PKG_OUTDATED,
} pkg_status_t;

typedef struct {
    char name[PKG_NAME_MAX];
    char version[PKG_VERSION_MAX];
    char description[PKG_DESC_MAX];
    char deps[PKG_MAX_DEPS][PKG_NAME_MAX];
    int  dep_count;
    pkg_status_t status;
    size_t installed_size;   /* bytes */
} package_t;

/* Package manager operations */
int heropkg_init(void);
int heropkg_install(const char *name);
int heropkg_remove(const char *name);
int heropkg_update_all(void);
int heropkg_list_installed(void);
int heropkg_search(const char *query);
int heropkg_info(const char *name);

/* Pre-defined developer packages */
typedef struct {
    const char *name;
    const char *version;
    const char *description;
} pkg_entry_t;

static const pkg_entry_t hero_repo[] = {
    { "bun",      "1.1.x",  "Fast JavaScript runtime and bundler" },
    { "node",     "22.x",   "Node.js JavaScript runtime" },
    { "python",   "3.12.x", "Python programming language" },
    { "go",       "1.22.x", "Go programming language" },
    { "rust",     "1.78.x", "Rust systems programming language" },
    { "git",      "2.45.x", "Distributed version control system" },
    { "make",     "4.4.x",  "GNU make build tool" },
    { "cmake",    "3.29.x", "Cross-platform build system generator" },
    { "ninja",    "1.11.x", "Fast build system" },
    { "clang",    "18.x",   "LLVM C/C++ compiler frontend" },
    { "micro",    "2.0.x",  "Modern terminal text editor" },
    { "htop",     "3.3.x",  "Interactive process viewer" },
    { "curl",     "8.7.x",  "URL transfer tool" },
    { "wget",     "1.21.x", "Non-interactive network downloader" },
    { "heropkg",  "0.1.x",  "HeroOS package manager (self)" },
    { "heroserve","0.1.x",  "HeroOS zero-config HTTP server" },
    { NULL, NULL, NULL }
};

#endif /* HEROPKG_H */
