# cm2008-runtime — CM2008 runtime-research front-end (headless)

Read-only runtime-research tooling for Championship Manager 2008. It inspects the owner's
original DVD ISO + retail PE, detects the protection, assesses the host's execution options, and
routes lawful runtime captures into `cm2008-lab`. It **never** runs the game, bypasses SecuROM /
anti-debug, or modifies the ISO. No game data is committed.

## Build & run
    cd runtime-research && make
    ./cm2008-runtime source inventory          # list the ISO; flag crack dirs DO-NOT-RUN
    ./cm2008-runtime source inspect            # parse the retail PE + SecuROM detection
    ./cm2008-runtime environment check         # host arch + exec/instrumentation tooling
    ./cm2008-runtime target list
    ./cm2008-runtime target inspect picker
    ./cm2008-runtime runtime capability-report # what is observable here vs UNAVAILABLE
    ./cm2008-runtime experiment picker         # capture plan (defers live capture to the user+lab)

Override paths: `source inspect --exe PATH`, `source inventory --iso PATH`.

## Status
Phase 1 done: ISO inventory, retail PE identification (x86-32 PE32 GUI), SecuROM 7.x detection,
host compatibility matrix — see COMPATIBILITY_MATRIX.md. Live instrumentation of the protected
picker is blocked by SecuROM on Apple Silicon within the rules; the sanctioned evidence path is
the user's own lawful capture (research/RUNTIME_CAPTURE_HOWTO.md) → `cm2008-lab trace ingest`.
