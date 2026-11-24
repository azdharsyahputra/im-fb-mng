#include "lab_cli.h"
#include <string.h>
const char *opt(int argc, char **argv, const char *name, const char *def) {
    for (int i = 0; i < argc - 1; i++) if (strcmp(argv[i], name) == 0) return argv[i + 1];
    return def;
}
int has(int argc, char **argv, const char *name) {
    for (int i = 0; i < argc; i++) if (strcmp(argv[i], name) == 0) return 1;
    return 0;
}
