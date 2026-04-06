/*
 * HeroShell — Built-in commands + REPL skeleton
 */

#include "heroshell.h"
#include <kernel/printk.h>
#include <kernel/proc/sched.h>
#include <kernel/fs/vfs.h>
#include <lib/string.h>

/* ─── Built-ins ──────────────────────────────────────────────────────────── */

int builtin_help(int argc, char **argv) {
    (void)argc; (void)argv;
    printk("HeroShell built-in commands:\n");
    printk("  help                  Show this help\n");
    printk("  exit [code]           Exit the shell\n");
    printk("  echo [args...]        Print arguments\n");
    printk("  cd <dir>              Change directory\n");
    printk("  ls [dir]              List directory contents\n");
    printk("  cat <file>            Print file contents\n");
    printk("  pwd                   Print working directory\n");
    printk("  clear                 Clear terminal\n");
    printk("  hero <subcommand>     HeroOS meta-tool\n");
    printk("    hero info           System information\n");
    printk("    hero serve [port]   Start HeroServe HTTP server\n");
    printk("    hero pkg <action>   Package manager\n");
    return 0;
}

int builtin_exit(int argc, char **argv) {
    int code = (argc > 1) ? (int)(*argv[1] - '0') : 0;
    (void)code;
    printk("Goodbye!\n");
    /* TODO: sys_exit(code) */
    return 0;
}

int builtin_echo(int argc, char **argv) {
    for (int i = 1; i < argc; i++) {
        printk("%s", argv[i]);
        if (i + 1 < argc) printk(" ");
    }
    printk("\n");
    return 0;
}

int builtin_cd(int argc, char **argv) {
    (void)argc; (void)argv;
    /* TODO: chdir syscall */
    printk("cd: not yet implemented\n");
    return 0;
}

int builtin_ls(int argc, char **argv) {
    const char *path = (argc > 1) ? argv[1] : "/";
    vfs_node_t *dir = vfs_open(path, O_RDONLY | O_DIRECTORY);
    if (!dir) { printk("ls: %s: No such file or directory\n", path); return 1; }

    vfs_dirent_t dirent;
    uint32_t idx = 0;
    while (vfs_readdir(dir, idx, &dirent)) {
        printk("%s  ", dirent.name);
        idx++;
    }
    if (idx > 0) printk("\n");
    vfs_close(dir);
    return 0;
}

int builtin_cat(int argc, char **argv) {
    if (argc < 2) { printk("cat: missing file argument\n"); return 1; }
    vfs_node_t *f = vfs_open(argv[1], O_RDONLY);
    if (!f) { printk("cat: %s: No such file or directory\n", argv[1]); return 1; }

    char buf[512];
    ssize_t n;
    off_t off = 0;
    while ((n = vfs_read(f, off, sizeof(buf) - 1, buf)) > 0) {
        buf[n] = '\0';
        printk("%s", buf);
        off += n;
    }
    vfs_close(f);
    return 0;
}

int builtin_pwd(int argc, char **argv) {
    (void)argc; (void)argv;
    printk("/\n");   /* TODO: track cwd per process */
    return 0;
}

int builtin_clear(int argc, char **argv) {
    (void)argc; (void)argv;
    /* ANSI clear screen sequence */
    printk("\033[2J\033[H");
    return 0;
}

int builtin_hero(int argc, char **argv) {
    if (argc < 2) {
        printk("Usage: hero <subcommand>\n");
        printk("  hero info   — system info\n");
        printk("  hero serve  — start HTTP server\n");
        printk("  hero pkg    — package manager\n");
        return 1;
    }
    if (strcmp(argv[1], "info") == 0) {
        printk("HeroOS v0.1.0 — One OS, Many Platforms\n");
        printk("Architecture: x86_64\n");
        /* TODO: print uptime, memory, CPU info */
    } else if (strcmp(argv[1], "serve") == 0) {
        uint16_t port = 8080;
        if (argc > 2) port = (uint16_t)(argv[2][0] - '0' + 8000);
        printk("HeroServe starting on port %u...\n", port);
        /* TODO: spawn heroserved */
    } else if (strcmp(argv[1], "pkg") == 0) {
        printk("HeroPkg: use 'heropkg install <package>'\n");
    } else {
        printk("hero: unknown subcommand '%s'\n", argv[1]);
        return 1;
    }
    return 0;
}

/* ─── Built-in table ─────────────────────────────────────────────────────── */

static const builtin_t builtins[] = {
    { "help",  "Show this help",              builtin_help  },
    { "exit",  "Exit shell",                  builtin_exit  },
    { "echo",  "Print text",                  builtin_echo  },
    { "cd",    "Change directory",            builtin_cd    },
    { "ls",    "List directory",              builtin_ls    },
    { "cat",   "Print file contents",         builtin_cat   },
    { "pwd",   "Print working directory",     builtin_pwd   },
    { "clear", "Clear the terminal",          builtin_clear },
    { "hero",  "HeroOS meta-command",         builtin_hero  },
};
#define BUILTIN_COUNT  (sizeof(builtins) / sizeof(builtins[0]))

/* ─── Line tokenizer ─────────────────────────────────────────────────────── */

static int tokenize(char *line, char **argv, int max_args) {
    int argc = 0;
    char *p = line;
    while (*p && argc < max_args - 1) {
        while (*p == ' ' || *p == '\t') p++;
        if (!*p) break;
        argv[argc++] = p;
        while (*p && *p != ' ' && *p != '\t') p++;
        if (*p) { *p = '\0'; p++; }
    }
    argv[argc] = NULL;
    return argc;
}

/* ─── Execute a command line ─────────────────────────────────────────────── */

int shell_exec_line(const char *line) {
    if (!line || !*line) return 0;

    char buf[SHELL_LINE_MAX];
    strncpy(buf, line, SHELL_LINE_MAX - 1);
    buf[SHELL_LINE_MAX - 1] = '\0';

    char *argv[SHELL_MAX_ARGS];
    int argc = tokenize(buf, argv, SHELL_MAX_ARGS);
    if (argc == 0) return 0;

    /* Check built-ins */
    for (size_t i = 0; i < BUILTIN_COUNT; i++) {
        if (strcmp(argv[0], builtins[i].name) == 0)
            return builtins[i].fn(argc, argv);
    }

    printk("%s: command not found\n", argv[0]);
    return 127;
}

/* ─── Shell init / REPL ──────────────────────────────────────────────────── */

void shell_init(void) {
    printk("HeroShell v0.1.0 — type 'help' for commands\n\n");
}

void shell_run(void) {
    /* TODO: read lines from keyboard via PTY, echo to terminal */
    for (;;) {
        printk(SHELL_PROMPT);
        sched_sleep(1000);   /* Placeholder until PTY/keyboard input is wired */
    }
}
