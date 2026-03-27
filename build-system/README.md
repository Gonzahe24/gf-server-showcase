# Build System

Top-level CMake configuration that builds the entire MMORPG server infrastructure.

## Structure

- **3 shared libraries**: `lapis` (framework), `gamedata` (data files), `protocol` (970 network commands)
- **6 server executables**: Login, Ticket, Gateway, World, Mission, Zone
- **Optional test suite**

## Build

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

Requires: CMake 3.16+, C++17 compiler, PostgreSQL (libpq), OpenSSL, pthreads.

Output binaries are placed in `build/bin/`, static libraries in `build/lib/`.
