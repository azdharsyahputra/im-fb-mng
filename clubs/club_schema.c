/* Club record schema — reconstructed from CM2008.exe FUN_00a5d0e0. */
#include "club_schema.h"
#include <string.h>

const char *const CM_CLUB_FIELD_NAMES[CM_CLUB_FIELD_COUNT] = {
    "ClubStringID", "AreaID", "NationalClub", "Administration",
    "BankBalance", "ClubCoEfficient", "ClubStatusID", "CurrencyID",
    "GroundCost", "KitAwayColour1", "KitAwayColour2", "KitHomeColour1",
    "KitHomeColour2", "Merchandising", "NickNameStringID", "OutstandingLoan",
    "RecordLeagueAttendance", "RecordLeagueClubID", "RLD_Day", "RLD_Month",
    "RLD_Year", "Reputation", "SeasonTicketPrice", "ShortNameStringID",
    "SponsorAmount", "SE_Day", "SE_Month", "SE_Year",
    "SS_Day", "SS_Month", "SS_Year", "TicketPrice",
    "Training", "TVRevenue", "MostAppearancesNameStringID", "MostAppearances",
    "MostGoalsNameStringID", "MostGoals", "YouthAccademy", "YouthDevelopment",
    "HomeMatchDayID", "YearFounded", "SeasonTicketHolders", "TransferBudget",
    "WageBudget", "MarqueePlayerWeeklyWageBudget", "AccumulatedInterest", "MonthlyLoanRepayment",
    "PercentTransferRevenue", "SquadsClubID", "FirstBoardMember", "ClubStaff",
    "IncomeThisMonth", "IncomeLastMonth", "IncomeThisSeason", "IncomeLastSeason",
    "ExpenditureThisMonth", "ExpenditureLastMonth", "ExpenditureThisSeason", "ExpenditureLastSeason",
    "BoardSeasonExpectation", "BoardSeasonExpectationCommentIndex", "BoardOpinion", "FANationalBoardOpinion",
    "FANationalBoardExpectation", "FANationalBoardExpectationData", "AverageYouthPotential", "UnderCaretakerManagement",
    "ProspectiveManagers", "ProspectiveManagersAssessment_Day", "ProspectiveManagersAssessment_Month", "ProspectiveManagersAssessment_Year",
    "AppliedManagers", "SuccessiveProspectiveManagerMisses", "HistoryClub", "HistoryClubThisSeason",
    "AverageAttendance", "UnderTrainingFacilitiesUpdate", "TrainingFacilitiesUpdateDaysRemaining", "ProposedTrainingFacilitiesChange",
    "TrainingFacilitiesUpdateDueToStart", "FanOpinion", "FanSeasonExpectation", "FanSeasonExpectationContinental",
    "FanSeasonExpectationCommentIndex", "SquadFirstTeam", "SquadReserveTeam", "SquadYouthTeam",
    "Playable", "ExpectedDomesticLeagueFinishPosition", "ExpectedFinishBracket", "Defunct",
    "ScoutAreaHistoryHead", "ScoutAreaHistory", "WeakPosition1", "WeakPosition2",
    "WeakPosition3", "WeakInDepth1", "WeakInDepth2", "WeakInDepth3",
    "WeakMinAbility1", "WeakMinAbility2", "WeakMinAbility3", "FirstRivalClubID",
    "FinancialStatus", "IsDeleted", "ExpectedPrizeAndTVMoney", "HeadSourceClubOfferID",
    "HeadDestinationClubOfferID", "RegisteredCompetitionID1", "RegisteredCompetitionID2",
};

const char *cm_club_schema_name(int i) {
    if (i < 0 || i >= CM_CLUB_FIELD_COUNT) return (const char *)0;
    return CM_CLUB_FIELD_NAMES[i];
}

int cm_club_schema_from_name(const char *name) {
    if (!name) return -1;
    for (int i = 0; i < CM_CLUB_FIELD_COUNT; i++)
        if (strcmp(name, CM_CLUB_FIELD_NAMES[i]) == 0) return i;
    return -1;
}
