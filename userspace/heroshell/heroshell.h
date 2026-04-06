/*
 * HeroShell — HeroOS Interactive Shell
 *
 * Features (planned):
 *   - Command parsing with pipes, redirection, and background jobs
 *   - Syntax highlighting and tab completion
 *   - Command history (up/down arrows)
 *   - Built-in commands: cd, ls, cat, echo, exit, hero, heropkg, heroserve
 *   - PATH-based executable lookup
 *   - Environment variables
 *   - Scripting support (if/for/while)
 */

#ifndef HEROSHELL_H
#define HEROSHELL_H

#include <kernel/types.h>

#define SHELL_LINE_MAX   1024
#define SHELL_HIST_SIZE  256
#define SHELL_MAX_ARGS   64
#define SHELL_PROMPT     "hero$ "

typedef struct {
    char    line[SHELL_LINE_MAX];
    char   *argv[SHELL_MAX_ARGS];
    int     argc;
} cmd_t;

/* Built-in command handler signature */
typedef int (*builtin_fn_t)(int argc, char **argv);

typedef struct {
    const char  *name;
    const char  *help;
    builtin_fn_t fn;
} builtin_t;

/* Shell public API */
void shell_init(void);
void shell_run(void);
int  shell_exec_line(const char *line);

/* Built-in commands */
int builtin_help(int argc, char **argv);
int builtin_exit(int argc, char **argv);
int builtin_echo(int argc, char **argv);
int builtin_cd(int argc, char **argv);
int builtin_ls(int argc, char **argv);
int builtin_cat(int argc, char **argv);
int builtin_pwd(int argc, char **argv);
int builtin_hero(int argc, char **argv);   /* hero meta-command */
int builtin_clear(int argc, char **argv);

#endif /* HEROSHELL_H */
