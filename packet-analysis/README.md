# Packet Analysis / Análisis de Paquetes

[English](#english) | [Español](#español)

---

## English

### Runtime Packet Capture with Frida

To validate that the reconstructed server produces correct output, we capture packets from the **live official client** using [Frida](https://frida.re/) — a dynamic instrumentation toolkit. Frida hooks functions at runtime, letting us intercept packets *after* RC4 decryption but *before* the game processes them.

### Workflow

1. **Hook the decryption output** — Identify the function that receives decrypted packet data (found via Ghidra static analysis). Attach a Frida interceptor to log every call.

2. **Capture sessions** — Play the game normally while Frida logs all packets with timestamps, sizes, and hex dumps to a text file.

3. **Parse and compare** — A Python script converts the raw capture logs into structured JSON. We then diff our server's output against the official capture to find serialization mismatches.

This workflow was critical for debugging issues like wrong field sizes (int16 vs int32), missing fields, and incorrect serialization order.

### Files

| File | Description |
|---|---|
| [`frida_capture.js`](frida_capture.js) | Simplified Frida hook script (technique demonstration) |
| [`parse_packets.py`](parse_packets.py) | Log-to-JSON parser for captured packet data |

---

## Español

### Captura de Paquetes en Tiempo Real con Frida

Para validar que el servidor reconstruido produce output correcto, capturamos paquetes del **cliente oficial en vivo** usando [Frida](https://frida.re/). Frida hookea funciones en tiempo de ejecución, permitiendo interceptar paquetes *después* del descifrado RC4 pero *antes* de que el juego los procese.

### Flujo de Trabajo

1. **Hookear la salida de descifrado** — Identificar la función que recibe datos de paquetes descifrados (encontrada por análisis estático en Ghidra). Adjuntar un interceptor Frida para loguear cada llamada.

2. **Capturar sesiones** — Jugar normalmente mientras Frida loguea todos los paquetes con timestamps, tamaños y hex dumps a un archivo de texto.

3. **Parsear y comparar** — Un script Python convierte los logs crudos a JSON estructurado. Luego se compara el output de nuestro servidor contra la captura oficial para encontrar diferencias de serialización.

### Archivos

| Archivo | Descripción |
|---|---|
| [`frida_capture.js`](frida_capture.js) | Script Frida simplificado (demostración de técnica) |
| [`parse_packets.py`](parse_packets.py) | Parser de logs a JSON para datos de paquetes capturados |
