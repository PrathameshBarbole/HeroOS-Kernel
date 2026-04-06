/*
 * HeroShell — Built-in commands + REPL skeleton
 */

#include "heroshell.h"
#include <kernel/printk.h>
#include <kernel/proc/sched.h>
#include <kernel/fs/vfs.h>
#include <lib/string.h>
#include <drivers/keyboard/keyboard.h>

/* ─── Terminal helpers ───────────────────────────────────────────────────── */

/* Erase `n` characters from the display (backspace + space + backspace). */
static void term_erase(int n) {
    for (int i = 0; i < n; i++) printk("\b \b");
}

/* ─── History ────────────────────────────────────────────────────────────── */

static char hist_buf[SHELL_HIST_SIZE][SHELL_LINE_MAX];
static int  hist_count = 0;   /* number of entries stored   */
static int  hist_head  = 0;   /* index of the oldest entry  */

static void hist_add(const char *line) {
    int idx = (hist_head + hist_count) % SHELL_HIST_SIZE;
    strncpy(hist_buf[idx], line, SHELL_LINE_MAX - 1);
    hist_buf[idx][SHELL_LINE_MAX - 1] = '\0';
    if (hist_count < SHELL_HIST_SIZE)
        hist_count++;
    else
        hist_head = (hist_head + 1) % SHELL_HIST_SIZE;
}

/* offset 0 = most recent entry, 1 = one before that, … */
static const char *hist_get(int offset) {
    if (offset < 0 || offset >= hist_count) return NULL;
    int idx = (hist_head + hist_count - 1 - offset) % SHELL_HIST_SIZE;
    return hist_buf[idx];
}

/* ─── Shell-exit flag ────────────────────────────────────────────────────── */

static bool shell_should_exit = false;

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
    (void)argc; (void)argv;
    shell_should_exit = true;
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
    shell_should_exit = false;
    printk("HeroShell v0.1.0 — type 'help' for commands\n\n");
}

void shell_run(void) {
    char line[SHELL_LINE_MAX];
    char saved_line[SHELL_LINE_MAX];  /* preserves typed text during history nav */
    int  pos      = 0;
    int  hist_nav = -1;               /* -1 = not navigating; 0 = most-recent   */

    for (;;) {
        /* Print prompt and reset line state */
        printk(SHELL_PROMPT);
        pos      = 0;
        hist_nav = -1;
        memset(line, 0, sizeof(line));
        memset(saved_line, 0, sizeof(saved_line));

        /* Read one line from the keyboard */
        for (;;) {
            key_event_t ev;
            /* Busy-wait with HLT until a key arrives */
            while (!keyboard_poll(&ev))
                __asm__ volatile("hlt");

            if (!ev.pressed) continue;

            /* ── Extended keys (arrow keys etc.) ───────────────────────── */
            if (ev.extended) {
                if (ev.scancode == KEY_ARROW_UP) {
                    int next = hist_nav + 1;
                    const char *entry = hist_get(next);
                    if (!entry) continue;
                    /* Save current typed text on first history step */
                    if (hist_nav < 0)
                        strncpy(saved_line, line, SHELL_LINE_MAX - 1);
                    hist_nav = next;
                    /* Erase current display and replace with history entry */
                    term_erase(pos);
                    strncpy(line, entry, SHELL_LINE_MAX - 1);
                    line[SHELL_LINE_MAX - 1] = '\0';
                    pos = (int)strlen(line);
                    printk("%s", line);
                } else if (ev.scancode == KEY_ARROW_DOWN) {
                    if (hist_nav < 0) continue;
                    term_erase(pos);
                    if (hist_nav > 0) {
                        hist_nav--;
                        const char *entry = hist_get(hist_nav);
                        strncpy(line, entry, SHELL_LINE_MAX - 1);
                        line[SHELL_LINE_MAX - 1] = '\0';
                    } else {
                        hist_nav = -1;
                        strncpy(line, saved_line, SHELL_LINE_MAX - 1);
                    }
                    pos = (int)strlen(line);
                    printk("%s", line);
                }
                continue;
            }

            /* ── Regular ASCII keys ────────────────────────────────────── */
            char c = ev.ascii;
            if (!c) continue;

            if (c == '\r' || c == '\n') {
                printk("\n");
                line[pos] = '\0';
                break;
            }

            if (c == '\b') {
                if (pos > 0) {
                    pos--;
                    line[pos] = '\0';
                    term_erase(1);
                }
                continue;
            }

            if (c == 0x1B) {
                /* ESC: discard line */
                term_erase(pos);
                pos = 0;
                line[0] = '\0';
                hist_nav = -1;
                continue;
            }

            if (pos < SHELL_LINE_MAX - 1) {
                line[pos++] = c;
                line[pos]   = '\0';
                printk("%c", c);
            }
        }

        /* Execute the completed line */
        if (pos > 0) {
            hist_add(line);
            shell_exec_line(line);
        }

        if (shell_should_exit) {
            printk("Goodbye!\n");
            break;
        }
    }
}

