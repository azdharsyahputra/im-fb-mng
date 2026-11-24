/*
 * CM2008 PlayerProperties.dat column schema — WORKSTREAM 2 (clean-room).
 *
 * ON-DISK LAYOUT (VERIFIED, byte-exact): 8-byte file header
 *   [0:4] magic/tag, [4:8] record count = 76051; then 76051 records of 158 bytes:
 *   u32 player_id + 77 x u16 attribute columns (little-endian, byte-aligned).
 *   Sentinel 0xFFFF = "not set". Valid attribute values observed in [0..100].
 *   Verified against extracted_game/DatabaseFiles/PlayerProperties.dat and the
 *   owner loader super-manager-engine/staff/staff.go (PropAttrCount=77).
 *
 * COLUMN -> ATTRIBUTE-NAME: *** NOT VERIFIED FOR ANY INDEX. ***
 *   No committed artifact anchors an on-disk column index to a name:
 *     - the 77-column field registrar is NOT in the committed Ghidra dumps;
 *     - the per-attribute leaf accessors (Player* + offset reads) are NOT
 *       decompiled in any dump (.c files lack the FUN_0081xxxx bodies);
 *     - CM2008.exe itself is absent (gitignored), so REA cannot recover them.
 *   The runtime enum dump (FUN in match_player/decompiled.c) uses a DIFFERENT
 *   in-memory layout (62-byte stride, 1-byte fields) than this on-disk u16
 *   record, so its order does NOT transfer to column indices.
 *   => Every column below is COLUMN_NN_UNKNOWN until a code anchor is found.
 *
 * INFERRED STRUCTURE (data route, PlayerProperties.dat distribution, 76051 recs):
 *   R1  cols  0..17 : heterogeneous scales/fills (48-86%% populated). Mixed meta +
 *                     footedness/positions/universal fields. col1 mean~90 (RightFoot?),
 *                     col7 mean~1 (rare specialist), col13 near-continuous mean~6.
 *   R2  cols 18..60 : one ATOMIC block of 43 cols, ~17.4%% populated (13236 recs),
 *                     94.9-100%% co-populated => the 43 "core" shared attributes
 *                     (technical+mental+physical+hidden). Count matches enum idx 0..42.
 *   R3  cols 61..76 : very sparse (0.2-1.2%% populated); cols 63..76 max=15 =>
 *                     1-15/1-20 specialist scale (goalkeeper/special candidates).
 *   These regions are INFERRED (count/atomicity/signature), NOT index-pinned.
 *
 * EVIDENCE-BACKED STORED ATTRIBUTE NAME SET (names present, index unknown):
 *   72 distinct stored-attribute names triangulated; 67 double-confirmed by BOTH
 *   the decompiled enum dump AND formula.xml GCPlayer int leaves. See
 *   research/PLAYER_ATTRIBUTE_MAPPING.md for the full list and provenance.
 */
#ifndef CM2008_PLAYER_ATTRIBUTE_SCHEMA_H
#define CM2008_PLAYER_ATTRIBUTE_SCHEMA_H

#define CM_PP_COLUMN_COUNT 77
#define CM_PP_RECORD_BYTES 158   /* u32 id + 77*u16 */
#define CM_PP_NULL16 0xFFFFu     /* "not set" sentinel */

/* Region hints for the data-route INFERENCE (not index-pinned names). */
#define CM_PP_R1_META_BEGIN   0   /* ..17  mixed meta/foot/position/universal */
#define CM_PP_R2_CORE_BEGIN  18   /* ..60  43 atomic core attributes (~17%% filled) */
#define CM_PP_R3_SPARSE_BEGIN 61  /* ..76  sparse specialist, 1-15/1-20 scale */

/* No column index is VERIFIED to a name -> all placeholders. */
enum cm_pp_column {
    CM_PP_COLUMN_00_UNKNOWN = 0,   /* R1 */
    CM_PP_COLUMN_01_UNKNOWN = 1,   /* R1 */
    CM_PP_COLUMN_02_UNKNOWN = 2,   /* R1 */
    CM_PP_COLUMN_03_UNKNOWN = 3,   /* R1 */
    CM_PP_COLUMN_04_UNKNOWN = 4,   /* R1 */
    CM_PP_COLUMN_05_UNKNOWN = 5,   /* R1 */
    CM_PP_COLUMN_06_UNKNOWN = 6,   /* R1 */
    CM_PP_COLUMN_07_UNKNOWN = 7,   /* R1 */
    CM_PP_COLUMN_08_UNKNOWN = 8,   /* R1 */
    CM_PP_COLUMN_09_UNKNOWN = 9,   /* R1 */
    CM_PP_COLUMN_10_UNKNOWN = 10,   /* R1 */
    CM_PP_COLUMN_11_UNKNOWN = 11,   /* R1 */
    CM_PP_COLUMN_12_UNKNOWN = 12,   /* R1 */
    CM_PP_COLUMN_13_UNKNOWN = 13,   /* R1 */
    CM_PP_COLUMN_14_UNKNOWN = 14,   /* R1 */
    CM_PP_COLUMN_15_UNKNOWN = 15,   /* R1 */
    CM_PP_COLUMN_16_UNKNOWN = 16,   /* R1 */
    CM_PP_COLUMN_17_UNKNOWN = 17,   /* R1 */
    CM_PP_COLUMN_18_UNKNOWN = 18,   /* R2_core */
    CM_PP_COLUMN_19_UNKNOWN = 19,   /* R2_core */
    CM_PP_COLUMN_20_UNKNOWN = 20,   /* R2_core */
    CM_PP_COLUMN_21_UNKNOWN = 21,   /* R2_core */
    CM_PP_COLUMN_22_UNKNOWN = 22,   /* R2_core */
    CM_PP_COLUMN_23_UNKNOWN = 23,   /* R2_core */
    CM_PP_COLUMN_24_UNKNOWN = 24,   /* R2_core */
    CM_PP_COLUMN_25_UNKNOWN = 25,   /* R2_core */
    CM_PP_COLUMN_26_UNKNOWN = 26,   /* R2_core */
    CM_PP_COLUMN_27_UNKNOWN = 27,   /* R2_core */
    CM_PP_COLUMN_28_UNKNOWN = 28,   /* R2_core */
    CM_PP_COLUMN_29_UNKNOWN = 29,   /* R2_core */
    CM_PP_COLUMN_30_UNKNOWN = 30,   /* R2_core */
    CM_PP_COLUMN_31_UNKNOWN = 31,   /* R2_core */
    CM_PP_COLUMN_32_UNKNOWN = 32,   /* R2_core */
    CM_PP_COLUMN_33_UNKNOWN = 33,   /* R2_core */
    CM_PP_COLUMN_34_UNKNOWN = 34,   /* R2_core */
    CM_PP_COLUMN_35_UNKNOWN = 35,   /* R2_core */
    CM_PP_COLUMN_36_UNKNOWN = 36,   /* R2_core */
    CM_PP_COLUMN_37_UNKNOWN = 37,   /* R2_core */
    CM_PP_COLUMN_38_UNKNOWN = 38,   /* R2_core */
    CM_PP_COLUMN_39_UNKNOWN = 39,   /* R2_core */
    CM_PP_COLUMN_40_UNKNOWN = 40,   /* R2_core */
    CM_PP_COLUMN_41_UNKNOWN = 41,   /* R2_core */
    CM_PP_COLUMN_42_UNKNOWN = 42,   /* R2_core */
    CM_PP_COLUMN_43_UNKNOWN = 43,   /* R2_core */
    CM_PP_COLUMN_44_UNKNOWN = 44,   /* R2_core */
    CM_PP_COLUMN_45_UNKNOWN = 45,   /* R2_core */
    CM_PP_COLUMN_46_UNKNOWN = 46,   /* R2_core */
    CM_PP_COLUMN_47_UNKNOWN = 47,   /* R2_core */
    CM_PP_COLUMN_48_UNKNOWN = 48,   /* R2_core */
    CM_PP_COLUMN_49_UNKNOWN = 49,   /* R2_core */
    CM_PP_COLUMN_50_UNKNOWN = 50,   /* R2_core */
    CM_PP_COLUMN_51_UNKNOWN = 51,   /* R2_core */
    CM_PP_COLUMN_52_UNKNOWN = 52,   /* R2_core */
    CM_PP_COLUMN_53_UNKNOWN = 53,   /* R2_core */
    CM_PP_COLUMN_54_UNKNOWN = 54,   /* R2_core */
    CM_PP_COLUMN_55_UNKNOWN = 55,   /* R2_core */
    CM_PP_COLUMN_56_UNKNOWN = 56,   /* R2_core */
    CM_PP_COLUMN_57_UNKNOWN = 57,   /* R2_core */
    CM_PP_COLUMN_58_UNKNOWN = 58,   /* R2_core */
    CM_PP_COLUMN_59_UNKNOWN = 59,   /* R2_core */
    CM_PP_COLUMN_60_UNKNOWN = 60,   /* R2_core */
    CM_PP_COLUMN_61_UNKNOWN = 61,   /* R3_sparse */
    CM_PP_COLUMN_62_UNKNOWN = 62,   /* R3_sparse */
    CM_PP_COLUMN_63_UNKNOWN = 63,   /* R3_sparse */
    CM_PP_COLUMN_64_UNKNOWN = 64,   /* R3_sparse */
    CM_PP_COLUMN_65_UNKNOWN = 65,   /* R3_sparse */
    CM_PP_COLUMN_66_UNKNOWN = 66,   /* R3_sparse */
    CM_PP_COLUMN_67_UNKNOWN = 67,   /* R3_sparse */
    CM_PP_COLUMN_68_UNKNOWN = 68,   /* R3_sparse */
    CM_PP_COLUMN_69_UNKNOWN = 69,   /* R3_sparse */
    CM_PP_COLUMN_70_UNKNOWN = 70,   /* R3_sparse */
    CM_PP_COLUMN_71_UNKNOWN = 71,   /* R3_sparse */
    CM_PP_COLUMN_72_UNKNOWN = 72,   /* R3_sparse */
    CM_PP_COLUMN_73_UNKNOWN = 73,   /* R3_sparse */
    CM_PP_COLUMN_74_UNKNOWN = 74,   /* R3_sparse */
    CM_PP_COLUMN_75_UNKNOWN = 75,   /* R3_sparse */
    CM_PP_COLUMN_76_UNKNOWN = 76,   /* R3_sparse */
    CM_PP_COLUMN_END = 77
};

/* Column index -> name is intentionally NOT provided: unproven. Use the loader
   in super-manager-engine/staff/staff.go (AttrVec, 77 u16) for raw access. */

#endif /* CM2008_PLAYER_ATTRIBUTE_SCHEMA_H */
