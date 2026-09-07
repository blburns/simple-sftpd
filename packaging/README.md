# Packaging

CPack in the top-level `CMakeLists.txt` is the supported package path (`make package`). Files here are maintainer scripts, installer sources, and assets that CPack uses. Docker is not part of this packaging path.

## Installed layout (must match production templates)

| Platform | Binary | Config | Data | Logs | FTP root |
|----------|--------|--------|------|------|----------|
| Linux (`CMAKE_INSTALL_PREFIX=/usr`) | `/usr/bin/simple-sftpd` | `/etc/simple-sftpd/simple-sftpd.conf` | `/var/lib/simple-sftpd` | `/var/log/simple-sftpd` | `/var/ftp` |
| macOS | `/usr/local/bin/simple-sftpd` | `/etc/simple-sftpd/simple-sftpd.conf` | `/var/lib/simple-sftpd` | `/var/log/simple-sftpd` | `/var/ftp` |
| Windows | `%PROGRAMFILES%\simple-sftpd\simple-sftpd.exe` | `%PROGRAMDATA%\simple-sftpd\simple-sftpd.conf` | `%PROGRAMDATA%\simple-sftpd` | `%PROGRAMDATA%\simple-sftpd\logs` | `%PROGRAMDATA%\simple-sftpd\ftp` |

Linux units start `--config` then `--foreground` at those paths so the flag wins over `foreground = false` in the production templates. Packages install templates and examples under `/etc/simple-sftpd/`, documentation under `/usr/share/doc/simple-sftpd/` (Linux) or `/usr/local/share/doc/simple-sftpd/` (macOS), and create the data/log/FTP directories and the `simple-sftpd` service user. DEB/RPM do not enable or start the daemon.

Package names follow `{name}-{version}-{platform}-{arch}` (no product-line infix), matching simple-ldapd.

## Building

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
make package          # platform packages into dist/
make package-source   # source tar.gz / zip
make package-all      # binary + source
```

On macOS, `make package` / `make package-pkg` runs CPack then `packaging/macos/pkg/rebuild-from-cpack.sh` so Installer.app does not see CPack’s leaked `Contents/` payload.

## Directory structure

```
packaging/
├── macos/pkg/rebuild-from-cpack.sh
├── macos/pkg/scripts/postinstall
├── linux/deb/{postinst,prerm,postrm}
├── linux/rpm/{preinstall,postinstall,preuninstall,postuninstall}.sh
├── windows/{nsis,msi}/
├── assets/{welcome,readme,conclusion}.html
└── LICENSE.txt
```

## Maintainer scripts

- **Linux DEB/RPM:** non-interactive; create the service user and directories; `daemon-reload`; do not prompt for a license; do not start the service.
- **macOS PKG:** create the service user, copy `templates/production.conf` if no config exists, `launchctl load` the LaunchDaemon.
- **REST/APPE + MODE Z** is a protocol policy, not a packaging concern.

## Notes

- Minimum macOS is 12.0 (`CMAKE_OSX_DEPLOYMENT_TARGET`).
- `hostArchitectures` is `arm64,x86_64` after the PKG rebuild.
- Google Test is not installed into packages (`INSTALL_GTEST OFF`).
