import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.*;
import ghidra.program.model.address.*;
import ghidra.program.model.symbol.*;
import java.io.PrintWriter;

// FindGeom — static locator over the UNPACKED cm2008_full DB (read-only; no game run).
// (1) Instruction stores to [reg+0xC0] / [reg+0xD0] (MOV/FST/FISTP), to find the geometry writer,
//     flagged by address range (sim 0x9b0000-0x9fffff vs other).
// (2) Thread-creation call sites (_beginthreadex / CreateThread / _beginthread) -> the worker entry.
// Usage: analyzeHeadless ... -postScript FindGeom.java OUT.txt
public class FindGeom extends GhidraScript {
    public void run() throws Exception {
        String[] a = getScriptArgs();
        PrintWriter w = new PrintWriter(a[0]);
        FunctionManager fm = currentProgram.getFunctionManager();

        w.println("### GEOMETRY STORES to +0xC0 / +0xD0 (op0 = memory dest) ###");
        int simHits = 0, otherHits = 0, cap = 400;
        InstructionIterator it = currentProgram.getListing().getInstructions(true);
        while (it.hasNext() && (simHits + otherHits) < cap) {
            Instruction insn = it.next();
            if (insn.getNumOperands() < 1) continue;
            String mnem = insn.getMnemonicString().toUpperCase();
            boolean storeMnem = mnem.startsWith("MOV") || mnem.equals("FSTP") || mnem.equals("FST")
                || mnem.equals("FISTP") || mnem.equals("FIST");
            if (!storeMnem) continue;
            String dst = insn.getDefaultOperandRepresentation(0);
            if (dst == null) continue;
            boolean c0 = dst.contains("0xc0]") || dst.contains("0xC0]");
            boolean d0 = dst.contains("0xd0]") || dst.contains("0xD0]");
            if (!c0 && !d0) continue;
            Address at = insn.getAddress();
            long off = at.getOffset();
            boolean sim = (off >= 0x9b0000L && off <= 0x9fffffL);
            Function fn = fm.getFunctionContaining(at);
            String fname = (fn == null) ? "?" : fn.getName();
            w.println((sim ? "[SIM] " : "[   ] ") + at + "  " + fname + "  " + mnem + "  " + dst
                + "  ; src=" + (insn.getNumOperands() > 1 ? insn.getDefaultOperandRepresentation(1) : ""));
            if (sim) simHits++; else otherHits++;
        }
        w.println("-- store hits: sim=" + simHits + " other=" + otherHits + (it.hasNext() ? " (capped)" : ""));

        w.println();
        w.println("### THREAD-CREATION call sites ###");
        String[] names = {"_beginthreadex", "beginthreadex", "CreateThread", "_beginthread"};
        SymbolTable st = currentProgram.getSymbolTable();
        ReferenceManager rm = currentProgram.getReferenceManager();
        for (String nm : names) {
            SymbolIterator si = st.getSymbols(nm);
            while (si.hasNext()) {
                Symbol s = si.next();
                w.println("symbol " + nm + " @ " + s.getAddress());
                ReferenceIterator ri = rm.getReferencesTo(s.getAddress());
                int n = 0;
                while (ri.hasNext() && n < 60) {
                    Reference r = ri.next();
                    Address from = r.getFromAddress();
                    Function fn = fm.getFunctionContaining(from);
                    w.println("   called from " + from + "  in " + (fn == null ? "?" : fn.getName()
                        + " @ " + fn.getEntryPoint()));
                    n++;
                }
                if (n == 0) w.println("   (no references)");
            }
        }
        w.close();
    }
}
