import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.address.*;
import java.io.PrintWriter;

public class Decomp extends GhidraScript {
    public void run() throws Exception {
        String[] a = getScriptArgs();
        PrintWriter w = new PrintWriter(a[0]);
        DecompInterface di = new DecompInterface();
        di.openProgram(currentProgram);
        FunctionManager fm = currentProgram.getFunctionManager();
        AddressFactory af = currentProgram.getAddressFactory();
        for (int i = 1; i < a.length; i++) {
            Address addr = af.getAddress(a[i]);
            Function fn = (addr == null) ? null : fm.getFunctionContaining(addr);
            if (fn == null) { w.println("=== " + a[i] + ": NO FUNCTION ==="); continue; }
            DecompileResults res = di.decompileFunction(fn, 90, monitor);
            w.println("=== " + a[i] + " " + fn.getName() + " ===");
            if (res != null && res.decompileCompleted()) w.println(res.getDecompiledFunction().getC());
            else w.println("DECOMP FAIL: " + (res==null?"null":res.getErrorMessage()));
        }
        w.close();
    }
}
