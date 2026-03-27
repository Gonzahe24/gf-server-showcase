# Reverse Engineering Process / Proceso de Ingeniería Inversa

[English](#english) | [Español](#español)

---

## English

### From Binary to C++17: Reconstructing an MMORPG Server

The original game server was a set of x86 ELF binaries compiled with GCC 4.4.5 on Debian Linux. No source code was available. Every struct, vtable, protocol format, and state machine had to be recovered through static analysis in **Ghidra**.

### Workflow

1. **Load binary into Ghidra** — Import the ELF, let auto-analysis run (takes 30-60 min for a 15MB binary). Ghidra recovers function boundaries, cross-references, and partial type info from DWARF symbols.

2. **Identify vtables** — Search for arrays of function pointers in `.rodata`. Each vtable corresponds to a C++ class. Cross-reference the first entry (usually the destructor) to find the constructor, which reveals member initialization and class hierarchy.

3. **Trace function calls** — Starting from known entry points (e.g., `main`, network dispatch loops), follow the call graph to map out how packets arrive, get dispatched, and trigger game logic.

4. **Reconstruct structs** — Ghidra shows field accesses as offsets (`*(this + 0x1c)`). By collecting all accesses across every function that uses a struct, you can reconstruct the full layout with correct types and sizes.

5. **Write modern C++17** — Translate the decompiled pseudocode into clean, idiomatic C++ with proper classes, namespaces, RAII, and type safety.

### Before/After Example

**Ghidra decompiled output** (simplified):
```c
void FUN_DEADBEEF(int *param_1, int *param_2) {
    int iVar1 = *(param_2 + 0x18);    // entity ID
    int *piVar2 = (int *)FUN_DEADBF00(*(param_1 + 0x40), iVar1);
    if (piVar2 != NULL) {
        *(piVar2 + 0x2c) = *(param_2 + 0x1c);  // update HP
        *(piVar2 + 0x30) = *(param_2 + 0x20);  // update MP
        FUN_DEADBF80(param_1, piVar2, 0x134);   // broadcast update
    }
}
```

**Reconstructed C++17**:
```cpp
void CZoneInterface::HandleVitalUpdate(CNetStream& stream) {
    uint32_t entityId = stream.Read<uint32_t>();
    auto* entity = m_entityMap.Find(entityId);
    if (entity) {
        entity->SetHP(stream.Read<int32_t>());
        entity->SetMP(stream.Read<int32_t>());
        BroadcastToNearby(entity, NCID_VITAL_UPDATE);
    }
}
```

### Headless Scripting

For batch decompilation of all server binaries, Ghidra's headless analyzer was used:

```bash
analyzeHeadless /project ServerProject -import ./ZoneServer \
    -scriptPath ./ghidra_scripts \
    -postScript ExportDecompilation.java \
    -processor x86:LE:32:default
```

This exports full decompilation to text files, enabling `grep`-based analysis across millions of lines of pseudocode — essential when tracking down how a specific struct field is used across 50,000+ functions.

---

## Español

### De Binario a C++17: Reconstruyendo un Servidor MMORPG

El servidor original era un conjunto de binarios ELF x86 compilados con GCC 4.4.5 en Debian Linux. No había código fuente disponible. Cada struct, vtable, formato de protocolo y máquina de estados tuvo que ser recuperado mediante análisis estático en **Ghidra**.

### Flujo de Trabajo

1. **Cargar binario en Ghidra** — Importar el ELF, dejar correr el auto-análisis (30-60 min para un binario de 15MB). Ghidra recupera límites de funciones, referencias cruzadas e información parcial de tipos desde símbolos DWARF.

2. **Identificar vtables** — Buscar arrays de punteros a funciones en `.rodata`. Cada vtable corresponde a una clase C++. Seguir la referencia cruzada de la primera entrada (generalmente el destructor) para encontrar el constructor, que revela la inicialización de miembros y la jerarquía de clases.

3. **Trazar llamadas a funciones** — Partiendo de puntos de entrada conocidos (ej. `main`, loops de despacho de red), seguir el grafo de llamadas para mapear cómo llegan los paquetes, se despachan, y activan la lógica del juego.

4. **Reconstruir structs** — Ghidra muestra accesos a campos como offsets (`*(this + 0x1c)`). Recolectando todos los accesos en cada función que usa un struct, se puede reconstruir el layout completo con tipos y tamaños correctos.

5. **Escribir C++17 moderno** — Traducir el pseudocódigo descompilado a C++ limpio e idiomático con clases, namespaces, RAII y seguridad de tipos.

### Scripting Headless

Para la descompilación por lotes, se usó el analizador headless de Ghidra, exportando a archivos de texto que permiten análisis basado en `grep` sobre millones de líneas de pseudocódigo.
