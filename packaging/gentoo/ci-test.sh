#!/bin/bash
# Runs inside the official Gentoo stage3 container, never on the host.
set -euo pipefail
emerge-webrsync
eselect profile set default/linux/amd64/23.0/desktop/gnome
mkdir -p /etc/portage/binrepos.conf /etc/portage/repos.conf /etc/portage/package.accept_keywords
cat > /etc/portage/binrepos.conf/thinkcad-ci.conf <<'CONF'
[gentoo]
priority = 9999
sync-uri = https://distfiles.gentoo.org/releases/amd64/binpackages/23.0/x86-64
CONF
getuto
# Dependencies must come from signed Gentoo binary packages; the app is compiled below.
emerge --getbinpkgonly --binpkg-respect-use=y --backtrack=100 --oneshot dev-build/cmake dev-build/ninja dev-qt/qtbase virtual/zlib media-fonts/dejavu
mkdir -p /var/db/repos/thinkcad
tar -xJf /release/ThinkCad-0.5.1-Gentoo-overlay.tar.xz -C /var/db/repos/thinkcad --strip-components=1
cat > /etc/portage/repos.conf/thinkcad.conf <<'CONF'
[thinkcad]
location = /var/db/repos/thinkcad
masters = gentoo
auto-sync = no
CONF
printf 'media-gfx/thinkcad ~amd64\n' > /etc/portage/package.accept_keywords/thinkcad
cp /release/ThinkCad-0.5.1-Source.tar.xz /var/cache/distfiles/
export ACCEPT_LICENSE='*'
# Build with examples and run the suite through the real cmake eclass.
USE='examples test X wayland' FEATURES='test' emerge --oneshot --usepkg=n media-gfx/thinkcad
QT_QPA_PLATFORM=offscreen thinkcad --smoke-test
# Compile and install the minimal X11 variant; examples must actually disappear.
USE='-examples -test X -wayland' emerge --oneshot --usepkg=n media-gfx/thinkcad
test -x /usr/bin/thinkcad
test ! -d /usr/share/thinkcad/examples
# A user sketch must compile from the installed SDK even without examples.
cat > /tmp/thinkcad-user.ino <<'SKETCH'
void setup(){pinMode(5,OUTPUT);}
void loop(){digitalWrite(5,HIGH);delay(5);}
SKETCH
g++ -std=c++20 -pthread -I/usr/share/thinkcad/sdk '-DTHINKERCAD_SKETCH="/tmp/thinkcad-user.ino"' /usr/share/thinkcad/sdk/runner.cpp -o /tmp/thinkcad-user-sketch
/tmp/thinkcad-user-sketch --iterations 1 > /tmp/thinkcad-user-output
grep -q '^WRITE 5 1$' /tmp/thinkcad-user-output
# Build the Wayland-only dependency configuration as well.
USE='-examples -test -X wayland' emerge --oneshot --usepkg=n media-gfx/thinkcad
# Invalid combinations are rejected by Portage REQUIRED_USE.
if USE='-X -wayland' emerge --pretend media-gfx/thinkcad; then
 echo 'Expected REQUIRED_USE failure' >&2
 exit 1
fi
