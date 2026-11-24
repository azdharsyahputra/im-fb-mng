# data/

cm2008-lab reads the owner's **local** extracted resolver files at runtime; **no game
data is committed** (the repo .gitignore blocks `*.dat`/`*.xml`).

Expected files (point the lab at them with `--dir` or `$RESOLVER_DIR`):

    MatchResolverDistributions.1.dat … MatchResolverDistributions.6.dat

Default directory: `$RESOLVER_DIR`, else `/Users/csadeveloper/cm/extracted_game/DatabaseFiles`.
