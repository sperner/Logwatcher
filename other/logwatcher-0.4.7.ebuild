# Copyright 1999-2013 Gentoo Foundation
# Distributed under the terms of the GNU General Public License v2
# $Header: $

EAPI=4

DESCRIPTION="Just-In-Time logwatcher with socket interface"
HOMEPAGE="https://github.com/sperner/Logwatcher"
SRC_URI="mirror://github.com/sperner/Logwatcher/${P}.tar.gz"

LICENSE="GPL-2"
SLOT="0"
KEYWORDS="~mips ~sparc ~x86 ~amd64"
#IUSE="ssl,sql"

#RDEPEND="ssl?	( dev-libs/openssl )
#	sql?	( dev-db/mysql )"

src_configure() {
	econf
#	econf \
#		$(use_enable ssl) \
#		$(use_enable sql)
}

src_install() {
	emake DESTDIR="${D}" install

#	doicon logwatcher.png
#	domenu logwatcher.desktop
#	dodoc readme
#	doman logwatcher.1
}
