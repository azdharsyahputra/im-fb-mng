/*
 * test_database_fields — regression + real-data validation for the DB field accessors.
 *
 * Part 1 (always): synthetic round-trip of the VERIFIED Club runtime bit accessors
 *   (cm_club_decode_runtime) — pack a word, decode, assert the shift/mask extraction,
 *   including the overlapping date/reputation/year words and the s31 sign-extension.
 * Part 2 (if the owner's files are present): decode real Club/Player/Contract records
 *   and assert the ON-DISK structural invariants proven in BITPACKED_FIELD_MAP.md
 *   (container math, -1.0 double at Club+22, YearFounded range, Contract start<=end,
 *   flag mostly 0, sequential ids). No invented field values are asserted.
 *
 * Build: cc -std=c11 -Wall -Wextra src/database/club_fields.c src/database/player_fields.c \
 *        src/database/contract_fields.c tests/test_database_fields.c -o tests/test_database_fields
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/database/club_fields.h"
#include "../src/database/player_fields.h"
#include "../src/database/contract_fields.h"

static int g_fail = 0;
#define CHECK(cond, msg) do { \
    if (cond) { printf("  ok   : %s\n", msg); } \
    else      { printf("  FAIL : %s\n", msg); g_fail++; } } while (0)

static void put_u32(uint8_t *p, size_t off, uint32_t v) {
    p[off]=(uint8_t)v; p[off+1]=(uint8_t)(v>>8); p[off+2]=(uint8_t)(v>>16); p[off+3]=(uint8_t)(v>>24);
}

/* ---------------- Part 1: synthetic round-trip of runtime bit accessors ---------- */
static void test_runtime_roundtrip(void) {
    puts("[club runtime] synthetic bit round-trip (VERIFIED offsets from FUN_00a2f330)");
    uint8_t obj[0x80];
    memset(obj, 0, sizeof obj);

    /* +0x04: NationalClub(b12)=1, Administration(b13)=0, low12 area=0x123 */
    put_u32(obj, 0x04, 0x123u | (1u << 12));
    /* +0x08: BankBalance s31 = -5 (bits0..30), bit31 clear */
    put_u32(obj, 0x08, (uint32_t)(-5) & 0x7fffffffu);
    /* +0x10: ClubStatusID = 4 */
    put_u32(obj, 0x10, 4u);
    /* +0x38: RecordLeagueClubID=0x0abc, RLD_Day=21, RLD_Month=11 */
    put_u32(obj, 0x38, 0x0abcu | (21u << 16) | (11u << 22));
    /* +0x3c: RLD_Year=1998, Reputation=100 */
    put_u32(obj, 0x3c, 1998u | (100u << 12));
    /* +0x6c: YouthDevelopment=42, HomeMatchDayID=9, YearFounded=1888 */
    put_u32(obj, 0x6c, 42u | (9u << 7) | (1888u << 13));

    cm_club_runtime c;
    cm_club_decode_runtime(obj, &c);

    CHECK(c.national_club == 1,            "NationalClub bit12");
    CHECK(c.administration == 0,           "Administration bit13");
    CHECK(c.bank_balance == -5,            "BankBalance s31 sign-extend");
    CHECK(c.club_status_id == 4 && c.status_valid, "ClubStatusID 3-bit");
    CHECK(c.record_league_club == 0x0abc,  "RecordLeagueClubID 16-bit");
    CHECK(c.record_league_date.day == 21 && c.record_league_date.month == 11, "RLD day/month share +0x38");
    CHECK(c.record_league_date.year == 1998, "RLD_Year 12-bit @ +0x3c");
    CHECK(c.reputation == 100,             "Reputation 7-bit @ +0x3c b12 (overlaps RLD_Year word)");
    CHECK(c.youth_development == 42 && c.yd_valid,  "YouthDevelopment 7-bit @ +0x6c");
    CHECK(c.home_match_day_id == 9,        "HomeMatchDayID 6-bit @ +0x6c b7");
    CHECK(c.year_founded == 1888 && c.yf_valid, "YearFounded 12-bit @ +0x6c b13 (overlaps YD word)");

    /* sentinels -> *_valid false */
    memset(obj, 0xff, sizeof obj);
    cm_club_decode_runtime(obj, &c);
    CHECK(!c.yf_valid && !c.yd_valid && !c.rla_valid && !c.status_valid,
          "all-1 word -> validity flags cleared (null sentinels)");
}

/* ---------------- Part 2: validation against the owner's real files -------------- */
static uint8_t *slurp(const char *path, size_t *n) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END); long sz = ftell(f); fseek(f, 0, SEEK_SET);
    if (sz <= 0) { fclose(f); return NULL; }
    uint8_t *b = malloc((size_t)sz);
    if (b && fread(b, 1, (size_t)sz, f) != (size_t)sz) { free(b); b = NULL; }
    fclose(f); if (n) *n = (size_t)sz; return b;
}

static void test_real_club(const char *dir) {
    char path[1024]; snprintf(path, sizeof path, "%s/Club.dat", dir);
    size_t n; uint8_t *buf = slurp(path, &n);
    if (!buf) { printf("  skip : Club.dat not present (%s)\n", path); return; }
    puts("[club on-disk] real Club.dat");
    cm_dat_table t;
    CHECK(cm_dat_open(&t, buf, n) == 0, "DATa envelope parses");
    CHECK(t.rec_size == 169 && t.count == 6571, "record size 169, count 6571");
    /* -1.0 double at +22 in (nearly) every record */
    int neg1 = 0; for (uint32_t i = 0; i < t.count; i++)
        if (cm_rd_f64(cm_dat_record(&t, i), 22) == -1.0) neg1++;
    CHECK(neg1 == (int)t.count, "double at +22 == -1.0 in 100% of records");
    /* YearFounded in a sane range for a healthy fraction */
    int yok = 0, ysamp = 0; for (uint32_t i = 0; i < t.count && ysamp < 4000; i++, ysamp++) {
        uint16_t y = cm_rd_u16(cm_dat_record(&t, i), 120);
        if (y >= 1850 && y <= 2015) yok++;
    }
    CHECK(yok > ysamp / 2, "YearFounded @ +120 in [1850,2015] for most records");
    cm_club_ondisk c; cm_club_decode_ondisk(cm_dat_record(&t, 0), &c);
    printf("    rec0: id=%u year=%u fin0=%.1f\n", c.record_id, c.year_founded, c.financial0);
    free(buf);
}

static void test_real_contract(const char *dir) {
    char path[1024]; snprintf(path, sizeof path, "%s/Contract.dat", dir);
    size_t n; uint8_t *buf = slurp(path, &n);
    if (!buf) { printf("  skip : Contract.dat not present\n"); return; }
    puts("[contract on-disk] real Contract.dat");
    cm_dat_table t;
    CHECK(cm_dat_open(&t, buf, n) == 0, "DATa envelope parses");
    CHECK(t.rec_size == 29 && t.count == 109484, "record size 29, count 109484");
    int both = 0, le = 0, flagset = 0, flag_nonbool = 0;
    for (uint32_t i = 0; i < t.count; i++) {
        cm_contract_ondisk c; cm_contract_decode_ondisk(cm_dat_record(&t, i), &c);
        if (c.flag) flagset++;
        if (c.flag > 1) flag_nonbool++;
        if (cm_date_valid(c.start_date) && cm_date_valid(c.end_date)) {
            both++;
            uint32_t a = (uint32_t)c.start_date.year*10000u + (uint32_t)c.start_date.month*100u + c.start_date.day;
            uint32_t b = (uint32_t)c.end_date.year*10000u   + (uint32_t)c.end_date.month*100u   + c.end_date.day;
            if (a <= b) le++;
        }
    }
    CHECK(both > 1000, "many records have two valid dates");
    CHECK(le == both, "start_date <= end_date in 100% of valid pairs");
    CHECK(flag_nonbool == 0, "flag @ +28 is boolean (only 0/1)");
    CHECK(flagset * 20 < (int)t.count, "flag @ +28 set in <5% of records (1.48% observed)");
    free(buf);
}

static void test_real_player(const char *dir) {
    char path[1024]; snprintf(path, sizeof path, "%s/Player.dat", dir);
    size_t n; uint8_t *buf = slurp(path, &n);
    if (!buf) { printf("  skip : Player.dat not present\n"); return; }
    puts("[player on-disk] real Player.dat");
    cm_dat_table t;
    CHECK(cm_dat_open(&t, buf, n) == 0, "DATa envelope parses");
    CHECK(t.rec_size == 41 && t.count == 85035, "record size 41, count 85035");
    /* record ids ascending across a sample */
    int asc = 1; uint32_t prev = 0;
    for (uint32_t i = 0; i < 2000; i++) {
        cm_player_ondisk p; cm_player_decode_ondisk(cm_dat_record(&t, i), &p);
        if (i && p.record_id <= prev) asc = 0; prev = p.record_id;
    }
    CHECK(asc, "record_id @ +0 strictly ascending over first 2000");
    free(buf);
}

int main(int argc, char **argv) {
    const char *dir = (argc > 1) ? argv[1]
                                 : "/Users/csadeveloper/cm/extracted_game/DatabaseFiles";
    test_runtime_roundtrip();
    test_real_club(dir);
    test_real_contract(dir);
    test_real_player(dir);
    if (g_fail) { printf("\n%d check(s) FAILED\n", g_fail); return 1; }
    puts("\nall checks passed");
    return 0;
}
