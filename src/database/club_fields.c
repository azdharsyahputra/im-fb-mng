/*
 * club_fields.c — Club accessors. See club_fields.h / research/BITPACKED_FIELD_MAP.md.
 * Evidence: FUN_00a5d0e0 (schema, raw/player_development.c:1) and FUN_00a2f330
 * (runtime bit accessors, raw/player_development.c:3466). On-disk layout validated
 * against extracted_game/DatabaseFiles/Club.dat (6571 x 169 B).
 */
#include "club_fields.h"

/* VERIFIED verbatim from FUN_00a5d0e0 (matches clubs/club_schema.c). */
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

int cm_club_field_index(const char *name) {
    if (!name) return -1;
    for (int i = 0; i < CM_CLUB_FIELD_COUNT; i++)
        if (strcmp(name, CM_CLUB_FIELD_NAMES[i]) == 0) return i;
    return -1;
}
const char *cm_club_field_name(int idx) {
    return (idx >= 0 && idx < CM_CLUB_FIELD_COUNT) ? CM_CLUB_FIELD_NAMES[idx] : NULL;
}

void cm_club_decode_runtime(const uint8_t *obj, cm_club_runtime *o) {
    o->national_club  = cm_bits(obj, 0x04, 12, 0x1);
    o->administration = cm_bits(obj, 0x04, 13, 0x1);
    o->bank_balance   = cm_bits_s31(obj, 0x08);
    o->club_coefficient = cm_bits_s31(obj, 0x0c); o->coef_valid = (o->club_coefficient != -1);
    o->club_status_id = cm_bits(obj, 0x10, 0, 0x7); o->status_valid = (o->club_status_id != 7);
    o->ground_cost    = cm_bits_s31(obj, 0x14);
    o->kit_away1 = cm_rd_u32(obj, 0x18); o->kit_away2 = cm_rd_u32(obj, 0x1c);
    o->kit_home1 = cm_rd_u32(obj, 0x20); o->kit_home2 = cm_rd_u32(obj, 0x24);
    o->merchandising  = cm_bits_s31(obj, 0x28);
    o->outstanding_loan = cm_bits_s31(obj, 0x30);
    o->loan_flag      = cm_bits(obj, 0x30, 31, 0x1);
    o->record_league_att = cm_bits(obj, 0x34, 0, 0x3ffff); o->rla_valid = (o->record_league_att != 0x3ffff);
    o->record_league_club = cm_bits(obj, 0x38, 0, 0xffff);  o->rlc_valid = (o->record_league_club != 0xffff);
    o->record_league_date.day   = (uint8_t) cm_bits(obj, 0x38, 16, 0x3f);
    o->record_league_date.month = (uint8_t) cm_bits(obj, 0x38, 22, 0x1f);
    o->record_league_date.year  = (uint16_t)cm_bits(obj, 0x3c, 0, 0xfff);
    o->reputation     = cm_bits(obj, 0x3c, 12, 0x7f);
    o->sponsor_amount = cm_bits_s31(obj, 0x48);
    o->sponsor_end.day   = (uint8_t) cm_bits(obj, 0x4c, 0, 0x3f);
    o->sponsor_end.month = (uint8_t) cm_bits(obj, 0x4c, 6, 0x1f);
    o->sponsor_end.year  = (uint16_t)cm_bits(obj, 0x4c, 11, 0xfff);
    o->sponsor_start.day   = (uint8_t) cm_bits(obj, 0x4c, 23, 0x3f);
    o->sponsor_start.month = (uint8_t) cm_bits(obj, 0x50, 0, 0x1f);
    o->sponsor_start.year  = (uint16_t)cm_bits(obj, 0x50, 5, 0xfff);
    o->ticket_price   = cm_bits_signed(obj, 0x50, 17, 11);
    o->training       = cm_bits(obj, 0x54, 0, 0xff);
    o->tv_revenue     = cm_bits_s31(obj, 0x58);
    o->most_appearances = cm_bits(obj, 0x60, 0, 0x3fffffff); o->ma_valid = (o->most_appearances != 0x3fffffff);
    o->most_goals     = cm_bits(obj, 0x68, 0, 0x3fffffff);   o->mg_valid = (o->most_goals != 0x3fffffff);
    o->youth_academy  = cm_bits(obj, 0x68, 30, 0x1);
    o->youth_development = cm_bits(obj, 0x6c, 0, 0x7f); o->yd_valid  = (o->youth_development != 0x7f);
    o->home_match_day_id = cm_bits(obj, 0x6c, 7, 0x3f); o->hmd_valid = (o->home_match_day_id != 0x3f);
    o->year_founded   = cm_bits(obj, 0x6c, 13, 0xfff); o->yf_valid  = (o->year_founded != 0xfff);
    o->season_ticket  = cm_bits(obj, 0x70, 0, 0x3fffffff);
}

void cm_club_decode_ondisk(const uint8_t *rec, cm_club_ondisk *o) {
    o->record_id   = cm_rd_u32(rec, 0);
    o->status_enum = cm_rd_u32(rec, 8);
    o->financial0  = cm_rd_f64(rec, 14);
    o->financial1  = cm_rd_f64(rec, 38);
    o->financial2  = cm_rd_f64(rec, 62);
    o->financial3  = cm_rd_f64(rec, 74);
    o->financial4  = cm_rd_f64(rec, 110);
    o->year_founded = cm_rd_u16(rec, 120);
}
