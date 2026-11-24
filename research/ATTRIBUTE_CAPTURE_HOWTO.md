# Attribute-column capture how-to (runtime route, no DRM touch)

Goal: pin the 77 `PlayerProperties.dat` columns to their attribute names by VALUE-MATCHING a few
real players' named attribute values (from your own game) against the on-disk columns. The solver
`super-manager-engine/cmd/attrmap` does the matching; this doc is how you produce its input.

No cracked exe, no DRM bypass, no patching. You read values your own game already shows/exports.

## What the solver needs

A JSON array of players, each with its **PlayerProperties record id** and its **named attribute
values** (as shown in-game, usually 1..20):

```json
[
  {"player_id": 12345, "attrs": {"Finishing": 14, "Pace": 17, "Tackling": 6, "Passing": 15}},
  {"player_id": 23456, "attrs": {"Finishing": 8,  "Pace": 12, "Tackling": 15, "Passing": 9}}
]
```

The more players and the more attributes per player, the fewer AMBIGUOUS results. ~8–15 varied
players usually pin the common attributes uniquely. Stored values are 0..100 and shown ~1..20, so
the solver auto-detects the scale (×5 etc.) — just enter the values as the game displays them.

The one hard requirement is the `player_id` that keys into `PlayerProperties.dat`. Pick whichever
method below gives you that id alongside the names.

## Method A — debugger at the loader (definitive, VERIFIED-grade)

On a Windows host (native or VM) running your own lawful, unmodified install, attach a READ-ONLY
debugger (x64dbg / Frida — no patching). Break at the PlayerProperties loader (the function that
copies the 77 columns into the in-memory player object) and log, per player: the record id and the
77 raw column values; separately log a named getter's value. This yields id↔column↔name directly;
feed the ids + a few named getters as the capture. (This also incidentally produces the picker
capture from `RUNTIME_CAPTURE_HOWTO.md` — same session.) SecuROM note: the game decrypts itself in
memory at runtime, so the loader is readable live; you are only observing, not modifying.

## Method B — a CM2008 database editor/export (easiest)

If you use any CM2008 editor that lists players with their internal record id AND their attributes,
export a handful to the JSON above. The id must be the PlayerProperties id (same id space as
`PlayerProperties.dat`, which is `u32 id + 77×u16`).

## Method C — in-game screens + an id bridge

Read a few players' attributes off the squad/player screens (names you can see), then resolve each
player's PlayerProperties id via the name↔id link (Player/PlayerNames tables). If you can give the
id this way, it works; if the UI shows no id and you have no bridge, use Method A or B.

## Run it

```bash
CAPTURE_JSON=capture.json PLAYER_PROPS_DAT=/path/to/PlayerProperties.dat go run ./cmd/attrmap
```

Output: per attribute, the matched column + scale + how many samples agreed, and SOLVED /
AMBIGUOUS (with the other candidate columns) / UNRESOLVED, plus the columns still unmapped. Add
more players to resolve AMBIGUOUS rows. A column accepted for an attribute means a single
(column, scale) reproduced EVERY sample — evidence-based, not guessed. Promote a SOLVED row to
VERIFIED in `src/database/player_attribute_schema.h` and `PLAYER_ATTRIBUTE_MAPPING.csv`.

No game data is embedded anywhere; the solver reads your local `PlayerProperties.dat` and your own
capture at runtime.
