# Phase 1 Verification Guide

Use this guide to verify each Phase 1 item from [PRODUCTION_READINESS_CHECKLIST.md](PRODUCTION_READINESS_CHECKLIST.md). Run on the target OS or in a VM.

## 1. systemd (Linux)

**Prereqs:** Linux with systemd, simple-sftpd built and installed (or use `tools/install-service.sh`).

```bash
# Install service (as root) – creates unit and starts service
sudo ./tools/install-service.sh

# Verify
sudo systemctl status simple-sftpd    # should be active (running)
sudo systemctl stop simple-sftpd
sudo systemctl start simple-sftpd
sudo systemctl restart simple-sftpd
journalctl -u simple-sftpd -n 50    # logs
cat /var/run/simple-sftpd.pid       # or PID file path from your config
```

**Success:** Service starts, stops, restarts; logs and PID file present.

---

## 2. launchd (macOS)

**Prereqs:** simple-sftpd installed (e.g. to `/usr/local`), config at `/usr/local/etc/simple-sftpd/simple-sftpd.conf`.

```bash
# Copy plist (adjust paths if your install is different)
sudo cp deployment/launchd/com.simple-sftpd.simple-sftpd.plist /Library/LaunchDaemons/
# Edit if needed: ProgramArguments, WorkingDirectory, StandardOutPath, StandardErrorPath

# Create dirs and set ownership
sudo mkdir -p /usr/local/var/simple-sftpd /usr/local/var/log/simple-sftpd
sudo chown simple-sftpd:simple-sftpd /usr/local/var/simple-sftpd /usr/local/var/log/simple-sftpd

# Load and start
sudo launchctl load /Library/LaunchDaemons/com.simple-sftpd.simple-sftpd.plist
sudo launchctl list | grep simple-sftpd   # should show PID

# Verify
sudo launchctl stop com.simple-sftpd.simple-sftpd
sudo launchctl start com.simple-sftpd.simple-sftpd
tail -f /usr/local/var/log/simple-sftpd/simple-sftpd.log
```

**Success:** Daemon loads, start/stop works; log file is written.

---

## 3. Windows service

**Prereqs:** NSSM installed, simple-sftpd installed (e.g. `C:\Program Files\simple-sftpd\`), config at path used in script.

```cmd
REM Run as Administrator
cd C:\Path\To\simple-sftpd
etc\windows\install-service.bat

REM Verify
sc query simple-sftpd
net start simple-sftpd
net stop simple-sftpd
REM Check logs in %LOG_DIR% (e.g. C:\Program Files\simple-sftpd\logs)
```

**Success:** Service installs, starts, stops; log files created.

---

## 4. Install scripts

- **Linux:** Run `tools/install-service.sh` on a systemd system (as root). Confirm it creates user, dirs, config, unit file, and starts the service (see §1).
- **Windows:** Run `etc/windows/install-service.bat` as Administrator with NSSM and simple-sftpd in place (see §3).

**Success:** No errors; service is installed and controllable.

---

## 5. Linux build

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
./simple-sftpd --test-config ../config/simple/simple-sftpd.conf
./simple-sftpd --config ../config/simple/simple-sftpd.conf &
# From another terminal: ftp localhost, login (e.g. test/test), dir, get a file, quit
kill %1
```

**Success:** Build completes; config test passes; one transfer works.

---

## 6. Windows build

On Windows with Visual Studio or MinGW:

```cmd
mkdir build && cd build
cmake .. -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release
simple-sftpd.exe --test-config ..\config\simple\simple-sftpd.conf
```

**Success:** Build completes; `--test-config` exits 0.

---

## 7. Docker

```bash
docker build -t simple-sftpd:test -f Dockerfile .
docker run -d -p 2121:21 --name sftpd-test simple-sftpd:test
docker exec sftpd-test nc -z localhost 21 && echo "FTP port open"
# From host (if ftp client installed): ftp localhost 2121, login test/test, dir
docker stop sftpd-test && docker rm sftpd-test
```

**Success:** Image builds; container runs; port 21 is open; you can connect and list.

---

## 8. Packaging

Native CPack is the packaging path (`make package` / `make package-pkg`). Docker is not used.

**macOS (PKG):**

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
make package-pkg
# artifact: dist/simple-sftpd-VERSION-macos-intel.pkg (or macos-apple)
```

`packaging/macos/pkg/rebuild-from-cpack.sh` runs after CPack so Installer.app does not see a leaked `Contents/` payload.

**Linux (DEB):**

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DCPACK_GENERATOR=DEB
make package
sudo dpkg -i simple-sftpd-*.deb
simple-sftpd --version
# Packages do not start the daemon; enable/start yourself after editing TLS and users.
```

**Success:** Package builds; binary is present; service is installed but not started.

---

*After completing these, check off the corresponding items in [PRODUCTION_READINESS_CHECKLIST.md](PRODUCTION_READINESS_CHECKLIST.md).*

---

## Phase 1 results (2026-08-30, macOS)

Recorded on this development machine. Check only what actually ran.

| Item | Result |
|------|--------|
| Config test | **Pass** — `simple-sftpd test --config` on `config/simple/` INI, JSON, and YAML |
| launchd plist | **Pass** — `plutil -lint` on `etc/launchd/` and `deployment/launchd/` plists |
| launchd start/stop | **Pass (user LaunchAgent)** — bootstrap `gui/$UID`, 220 welcome + QUIT 221 on 127.0.0.1:21213, then bootout. System-wide `/Library/LaunchDaemons` install was **not** run |
| Foreground smoke | **Pass** — same 220/221 path (also via launchd agent) |
| Docker | **Not used** — packaging is native CPack (simple-ldapd model) |
| Packaging | **Pass (macOS)** — CPack + `rebuild-from-cpack.sh` → `simple-sftpd-0.4.0-macos-intel.pkg` (no `-production-` infix; no gtest payload) |
| Linux systemd / DEB / RPM | **Not run here** — no Linux host; treat as CI-aspirational |
| Windows service / build | **Not run** — no Windows host |

Windows and Linux remain documented / CI-aspirational.
