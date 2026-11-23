# CM2008 Data-Model Catalog (recovered schemas)

Field-registration functions (>=8 fields) across all subsystem dumps, with recovered field lists. Observed verbatim from decompilation (docs/ghidra_dumps/raw/<subsystem>.c). Fields uint32 unless noted.

| # | subsystem | function | fields |
|--:|---|---|--:|
| 1 | player_development | `FUN_00a5d0e0` | 111 |
| 2 | competitions | `FUN_00a5d0e0` | 111 |
| 3 | competitions | `FUN_00a647b0` | 52 |
| 4 | match_reporting | `FUN_00a647b0` | 52 |
| 5 | tactics | `FUN_00a76dc0` | 50 |
| 6 | player_development | `FUN_00a61c90` | 49 |
| 7 | competitions | `FUN_00a61c90` | 49 |
| 8 | players | `FUN_00a84df0` | 41 |
| 9 | player_development | `FUN_00a84df0` | 41 |
| 10 | tactics | `FUN_00a84df0` | 41 |
| 11 | players | `FUN_00a7c540` | 39 |
| 12 | player_development | `FUN_00a7c540` | 39 |
| 13 | match_engine | `FUN_0077ddc0` | 32 |
| 14 | players | `FUN_00a72db0` | 31 |
| 15 | players | `FUN_00a896c0` | 31 |
| 16 | player_development | `FUN_00a896c0` | 31 |
| 17 | match_reporting | `FUN_00a72db0` | 31 |
| 18 | competitions | `FUN_00a667f0` | 30 |
| 19 | player_development | `FUN_00a5f970` | 29 |
| 20 | competitions | `FUN_00a6d620` | 29 |
| 21 | match_reporting | `FUN_00a7a880` | 28 |
| 22 | competitions | `FUN_00a62de0` | 27 |
| 23 | competitions | `FUN_007b3af0` | 22 |
| 24 | competitions | `FUN_007b5e50` | 22 |
| 25 | tactics | `FUN_00a75ca0` | 21 |
| 26 | players | `FUN_00a78660` | 16 |
| 27 | player_development | `FUN_00a8a060` | 14 |
| 28 | competitions | `FUN_007b78f0` | 14 |
| 29 | competitions | `FUN_007b6ac0` | 14 |
| 30 | competitions | `FUN_007b5590` | 14 |
| 31 | competitions | `FUN_007b4760` | 14 |
| 32 | tactics | `FUN_00a76870` | 14 |
| 33 | competitions | `FUN_00a6bf70` | 10 |
| 34 | competitions | `FUN_00a6bc70` | 8 |


## player_development `FUN_00a5d0e0` (111 fields)

ClubStringID, AreaID, NationalClub, Administration, BankBalance, ClubCoEfficient, ClubStatusID, CurrencyID, GroundCost, KitAwayColour1, KitAwayColour2, KitHomeColour1, KitHomeColour2, Merchandising, NickNameStringID, OutstandingLoan, RecordLeagueAttendance, RecordLeagueClubID, RLD_Day, RLD_Month, RLD_Year, Reputation, SeasonTicketPrice, ShortNameStringID, SponsorAmount, SE_Day, SE_Month, SE_Year, SS_Day, SS_Month, SS_Year, TicketPrice, Training, TVRevenue, MostAppearancesNameStringID, MostAppearances, MostGoalsNameStringID, MostGoals, YouthAccademy, YouthDevelopment, HomeMatchDayID, YearFounded, SeasonTicketHolders, TransferBudget, WageBudget, MarqueePlayerWeeklyWageBudget, AccumulatedInterest, MonthlyLoanRepayment, PercentTransferRevenue, SquadsClubID, FirstBoardMember, ClubStaff, IncomeThisMonth, IncomeLastMonth, IncomeThisSeason, IncomeLastSeason, ExpenditureThisMonth, ExpenditureLastMonth, ExpenditureThisSeason, ExpenditureLastSeason, BoardSeasonExpectation, BoardSeasonExpectationCommentIndex, BoardOpinion, FANationalBoardOpinion, FANationalBoardExpectation, FANationalBoardExpectationData, AverageYouthPotential, UnderCaretakerManagement, ProspectiveManagers, ProspectiveManagersAssessment_Day, ProspectiveManagersAssessment_Month, ProspectiveManagersAssessment_Year, AppliedManagers, SuccessiveProspectiveManagerMisses, HistoryClub, HistoryClubThisSeason, AverageAttendance, UnderTrainingFacilitiesUpdate, TrainingFacilitiesUpdateDaysRemaining, ProposedTrainingFacilitiesChange, TrainingFacilitiesUpdateDueToStart, FanOpinion, FanSeasonExpectation, FanSeasonExpectationContinental, FanSeasonExpectationCommentIndex, SquadFirstTeam, SquadReserveTeam, SquadYouthTeam, Playable, ExpectedDomesticLeagueFinishPosition, ExpectedFinishBracket, Defunct, ScoutAreaHistoryHead, ScoutAreaHistory, WeakPosition1, WeakPosition2, WeakPosition3, WeakInDepth1, WeakInDepth2, WeakInDepth3, WeakMinAbility1, WeakMinAbility2, WeakMinAbility3, FirstRivalClubID, FinancialStatus, IsDeleted, ExpectedPrizeAndTVMoney, HeadSourceClubOfferID, HeadDestinationClubOfferID, RegisteredCompetitionID1, RegisteredCompetitionID2

## competitions `FUN_00a5d0e0` (111 fields)

ClubStringID, AreaID, NationalClub, Administration, BankBalance, ClubCoEfficient, ClubStatusID, CurrencyID, GroundCost, KitAwayColour1, KitAwayColour2, KitHomeColour1, KitHomeColour2, Merchandising, NickNameStringID, OutstandingLoan, RecordLeagueAttendance, RecordLeagueClubID, RLD_Day, RLD_Month, RLD_Year, Reputation, SeasonTicketPrice, ShortNameStringID, SponsorAmount, SE_Day, SE_Month, SE_Year, SS_Day, SS_Month, SS_Year, TicketPrice, Training, TVRevenue, MostAppearancesNameStringID, MostAppearances, MostGoalsNameStringID, MostGoals, YouthAccademy, YouthDevelopment, HomeMatchDayID, YearFounded, SeasonTicketHolders, TransferBudget, WageBudget, MarqueePlayerWeeklyWageBudget, AccumulatedInterest, MonthlyLoanRepayment, PercentTransferRevenue, SquadsClubID, FirstBoardMember, ClubStaff, IncomeThisMonth, IncomeLastMonth, IncomeThisSeason, IncomeLastSeason, ExpenditureThisMonth, ExpenditureLastMonth, ExpenditureThisSeason, ExpenditureLastSeason, BoardSeasonExpectation, BoardSeasonExpectationCommentIndex, BoardOpinion, FANationalBoardOpinion, FANationalBoardExpectation, FANationalBoardExpectationData, AverageYouthPotential, UnderCaretakerManagement, ProspectiveManagers, ProspectiveManagersAssessment_Day, ProspectiveManagersAssessment_Month, ProspectiveManagersAssessment_Year, AppliedManagers, SuccessiveProspectiveManagerMisses, HistoryClub, HistoryClubThisSeason, AverageAttendance, UnderTrainingFacilitiesUpdate, TrainingFacilitiesUpdateDaysRemaining, ProposedTrainingFacilitiesChange, TrainingFacilitiesUpdateDueToStart, FanOpinion, FanSeasonExpectation, FanSeasonExpectationContinental, FanSeasonExpectationCommentIndex, SquadFirstTeam, SquadReserveTeam, SquadYouthTeam, Playable, ExpectedDomesticLeagueFinishPosition, ExpectedFinishBracket, Defunct, ScoutAreaHistoryHead, ScoutAreaHistory, WeakPosition1, WeakPosition2, WeakPosition3, WeakInDepth1, WeakInDepth2, WeakInDepth3, WeakMinAbility1, WeakMinAbility2, WeakMinAbility3, FirstRivalClubID, FinancialStatus, IsDeleted, ExpectedPrizeAndTVMoney, HeadSourceClubOfferID, HeadDestinationClubOfferID, RegisteredCompetitionID1, RegisteredCompetitionID2

## competitions `FUN_00a647b0` (52 fields)

uStatus, RoundOrder, NoOfGroups, RoundNo, StartDayNoID, StartDayID, StartMonthID, EndDayNoID, EndDayID, EndMonthID, CompetitionID, NoOfSquads, TimesPlayed, RoundPosition, FixturesGenerated, RoundTypeID, ChangeKitID, ClearDisciplineCards, DrawDayTimeID, EndDayTimeID, HomeGateReceiptPercent, NeutralVenue, NoSwapPlayers, PlayerRegistrationDayTimeID, ReplayEndDayTimeID, ReplayVenueChangeID, StartDayTimeID, TVMoney, UniqueFixture, VenueQuality, RegistrationNumBeforeStart, DrawNumAfterPrevStart, StartNumAfterPrevStart, EndNumAfterStart, ReplayNumAfterEnd, Group, DrawNumAfterPrevEnd, StartNumAfterPrevEnd, OfficialLevelID, MatchRulesMask, ReplayMatchRulesMask, IsFinalGroupGamePlay, ContainsTitleHolder, IsRoutingUpdated, bPlacingUpdated, IsDeleted, MaxPlayed, MaxHomeGamesBroadcast, RoundNameStringID, uNumberOfFixtures, uNumberOfPopulatedFixtures, OriginalNoOfSquads

## match_reporting `FUN_00a647b0` (52 fields)

uStatus, RoundOrder, NoOfGroups, RoundNo, StartDayNoID, StartDayID, StartMonthID, EndDayNoID, EndDayID, EndMonthID, CompetitionID, NoOfSquads, TimesPlayed, RoundPosition, FixturesGenerated, RoundTypeID, ChangeKitID, ClearDisciplineCards, DrawDayTimeID, EndDayTimeID, HomeGateReceiptPercent, NeutralVenue, NoSwapPlayers, PlayerRegistrationDayTimeID, ReplayEndDayTimeID, ReplayVenueChangeID, StartDayTimeID, TVMoney, UniqueFixture, VenueQuality, RegistrationNumBeforeStart, DrawNumAfterPrevStart, StartNumAfterPrevStart, EndNumAfterStart, ReplayNumAfterEnd, Group, DrawNumAfterPrevEnd, StartNumAfterPrevEnd, OfficialLevelID, MatchRulesMask, ReplayMatchRulesMask, IsFinalGroupGamePlay, ContainsTitleHolder, IsRoutingUpdated, bPlacingUpdated, IsDeleted, MaxPlayed, MaxHomeGamesBroadcast, RoundNameStringID, uNumberOfFixtures, uNumberOfPopulatedFixtures, OriginalNoOfSquads

## tactics `FUN_00a76dc0` (50 fields)

NameID, TeamPassing, TeamTackling, TeamClosingDown, TeamMentality, CornersLeft, CornersRight, FreeKicksLeft, FreeKicksRight, UserTactics, OffsideTrap, CounterAttack, MenBehindBall, ThrowInLeft, ThrowInRight, OffsideTrapDepth, IsDeleted, Passing, ClosingDown, Tackling, Marking, Mentality, CrossFrom, CrossAim, CrossHeight, Distribution, DefendFreeKicks, AttackFreeKicks, DefendCorner, AttackCorner, AttackThrowInLeft, AttackThrowInRight, LongShots, ForwardRuns, RunWithBall, FreeRole, HoldUpBall, MarkingPosition, Column, AttackRunsRow, AttackRunsColumn, DefendRunsRow, DefendRunsColumn, FeedsRow1, FeedsColumn1, FeedsRow2, FeedsColumn2, ExplicitRole, Penalty, FreeKick

## player_development `FUN_00a61c90` (49 fields)

auHostAreaID, SponsorStringID, AnnualSalaryCap, GamePointsID, FirstFixtureId, CompetitionNameID, LastFixtureId, EligibilityGlobalRegionID, BreakEndDayTimeID, BreakStartDayTimeID, SeasonID, Status, U21InSquad, MustHaveSubGoalKeeper, ReserveSchedulingCompId, YouthSchedulingCompId, CurrencyID, PreviousCompetition, FriendlySchedulingCompId, FixtureSpreadMin, NextCompetition, TransferWindowID, LoanWindowID, IsAggregate, IsPool, IsNonCompetitive, uCurrentRoundNumber, uStartRoundNumber, uEndRoundNumber, uOriginalEndRoundNumber, MaxNonGlobalRegionInMatchSquad, MaxNonGlobalRegionAtClub, MaxNonGlobalRegionBoughtPerSeason, MaxTeamsPerCountry, uAverageTrainingFacilityValue, MaxLoanPlayerUsed, RefereeAgeLimit, MaxSquadSize, IsRoutingReady, IsRoutingToUpdated, IsRoutingFromUpdated, ClearDisciplineCards, NoSubstitutesNamed, NoSubstitutesUsed, MinNumRegisteredGK, MinNumRegisteredYTH, MaxNumMarqueePlayers, bPlacingUpdated, ExcludeSameNationRef

## competitions `FUN_00a61c90` (49 fields)

auHostAreaID, SponsorStringID, AnnualSalaryCap, GamePointsID, FirstFixtureId, CompetitionNameID, LastFixtureId, EligibilityGlobalRegionID, BreakEndDayTimeID, BreakStartDayTimeID, SeasonID, Status, U21InSquad, MustHaveSubGoalKeeper, ReserveSchedulingCompId, YouthSchedulingCompId, CurrencyID, PreviousCompetition, FriendlySchedulingCompId, FixtureSpreadMin, NextCompetition, TransferWindowID, LoanWindowID, IsAggregate, IsPool, IsNonCompetitive, uCurrentRoundNumber, uStartRoundNumber, uEndRoundNumber, uOriginalEndRoundNumber, MaxNonGlobalRegionInMatchSquad, MaxNonGlobalRegionAtClub, MaxNonGlobalRegionBoughtPerSeason, MaxTeamsPerCountry, uAverageTrainingFacilityValue, MaxLoanPlayerUsed, RefereeAgeLimit, MaxSquadSize, IsRoutingReady, IsRoutingToUpdated, IsRoutingFromUpdated, ClearDisciplineCards, NoSubstitutesNamed, NoSubstitutesUsed, MinNumRegisteredGK, MinNumRegisteredYTH, MaxNumMarqueePlayers, bPlacingUpdated, ExcludeSameNationRef

## players `FUN_00a84df0` (41 fields)

PersonID, TrainingStyleID, FormationID, MarkingStyleID, PlayingMentality, PassingStyleID, PressingStyleID, PlayOffsideTrap, PlayRoamingPlayers, Determination, Professionalism, Pressure, Loyalty, Sportmanship, Adaptibility, Ambition, Superstition, CoachingGoalkeepers, CoachingOutfieldPlayers, Management, Physiotherapy, Tactics, CoachingYouthPlayers, TransferAcumen, LevelOfTrust, Generosity, ScoutingOnAbility, ScoutingOnPotential, Motivating, Strictness, Realism, ContractID, PlaysNationalTeam, Shortlist, ScoutingOrderID, ManagerHistoryID, SeekingEmployment, MarkedForAIBackroomStaffManagementTransfer, IsDeleted, CurrentAbility, PotentialAbility

## player_development `FUN_00a84df0` (41 fields)

PersonID, TrainingStyleID, FormationID, MarkingStyleID, PlayingMentality, PassingStyleID, PressingStyleID, PlayOffsideTrap, PlayRoamingPlayers, Determination, Professionalism, Pressure, Loyalty, Sportmanship, Adaptibility, Ambition, Superstition, CoachingGoalkeepers, CoachingOutfieldPlayers, Management, Physiotherapy, Tactics, CoachingYouthPlayers, TransferAcumen, LevelOfTrust, Generosity, ScoutingOnAbility, ScoutingOnPotential, Motivating, Strictness, Realism, ContractID, PlaysNationalTeam, Shortlist, ScoutingOrderID, ManagerHistoryID, SeekingEmployment, MarkedForAIBackroomStaffManagementTransfer, IsDeleted, CurrentAbility, PotentialAbility

## tactics `FUN_00a84df0` (41 fields)

PersonID, TrainingStyleID, FormationID, MarkingStyleID, PlayingMentality, PassingStyleID, PressingStyleID, PlayOffsideTrap, PlayRoamingPlayers, Determination, Professionalism, Pressure, Loyalty, Sportmanship, Adaptibility, Ambition, Superstition, CoachingGoalkeepers, CoachingOutfieldPlayers, Management, Physiotherapy, Tactics, CoachingYouthPlayers, TransferAcumen, LevelOfTrust, Generosity, ScoutingOnAbility, ScoutingOnPotential, Motivating, Strictness, Realism, ContractID, PlaysNationalTeam, Shortlist, ScoutingOrderID, ManagerHistoryID, SeekingEmployment, MarkedForAIBackroomStaffManagementTransfer, IsDeleted, CurrentAbility, PotentialAbility

## players `FUN_00a7c540` (39 fields)

AskingPrice, PlaysNationalTeam, PersonID, AvailabilityID, DescriptionID, MediaPersonalityID, Morale, CareerPersonalityID, RoleID, InternationalGoals, ContractID, InternationalCaps, InjuryID, FinancialPersonalityID, Condition, HistoryPlayer, InternationalInstruction, TrainingShedule, PlayerProperties, RetiredFromInternationals, OtherCupTiedFlags, WeeksFine, RegistrationFlags, Improvement, ThisCupTiedFlags, UnavailableReason, UnavailableCounter, ShortlistID, OfferedToUser, MatchFitness, IsPhantom, IsImmortal, IsFake, IsDeleted, CurrentAbility, PotentialAbility, HomeReputation, WorldReputation, TendencyTrainingOutfielder

## player_development `FUN_00a7c540` (39 fields)

AskingPrice, PlaysNationalTeam, PersonID, AvailabilityID, DescriptionID, MediaPersonalityID, Morale, CareerPersonalityID, RoleID, InternationalGoals, ContractID, InternationalCaps, InjuryID, FinancialPersonalityID, Condition, HistoryPlayer, InternationalInstruction, TrainingShedule, PlayerProperties, RetiredFromInternationals, OtherCupTiedFlags, WeeksFine, RegistrationFlags, Improvement, ThisCupTiedFlags, UnavailableReason, UnavailableCounter, ShortlistID, OfferedToUser, MatchFitness, IsPhantom, IsImmortal, IsFake, IsDeleted, CurrentAbility, PotentialAbility, HomeReputation, WorldReputation, TendencyTrainingOutfielder

## match_engine `FUN_0077ddc0` (32 fields)

eGameStateInPlay, eGameStateHandsKickActivated, eGameStateHandsKickSpotted, eGameStateThrowInSpotted, eGameStateGoalKickSpotted, eGameStateCornerKickSpotted, eGameStateGoalSpotted, eGameStateSpare, eGameStateBackpassSpotted, eGameStateKickOffSpotted, eGameStateSubstitutionHappening, eGameStateHalfOver, eGameStateGameOver, eGameStateApresGoal, eGameStateKickoffWaiting, eGameStatePenaltySpotted, eGameStateThrowInActivated, eGameStateGoalKickActivated, eGameStateCornerKickActivated, eGameStateGoalActivated, eGameStatePenaltyActivated, eGameStatePenaltyShootout, eGameStateCloseFKActivated, eGameStateBackpassActivated, eGameStateHalfActivated, eGameStateFreeKickActivated, eGameStateFreeKickSpotted, eGameStateFoulSpotted, eGameStateOffsideSpotted, eGameStateSetPieceFreeKickActivated, eGameStateSetPieceFreeKickSpotted, eGameStateNums

## players `FUN_00a72db0` (31 fields)

MatchSeed, HomePersonality, AwayPersonality, MatchInputsID, MakeUnReplayableAfterAnalysis, IsDeleted, HomePossession, HomeTeamDutiesID, HomePassingStyle, HomePressingStyle, HomeOffsideTrap, HomePlayingMentality, HomeModifiedUp, HomeManagerNameID, HomeLikelyResult, HomePerformanceLevel, HomePlayerModified, HomeTalkType, AwayTeamDutiesID, AwayPassingStyle, AwayPressingStyle, AwayOffsideTrap, AwayPlayingMentality, AwayModifiedUp, AwayManagerNameID, AwayLikelyResult, AwayPerformanceLevel, AwayPlayerModified, AwayTalkType, HomePlayerPersonalStatsID, AwayPlayerPersonalStatsID

## players `FUN_00a896c0` (31 fields)

PersonalityID, AssistantTakesDomesticTraining, AssistantBuysDomesticPlayers, AssistantOffersDomesticContracts, AssistantHiresAndFiresDomesticCoachingStaff, AssistantArrangesDomesticFriendlyMatches, AssistantControlsDomesticFirstTeam, AssistantSelectsDomesticFirstTeam, AssistantSelectsDomesticFirstTeamTactics, AssistantControlsDomesticReserveTeam, AssistantSelectsDomesticReserveTeam, AssistantSelectsDomesticReserveTeamTactics, AssistantControlsDomesticYouthTeam, AssistantSelectsDomesticYouthTeam, AssistantSelectsDomesticYouthTeamTactics, SugarDaddy, AsstSelectDomesticUnfitSeniors, uBoardRequestStateExtraTransferFunds, uBoardRequestStateExtraWageFunds, uBoardRequestStateExpandStadium, uBoardRequestStateImproveFacilities, uBoardRequesStatetExtraRebuildTime, FinedPlayers, DisciplinedPlayers, PendingUserSacking, BoardRequestCounterExtraTransferFunds, BoardRequestCounterExtraWageFunds, BoardRequestCounterExpandStadium, BoardRequestCounterImproveFacilities, BoardRequestCounterExtraRebuildTime, IsDeleted

## player_development `FUN_00a896c0` (31 fields)

PersonalityID, AssistantTakesDomesticTraining, AssistantBuysDomesticPlayers, AssistantOffersDomesticContracts, AssistantHiresAndFiresDomesticCoachingStaff, AssistantArrangesDomesticFriendlyMatches, AssistantControlsDomesticFirstTeam, AssistantSelectsDomesticFirstTeam, AssistantSelectsDomesticFirstTeamTactics, AssistantControlsDomesticReserveTeam, AssistantSelectsDomesticReserveTeam, AssistantSelectsDomesticReserveTeamTactics, AssistantControlsDomesticYouthTeam, AssistantSelectsDomesticYouthTeam, AssistantSelectsDomesticYouthTeamTactics, SugarDaddy, AsstSelectDomesticUnfitSeniors, uBoardRequestStateExtraTransferFunds, uBoardRequestStateExtraWageFunds, uBoardRequestStateExpandStadium, uBoardRequestStateImproveFacilities, uBoardRequesStatetExtraRebuildTime, FinedPlayers, DisciplinedPlayers, PendingUserSacking, BoardRequestCounterExtraTransferFunds, BoardRequestCounterExtraWageFunds, BoardRequestCounterExpandStadium, BoardRequestCounterImproveFacilities, BoardRequestCounterExtraRebuildTime, IsDeleted

## match_reporting `FUN_00a72db0` (31 fields)

MatchSeed, HomePersonality, AwayPersonality, MatchInputsID, MakeUnReplayableAfterAnalysis, IsDeleted, HomePossession, HomeTeamDutiesID, HomePassingStyle, HomePressingStyle, HomeOffsideTrap, HomePlayingMentality, HomeModifiedUp, HomeManagerNameID, HomeLikelyResult, HomePerformanceLevel, HomePlayerModified, HomeTalkType, AwayTeamDutiesID, AwayPassingStyle, AwayPressingStyle, AwayOffsideTrap, AwayPlayingMentality, AwayModifiedUp, AwayManagerNameID, AwayLikelyResult, AwayPerformanceLevel, AwayPlayerModified, AwayTalkType, HomePlayerPersonalStatsID, AwayPlayerPersonalStatsID

## competitions `FUN_00a667f0` (30 fields)

SquadID, GroupID, GroupNumber, Status, BonusPoints, PreviousPreviousSeasonAggregatePoints, PreviousSeasonAggregatePoints, HomeWon, HomeDrawn, HomeLost, HomeFor, HomeAgainst, HomeYellowCards, HomeRedCards, AwayWon, AwayDrawn, AwayLost, AwayFor, AwayAgainst, AwayYellowCards, AwayRedCards, ShootoutFor, GamesPlayed, PlayedThisMonth, RoundPosition, WonThisMonth, LostThisMonth, IsRouted, IsDeleted, fPoints

## player_development `FUN_00a5f970` (29 fields)

Training, BankBalance, LoansOutstanding, TicketPrice, Merchandising, GroundCost, SponsorshipPerYear, Attendence, StadiumCapacity, Reputation, SeasonTicketSales, FirstTeamPlayerPotential, FirstTeamPlayerSalary, ManagerSalary, NumPlayersInFirstSquad, NumPlayersInReserveSquad, NumPlayersInYouthSquad, NumCoaches, NumScouts, NumPhysios, NumBoardMembers, FirstTeamPlayerGlobalRep, FirstTeamPlayerDomesticRep, FirstTeamPlayerAbility, StaffAbility, BoardAbility, RefereeAbility, ClubID, IsDeleted

## competitions `FUN_00a6d620` (29 fields)

PlayerNextEntity, CumulativeRating, IsCurrentRecord, TransferID, ShotsOnTargetOrShotsHeld, IsOutfieldPlayerRecord, CompetitionNameID, CompetitionSeasonID, CurrentStatsMonth, DomesticSeasonID, CleanSheetOrAssistances, CumulativeRatingThisMonth, TacklesWonOrShotsFumbled, CrossesFumbled, FoulsMade, PassesCompleted, ShotsOrShotsTipped, FoulsAgainst, DribblesOrCrossesTaken, TacklesAttemptedOrShotsParried, KindOfCompetition, AppearancesThisMonth, PassesAttemptedOrCrossesPunches, ManOfTheMatch, Appearances, GoalsOrConcededGoals, SubAppearances, YellowCards, RedCards

## match_reporting `FUN_00a7a880` (28 fields)

FirstNameID, LastNameID, FavouriteClubID, DB_Day, DB_Month, DB_Year, AreaID, DataID, ReplaySpeed, ReplayHighlightSpeed, MatchSpeed, MatchHighlightSpeed, m_uAnalysisMatchSpeed, m_uAnalysisAlternativeWindowX, m_uAnalysisAlternativeWindowY, m_uHighLightsFlags, m_uMatchCameraViewHash, m_bMatchPlayerNames, m_bMatchPlayerNumbers, m_bMatchBallTrace, m_bMatchGoalReplays, m_bMatchCustomHighlightOptions, MatchHighlights, ReplayHighlights, m_bAnalysisPlayerNames, m_bAnalysisPlayerNumbers, m_bAnalysisBallTrace, m_bAnalysisDisplayAlternateCameraView

## competitions `FUN_00a62de0` (27 fields)

NameStringID, AreaID, IsLeague, CupTiedGroupID, DisciplineGroupID, PositionsRecorded, FrequencyInMonths, Ranking, TopClubsInterested, CompNameType, IsPlayable, FixtureRatingsByArea, Scope, AbbreviationStringID, CompetitionNameOrder, FirstTeams, RevervesTeams, YouthTeams, LatestCompetition, IsInternationalQualifying, YearsUntilNextPlayed, SpansSeasons, RelatedCompNameID, MaximumAverageAttendance, MinimumAverageAttendance, SalaryRangeMultiplier, SalaryRangeExponent

## competitions `FUN_007b3af0` (22 fields)

TDM_StaffID, scoutname, player1strength2, player3weakness3, player2weakness2, numberofplayersfound, player3strength1, player2name, player3name, player1weakness3, player2weakness1, player3weakness2, player2generaldescription, player1name, player3generaldescription, player3weakness1, ScoutedCup, player2strength1, player1generaldescription, player1weakness2, player3strength3, player2strength3

## competitions `FUN_007b5e50` (22 fields)

TDM_StaffID, scoutname, ScoutedLeague, player1strength2, player3strength2, player2weakness2, player1strength1, player2weakness3, player2name, player3name, player1weakness3, player2weakness1, player3weakness2, player2generaldescription, player1name, player3generaldescription, player3weakness1, player2strength1, player1generaldescription, player1weakness2, player3strength3, player2strength3

## tactics `FUN_00a75ca0` (21 fields)

PositionTactics, Passing, ClosingDown, Tackling, Marking, Mentality, CrossFrom, CrossAim, CrossHeight, Distribution, DefendFreeKicks, AttackFreeKicks, DefendCorner, AttackCorner, AttackThrowInLeft, AttackThrowInRight, LongShots, ForwardRuns, RunWithBall, FreeRole, HoldUpBall

## players `FUN_00a78660` (16 fields)

PersonID, Determination, Professionalism, Pressure, Loyalty, Sportsmanship, Adaptability, Ambition, Superstition, TransferAcumen, LevelOfTrust, Generosity, Realism, IsDeleted, CurrentAbility, PotentialAbility

## player_development `FUN_00a8a060` (14 fields)

PersonalityID, IsDeleted, UserShortcuts, DomesticClubID, NationalClubID, StaffMemberControlsDomesticReserveTeam, StaffMemberControlsDomesticYouthTeam, StaffID, NationalShortlistID, IsOnHoliday, FirstTeamTrainingSchedule, ReserveTeamTrainingSchedule, YouthTeamTrainingSchedule, GoalkeepersTrainingSchedule

## competitions `FUN_007b78f0` (14 fields)

TDM_StaffID, player1weakness1, player1strength2, player2generaldescription, player2weakness3, player2strength2, player2weakness1, player2name, player2weakness2, player2strength1, player1weakness3, player1generaldescription, player1strength3, player1weakness2

## competitions `FUN_007b6ac0` (14 fields)

TDM_StaffID, player2strength3, ScoutedLeague, player1strength2, numberofplayersfound, player2weakness3, player1strength1, player2name, player2weakness2, player2strength1, player1weakness3, player1generaldescription, player1strength3, player1weakness2

## competitions `FUN_007b5590` (14 fields)

TDM_StaffID, player1weakness1, player2generaldescription, player1name, player2strength2, player2weakness2, player2weakness1, player2name, ScoutedCup, player2strength1, player1weakness3, player1generaldescription, player1strength3, player1weakness2

## competitions `FUN_007b4760` (14 fields)

TDM_StaffID, player1weakness1, player1strength2, player2generaldescription, player2weakness3, player2strength2, player1strength1, player2name, ScoutedCup, player2strength1, player1weakness3, player1generaldescription, player1strength3, player1weakness2

## tactics `FUN_00a76870` (14 fields)

Passing, Tackling, ClosingDown, Mentality, CornersLeft, CornersRight, FreeKicksLeft, FreeKicksRight, UserTactics, ThrowInLeft, ThrowInRight, OffsideTrap, OffsideTrapDepth, CounterAttack

## competitions `FUN_00a6bf70` (10 fields)

CompType, KnockedOut, Winner, Drawn, Against, Points, HighestPos, LowestPos, FinalPos, CompetitionNameID

## competitions `FUN_00a6bc70` (8 fields)

CompetitionName, WinnerClubIDorStringID, RunnerUpClubIDorStringID, ThirdPlaceClubIDorStringID, bWinnerUsesClubID, bRunnerUpUsesClubID, bThirdPlaceUsesClubID, StartYear
