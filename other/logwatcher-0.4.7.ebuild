# Copyright 1999-2013 Gentoo Foundation
# Distributed under the terms of the GNU General Public License v2
# $Header: $

EAPI=4

inherit git-2

DESCRIPTION="Just-In-Time logwatcher with socket interface"
HOMEPAGE="https://github.com/sperner/Logwatcher"
EGIT_PROJECT="Logwatcher"
EGIT_REPO_URI="https://github.com/sperner/Logwatcher/"

LICENSE="GPL-3"
SLOT="0"
KEYWORDS="~mips ~sparc ~x86 ~amd64"
#IUSE="ssl,sql"

DEPEND="sys-devel/autoconf
	sys-devel/automake"

#RDEPEND="ssl?	( dev-libs/openssl )
#	sql?	( dev-db/mysql )"


#src_configure() {
#	econf
#	econf \
#		$(use_enable ssl) \
#		$(use_enable sql)
#}

src_install() {
	dobin logwatcher

	insinto /etc
	doins config/logwatcher.conf
	doins config/logwatcher_files.conf

	exeinto /etc/init.d
	doexe other/logwatcher

	dodir /var/cache/logwatcher

#	doicon logwatcher.png
#	domenu logwatcher.desktop
	dodoc docs/{authors,changelog,install,license,news,readme}
	doman other/logwatcher.1
	doman other/logwatcher.conf.1
}
