# Automation Tools / Herramientas de Automatización

[English](#english) | [Español](#español)

---

## English

### Python Tooling for Development Workflow

The server is developed on Windows and cross-compiled for Linux on a remote build machine. Python scripts automate the build-deploy-test cycle using SSH/SFTP via the [Paramiko](https://www.paramiko.org/) library.

### Why Paramiko?

The Windows development environment requires programmatic SSH access to the Linux build server. Paramiko provides a pure-Python SSH2 implementation that works reliably across platforms without requiring external tools like PuTTY or WSL.

### Files

| File | Description |
|---|---|
| [`deploy_example.py`](deploy_example.py) | Simplified deployment script showing the SSH/SFTP pattern |

### Workflow

1. **SFTP Upload** — Push modified `.cpp`/`.h` files to the remote source tree
2. **Remote Build** — Execute `cmake` and `make` over SSH
3. **Deploy** — Copy the built binary to the server's runtime directory
4. **Verify** — Check the process is running and accepting connections

---

## Español

### Herramientas Python para el Flujo de Desarrollo

El servidor se desarrolla en Windows y se compila de forma cruzada para Linux en una máquina de build remota. Scripts Python automatizan el ciclo build-deploy-test usando SSH/SFTP mediante la librería [Paramiko](https://www.paramiko.org/).

### Por qué Paramiko?

El entorno de desarrollo en Windows requiere acceso SSH programático al servidor de build Linux. Paramiko provee una implementación pura en Python de SSH2 que funciona de forma confiable entre plataformas sin requerir herramientas externas como PuTTY o WSL.

### Archivos

| Archivo | Descripción |
|---|---|
| [`deploy_example.py`](deploy_example.py) | Script de deployment simplificado mostrando el patrón SSH/SFTP |

### Flujo de Trabajo

1. **Subida SFTP** — Enviar archivos `.cpp`/`.h` modificados al árbol de fuentes remoto
2. **Build Remoto** — Ejecutar `cmake` y `make` por SSH
3. **Deploy** — Copiar el binario compilado al directorio de ejecución del servidor
4. **Verificar** — Comprobar que el proceso está corriendo y aceptando conexiones
