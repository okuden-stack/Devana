# platform/

Cross-platform build infrastructure for Devana. Handles building a Linux binary from macOS using either a local Docker container or a remote home server over SSH.

```
platform/
├── Dockerfile.arch          ← Arch Linux build image
├── docker-entrypoint.sh     ← build script that runs inside the container
├── docker-build.sh          ← Mac-side wrapper for Docker builds
└── sync-server.sh           ← rsync + SSH remote build + binary pull
```

---

## Prerequisites

### Docker builds (offline)

Install Docker Desktop:
```bash
brew install --cask docker
```
Then launch Docker.app. You don't need ZeroMQ or any Linux toolchain installed locally.

### Server builds (on network)

Your home server needs:
```bash
# On the Arch server
sudo pacman -S cmake ninja gcc qt6-base zeromq cppzmq pkgconf python git
```

Configure your SSH connection in `tools/scripts/setup/env.local.zsh`:
```bash
cp tools/scripts/setup/env.local.zsh.example \
   tools/scripts/setup/env.local.zsh
```

Then fill in:
```bash
export DEVANA_DEPLOY_HOST="homeserver"   # hostname or IP
export DEVANA_DEPLOY_USER="aleks"
export DEVANA_DEPLOY_PATH="~/projects/devana"
```

---

## Docker builds

Produces `dist/linux/Devana` — a native Linux/x86_64 binary built in an Arch container.
Works completely offline once the image is built.

```bash
# Standard build (ZMQ harness disabled)
./platform/docker-build.sh

# Build with ZMQ test harness enabled
./platform/docker-build.sh --zmq

# Force rebuild the Docker image (e.g. after Dockerfile changes)
./platform/docker-build.sh --rebuild

# Drop into the container shell for debugging
./platform/docker-build.sh --shell
```

Or use the shell aliases (from project root with direnv active):

| Alias  | Command                              |
|--------|--------------------------------------|
| `dlb`  | `docker-build.sh`                    |
| `dlbz` | `docker-build.sh --zmq`              |
| `dlbr` | `docker-build.sh --rebuild`          |
| `dlbs` | `docker-build.sh --shell`            |

**First run** downloads the Arch base image and installs packages (~3 min, ~800 MB).
Every subsequent run reuses the cached image and just builds (~30 sec).

---

## Server builds

Rsyncs source to your home server, triggers a build there, and pulls the binary back.
Requires `DEVANA_DEPLOY_HOST` to be set in `env.local.zsh`.

```bash
# Full cycle: sync source → remote build → pull binary
./platform/sync-server.sh

# Sync source only (no build)
./platform/sync-server.sh --source-only

# Trigger remote build + pull (source already on server)
./platform/sync-server.sh --build-only

# Pull the last built binary without rebuilding
./platform/sync-server.sh --pull

# Push a locally built binary (from docker-build) to the server
./platform/sync-server.sh --deploy-only

# Check remote build status and last log lines
./platform/sync-server.sh --status
```

Shell aliases:

| Alias   | Command                          |
|---------|----------------------------------|
| `dls`   | `sync-server.sh` (full cycle)    |
| `dlss`  | `sync-server.sh --source-only`   |
| `dlsp`  | `sync-server.sh --pull`          |
| `dlst`  | `sync-server.sh --status`        |

If the server is unreachable the script exits immediately with a clear message
rather than hanging — safe to run without thinking about whether you're on network.

---

## Typical workflows

### Travelling (offline)

```bash
# Develop on Mac as normal, then build for Linux
dlb

# Binary is at:
dist/linux/Devana
```

### At home / on network

```bash
# Sync and build on the server, pull binary back
dls

# Or check what's already built on the server
dlst
```

### Offline dev → sync when back on network

```bash
# Build locally while offline
dlb

# Later, when home, push the local binary to the server
./platform/sync-server.sh --deploy-only

# Or do a full remote rebuild to confirm clean server build
dls
```

### Debugging a build failure

```bash
# Drop into the container shell and run the build manually
dlbs

# Inside the container:
cp -r /src/. /staging && cd /staging
python3 tools/devana build --regen
cmake -B /build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build /build
```

---

## Output

All Linux binaries land in `dist/linux/` which is gitignored:

```
dist/
└── linux/
    └── Devana    ← ELF 64-bit LSB executable, x86-64
```

---

## Adding other platforms

Add a new `Dockerfile.<target>` and a corresponding build wrapper script following
the same pattern. The `platform/` directory is intended to hold one subdirectory
or Dockerfile per target as the project grows.
