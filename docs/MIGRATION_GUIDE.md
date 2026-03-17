# Migration and Upgrade Guide

Guidance for upgrading between versions of simple-sftpd and migrating config or data.

## Before upgrading

1. **Backup** configuration files, `users.json`, and any custom SSL certificates or keys.
2. **Read release notes** for the target version for breaking changes and new options.
3. **Test in staging** with a copy of production config and data.

## General upgrade steps

1. Stop the service gracefully (e.g. `systemctl stop simple-sftpd` or equivalent).
2. Replace the binary (or install the new package).
3. If the default config schema changed, merge or replace config; use `--test-config` to validate.
4. Start the service and run smoke tests (login, list, upload, download).
5. Check logs for warnings or deprecations.

## Config file format changes

- **INI → JSON/YAML:** The server supports INI, JSON, and YAML. You can migrate by converting your INI to JSON or YAML using the examples in `config/` and then switching the path in your service/script.
- **New options:** New versions may add optional config keys. Old configs usually keep working; add new options only if you need the feature.
- **Renamed or removed options:** Release notes will list them. Search your config for old names and update or remove.

## User storage (users.json)

- The path to `users.json` is often under `/etc/simple-sftpd/` or set in config. After upgrade, ensure the path is unchanged or update config and service to the new path.
- If the user schema (fields or format) changes, release notes will describe migration. Backup `users.json` before upgrading.

## Service and packaging

- **systemd:** If the unit file changed, compare with `deployment/systemd/simple-sftpd.service`. Merge any needed changes (e.g. paths, Type=simple), then `systemctl daemon-reload` and restart.
- **launchd:** If the plist changed, compare with `deployment/launchd/com.simple-sftpd.simple-sftpd.plist`, update paths, then `launchctl unload` / `launchctl load`.
- **Packages (DEB/RPM):** Upgrading with `dpkg -i` or `rpm -U` typically preserves config under `/etc`. Confirm config and data paths after upgrade.

## Rollback

If you need to roll back:

1. Stop the service.
2. Restore the previous binary or reinstall the previous package.
3. Restore config and `users.json` from backup if they were changed.
4. Start the service and verify.

## Version-specific notes

- **0.1.x → 0.2.x:** SSL/TLS, PAM, chroot, and other security options were added. Existing configs continue to work; enable new options as needed (see [PRODUCTION_DEPLOYMENT.md](PRODUCTION_DEPLOYMENT.md)).
- **Future:** Check the release notes and this guide for each major/minor version.

---

*See [TROUBLESHOOTING.md](TROUBLESHOOTING.md) and [PRODUCTION_DEPLOYMENT.md](PRODUCTION_DEPLOYMENT.md).*
