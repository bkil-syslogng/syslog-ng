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

build_riemann_client() {
  if ! pkg-config --exists "riemann-client >= 1.6.0"; then
    git clone https://github.com/algernon/riemann-c-client.git || exit 1
    cd riemann-c-client || exit 1
    autoreconf -i || exit 1
    ./configure --prefix="$SLNG_CACHE/usr" \
      --disable-dependency-tracking || exit 1
    make -j install || exit 1
    cd .. || exit 1
    rm -Rf riemann-c-client
  fi
  true
}

build_riemann_client "$@"
