/* lab_cli — tiny shared flag helpers (defined in main.c). */
#ifndef CM2008_LAB_CLI_H
#define CM2008_LAB_CLI_H
const char *opt(int argc, char **argv, const char *name, const char *def);
int         has(int argc, char **argv, const char *name);
#endif
