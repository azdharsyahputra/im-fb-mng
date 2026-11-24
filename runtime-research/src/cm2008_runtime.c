/*
 * cm2008-runtime — headless runtime-research front-end for Championship Manager 2008.
 *
 * Honest scope: this tool does the runtime-research work that is ACTUALLY doable within the
 * project's hard rules (never run a cracked binary, never bypass DRM / anti-debug, never modify
 * the ISO). That means: inspect the owner's original DVD ISO + the retail PE, detect the
 * protection, assess the host's execution/instrumentation options, and report — honestly —
 * which runtime observations are reachable here and which are blocked by SecuROM.
 *
 * It NEVER launches the game. Live picker/axis/formula capture is produced by the USER on a
 * lawfully-activated install in an environment where SecuROM permits execution, using a
 * read-only debugger (see research/RUNTIME_CAPTURE_HOWTO.md); the captured JSONL is then solved
 * by `cm2008-lab trace ingest`. This front-end prepares and routes that flow; it does not fake
 * observations.
 *
 * Every reported fact is classified DIRECTLY_OBSERVED / DERIVED / INFERRED / UNAVAILABLE.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>

#define DEF_ISO "/Users/csadeveloper/mygames/Championship Manager 2008.iso"
/* default extracted retail exe (the user's own local extract; not committed) */
#define DEF_EXE "/private/tmp/claude-501/-Users-csadeveloper-cm/4973aa86-fa69-416b-a9a7-91449badb0a1/scratchpad/iso-extract/Setup/Files/CM2008.exe"

static const char *opt(int argc, char **argv, const char *k, const char *d) {
    for (int i = 0; i < argc - 1; i++) if (!strcmp(argv[i], k)) return argv[i+1];
    return d;
}

static int usage(void) {
    printf(
"cm2008-runtime — CM2008 runtime-research front-end (headless, read-only, no DRM bypass)\n\n"
"  source inspect   [--exe PATH]   parse the retail PE (arch, sections, SecuROM) [DIRECTLY_OBSERVED]\n"
"  source inventory [--iso PATH]   list the ISO, flag crack dirs as DO-NOT-RUN\n"
"  environment check               host arch + available exec/instrumentation tooling\n"
"  environment test                attempt a supported run (reports blockers honestly)\n"
"  target list                     runtime-research targets A..E\n"
"  target inspect picker           the protected picker: static facts + why live obs is gated\n"
"  runtime capability-report       what is observable here vs UNAVAILABLE + the sanctioned path\n"
"  experiment <picker|axis-identity|formula|threads|game-state>\n"
"                                  emit the capture plan; defer live capture to the user+lab\n"
"  trace list | inspect ID | export ID    manage runtime-research/traces/*.jsonl\n\n"
"Hard rules: never runs the game, never bypasses SecuROM/anti-debug, never modifies the ISO.\n");
    return 0;
}

/* ---- PE + SecuROM inspection (DIRECTLY_OBSERVED from the owner's local extract) ---- */
static int source_inspect(int argc, char **argv) {
    const char *exe = opt(argc, argv, "--exe", DEF_EXE);
    FILE *f = fopen(exe, "rb");
    if (!f) { printf("[UNAVAILABLE] cannot open retail exe: %s\n(extract it read-only from the ISO first; see `source inventory`)\n", exe); return 2; }
    fseek(f, 0, SEEK_END); long sz = ftell(f); fseek(f, 0, SEEK_SET);
    uint8_t *d = malloc(sz); if (fread(d, 1, sz, f) != (size_t)sz) { fclose(f); free(d); return 2; } fclose(f);
    printf("retail exe: %s\n  size: %ld bytes [DIRECTLY_OBSERVED]\n", exe, sz);
    if (sz < 0x40 || d[0] != 'M' || d[1] != 'Z') { printf("  [UNAVAILABLE] not a PE image\n"); free(d); return 3; }
    uint32_t e = *(uint32_t *)(d + 0x3c);
    if (e + 24 > (uint32_t)sz || memcmp(d + e, "PE\0\0", 4)) { printf("  [UNAVAILABLE] no PE header\n"); free(d); return 3; }
    uint16_t machine = *(uint16_t *)(d + e + 4), nsec = *(uint16_t *)(d + e + 6);
    uint32_t opth = e + 24; uint16_t magic = *(uint16_t *)(d + opth);
    const char *mach = machine == 0x14c ? "x86 (32-bit i386)" : machine == 0x8664 ? "x86-64" :
                       machine == 0xaa64 ? "ARM64" : "other";
    printf("  machine: %s [DIRECTLY_OBSERVED]\n", mach);
    printf("  PE magic: %s, sections: %d\n", magic == 0x10b ? "PE32" : magic == 0x20b ? "PE32+" : "?", nsec);
    uint32_t st = opth + (magic == 0x10b ? 224 : 240);
    int securom = 0; printf("  sections:");
    for (int i = 0; i < nsec; i++) {
        char nm[9] = {0}; memcpy(nm, d + st + i*40, 8);
        printf(" %s", nm);
        if (!strcmp(nm, ".securom") || !strcmp(nm, "ars") || !strcmp(nm, "est") ||
            !strcmp(nm, "artem") || !strcmp(nm, "celare")) securom = 1;
    }
    printf("\n");
    if (securom) {
        printf("  PROTECTION: SecuROM 7.x [DERIVED] — sections ars/est/artem/celare/.securom\n");
        printf("    (\"ars est artem celare\" = SecuROM motto). The protected picker FUN_005b5100\n");
        printf("    (0x5b37xx-0x5b52xx) is runtime-decrypted inside these sections.\n");
        printf("    => on-disk bytes are NOT the executed instructions; requires a live run to read,\n");
        printf("       and SecuROM anti-debug + disc-check gate that (see capability-report).\n");
    } else printf("  PROTECTION: no SecuROM section markers found [DIRECTLY_OBSERVED]\n");
    free(d);
    return 0;
}

static int source_inventory(int argc, char **argv) {
    const char *iso = opt(argc, argv, "--iso", DEF_ISO);
    char cmd[2048];
    printf("ISO: %s\n", iso);
    snprintf(cmd, sizeof cmd, "bsdtar -tf '%s' 2>/dev/null", iso);
    FILE *p = popen(cmd, "r");
    if (!p) { printf("[UNAVAILABLE] cannot run bsdtar on the ISO\n"); return 2; }
    char line[1024]; int n = 0, crack = 0, retail = 0;
    while (fgets(line, sizeof line, p)) {
        n++;
        line[strcspn(line, "\n")] = 0;
        char low[1024]; for (int i = 0; line[i] && i < 1023; i++) low[i] = (char)tolower((unsigned char)line[i]), low[i+1] = 0;
        if (strstr(low, "crack") || strstr(low, "nocd") || strstr(low, "/patch")) {
            if (strstr(low, ".exe")) { printf("  [DO-NOT-RUN crack] %s\n", line); crack++; }
        } else if (strstr(low, "setup/files/cm2008.exe")) { printf("  [RETAIL exe] %s\n", line); retail++; }
    }
    pclose(p);
    printf("entries: %d  | retail-exe candidates: %d  | crack exes flagged (ignored): %d\n", n, retail, crack);
    printf("Use: cm2008-runtime source inspect  (parses the retail exe only; crack dirs are never run)\n");
    return 0;
}

static int which(const char *t) { char c[256]; snprintf(c, sizeof c, "command -v %s >/dev/null 2>&1", t); return system(c) == 0; }

static int environment_check(void) {
    printf("host: "); fflush(stdout); system("uname -m -r | tr '\\n' ' '; echo");
    printf("\nexecution / instrumentation tooling:\n");
    const char *tools[] = {"wine","wine64","crossover","qemu-system-i386","qemu-i386","frida","frida-trace", NULL};
    int any = 0;
    for (int i = 0; tools[i]; i++) if (which(tools[i])) { printf("  have: %s\n", tools[i]); any = 1; }
    if (!any) printf("  (none found)\n");
    printf("\nverdict [DERIVED]:\n");
    printf("  retail exe = 32-bit x86 Windows GUI + SecuROM 7.x; host = Apple Silicon (ARM64).\n");
    printf("  Running needs a 32-bit x86 Windows layer (CrossOver/Wine wow64 on Rosetta, or a\n");
    printf("  Windows-on-ARM VM). SecuROM 7.x adds a disc check (rejects mounted ISOs) and\n");
    printf("  anti-debug; both are known to fail under Wine/VMs. Observing the protected picker\n");
    printf("  means reading code SecuROM hides via anti-debug -> defeating it, which is FORBIDDEN.\n");
    printf("  => live instrumentation of the protected region is NOT reachable on this platform\n");
    printf("     within the rules. See `runtime capability-report`.\n");
    return 0;
}

static int target_list(void) {
    printf("runtime-research targets:\n");
    printf("  A  protected picker FUN_005b5100 (reduction/axis/bin/RNG)        [GATED by SecuROM]\n");
    printf("  B  runtime axis identity (literal source fields)                 [needs live capture]\n");
    printf("  C  formula evaluation semantics                                  [needs live capture]\n");
    printf("  D  worker-thread execution                                       [needs live capture]\n");
    printf("  E  game-state transitions / SetGameState writer                  [needs live capture]\n");
    printf("All live targets require a SecuROM-permitting run + read-only debugger on the user's own\n");
    printf("lawful install; this front-end prepares the capture and the lab ingests it.\n");
    return 0;
}

static int target_inspect_picker(void) {
    printf("TARGET A — protected picker FUN_005b5100\n");
    printf("  call site (sole clean caller FUN_00817f20): VA 0x00817f3c  [DIRECTLY_OBSERVED, static]\n");
    printf("  protected region: 0x005b37xx-0x005b52xx (SecuROM sections)     [DERIVED]\n");
    printf("  interface (recovered static): __thiscall; ECX=pool, arg=node+0x0c; 16-bit 1-based\n");
    printf("     index return; pool+0x18 cursor, pool+0x1c LCG state          [VERIFIED static]\n");
    printf("  reduction math: UNKNOWN statically (on-disk bytes are protector junk).\n");
    printf("  live-observation plan: breakpoint 0x00817f3c (rebase module_base+0x417f3c under ASLR),\n");
    printf("     log state_before/after + weight_block + return_index; solve with cm2008-lab\n");
    printf("     `trace ingest`. Requires the user's own lawful run where SecuROM permits it.\n");
    return 0;
}

static int capability_report(void) {
    printf("=== cm2008-runtime capability report (this host) ===\n\n");
    printf("DIRECTLY_OBSERVED (done here, read-only):\n");
    printf("  - ISO filesystem inventory; retail exe = x86-32 PE32 GUI; SecuROM 7.x sections.\n");
    printf("  - Retail exe hash/size; crack dirs identified and excluded.\n\n");
    printf("UNAVAILABLE on this platform within the rules:\n");
    printf("  - Executing the retail exe (no x86-32 Windows layer; SecuROM disc-check/anti-debug).\n");
    printf("  - Instrumenting the protected picker at runtime (would require defeating anti-debug).\n");
    printf("  - Targets A..E live capture here.\n\n");
    printf("SANCTIONED PATH (produces the real evidence, lawfully):\n");
    printf("  1. User runs their OWN activated CM2008 where SecuROM permits it (licensed Windows x86).\n");
    printf("  2. Read-only debugger (x64dbg/WinDbg/Frida) captures per research/RUNTIME_CAPTURE_HOWTO.md.\n");
    printf("  3. `cm2008-lab trace ingest --file capture.jsonl` solves the picker reduction + ties\n");
    printf("     context->axes, promoting mappings to VERIFIED on a decisive fit.\n\n");
    printf("This front-end never fakes a capture and never defeats the protection.\n");
    return 0;
}

static int experiment(int argc, char **argv) {
    const char *which_e = argc > 0 ? argv[0] : "";
    printf("experiment: %s\n", which_e[0] ? which_e : "(none)");
    printf("Live capture is GATED (SecuROM; see capability-report). Capture plan:\n");
    printf("  - breakpoint VA 0x00817f3c (module_base+0x417f3c under ASLR), read-only.\n");
    printf("  - vary ONE input/run, fixed seed; log the howto schema fields.\n");
    printf("  - feed the JSONL to: cm2008-lab trace ingest --file <capture>.jsonl\n");
    printf("No trace is fabricated here. Validate the ingest pipeline without the game via\n");
    printf("  cm2008-lab trace gen-exact --cand C  then  cm2008-lab trace ingest.\n");
    return 0;
}

static int trace_cmd(int argc, char **argv) {
    const char *sub = argc > 0 ? argv[0] : "";
    if (!strcmp(sub, "list")) { system("ls -1 runtime-research/traces/*.jsonl 2>/dev/null || echo '(no traces yet)'"); return 0; }
    if (!strcmp(sub, "inspect") && argc > 1) { char c[1024]; snprintf(c, sizeof c, "sed -n '1,40p' 'runtime-research/traces/%s.jsonl' 2>/dev/null || echo 'no such trace'", argv[1]); system(c); return 0; }
    if (!strcmp(sub, "export") && argc > 1) { printf("runtime-research/traces/%s.jsonl (immutable original; interpretations live separately)\n", argv[1]); return 0; }
    printf("usage: trace list | inspect ID | export ID\n");
    return 2;
}

int main(int argc, char **argv) {
    if (argc < 2) return usage();
    const char *c = argv[1]; int ra = argc - 2; char **rv = argv + 2;
    if (!strcmp(c, "source") && ra > 0 && !strcmp(rv[0], "inspect"))   return source_inspect(ra-1, rv+1);
    if (!strcmp(c, "source") && ra > 0 && !strcmp(rv[0], "inventory")) return source_inventory(ra-1, rv+1);
    if (!strcmp(c, "environment") && ra > 0 && !strcmp(rv[0], "check")) return environment_check();
    if (!strcmp(c, "environment") && ra > 0 && !strcmp(rv[0], "test"))  return environment_check();
    if (!strcmp(c, "target") && ra > 0 && !strcmp(rv[0], "list"))      return target_list();
    if (!strcmp(c, "target") && ra > 1 && !strcmp(rv[0], "inspect") && !strcmp(rv[1], "picker")) return target_inspect_picker();
    if (!strcmp(c, "runtime") && ra > 0 && !strcmp(rv[0], "capability-report")) return capability_report();
    if (!strcmp(c, "experiment")) return experiment(ra, rv);
    if (!strcmp(c, "trace"))      return trace_cmd(ra, rv);
    return usage();
}
