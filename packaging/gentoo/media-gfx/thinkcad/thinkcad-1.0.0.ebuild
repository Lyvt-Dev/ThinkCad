# Copyright 2026 ThinkCad contributors
# Distributed under the terms of the GNU General Public License v2

EAPI=8
inherit cmake xdg

DESCRIPTION="Native 2D microcontroller circuit simulator"
HOMEPAGE="https://github.com/Lyvt-Dev/ThinkCad"
SRC_URI="https://github.com/Lyvt-Dev/ThinkCad/releases/download/v${PV}/ThinkCad-${PV}-Source.tar.xz"
S="${WORKDIR}/ThinkCad-${PV}"

LICENSE="all-rights-reserved BSD"
SLOT="0"
KEYWORDS="~amd64"
IUSE="+examples test +X +wayland"
REQUIRED_USE="|| ( X wayland )"
RESTRICT="!test? ( test )"

COMMON_DEPEND="
 >=dev-qt/qtbase-6.11:6[gui,network,ssl,widgets]
 >=dev-qt/qtmultimedia-6.11:6
 >=dev-qt/qtserialport-6.11:6
 X? ( dev-qt/qtbase:6[X] )
 wayland? ( dev-qt/qtbase:6[wayland] )
 virtual/zlib:="
DEPEND="${COMMON_DEPEND}"
RDEPEND="${COMMON_DEPEND} >=sys-devel/gcc-12[cxx]"
BDEPEND="test? ( >=sys-devel/gcc-12[cxx] media-fonts/dejavu )"

src_configure() {
 local mycmakeargs=(
  -DTHINKERCAD_GUI=ON
  -DTHINKCAD_INSTALL_EXAMPLES=$(usex examples)
  -DBUILD_TESTING=$(usex test)
 )
 cmake_src_configure
}

src_test() {
 export QT_QPA_PLATFORM=offscreen
 cmake_src_test
}

pkg_postinst() {
 xdg_pkg_postinst
 elog "Start ThinkCad with: thinkcad"
 elog "Sketches use the active g++ compiler; select a different compiler in Settings if needed."
 elog "Updates are managed by Portage, not by an in-app root installer."
}
