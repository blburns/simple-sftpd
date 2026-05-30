# BSD make wrapper for simple-sftpd
# FreeBSD (and some other systems) ship BSD make as "make". This project requires GNU Make.
# Linux/macOS GNU make reads GNUmakefile directly and ignores this stub.

GMMAKE ?= gmake
PROJECT_NAME = simple-sftpd
VERSION = 0.3.0

# Common entry points
all build clean install uninstall test package deps dev-deps help help-all \
dev-build dev-test format lint package-source package-all package-deb package-rpm \
package-dmg package-pkg static-build static-test static-package static-zip static-all:
	@if ! command -v $(GMMAKE) >/dev/null 2>&1; then \
		echo "GNU Make is required."; \
		echo "  FreeBSD: pkg install gmake"; \
		echo "  Then run: gmake  (or make via this wrapper)"; \
		exit 1; \
	fi
	@$(GMMAKE) $@

# Catch-all for other targets (service-install, rebuild, etc.)
%:
	@if ! command -v $(GMMAKE) >/dev/null 2>&1; then \
		echo "GNU Make is required."; \
		echo "  FreeBSD: pkg install gmake"; \
		exit 1; \
	fi
	@$(GMMAKE) $@
