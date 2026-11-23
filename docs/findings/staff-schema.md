# Finding: Staff / manager (person) record schema

**Status:** reverse-engineered (schema) · **Confidence:** Observed
**Evidence:** clean decompile `docs/ghidra_dumps/raw/tactics.c` — FUN_00a84df0.

`FUN_00a84df0` registers the non-player **person** (manager/coach/staff) record,
field-by-field. 40 fields recovered verbatim (set, order, and the game's own
spellings including the typos `Sportmanship`, `Adaptibility`):

- **Identity:** PersonID
- **Tactical preferences:** TrainingStyleID, FormationID, MarkingStyleID,
  PlayingMentality, PassingStyleID, PressingStyleID, PlayOffsideTrap,
  PlayRoamingPlayers
- **Personality (hidden):** Determination, Professionalism, Pressure, Loyalty,
  Sportmanship, Adaptibility, Ambition, Superstition
- **Staff attributes:** CoachingGoalkeepers, CoachingOutfieldPlayers, Management,
  Physiotherapy, Tactics, CoachingYouthPlayers, TransferAcumen, LevelOfTrust,
  Generosity, ScoutingOnAbility, ScoutingOnPotential, Motivating, Strictness,
  Realism
- **State / references:** ContractID, PlaysNationalTeam, Shortlist,
  ScoutingOrderID, ManagerHistoryID, SeekingEmployment,
  MarkedForAIBackroomStaffManagementTransfer, IsDeleted, CurrentAbility

Reconstructed as `staff/person_schema.h/.c` (enum + name table + lookup), tested.
Value ranges/types beyond "uint32 field" are INFERRED.
