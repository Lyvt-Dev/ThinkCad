#!/usr/bin/env bash
set -euo pipefail
# Run inside the SAME MSYS2 installation used to populate the installer.
# Source packages contain the matching upstream inputs, PKGBUILDs and patches.
mkdir -p out/toolchain-sources out/installer
pacman -Q | awk '$1 ~ /^mingw-w64-ucrt-x86_64-/ {print}' > out/toolchain-sources/PACKAGES.txt
while read -r package version; do
    desc="/var/lib/pacman/local/$package-$version/desc"
    base=$(sed -n '/^%BASE%$/{n;p;}' "$desc")
    if [[ -z "$base" ]]; then echo "Missing package base: $package" >&2; exit 1; fi
    version_without_epoch=${version#*:}
    archive="$base-$version_without_epoch.src.tar.zst"
    if [[ -f "out/toolchain-sources/$archive" ]]; then continue; fi
    curl --fail --location --retry 3 "https://repo.msys2.org/mingw/sources/$archive" -o "out/toolchain-sources/$archive"
    # Reject damaged archives. HTTPS remains verified; no insecure curl options.
    bsdtar -tf "out/toolchain-sources/$archive" > /dev/null
    sha256sum "out/toolchain-sources/$archive" >> out/toolchain-sources/SHA256SUMS.txt
done < out/toolchain-sources/PACKAGES.txt
tar -cf out/installer/ThinkCad-Toolchain-Sources.tar -C out toolchain-sources
