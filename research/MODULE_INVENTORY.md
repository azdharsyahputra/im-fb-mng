# CM2008 game-core module inventory (VERIFIED from RTTI)

162 `GC*Module` classes recovered from MSVC RTTI type descriptors (`.?AVGC…Module@@` in .data), the
complete subsystem map of CM2008's "GC" (game-core) layer. These are the engine's manager/module objects;
each is an MSVC C++ class (most polymorphic, walkable to its vtable via MATCH_VTABLE_DISPATCH_MATRIX.md §B).
Grouping is by subsystem; cross-refs note what is already reconstructed/ported.

## Match / gameplay
GCMatchModule, GCMatchEventModule, GCMatchInputModule, GCMatchReportModule, GCMatchReportMedModule,
GCMatchReportMaxModule, GCMatchReportReplayableModule, GCFieldPositionModule, GCPreMatchMessageModule,
GCOfficialModule, GCOfficialsModule.
→ see MATCH_ENGINE_ARCHITECTURE.md (quick-sim resolver GCMatchResolverWrapperImpl @0x9f + the ~255-state
player FSM + 35-state official FSM). GCFieldPositionModule has a 7-deep inheritance chain.

## Tactics / squad selection
GCTacticsModule, GCTacticsDeltaModule, GCSquadTacticsModule, GCUserSquadTacticsModule,
GCPositionTacticsModule, GCPositionTeamTacticsModule, GCTeamTacticsFormationModule, GCSquadModule,
GCSquadsClubModule, GCTeamModule, GCUserTeamDutiesModule.
→ tactics/formations partially ported (super-manager-engine/tactics; name-keyed .tac, ZoneCounts).

## Person / player data & properties
GCPersonModule, GCPersonNameModule, GCPersonLanguagesModule, GCPersonalityModule,
GCPersonalityDataModule, GCPlayerModule, GCPlayerPersonalModule, GCPlayerOutputDataModule,
GCPlayerReputationModule, GCPropertyModule, GCPropertyPlayerModule, GCPropertyGoalKeeperModule,
GCStaffModule, GCTendencyStereotypeModule, GCRandomNameModule, GCAgeingModule.
→ player-ability transforms ported (cm2008-formulas/playerability); condensed rating VERIFIED.

## Contracts / finance
GCContractModule, GCContractDetailsModule, GCContractBonusModule, GCContractClauseModule,
GCContractReleaseModule, GCContractMonitorModule, GCFinanceModule, GCClubIncomeModule,
GCClubExpenditureModule, GCRoundIncomeModule, GCRoundPrizeModule, GCCurrencyModule.
→ wage/finance cluster ported (cm2008-formulas/wagefinance); Finance model VERIFIED (club rating.go).

## Transfers / market
GCTransferModule, GCTransferWindowModule, GCPlayerTransferModule, GCPlayerTransferPaymentModule,
GCStaffTransferModule, GCHistoryTransferModule, GCExchangePlayerModule, GCOfferModule,
GCPlayerEnquiryModule, GCPlayerEnquiryProcessModule, GCFirstOptionModule, GCLoanModule,
GCLoanMonitorModule, GCCompetitionLoanModule, GCTrialModule, GCTrialMonitorModule, GCAuctionModule,
GCAuctionMonitorModule, GCWorkPermitRequestModule, GCWorkPermitProcessModule.
→ transfer-value cluster ported (cm2008-formulas/transfervalue).

## Scouting / shortlists / manager AI
GCScoutingModule, GCScoutingOrderModule, GCPlayerShortlistEntryModule, GCManagerShortlistEntryModule,
GCNationalShortlistEntryModule, GCManagerTransferListModule, GCManagerAssessSquadModule,
GCManagerAssessShortlistModule, GCManagerFiringModule, GCManagerMediaModule,
GCClubAssessProspectiveManagersModule, GCClubCollateProspectiveManagersModule, GCOpinionMonitorModule.
→ manager-AI cluster ported (cm2008-formulas/managerai).

## Happiness / morale / personality dynamics
GCHappinessModule, GCHappyMemoryModule, GCHappyRecordModule.
→ happiness cluster ported (cm2008-formulas/happiness); per-effect magnitudes still runtime-UNKNOWN.

## Training / development / physio / injury
GCTrainingModule, GCTrainingScheduleModule, GCPlayerDevelopmentFeedbackModule, GCPhysioModule,
GCInjuryModule, GCInjuryTypeModule, GCRetirementModule.
→ injury model ported (super-manager-engine/injury; 42 types VERIFIED).

## Board / club
GCBoardModule, GCBoardProcessModule, GCClubModule, GCClubStaffModule, GCClubValidateModule,
GCClubCoefficientModule, GCRivalClubModule, GCFanModule, GCFacilitiesMonitorModule.

## Competition / season / fixtures / rounds
GCCompetitionModule, GCCompetitionNameModule, GCCompNameSquadModule, GCSeasonModule, GCFixtureModule,
GCFixtureSchedulerModule, GCPreDeterminedFixtureModule, GCGroupModule, GCRoundModule, GCRoundOrderModule,
GCRoundPositionModule, GCRoundRouterModule, GCRoundRoutingModule, GCRoundVenueModule,
GCRoundPlayTimeModule, GCRoundBroadcastTimeModule, GCRoundFixtureAffectModule, GCBroadcastTimeModule,
GCCountryCoefficientModule, GCPreSeasonFriendlyOpponentModule, GCPreSeasonFriendlyRulesModule,
GCFixtureSimulator (not Module-suffixed; the quick-sim entry).
→ league/season partially ported (super-manager-engine/league; circle-method VERIFIED).

## International
GCInternationalManagementModule, GCInternationalManagementExpectationsAndOpinionsModule,
GCInternationalSquadModule, GCInternationalWeekModule.

## History / awards
GCHistoryClubModule, GCHistoryCompetitionModule, GCHistoryCompetitionClubModule,
GCHistoryManagerModule, GCHistoryPlayerModule, GCAwardModule, GCAwardDefModule,
GCAwardAnnualIndexModule, GCAwardMonthlyIndexModule.

## Mail / media
GCMailCreatorModule, GCMailRepositoryModule.
→ mail templates recovered (ISO_GAME_DATA.md; StringGroupID + typed Parameters).

## Geography / world / calendar
GCAreaModule, GCAreaLanguageModule, GCCityModule, GCCountyModule, GCRegionModule, GCClimateModule,
GCColourModule, GCLanguageModule, GCCalendarModule, GCDateTimeModule, GCVenueModule, GCVenueClubModule,
GCVenueClubSquadModule.

## Infrastructure / engine
GCGameModule (root base of the module hierarchy), GCMainModule, GCProcessModule, GCProgramStepModule,
GCFormulaModule (formula.xml evaluator), GCMetricModule, GCStringModule, GCSoundModule, GCLoggingModule,
GCTestModule, GCUserModule, GCDataEntity/GCNamedEntity (base data classes), GCFunction* (formula AST).
Plus ATL/COM infra (CAtlModule, CComModule).

---
This inventory maps the whole GC game-core. Combined with the 517-class match/FE RTTI set and the two FSMs,
the engine's object architecture is now broadly charted; any module's vtable (and thus its method set) is
recoverable by the RTTI walk without decompiling bodies.
