#!/usr/bin/env python3
"""
dump_formula_from_memory.py — recover the DECRYPTED formula.xml from a running,
legitimately-owned, unmodified CM2008 by reading YOUR OWN process memory.

WHY THIS IS LEGITIMATE: CM2008 decrypts DataXML/formula.xml.blowfish into memory
itself at load time. This tool only READS the already-decrypted buffer out of the
cm2008.exe process you launched. It does NOT inject code, write memory, attach a
debugger, disable anti-debugging, modify the executable, or touch DRM. It opens the
process with READ-ONLY rights (PROCESS_QUERY_INFORMATION | PROCESS_VM_READ) and scans.

REQUIREMENTS / RULES:
- Windows (uses the Win32 API via ctypes). Run it on the same machine as the game.
- Use a LEGITIMATELY-OWNED, UNMODIFIED CM2008 install. Do NOT use a cracked build.
- Run normal Python (not elevated unless your OS requires it to read the process).

USAGE:
    1. Launch CM2008 normally; let it reach the main menu / load a game (formulae load
       at startup). 2. Run:  python dump_formula_from_memory.py
    It scans cm2008.exe memory for the decrypted XML and writes formula.xml here.
    3. Validate:  xmllint --schema champman.xsd formula.xml --noout

The decrypted header is known: <?xml version="1.0" encoding="iso-8859-1"?> and the
formula file's root is <dataroot> containing <formrule>/<formula>/<rule> (per
champman.xsd) — that is how this picks the formula buffer apart from ProgramStep.xml
(<Program>) and the FrontEnd screens (urn:BGStudios:FELayout).
"""
import ctypes, ctypes.wintypes as w, sys, re

PROCESS_QUERY_INFORMATION = 0x0400
PROCESS_VM_READ           = 0x0010
MEM_COMMIT  = 0x1000
PAGE_GUARD  = 0x100
PAGE_NOACCESS = 0x01
READABLE = (0x02,0x04,0x20,0x40,0x80)  # R, RW, X-R, X-RW, X-WC (readable protections)

XML_HEAD = b'<?xml version="1.0" encoding="iso-8859-1"'
FORMULA_MARKERS = (b'<formrule', b'<formula ', b'<formulae', b'<rule ')
END = b'</dataroot>'

class MEMORY_BASIC_INFORMATION(ctypes.Structure):
    _fields_ = [("BaseAddress", ctypes.c_void_p),("AllocationBase", ctypes.c_void_p),
                ("AllocationProtect", w.DWORD),("RegionSize", ctypes.c_size_t),
                ("State", w.DWORD),("Protect", w.DWORD),("Type", w.DWORD)]

def find_pid(name="cm2008.exe"):
    k32 = ctypes.windll.kernel32; ps = ctypes.windll.psapi
    TH32CS_SNAPPROCESS = 0x2
    class PE32(ctypes.Structure):
        _fields_=[("dwSize",w.DWORD),("cntUsage",w.DWORD),("th32ProcessID",w.DWORD),
                  ("th32DefaultHeapID",ctypes.POINTER(ctypes.c_ulong)),("th32ModuleID",w.DWORD),
                  ("cntThreads",w.DWORD),("th32ParentProcessID",w.DWORD),("pcPriClassBase",ctypes.c_long),
                  ("dwFlags",w.DWORD),("szExeFile",ctypes.c_char*260)]
    snap = k32.CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0)
    e = PE32(); e.dwSize = ctypes.sizeof(PE32)
    pids=[]
    if k32.Process32First(snap, ctypes.byref(e)):
        while True:
            if e.szExeFile.decode(errors="ignore").lower()==name.lower():
                pids.append(e.th32ProcessID)
            if not k32.Process32Next(snap, ctypes.byref(e)): break
    k32.CloseHandle(snap)
    return pids

def scan(pid):
    k32 = ctypes.windll.kernel32
    h = k32.OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, False, pid)
    if not h: raise SystemExit(f"OpenProcess failed (err {ctypes.get_last_error()}). Try a shell with rights to read the process.")
    mbi = MEMORY_BASIC_INFORMATION(); addr = 0
    best = None
    read = k32.ReadProcessMemory
    while k32.VirtualQueryEx(h, ctypes.c_void_p(addr), ctypes.byref(mbi), ctypes.sizeof(mbi)):
        base = mbi.BaseAddress or 0; size = mbi.RegionSize or 0
        prot = mbi.Protect
        if mbi.State==MEM_COMMIT and (prot & 0xff) in READABLE and not (prot & PAGE_GUARD):
            buf = ctypes.create_string_buffer(size); got = ctypes.c_size_t(0)
            if read(h, ctypes.c_void_p(base), buf, size, ctypes.byref(got)) and got.value:
                data = buf.raw[:got.value]
                pos = 0
                while True:
                    i = data.find(XML_HEAD, pos)
                    if i < 0: break
                    pos = i+1
                    window = data[i:i+2000]
                    if any(m in window for m in FORMULA_MARKERS):
                        j = data.find(END, i)
                        if j > i:
                            chunk = data[i:j+len(END)]
                            if best is None or len(chunk) > len(best):
                                best = chunk
        addr = base + size
        if addr >= 0x7fffffff: break
    k32.CloseHandle(h)
    return best

if __name__ == "__main__":
    if sys.platform != "win32":
        raise SystemExit("Windows only. Run on the machine where CM2008 is running.")
    ctypes.windll.kernel32.SetLastError(0)
    name = sys.argv[1] if len(sys.argv)>1 else "cm2008.exe"
    pids = find_pid(name)
    if not pids: raise SystemExit(f"{name} not running. Launch CM2008 first, reach the menu, then re-run.")
    for pid in pids:
        print(f"scanning {name} pid={pid} ...")
        try: out = scan(pid)
        except SystemExit as e: print(e); continue
        if out:
            open("formula.xml","wb").write(out)
            print(f"WROTE formula.xml ({len(out)} bytes). Validate: xmllint --schema champman.xsd formula.xml --noout")
            break
    else:
        print("No formula buffer found. Make sure the game finished loading (try entering a game), then re-run.")
