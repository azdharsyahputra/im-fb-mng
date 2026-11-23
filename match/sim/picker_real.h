/*
 * Resolver "picker" FUN_005b5100 — VERIFIED exact algorithm (clean-room).
 *
 * MAJOR CORRECTION: this function is NOT a weighted inverse-CDF sampler and consumes
 * NO RNG. The prior "anti-disassembly / runtime-only" verdict was wrong — the bytes at
 * VA 0x5b5100 are ordinary x86 (entropy 4.9/8, valid prologue); Ghidra merely failed to
 * decompile them (the same false alarm as the movement cluster). Disassembled directly:
 *
 *   picker(this [ecx], int* target [stack arg]):
 *     esi = this->cursor;            // [this+0x2c]
 *     ebx = 1;                       // 1-based index
 *     if (esi == this->end) return 0;// [this+0x30]; empty -> 0
 *     do {
 *        entry = *esi;               // pool entry pointer (array stride 4)
 *        val   = *(int*)(entry+0xc); // helper 0x5b52f0 = lea eax,[ecx+0xc]; then deref
 *        if (val == *target) return ebx;   // first entry whose key == target
 *        esi += 4; ebx += 1;
 *     } while (esi != this->end);
 *     return 0;                      // not found -> 0
 *
 * i.e. a LINEAR SEARCH returning the 1-based index of the first pool entry whose key
 * (*(entry+0xc)) equals the target value (*target). Deterministic.
 *
 * Caller FUN_00817f20 (VERIFIED): target = *(node+0xc); idx = picker(pool, &target);
 * selected = pool[idx-1] ( [pool + idx*4 - 4] ); stores it at pool+0x18 (cursor) and
 * recurses. So the "picker" resolves which pool entry a formula node's selector key
 * refers to — a key->index lookup, not random sampling. (Any randomness in the resolver
 * lives elsewhere, not here.)
 */
#ifndef CM2008_PICKER_REAL_H
#define CM2008_PICKER_REAL_H
#include <stdint.h>

/* Exact reconstruction: 1-based index of the first entry whose key == target, else 0.
 * `keys[i]` models *(entry_i + 0xc) for the pool entries in [cursor..end). */
static inline int cm_picker_index(const int32_t *keys, int count, int32_t target){
    if (!keys || count <= 0) return 0;          /* cursor==end -> 0 */
    for (int i = 0; i < count; i++)
        if (keys[i] == target) return i + 1;    /* 1-based */
    return 0;                                   /* not found -> 0 */
}

/* The caller's use of the index: selected = pool[idx-1]; idx==0 means "none". */
static inline int cm_picker_selected_valid(int idx){ return idx >= 1; }

#endif
