/*
 * Tactical tunable getters — 1:1 reconstruction of CM2008.exe data getters.
 */
#include "config.h"
#include <stddef.h>

/*
 * Reconstruction of FUN_0083ea10 (clean decompile in
 * docs/ghidra_dumps/raw/tactics.c). Original behaviour:
 *   - resolve field "MarkingDistanceScale" in "PlayerData" (cached handle);
 *   - if the field is not present (handle == 0) return 0.02f;
 *   - otherwise return the field's float value.
 * Confidence: Observed (default 0.02 and the lookup-with-fallback are both read
 * directly from the decompilation).
 */
float cm_marking_distance_scale(const cm_data_source *src) {
    float v;
    if (src && src->get_float && src->get_float(src->ctx, "MarkingDistanceScale", &v))
        return v;
    return 0.02f; /* Observed hardcoded default */
}
