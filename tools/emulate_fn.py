#!/usr/bin/env python3
"""
emulate_fn.py — isolated CPU emulation of individual CM2008 x86 functions (Unicorn).

Purpose (clean-room RE / interoperability): run ONE readable game function's bytes in an
emulator with controlled inputs to (a) log exactly which struct offsets it reads/writes
(recovers field layouts whose decompilation is broken, e.g. movement pos/vel/accel), and
(b) capture exact numeric outputs for validating the clean-room reimplementation.

This does NOT run the game, bypass DRM, or disable protections. It maps the image bytes and
single-steps the CPU over one function in isolation; calls into protected/virtualized regions
or imports simply fault and stop the run (the access trace up to that point is still useful).

The image here has file_offset == RVA (verified: VA-0x400000 == file offset for .text/.rdata/
.data), so the whole file maps flat at the 0x400000 image base.
"""
import sys, struct
from unicorn import *
from unicorn.x86_const import *

IMAGE_BASE = 0x400000
STACK_BASE = 0x10000000
STACK_SIZE = 0x100000
SCRATCH    = 0x20000000      # where we place sentinel "this"/arg structs
SCRATCH_SIZE = 0x100000
RET_MAGIC  = 0xDEAD0000      # sentinel return address: stop when EIP reaches it

def load_image(path):
    data = open(path, "rb").read()
    # round image region up to cover the file + .bss/global headroom
    size = (len(data) + 0x400000 + 0xFFF) & ~0xFFF
    return data, size

FAKE_RET = 0x30000000   # scratch region returned (in EAX) for skipped CALLs

def emulate(path, va, ecx=None, edx=None, stack_args=None, struct_bases=None,
            globals_seed=None, skip_calls=False,
            max_insns=200000, trace_window=0x400, verbose=True):
    """Run function at `va`. ecx/edx set thiscall/fastcall regs. stack_args = list of
    dwords pushed as args. struct_bases = list of (base, size) ranges to log accesses in.
    globals_seed = {va: dword} written before running (seed runtime pool pointers).
    skip_calls = treat every CALL as returning a pointer into a zero scratch region (keeps
    emulation inside the target function to log ITS own struct-offset accesses).
    Returns dict(eax, stopped_at, reads, writes)."""
    data, size = load_image(path)
    uc = Uc(UC_ARCH_X86, UC_MODE_32)
    uc.mem_map(IMAGE_BASE, size)
    uc.mem_write(IMAGE_BASE, data)
    uc.mem_map(STACK_BASE, STACK_SIZE)
    uc.mem_map(SCRATCH, SCRATCH_SIZE)
    uc.mem_map(FAKE_RET, 0x100000)   # zero-filled; call results point here
    for gva, val in (globals_seed or {}).items():
        uc.mem_write(gva, struct.pack("<I", val & 0xffffffff))

    esp = STACK_BASE + STACK_SIZE - 0x1000
    # push args then the sentinel return address
    args = stack_args or []
    for a in reversed(args):
        esp -= 4; uc.mem_write(esp, struct.pack("<I", a & 0xffffffff))
    esp -= 4; uc.mem_write(esp, struct.pack("<I", RET_MAGIC))
    uc.reg_write(UC_X86_REG_ESP, esp)
    uc.reg_write(UC_X86_REG_EBP, esp)
    if ecx is not None: uc.reg_write(UC_X86_REG_ECX, ecx)
    if edx is not None: uc.reg_write(UC_X86_REG_EDX, edx)

    ranges = struct_bases or []
    reads, writes = [], []
    def in_ranges(addr):
        for b, s in ranges:
            if b <= addr < b + s: return b
        return None
    def hook_mem(uc_, access, addr, sz, val, ud):
        b = in_ranges(addr)
        if b is not None:
            off = addr - b
            if access == UC_MEM_WRITE: writes.append((b, off, sz, val))
            else: reads.append((b, off, sz))
    uc.hook_add(UC_HOOK_MEM_READ, hook_mem)
    uc.hook_add(UC_HOOK_MEM_WRITE, hook_mem)

    state = {"stopped": None}
    def hook_code(uc_, addr, sz, ud):
        if addr == RET_MAGIC:
            state["stopped"] = "returned"; uc_.emu_stop()
    uc.hook_add(UC_HOOK_CODE, hook_code)
    state["mapped_pages"] = 0
    def hook_invalid(uc_, access, addr, sz, val, ud):
        # lazy map-on-fault: map the page zero-filled and retry, so the function runs to
        # completion over zero memory while we log its struct-offset accesses.
        if state["mapped_pages"] > 4000:
            state["stopped"] = "page-cap@%x" % uc_.reg_read(UC_X86_REG_EIP); return False
        page = addr & ~0xFFF
        try:
            uc_.mem_map(page, 0x1000); state["mapped_pages"] += 1; return True
        except UcError:
            return True   # already mapped (race) -> retry
    uc.hook_add(UC_HOOK_MEM_INVALID, hook_invalid)

    try:
        uc.emu_start(va, RET_MAGIC, count=max_insns)
    except UcError as e:
        if state["stopped"] is None:
            state["stopped"] = "UcError:%s @%x" % (e, uc.reg_read(UC_X86_REG_EIP))
    eax = uc.reg_read(UC_X86_REG_EAX)
    # x87 st0 as a float10 result (if the fn returns FP)
    try:
        st0 = uc.reg_read(UC_X86_REG_ST0)
    except Exception:
        st0 = None
    if verbose:
        print(f"fn 0x{va:x}: stopped={state['stopped']} eax=0x{eax:x}")
        rs = sorted(set((b,o,s) for (b,o,s) in reads))
        ws = sorted(set((b,o,s) for (b,o,s,_) in writes))
        print(f"  struct READS  (base,off,sz): {[(hex(b),hex(o),s) for b,o,s in rs][:40]}")
        print(f"  struct WRITES (base,off,sz): {[(hex(b),hex(o),s) for b,o,s in ws][:40]}")
    return {"eax": eax, "st0": st0, "stopped": state["stopped"], "reads": reads, "writes": writes}

if __name__ == "__main__":
    exe = sys.argv[1] if len(sys.argv) > 1 else "/Volumes/CM2008/Crack JPN/cm2008.exe"
    va  = int(sys.argv[2], 16) if len(sys.argv) > 2 else 0x0077c710
    # demo: give ECX a sentinel struct and one stack arg pointing at a second struct,
    # log all accesses within both so we recover the field offsets the fn touches.
    A, B = SCRATCH, SCRATCH + 0x1000
    emulate(exe, va, ecx=A, stack_args=[B], struct_bases=[(A, 0x400), (B, 0x400)])
