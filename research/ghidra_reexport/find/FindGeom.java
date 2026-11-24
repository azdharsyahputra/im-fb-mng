import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.address.*;
import ghidra.program.model.symbol.*;
import java.io.PrintWriter;

// FindGeom — static locator over the UNPACKED cm2008_full DB (read-only; no game run).
// (1) stores to [reg+0xC0]/[reg+0xD0] (MOV/FST/FISTP) to find the geometry writer, range-flagged.
// (2) thread-creation call sites (_beginthreadex/CreateThread) -> worker entry.
// Usage: analyzeHeadless ... -postScript FindGeom.java OUT.txt
public class FindGeom extends GhidraScript {
    public void run() throws Exception {
        String[] a = getScriptArgs();
        PrintWriter w = new PrintWriter(a[0]);
        FunctionManager fm = currentProgram.getFunctionManager();

        w.println("### GEOMETRY STORES to +0xC0 / +0xD0 (op0 memory dest) ###");
        int sim = 0, other = 0;
        InstructionIterator it = currentProgram.getListing().getInstructions(true);
        while (it.hasNext() && (sim + other) < 400) {
            Instruction insn = it.next();
            if (insn.getNumOperands() < 1) continue;
            String mnem = insn.getMnemonicString().toUpperCase();
            if (!(mnem.startsWith("MOV") || mnem.equals("FSTP") || mnem.equals("FST")
                  || mnem.equals("FISTP") || mnem.equals("FIST"))) continue;
            String dst = insn.getDefaultOperandRepresentation(0);
            if (dst == null) continue;
            String dl = dst.toLowerCase();
            if (!(dl.contains("0xc0]") || dl.contains("0xd0]"))) continue;
            Address at = insn.getAddress();
            long off = at.getOffset();
            boolean inSim = (off >= 0x9b0000L && off <= 0x9fffffL);
            Function fn = fm.getFunctionContaining(at);
            String src = insn.getNumOperands() > 1 ? insn.getDefaultOperandRepresentation(1) : "";
            w.println((inSim ? "[SIM] " : "[   ] ") + at.toString() + "  "
                + (fn == null ? "?" : fn.getName()) + "  " + mnem + "  " + dst + "  ; src=" + src);
            if (inSim) sim++; else other++;
        }
        w.println("-- store hits: sim=" + sim + " other=" + other);

        w.println();
        w.println("### THREAD-CREATION call sites ###");
        SymbolTable stab = currentProgram.getSymbolTable();
        ReferenceManager rm = currentProgram.getReferenceManager();
        String[] names = { "_beginthreadex", "beginthreadex", "CreateThread", "_beginthread" };
        for (int k = 0; k < names.length; k++) {
            SymbolIterator si = stab.getSymbols(names[k]);
            while (si.hasNext()) {
                Symbol s = si.next();
                w.println("symbol " + names[k] + " @ " + s.getAddress().toString());
                ReferenceIterator ri = rm.getReferencesTo(s.getAddress());
                int n = 0;
                while (ri.hasNext() && n < 60) {
                    Reference r = ri.next();
                    Address from = r.getFromAddress();
                    Function fn = fm.getFunctionContaining(from);
                    w.println("   from " + from.toString() + "  in "
                        + (fn == null ? "?" : fn.getName() + " @ " + fn.getEntryPoint().toString()));
                    n++;
                }
            }
        }
        w.close();
    }
}
