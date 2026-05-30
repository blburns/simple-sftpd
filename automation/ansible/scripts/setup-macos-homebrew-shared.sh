#!/bin/bash
# One-time setup: share a single Homebrew install across multiple macOS users.
# Run on the build VM as root (or via sudo). Does NOT reassign ownership to one user.
#
# Example (Intel Mac, Homebrew under /usr/local):
#   sudo ./setup-macos-homebrew-shared.sh admin build
#
# Example (Apple Silicon):
#   sudo ./setup-macos-homebrew-shared.sh admin build
#
# After setup, each listed user can run brew without chown'ing the tree to build only.

set -euo pipefail

GROUP_NAME="${HOMEBREW_GROUP:-homebrew}"

usage() {
    cat <<EOF
Usage: sudo $0 USER [USER ...]

Configure group-writable Homebrew so multiple accounts can install/update packages.

Environment:
  HOMEBREW_GROUP   Group name (default: homebrew)

Example:
  sudo $0 admin build
EOF
}

if [[ "${EUID:-$(id -u)}" -ne 0 ]]; then
    echo "Run as root: sudo $0 USER [USER ...]" >&2
    exit 1
fi

if [[ $# -lt 1 ]]; then
    usage
    exit 1
fi

if [[ -d /opt/homebrew ]]; then
    BREW_PREFIX="/opt/homebrew"
elif [[ -d /usr/local/Homebrew ]]; then
    BREW_PREFIX="/usr/local"
else
    echo "Homebrew prefix not found (/opt/homebrew or /usr/local/Homebrew)" >&2
    exit 1
fi

echo "Using Homebrew prefix: $BREW_PREFIX"
echo "Shared group: $GROUP_NAME"

if ! dscl . -read "/Groups/${GROUP_NAME}" &>/dev/null; then
    echo "Creating group: $GROUP_NAME"
    dseditgroup -o create "$GROUP_NAME"
fi

for user in "$@"; do
    if ! id "$user" &>/dev/null; then
        echo "User not found: $user" >&2
        exit 1
    fi
    echo "Adding $user to group $GROUP_NAME"
    dseditgroup -o edit -a "$user" -t user "$GROUP_NAME" || true
done

# Directories that must be group-writable for shared Homebrew use
DIRS=(
    "$BREW_PREFIX"
    "${BREW_PREFIX}/Homebrew"
    "${BREW_PREFIX}/Cellar"
    "${BREW_PREFIX}/Caskroom"
    "${BREW_PREFIX}/bin"
    "${BREW_PREFIX}/share"
    "${BREW_PREFIX}/lib"
    "${BREW_PREFIX}/include"
    "${BREW_PREFIX}/opt"
    "${BREW_PREFIX}/var/homebrew"
    "${BREW_PREFIX}/etc/bash_completion.d"
    "${BREW_PREFIX}/man"
    "${BREW_PREFIX}/sbin"
)

apply_group_perms() {
    local dir="$1"
    if [[ ! -e "$dir" ]]; then
        return 0
    fi
    echo "  $dir"
    chgrp -R "$GROUP_NAME" "$dir"
    chmod -R g+rwX "$dir"
    find "$dir" -type d -exec chmod g+s {} \;
}

echo "Applying group permissions..."
for dir in "${DIRS[@]}"; do
    apply_group_perms "$dir"
done

# Intel layout: repository may live under /usr/local/Homebrew
if [[ -d /usr/local/Homebrew && "$BREW_PREFIX" != "/usr/local/Homebrew" ]]; then
    apply_group_perms "/usr/local/Homebrew"
fi

cat <<EOF

Done.

Next steps for each user ($*):
  1. Log out and back in (or \`newgrp $GROUP_NAME\`) so the group membership applies
  2. Ensure PATH includes Homebrew:
       eval "\$(/opt/homebrew/bin/brew shellenv)"   # Apple Silicon
       eval "\$(/usr/local/bin/brew shellenv)"        # Intel
  3. Test: brew doctor && brew update

All users in group '$GROUP_NAME' can use the same Homebrew install.
EOF
