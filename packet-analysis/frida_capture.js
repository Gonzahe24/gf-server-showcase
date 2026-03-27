/**
 * frida_capture.js - Simplified Frida packet capture hook
 *
 * Educational/portfolio example showing the dynamic instrumentation
 * technique used to capture decrypted network packets at runtime.
 * Real addresses and game-specific details have been removed.
 *
 * Usage: frida -p <PID> -l frida_capture.js
 */

// Address of the post-decryption receive handler (found via Ghidra)
const ADDR_RECV_HANDLER = ptr("0xDEADBEEF");

// Address of a helper that returns the connection label string
const ADDR_GET_CONN_NAME = ptr("0xDEADBEE0");

const getConnName = new NativeFunction(ADDR_GET_CONN_NAME, 'pointer', ['pointer']);

let packetCount = 0;

Interceptor.attach(ADDR_RECV_HANDLER, {
    /**
     * onEnter fires when the hooked function is called.
     * args[0] = this pointer (connection object)
     * args[1] = pointer to decrypted packet buffer
     * args[2] = packet length
     */
    onEnter(args) {
        const connPtr  = args[0];
        const dataPtr  = args[1];
        const dataLen  = args[2].toInt32();

        if (dataLen < 4) return;  // too short to have a header

        packetCount++;

        // Read the 2-byte command ID from the packet header
        const ncid = dataPtr.readU16();

        // Get a human-readable connection label
        const namePtr = getConnName(connPtr);
        const connName = namePtr.readCString() || "Unknown";

        // Format timestamp
        const now = new Date();
        const ts = [now.getHours(), now.getMinutes(), now.getSeconds()]
            .map(n => String(n).padStart(2, '0')).join(':');
        const ms = String(now.getMilliseconds()).padStart(3, '0');

        // Log header
        const hexNcid = '0x' + ncid.toString(16).toUpperCase().padStart(4, '0');
        send(`>>> [${ts}.${ms}] RECV #${packetCount} | ${connName} | NCID ${hexNcid} | ${dataLen} bytes`);

        // Hex dump (first 64 bytes max)
        const dumpLen = Math.min(dataLen, 64);
        const bytes = dataPtr.readByteArray(dumpLen);
        send(hexDump(bytes));
    }
});

function hexDump(buf) {
    const arr = new Uint8Array(buf);
    let lines = [];
    for (let i = 0; i < arr.length; i += 16) {
        const hex = Array.from(arr.slice(i, i + 16))
            .map(b => b.toString(16).padStart(2, '0'))
            .join(' ');
        lines.push(`  ${i.toString(16).padStart(4, '0')}: ${hex}`);
    }
    return lines.join('\n');
}

console.log(`[*] Packet capture hook installed at ${ADDR_RECV_HANDLER}`);
