#!/bin/bash
# Build on Ubuntu 22.04 (glibc 2.35); tools.json pins third-party build tools.
set -euo pipefail
source_dir=$(cd "$(dirname "$0")/../.." && pwd)
work=${THINKCAD_APPIMAGE_WORK:-$source_dir/out/appimage-work}
output=${THINKCAD_APPIMAGE_OUT:-$source_dir/out/appimage}
mkdir -p "$work" "$output"
python3 "$source_dir/packaging/appimage/download-tools.py" "$work/tools"
export APPIMAGE_EXTRACT_AND_RUN=1
export PATH="$work/tools:$PATH"
export QMAKE=${QMAKE:-$(command -v qmake6 || command -v qmake)}
version=$(sed -n 's/project(ThinkCad VERSION \([0-9.]*\).*/\1/p' "$source_dir/CMakeLists.txt")
cmake -S "$source_dir" -B "$work/build" -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr -DBUILD_TESTING=OFF
cmake --build "$work/build" --parallel 2
rm -rf "$work/AppDir"
DESTDIR="$work/AppDir" cmake --install "$work/build"
mkdir -p "$work/AppDir/usr/share/licenses/thinkcad"
cp "$source_dir/packaging/windows/LGPL-3.txt" "$work/AppDir/usr/share/licenses/thinkcad/"
cp "$source_dir/packaging/appimage/THIRD_PARTY.md" "$work/AppDir/usr/share/licenses/thinkcad/"
mkdir -p "$work/AppDir/usr/bin/toolchain/bin" "$work/AppDir/usr/bin/verifier"
tar -xf "$work/tools/zig-x86_64-linux-0.14.1.tar.xz" -C "$work/AppDir/usr/bin/toolchain" --strip-components=1
cat > "$work/AppDir/usr/bin/toolchain/bin/g++" <<'COMPILER'
#!/bin/sh
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
echo "Portable compiler: preparing sketch (the first build also prepares the C++ standard library)." >&2
exec "$root/zig" c++ -target x86_64-linux-musl "$@"
COMPILER
chmod +x "$work/AppDir/usr/bin/toolchain/bin/g++"
# Bundle the verifier and its corresponding license/trust roots, as on Windows.
gh_version=2.102.0
gh_archive="gh_${gh_version}_linux_amd64.tar.gz"
curl --fail --location --retry 3 "https://github.com/cli/cli/releases/download/v${gh_version}/${gh_archive}" -o "$work/$gh_archive"
curl --fail --location --retry 3 "https://github.com/cli/cli/releases/download/v${gh_version}/gh_${gh_version}_checksums.txt" -o "$work/gh-checksums.txt"
(cd "$work" && awk -v file="$gh_archive" '$2==file' gh-checksums.txt > gh-selected.sha256 && test -s gh-selected.sha256 && sha256sum --check gh-selected.sha256)
mkdir -p "$work/gh";tar -xf "$work/$gh_archive" -C "$work/gh" --strip-components=1
cp "$work/gh/bin/gh" "$work/AppDir/usr/bin/verifier/gh"
cp "$work/gh/LICENSE" "$work/AppDir/usr/bin/verifier/GITHUB_CLI_LICENSE.txt"
"$work/gh/bin/gh" attestation trusted-root > "$work/AppDir/usr/bin/verifier/trusted_root.jsonl"
# The Qt plugin deployer includes Qt dependencies/plugins; do not bundle host glibc.
# Explicitly stage platform plugins; EXTRA_QT_MODULES selects modules, not directories.
plugin_libraries=()
qt_plugins=$("$QMAKE" -query QT_INSTALL_PLUGINS)
for category in platforms multimedia tls imageformats wayland-shell-integration wayland-graphics-integration-client wayland-decoration-client; do
    mkdir -p "$work/AppDir/usr/plugins/$category"
    for plugin in "$qt_plugins/$category/"*.so; do
        [ -f "$plugin" ] || continue
        target="$work/AppDir/usr/plugins/$category/$(basename "$plugin")"
        cp "$plugin" "$target"
        plugin_libraries+=(--library "$target")
    done
done
python3 - "$source_dir/assets/logo.png" "$work/thinkcad.svg" <<'ICON'
import base64,pathlib,struct,sys
png=pathlib.Path(sys.argv[1]).read_bytes();width,height=struct.unpack('>II',png[16:24])
pathlib.Path(sys.argv[2]).write_text(f'<svg xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink" width="512" height="512" viewBox="0 0 {width} {height}"><image width="{width}" height="{height}" xlink:href="data:image/png;base64,{base64.b64encode(png).decode()}"/></svg>')
ICON
"$work/tools/linuxdeploy-x86_64.AppImage" --appdir "$work/AppDir" --executable "$work/AppDir/usr/bin/thinkcad" --desktop-file "$source_dir/packaging/linux/io.github.LyvtDev.ThinkCad.desktop" --icon-file "$work/thinkcad.svg" --library /usr/lib/x86_64-linux-gnu/libssl.so.3 --library /usr/lib/x86_64-linux-gnu/libcrypto.so.3 --plugin qt "${plugin_libraries[@]}"
# Bundle vendor-neutral OpenGL dispatch libraries, not vendor GPU drivers.
# linuxdeploy excludes these by default, but a clean Gentoo installation may
# have no system GLX even when using the offscreen platform.
for library in libGL.so.1 libGLX.so.0 libGLdispatch.so.0 libOpenGL.so.0 libEGL.so.1 libfontconfig.so.1 libfreetype.so.6 libexpat.so.1 libX11.so.6 libX11-xcb.so.1 libxcb.so.1 libdrm.so.2 libwayland-client.so.0; do
    cp -L "/usr/lib/x86_64-linux-gnu/$library" "$work/AppDir/usr/lib/$library"
    patchelf --set-rpath '$ORIGIN' "$work/AppDir/usr/lib/$library"
done
mkdir -p "$work/AppDir/usr/share/licenses/glvnd"
for package in libgl1 libglx0 libglvnd0 libopengl0 libegl1 libfontconfig1 libfreetype6 libexpat1 libx11-6 libx11-xcb1 libxcb1 libdrm2 libwayland-client0; do
    cp "/usr/share/doc/$package/copyright" "$work/AppDir/usr/share/licenses/glvnd/$package.txt"
done
mkdir -p "$work/AppDir/usr/share/fonts" "$work/AppDir/usr/share/licenses/dejavu"
cp /usr/share/fonts/truetype/dejavu/DejaVuSans*.ttf "$work/AppDir/usr/share/fonts/"
cp /usr/share/doc/fonts-dejavu-core/copyright "$work/AppDir/usr/share/licenses/dejavu/"
cat > "$work/AppDir/usr/share/thinkcad/fonts.conf" <<'FONTS'
<?xml version="1.0"?>
<!DOCTYPE fontconfig SYSTEM "urn:fontconfig:fonts.dtd">
<fontconfig>
  <include ignore_missing="yes">/etc/fonts/fonts.conf</include>
  <dir prefix="relative">../fonts</dir>
  <cachedir prefix="xdg">fontconfig</cachedir>
</fontconfig>
FONTS
# Match the Icon= identifier regardless of linuxdeploy's source icon basename.
cp "$work/thinkcad.svg" "$work/AppDir/thinkcad.svg"
rm -f "$work/AppDir/AppRun"
cp "$source_dir/packaging/appimage/AppRun" "$work/AppDir/AppRun";chmod +x "$work/AppDir/AppRun"
THINKCAD_SMOKE_TIMEOUT_MS=300000 QT_QPA_PLATFORM=offscreen "$work/AppDir/AppRun" --smoke-test
# No FUSE is required for this build or smoke test.
ARCH=x86_64 "$work/tools/appimagetool-x86_64.AppImage" --runtime-file "$work/tools/runtime-x86_64" "$work/AppDir" "$output/ThinkCad-${version}-x86_64.AppImage"
chmod +x "$output/ThinkCad-${version}-x86_64.AppImage"
(cd "$output" && sha256sum "ThinkCad-${version}-x86_64.AppImage" > "ThinkCad-${version}-x86_64.AppImage.sha256")
THINKCAD_SMOKE_TIMEOUT_MS=300000 QT_QPA_PLATFORM=offscreen APPIMAGE_EXTRACT_AND_RUN=1 "$output/ThinkCad-${version}-x86_64.AppImage" --smoke-test
