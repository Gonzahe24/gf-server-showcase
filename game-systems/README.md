# Game Systems / Sistemas de Juego

[English](#english) | [Español](#español)

---

## English

### Reverse-Engineered Game Logic

These files demonstrate core game systems reconstructed from Ghidra decompilation of the original server binaries. Every formula, threshold, and constant was extracted by tracing the original assembly — not guessed or approximated.

### Files

| File | Description |
|---|---|
| [`combat_formula.cpp`](combat_formula.cpp) | Damage calculation with layered modifiers (stats, weapon, buffs, conversion) |
| [`buff_system.cpp`](buff_system.cpp) | Buff application, stacking/restack logic, timed tick system |

### Key Insights from RE

- **Damage is layered**: Base stat damage -> weapon damage -> equipment bonus -> buff multipliers -> HP/MP conversion, each as a separate additive or multiplicative step.
- **Buff stacking uses level comparison**: Same level resets duration; higher level on caster rejects the new buff; lower-level existing buffs get replaced.
- **Durations are in tenths of seconds**: A buff lasting "280" ticks = 28 real seconds. The tick function runs once per real second, decrementing by 10.
- **Command pattern for buffs**: Each buff executes stat-modification commands on apply, and rolls them back on removal — ensuring clean state even if the server crashes mid-buff.

---

## Español

### Lógica de Juego por Ingeniería Inversa

Estos archivos demuestran sistemas centrales del juego reconstruidos desde la descompilación en Ghidra de los binarios originales del servidor. Cada fórmula, umbral y constante fue extraída trazando el ensamblador original — nada fue adivinado ni aproximado.

### Archivos

| Archivo | Descripción |
|---|---|
| [`combat_formula.cpp`](combat_formula.cpp) | Cálculo de daño con modificadores en capas (stats, arma, buffs, conversión) |
| [`buff_system.cpp`](buff_system.cpp) | Aplicación de buffs, lógica de stacking/restack, sistema de ticks temporizados |

### Descubrimientos Clave de la RE

- **El daño es por capas**: Daño base de stats -> daño de arma -> bonus de equipo -> multiplicadores de buff -> conversión HP/MP, cada uno como paso separado aditivo o multiplicativo.
- **El stacking de buffs usa comparación de nivel**: Mismo nivel reinicia duración; nivel más alto en el caster rechaza el nuevo buff; buffs existentes de menor nivel son reemplazados.
- **Las duraciones son en décimas de segundo**: Un buff de "280" ticks = 28 segundos reales. La función tick corre una vez por segundo real, decrementando en 10.
- **Patrón command para buffs**: Cada buff ejecuta comandos de modificación de stats al aplicarse, y los revierte al removerse — garantizando estado limpio incluso si el servidor crashea a mitad de un buff.
