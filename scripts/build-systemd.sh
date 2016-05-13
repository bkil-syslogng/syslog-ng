#!/bin/sh -x
#############################################################################
# Copyright (c) 2016 Balabit
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 2 as published
# by the Free Software Foundation, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
#
# As an additional exemption you are allowed to compile & link against the
# OpenSSL libraries as published by the OpenSSL project. See the file
# COPYING for details.
#
#############################################################################

build_systemd() {
  pkg-config --exists "libsystemd >= 209" && return 0

  git clone -b v210 https://github.com/systemd/systemd.git || exit 1
  cd systemd || exit 1
  ./autogen.sh || exit 1

  ./configure \
    --prefix="$SLNG_CACHE"/usr \
    --with-rootprefix="$SLNG_CACHE"/usr \
    --datadir="$SLNG_CACHE"/usr/share \
    --with-sysvinit-path="$SLNG_CACHE"/etc/init.d \
    --with-sysvrcnd-path="$SLNG_CACHE"/etc/rc.d \
    --with-dbuspolicydir="$SLNG_CACHE"/etc/dbus-1/system.d \
    --with-dbussessionservicedir="$SLNG_CACHE"/usr/share/dbus-1/services \
    --with-dbussystemservicedir="$SLNG_CACHE"/usr/share/dbus-1/system-services \
    --with-dbusinterfacedir="$SLNG_CACHE"/usr/share/dbus-1/interfaces \
    --with-bashcompletiondir="$SLNG_CACHE"/usr/share/bash-completion/completions \
    --with-rc-local-script-path-start="$SLNG_CACHE"/etc/rc.local \
    --with-rc-local-script-path-stop="$SLNG_CACHE"/usr/sbin/halt.local \
    `get_configure_disables` || exit 1

  local CORES="$(expr 1 + `grep -c "^processor" /proc/cpuinfo`)"
  make -j $CORES install
  cd .. || exit 1
  rm -Rf systemd

  true
}

get_configure_disables() {
  ./configure --help |
  grep -iE -- "--(disable|without)-" |
  grep -vE -- "\
--disable-(option-checking|FEATURE|largefile|silent-rules|libtool-lock)|\
--without-PACKAGE" |
  cut -d " " -f 3
}

build_systemd "$@"
